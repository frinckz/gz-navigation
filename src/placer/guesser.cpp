#include "guesser.h"
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

void GuesserGameMode::Guess(CVector3f position) {
    auto distance = calculateDistance(m_targetPosition.position, position);
    // full points within PERFECT_DISTANCE, then exponential decay: ~37 at 500 m, ~5 at 1.5 km
    double score = distance <= PERFECT_DISTANCE
        ? MAX_SCORE
        : MAX_SCORE * std::exp(-(distance - PERFECT_DISTANCE) / SCORE_DECAY_DISTANCE);
    m_score += score;
    // TODO: Show score on map
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