#include "placer.h"
#include "log.h"
#include "game/game_world.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

Placer* Placer::sm_placer = nullptr;
bool Placer::sm_destroyed = false;

// File format: one entry per line, "x y z (mouse.x mouse.y mouse.z)"
std::string Placer::GetPositionsFilePath() {
    char modulePath[MAX_PATH];
    GetModuleFileNameA(nullptr, modulePath, MAX_PATH);
    return (std::filesystem::path(modulePath).parent_path() / "gz_nav" / "positions.txt").string();
}

Placer::Placer() {
    ReloadSavedPositions();
}

void Placer::ReloadSavedPositions() {
    m_randomPositions.clear();
    std::ifstream file(GetPositionsFilePath());
    std::string line;
    while (std::getline(file, line)) {
        std::replace_if(line.begin(), line.end(), [](char c) { return c == '(' || c == ')'; }, ' ');
        std::istringstream stream(line);
        RandomPosition entry{};
        if (stream >> entry.position.x >> entry.position.y >> entry.position.z
                   >> entry.aimPosition.x >> entry.aimPosition.y >> entry.aimPosition.z) {
            m_randomPositions.push_back(entry);
        }
    }
    gz::Log("Placer: loaded %zu saved positions", m_randomPositions.size());
}

Placer::~Placer() {
    // positions are written on every save, so there is nothing left to flush
    sm_placer = nullptr;
    sm_destroyed = true;
    gz::Log("Placer: destroyed");
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

bool Placer::RandomizePosition() {
    auto nextPosition = ChooseRandomItem(m_randomPositions);
    if (!nextPosition) {
        gz::Log("Placer: no saved positions to randomize");
        return false;
    }
    auto* gameWorld = gz::CGameWorld::instance();
    if (!gameWorld) {
        return false;
    }
    const CVector3f& aim = nextPosition->aimPosition;
    // entries without a usable aim position (0 0 0) only change the position
    bool hasAim = aim.x != 0.0f || aim.y != 0.0f || aim.z != 0.0f;
    gameWorld->SafeTeleportToPosition(nextPosition->position, nullptr, hasAim ? &aim : nullptr);
    return true;
}

template <typename T>
const T* Placer::ChooseRandomItem(const std::vector<T>& v)
{
    if (v.empty()) {
        return nullptr;
    }
    static thread_local std::mt19937 rng{ std::random_device{}() };
    std::uniform_int_distribution<size_t> dist(0, v.size() - 1);
    return &v[dist(rng)];
}
