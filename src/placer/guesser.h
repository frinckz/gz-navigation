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

    bool m_guessed = false;         // the current round was guessed, waiting for the next round
    double m_roundScore = 0;        // points of the guessed round
    double m_roundDistance = 0;     // meters between the guess and the target of the guessed round
    CVector3f m_guessPosition{};    // marker position at the time of the guess, the user's marker can be moved afterwards

  public:
    static GuesserGameMode* instance() {
        static GuesserGameMode instance;
        return &instance;
    }

    // Scores the user's map marker against the target; false if the round is already guessed or no marker is placed
    bool Guess();
    // Teleports the player back to the target of the current round
    void ResetToTarget() const;
    void AdvanceToNextRound();
    void Start();

    // A guess needs the user's map marker and a round that was not guessed yet
    [[nodiscard]] bool CanGuess() const;
    [[nodiscard]] bool HasGuessed() const;
    [[nodiscard]] double GetScore() const;
    [[nodiscard]] long GetRound() const;
    [[nodiscard]] double GetRoundScore() const;
    [[nodiscard]] double GetRoundDistance() const;
    [[nodiscard]] const CVector3f& GetTargetPosition() const { return m_targetPosition.position; }
    [[nodiscard]] const CVector3f& GetGuessPosition() const { return m_guessPosition; }
};
