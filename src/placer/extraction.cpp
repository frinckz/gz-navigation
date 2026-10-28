#include "chrono"

#include "extraction.h"
#include "placer.h"

void ExtractionGameMode::Start(long long timeLimitInMinutes) {
    m_timeLimitInMinutes = timeLimitInMinutes;
    if (auto placed = Placer::instance()->Place()) {
        m_startLocation = *placed;
    }
    if (auto extraction = Placer::instance()->GetRandomExtractionPoint()) {
        m_targetLocation = *extraction;
    }
    m_startTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}

void ExtractionGameMode::Update() {
    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    if ((time - m_startTime) > m_timeLimitInMinutes) {
        ExtractionGameMode::Fail();
    }
}

void ExtractionGameMode::AdvanceToNextRound() {
    m_round++;
    if (auto placed = Placer::instance()->Place()) {
        m_startLocation = *placed;
    }
    if (auto extraction = Placer::instance()->GetRandomExtractionPoint()) {
        m_targetLocation = *extraction;
    }
    m_startTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}

void ExtractionGameMode::Win() {
    // TODO: play credits
}

void ExtractionGameMode::Fail() {
    // TODO: play credits
}