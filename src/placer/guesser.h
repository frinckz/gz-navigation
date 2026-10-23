#pragma once
#include "data_types.h"
#include "random_position.h"

class GuesserGameMode
{
  private:
    RandomPosition m_targetPosition{};
    double m_score = 0;
    long m_round = 0;
    std::vector<RandomPosition> m_positionBuffer;

  public:
    void Guess(CVector3f position);
    void AdvanceToNextRound();
    double GetScore() const;
    long GetRound() const;
};