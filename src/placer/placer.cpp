#include "placer.h"
#include "log.h"
#include "game/game_world.h"
#include "game/world_time.h"
#include "game/landscape_manager.h"
#include "game/weather.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

std::vector<RandomPosition> Placer::m_randomPositions = {};

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

// File format: one entry per line, "x y z (mouse.x mouse.y mouse.z)"; '#' starts a comment, empty lines are ignored
std::string Placer::GetPositionsFilePath() {
    char modulePath[MAX_PATH];
    GetModuleFileNameA(nullptr, modulePath, MAX_PATH);
    return (std::filesystem::path(modulePath).parent_path() / "gz_nav" / "positions.txt").string();
}

Placer::Placer() {
    ReloadSavedPositions();
}

std::optional<RandomPosition> Placer::Place(std::vector<RandomPosition> positions) {
    RandomizeTime();
    RandomizeWeather();
    return RandomizePosition(positions);
}

void Placer::ReloadSavedPositions() {
    m_randomPositions.clear();
    std::ifstream file(GetPositionsFilePath());
    std::string line;
    size_t lineNumber = 0;
    while (std::getline(file, line)) {
        lineNumber++;
        // everything after '#' is a comment
        line = line.substr(0, line.find('#'));
        if (line.find_first_not_of(" \t\r") == std::string::npos) {
            continue;
        }
        std::replace_if(line.begin(), line.end(), [](char c) { return c == '(' || c == ')'; }, ' ');
        std::istringstream stream(line);
        RandomPosition entry{};
        if (stream >> entry.position.x >> entry.position.y >> entry.position.z
                   >> entry.aimPosition.x >> entry.aimPosition.y >> entry.aimPosition.z) {
            m_randomPositions.push_back(entry);
        } else {
            gz::Log("Placer: ignoring invalid line %zu in %s", lineNumber, GetPositionsFilePath().c_str());
        }
    }
    gz::Log("Placer: loaded %zu saved positions", m_randomPositions.size());
}

void Placer::SavePosition(CVector3f position, CVector3f aimPosition) {
    m_randomPositions.push_back({ position, aimPosition });
    std::ofstream file(GetPositionsFilePath(), std::ios::app);
    if (file) {
        file << std::setprecision(9) << position.x << ' ' << position.y << ' ' << position.z
             << " (" << aimPosition.x << ' ' << aimPosition.y << ' ' << aimPosition.z << ")\n";
    } else {
        gz::Log("Placer: failed to write position %s", GetPositionsFilePath().c_str());
    }
    gz::Log("Placer: saved position %.1f, %.1f, %.1f (%zu total)",
        position.x, position.y, position.z, m_randomPositions.size());
}

std::optional<RandomPosition> Placer::RandomizePosition(std::vector<RandomPosition> positions) {
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
