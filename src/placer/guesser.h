#pragma once
#include <vector>

#include "data_types.h"
#include "placer/placer_location.h"

class GuesserGameMode
{
  private:
    GuesserGameMode() = default;
    ~GuesserGameMode() = default;

    PlacerLocation m_targetPosition{};
    double m_score = 0;
    long m_round = 0;
    std::vector<PlacerLocation> m_positionBuffer;

  public:
    static GuesserGameMode* instance() {
        static GuesserGameMode instance;
        return &instance;
    }

    void Guess(CVector3f position);
    void AdvanceToNextRound();
    [[nodiscard]] double GetScore() const;
    [[nodiscard]] long GetRound() const;
    void Start();
};
