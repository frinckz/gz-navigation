#include "guesser.h"
#include "game/game_world.h"
#include "game/map.h"
#include "placer/placer.h"

#include <algorithm>
#include <cmath>


const float MAX_X = 5000;
const float MAX_Y = 5000;

const double MAX_SCORE = 100;
const double PERFECT_DISTANCE = 5;        // meters
const double SCORE_DECAY_DISTANCE = 500;  // meters, the score drops to ~37% per this distance

double calculateDistance(CVector3f const& a, CVector3f const& b) {
    // horizontal distance; the marker's height is only an estimate above the terrain
    const double dx = a.x - b.x;
    const double dz = a.z - b.z;
    return sqrt(dx * dx + dz * dz);
}

bool GuesserGameMode::CanGuess() const {
    CVector3f marker{};
    return !m_guessed && gz::CMap::GetLocalUserWaypoint(marker);
}

bool GuesserGameMode::Guess() {
    CVector3f marker{};
    if (m_guessed || !gz::CMap::GetLocalUserWaypoint(marker)) {
        return false;
    }
    m_guessPosition = marker;
    m_roundDistance = calculateDistance(m_targetPosition.position, marker);
    // full points within PERFECT_DISTANCE, then exponential decay: ~37 at 500 m, ~5 at 1.5 km
    m_roundScore = m_roundDistance <= PERFECT_DISTANCE
        ? MAX_SCORE
        : MAX_SCORE * std::exp(-(m_roundDistance - PERFECT_DISTANCE) / SCORE_DECAY_DISTANCE);
    m_score += m_roundScore;
    m_guessed = true;
    return true;
}

void GuesserGameMode::ResetToTarget() {
    gz::CMap::Close();
    auto* gameWorld = gz::CGameWorld::instance();
    if (!gameWorld) {
        return;
    }
    const CVector3f& aim = m_targetPosition.aimPosition;
    // targets without a usable aim position (0 0 0) only change the position
    const bool hasAim = aim.x != 0.0f || aim.y != 0.0f || aim.z != 0.0f;
    gameWorld->SafeTeleportToPosition(m_targetPosition.position, nullptr, hasAim ? &aim : nullptr);
}

// Available locations that were not visited yet; once everything was visited all locations are allowed again
static std::vector<PlacerLocation> UnvisitedCandidates() {
    std::vector<PlacerLocation> candidates{};
    std::copy_if(Placer::m_randomPositions.begin(), Placer::m_randomPositions.end(),
                 std::back_inserter(candidates), [](const PlacerLocation& pos) { return !Placer::IsVisited(pos); });
    return candidates.empty() ? Placer::m_randomPositions : candidates;
}

void GuesserGameMode::AdvanceToNextRound() {
    m_round++;
    m_guessed = false;
    gz::CMap::Close();
    // the teleport may still be running (fast travel), so the chosen position is the target, not the player position
    if (auto placed = Placer::instance()->Place(UnvisitedCandidates())) {
        m_targetPosition = *placed;
    }
}

long GuesserGameMode::GetRound() const {
    return m_round;
}

double GuesserGameMode::GetScore() const {
    return m_score;
}

void GuesserGameMode::Start() {
    if (auto placed = Placer::instance()->Place(UnvisitedCandidates())) {
        m_targetPosition = *placed;
    }
}

bool GuesserGameMode::HasGuessed() const {
    return m_guessed;
}

double GuesserGameMode::GetRoundScore() const {
    return m_roundScore;
}

double GuesserGameMode::GetRoundDistance() const {
    return m_roundDistance;
}
