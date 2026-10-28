#pragma once

#include "placer_location.h"

class ExtractionGameMode {
  private:
    PlacerLocation m_targetLocation;
    PlacerLocation m_startLocation;
    long long m_startTime = 0;
    long long m_timeLimitInMinutes = -1;
    long m_round = 0;

    void Fail();
    void Win();
    void AdvanceToNextRound();

    static ExtractionGameMode* sm_instance;
    ExtractionGameMode() = default;
  public:
    static ExtractionGameMode* instance() {
        static ExtractionGameMode instance;
        return &instance;
    }
    void Start(long long timeLimitInMinutes);
    void Update();
};