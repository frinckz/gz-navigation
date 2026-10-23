#pragma once

#include <optional>
#include <random>
#include <string>
#include <vector>

#include "data_types.h"
#include "random_position.h"

class Placer {

  private:
    Placer();
    ~Placer() = default;
    static std::string GetPositionsFilePath();


    template <typename T>
    const T* ChooseRandomItem(const std::vector<T>& v);

  public:
    static std::vector<RandomPosition> m_randomPositions;

    static Placer* instance() {
        static Placer instance;
        return &instance;
    }

    void SavePosition(CVector3f position, CVector3f aimPosition);

    // Discards the in-memory list and re-reads the positions file
    void ReloadSavedPositions();

    // Teleports the player to a random saved position and returns it; std::nullopt if none are saved
    std::optional<RandomPosition> RandomizePosition(std::vector<RandomPosition> positions = Placer::m_randomPositions);

    // Sets a random time of day, day (06:00-18:00) is 1.5x more likely than night; returns false without world time
    bool RandomizeTime();

    // Takes over the weather with a random preset: 10% snowing, 5% snowy ground, 20% raining, 10% wet ground, 55% dry;
    // wind is 50% likely while snowing, 30% while raining and 10% when dry. Returns false without weather
    bool RandomizeWeather();

    // Randomizes time, weather and position; returns the chosen position (the teleport may finish frames later)
    std::optional<RandomPosition> Place(std::vector<RandomPosition> positions = Placer::m_randomPositions);
};
