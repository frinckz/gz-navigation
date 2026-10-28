#pragma once

#include <optional>
#include <random>
#include <string>
#include <vector>

#include "data_types.h"
#include "placer_location.h"

class Placer {

  private:
    Placer();
    ~Placer() = default;
    std::string GetPositionsFilePath();
    std::string GetVisitedFilePath();
    // Records a location the player was placed at in visited_locations.txt
    void AppendVisitedLocation(const PlacerLocation& location);


    // Picks a location, the farther from the last visited one the more likely (see LocationWeight in placer.cpp)
    const PlacerLocation* ChooseWeightedLocation(const std::vector<PlacerLocation>& locations);

    // Teleports the player to a random saved position and returns it; std::nullopt if none are saved
    std::optional<PlacerLocation> RandomizePosition(std::vector<PlacerLocation> positions = {});

    // Sets a random time of day, day (06:00-18:00) is 1.5x more likely than night; returns false without world time
    bool RandomizeTime();

    // Takes over the weather with a random preset: 10% snowing, 5% snowy ground, 20% raining, 10% wet ground, 55% dry;
    // wind is 50% likely while snowing, 30% while raining and 10% when dry. Returns false without weather
    bool RandomizeWeather();

  public:
    std::vector<PlacerLocation> m_randomLocations{};
    // The newest visited locations, loaded from visited_locations.txt
    std::vector<PlacerLocation> m_visitedLocations{};
    std::vector<PlacerLocation> m_extractionPoints{};

    std::vector<PlacerLocation> GetUnvisitedLocations();

    // True if the player was already placed at this location (matched by position)
    bool IsVisited(const PlacerLocation& location);

    static Placer* instance() {
        static Placer instance;
        return &instance;
    }

    void SavePosition(CVector3f position, CVector3f aimPosition);

    // Discards the in-memory list and re-reads the positions file
    void ReloadSavedPositions();

    // Randomizes time, weather and position; returns the chosen position (the teleport may finish frames later)
    std::optional<PlacerLocation> Place(std::vector<PlacerLocation> positions = {});

    const PlacerLocation* GetRandomExtractionPoint();
};
