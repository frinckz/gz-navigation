#include "placer.h"
#include "log.h"
#include "game/game_world.h"
#include "game/world_time.h"
#include "game/landscape_manager.h"
#include "game/weather.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

std::vector<PlacerLocation> Placer::m_randomPositions = {};
std::vector<PlacerLocation> Placer::m_visitedPositions = {};

// the visited file keeps at most this many times the number of available locations
static constexpr size_t VISITED_LIMIT_FACTOR = 2;
static constexpr size_t VISITED_RECORD_LINES = 4;

// day and night are both 12 hours long, so day is DAY_WEIGHT times as likely per hour as night
static constexpr float DAY_START  = 6.0f;
static constexpr float DAY_END    = 18.0f;
static constexpr float DAY_WEIGHT = 1.5f;

// order matches the chances in RandomizeWeather
enum class WeatherKind { Snowing, Snowed, Raining, Rained, Dry };
static constexpr const char* WEATHER_NAMES[] = { "snowing", "snowy ground", "raining", "wet ground", "dry" };

static constexpr float CALM_WIND      = 2.0f;
static constexpr float WINDY_MIN      = 4.0f;
static constexpr float WINDY_MAX      = 10.0f;
static constexpr float RAIN_LIGHTNING = 0.81f;

static std::mt19937& Rng() {
    static thread_local std::mt19937 rng{ std::random_device{}() };
    return rng;
}

// the product of two uniforms has density -ln(t) on [0, 1): low intensities are common, the maximum is rare
static float LogFalloff(float min, float max) {
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    return min + (max - min) * unit(Rng()) * unit(Rng());
}

static float UniformRange(float min, float max) {
    return std::uniform_real_distribution<float>(min, max)(Rng());
}

// File format: one entry per line, "x y z (aim.x aim.y aim.z)"; empty lines are ignored.
// A header line "# (<name>, <region>, <type>)" sets the metadata of all entries below it;
// any other '#' line (or trailing '#' text) is a comment.
std::string Placer::GetPositionsFilePath() {
    char modulePath[MAX_PATH];
    GetModuleFileNameA(nullptr, modulePath, MAX_PATH);
    return (std::filesystem::path(modulePath).parent_path() / "gz_nav" / "locations" / "available_locations.txt").string();
}

static std::string Trim(const std::string& s) {
    const size_t begin = s.find_first_not_of(" \t\r");
    if (begin == std::string::npos) {
        return "";
    }
    return s.substr(begin, s.find_last_not_of(" \t\r") - begin + 1);
}

// Parses "# (<name>, <region>, <type>)"; returns false for any other line
static bool ParseMetaHeader(const std::string& line, PlacerLocationMetadata& meta) {
    const std::string trimmed = Trim(line);
    if (trimmed.empty() || trimmed[0] != '#') {
        return false;
    }
    const std::string body = Trim(trimmed.substr(1));
    if (body.size() < 2 || body.front() != '(' || body.back() != ')') {
        return false;
    }
    const std::string inner = body.substr(1, body.size() - 2);
    const size_t first = inner.find(',');
    const size_t second = first == std::string::npos ? first : inner.find(',', first + 1);
    if (second == std::string::npos || inner.find(',', second + 1) != std::string::npos) {
        return false;
    }
    meta.name = Trim(inner.substr(0, first));
    meta.region = Trim(inner.substr(first + 1, second - first - 1));
    meta.type = Trim(inner.substr(second + 1));
    return true;
}

std::string Placer::GetVisitedFilePath() {
    return (std::filesystem::path(GetPositionsFilePath()).parent_path() / "visited_locations.txt").string();
}

// Local time as ISO 8601, e.g. 2026-10-26T14:03:11
static std::string IsoTimestamp() {
    const auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm localTime;
    localtime_s(&localTime, &time);
    std::ostringstream oss;
    oss << std::put_time(&localTime, "%Y-%m-%dT%H:%M:%S");
    return oss.str();
}

// Appends a fixed size record of 4 lines: "# (name, region, type)", "# <ISO date>", the position and a blank spacer.
// The file is trimmed to the newest VISITED_LIMIT_FACTOR * available locations records by line count.
void Placer::AppendVisitedLocation(const PlacerLocation& location) {
    std::vector<std::string> lines;
    {
        std::ifstream in(GetVisitedFilePath());
        std::string line;
        while (std::getline(in, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            lines.push_back(std::move(line));
        }
    }

    const auto& meta = location.locationMetadata;
    std::ostringstream position;
    position << std::setprecision(9) << location.position.x << ' ' << location.position.y << ' ' << location.position.z
             << " (" << location.aimPosition.x << ' ' << location.aimPosition.y << ' ' << location.aimPosition.z << ")";
    lines.push_back("# (" + meta.name + ", " + meta.region + ", " + meta.type + ")");
    lines.push_back("# " + IsoTimestamp());
    lines.push_back(position.str());
    lines.push_back("");

    const size_t maxLines = std::max<size_t>(1, VISITED_LIMIT_FACTOR * m_randomPositions.size()) * VISITED_RECORD_LINES;
    if (lines.size() > maxLines) {
        lines.erase(lines.begin(), lines.end() - maxLines);
    }

    m_visitedPositions.push_back(location);
    if (m_visitedPositions.size() > maxLines / VISITED_RECORD_LINES) {
        m_visitedPositions.erase(m_visitedPositions.begin(), m_visitedPositions.end() - maxLines / VISITED_RECORD_LINES);
    }

    std::ofstream out(GetVisitedFilePath(), std::ios::trunc);
    if (!out) {
        gz::Log("Placer: failed to write visited location %s", GetVisitedFilePath().c_str());
        return;
    }
    for (const auto& line : lines) {
        out << line << '\n';
    }
}

Placer::Placer() {
    ReloadSavedPositions();
}

std::optional<PlacerLocation> Placer::Place(std::vector<PlacerLocation> positions) {
    RandomizeTime();
    RandomizeWeather();
    return RandomizePosition(positions);
}

// Reads all locations of a file in the format described above
static std::vector<PlacerLocation> LoadLocations(const std::string& path) {
    std::vector<PlacerLocation> locations;
    std::ifstream file(path);
    std::string line;
    size_t lineNumber = 0;
    PlacerLocationMetadata meta;
    while (std::getline(file, line)) {
        lineNumber++;
        if (ParseMetaHeader(line, meta)) {
            continue;
        }
        // everything after '#' is a comment
        line = line.substr(0, line.find('#'));
        if (line.find_first_not_of(" \t\r") == std::string::npos) {
            continue;
        }
        std::replace_if(line.begin(), line.end(), [](char c) { return c == '(' || c == ')'; }, ' ');
        std::istringstream stream(line);
        PlacerLocation entry{};
        entry.locationMetadata = meta;
        if (stream >> entry.position.x >> entry.position.y >> entry.position.z
                   >> entry.aimPosition.x >> entry.aimPosition.y >> entry.aimPosition.z) {
            locations.push_back(entry);
        } else {
            gz::Log("Placer: ignoring invalid line %zu in %s", lineNumber, path.c_str());
        }
    }
    return locations;
}

void Placer::ReloadSavedPositions() {
    m_randomPositions = LoadLocations(GetPositionsFilePath());
    m_visitedPositions = LoadLocations(GetVisitedFilePath());
    gz::Log("Placer: loaded %zu saved positions, %zu visited", m_randomPositions.size(), m_visitedPositions.size());
}

bool Placer::IsVisited(const PlacerLocation& location) {
    return std::any_of(m_visitedPositions.begin(), m_visitedPositions.end(), [&location](const PlacerLocation& visited) {
        return visited.position.x == location.position.x && visited.position.y == location.position.y
            && visited.position.z == location.position.z;
    });
}

void Placer::SavePosition(CVector3f position, CVector3f aimPosition) {
    static constexpr const char* SAVED_NAME = "Saved";
    static constexpr const char* SAVED_UNKNOWN = "Unknown";
    m_randomPositions.push_back({ position, aimPosition, { SAVED_NAME, SAVED_UNKNOWN, SAVED_UNKNOWN } });
    std::ofstream file(GetPositionsFilePath(), std::ios::app);
    if (file) {
        // header so the entry doesn't inherit the metadata of the block above it
        file << "\n# (" << SAVED_NAME << ", " << SAVED_UNKNOWN << ", " << SAVED_UNKNOWN << ")\n";
        file << std::setprecision(9) << position.x << ' ' << position.y << ' ' << position.z
             << " (" << aimPosition.x << ' ' << aimPosition.y << ' ' << aimPosition.z << ")\n";
    } else {
        gz::Log("Placer: failed to write position %s", GetPositionsFilePath().c_str());
    }
    gz::Log("Placer: saved position %.1f, %.1f, %.1f (%zu total)",
        position.x, position.y, position.z, m_randomPositions.size());
}

std::optional<PlacerLocation> Placer::RandomizePosition(std::vector<PlacerLocation> positions) {
    auto nextPosition = ChooseRandomItem(positions);
    if (!nextPosition) {
        gz::Log("Placer: no saved positions to randomize");
        return std::nullopt;
    }
    auto* gameWorld = gz::CGameWorld::instance();
    if (!gameWorld) {
        return std::nullopt;
    }
    const CVector3f& aim = nextPosition->aimPosition;
    // entries without a usable aim position (0 0 0) only change the position
    bool hasAim = aim.x != 0.0f || aim.y != 0.0f || aim.z != 0.0f;
    gameWorld->SafeTeleportToPosition(nextPosition->position, nullptr, hasAim ? &aim : nullptr);
    AppendVisitedLocation(*nextPosition);
    return *nextPosition;
}

bool Placer::RandomizeTime() {
    auto* worldTime = gz::CWorldTime::instance();
    if (!worldTime) {
        return false;
    }
    const float dayHours   = DAY_END - DAY_START;
    const float nightHours = 24.0f - dayHours;
    std::bernoulli_distribution isDay(DAY_WEIGHT * dayHours / (DAY_WEIGHT * dayHours + nightHours));
    const bool day = isDay(Rng());

    std::uniform_real_distribution<float> offset(0.0f, day ? dayHours : nightHours);
    // night wraps around midnight, SetTime handles values >= 24
    const float time = (day ? DAY_START : DAY_END) + offset(Rng());
    worldTime->SetTime(time);

    int hours = 0, minutes = 0;
    worldTime->GetTimeAsHHMM(hours, minutes);
    gz::Log("Placer: set time to %02d:%02d (%s)", hours, minutes, day ? "day" : "night");
    return true;
}

bool Placer::RandomizeWeather() {
    auto* landscapeManager = gz::CLandscapeManager::instance();
    auto* weather = landscapeManager ? landscapeManager->GetWeather() : nullptr;
    if (!weather) {
        return false;
    }
    std::discrete_distribution<int> kindDist({ 10.0, 5.0, 20.0, 10.0, 55.0 });
    const auto kind = static_cast<WeatherKind>(kindDist(Rng()));

    auto& wo = gz::Weather::g_overrides;
    wo = gz::Weather::Overrides{};
    wo.enabled = true;
    wo.overrideDownpour = true;
    wo.downpour = 0.0f;
    wo.snowRatio = 0.0f;
    wo.groundSnow = 0.0f;
    wo.groundWetness = 0.0f;
    wo.lightningChance = 0.0f;
    wo.gustStrength = CALM_WIND;

    double windChance = 0.0;
    switch (kind) {
    case WeatherKind::Snowing:
        wo.downpour = LogFalloff(0.1f, 1.0f);
        wo.snowRatio = 1.0f;
        wo.groundSnow = LogFalloff(0.2f, 1.0f);
        // precipitation needs a thick sky to look right, heavier downpour gets darker clouds
        wo.cloudDensity = 4.0f + 2.0f * wo.downpour;
        windChance = 0.5;
        break;
    case WeatherKind::Snowed:
        wo.groundSnow = LogFalloff(0.2f, 1.0f);
        wo.cloudDensity = UniformRange(1.6f, 4.0f);
        break;
    case WeatherKind::Raining:
        wo.downpour = LogFalloff(0.1f, 1.0f);
        wo.groundWetness = 0.5f + 0.5f * wo.downpour;
        wo.cloudDensity = 4.0f + 2.0f * wo.downpour;
        wo.lightningChance = RAIN_LIGHTNING;
        windChance = 0.3;
        break;
    case WeatherKind::Rained:
        wo.groundWetness = LogFalloff(0.3f, 1.0f);
        wo.cloudDensity = UniformRange(1.6f, 4.0f);
        break;
    case WeatherKind::Dry:
        wo.cloudDensity = UniformRange(1.0f, 4.0f);
        windChance = 0.1;
        break;
    }
    const bool windy = std::bernoulli_distribution(windChance)(Rng());
    if (windy) {
        wo.gustStrength = LogFalloff(WINDY_MIN, WINDY_MAX);
    }
    // without this the clouds keep their old density for minutes
    gz::Weather::ForceInstantUpdate(weather);

    gz::Log("Placer: set weather to %s%s (downpour %.2f, snow %.2f, wetness %.2f, clouds %.2f, wind %.1f)",
        WEATHER_NAMES[static_cast<int>(kind)], windy ? ", windy" : "",
        wo.downpour, wo.groundSnow, wo.groundWetness, wo.cloudDensity, wo.gustStrength);
    return true;
}

template <typename T>
const T* Placer::ChooseRandomItem(const std::vector<T>& v)
{
    if (v.empty()) {
        return nullptr;
    }
    std::uniform_int_distribution<size_t> dist(0, v.size() - 1);
    return &v[dist(Rng())];
}
