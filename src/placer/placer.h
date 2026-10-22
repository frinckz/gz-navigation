#pragma once

#include <random>
#include <string>
#include <vector>

#include "data_types.h"

struct RandomPosition {
    CVector3f position;
    CVector3f aimPosition;
};

class Placer {

  private:
    static Placer *sm_placer;
    static bool sm_destroyed;
    Placer();
    ~Placer();
    static std::string GetPositionsFilePath();

    std::vector<RandomPosition> m_randomPositions{};

    template <typename T>
    const T* ChooseRandomItem(const std::vector<T>& v);

  public:
    static Placer* instance() {
        if (!sm_destroyed && !sm_placer) {
            sm_placer = new Placer();
        }
        return sm_placer;
    }

    static void Destroy() {
        delete sm_placer;
    }

    void SavePosition(CVector3f position, CVector3f aimPosition);

    // Discards the in-memory list and re-reads the positions file
    void ReloadSavedPositions();

    // Teleports the player to a random saved position; returns false if none are saved
    bool RandomizePosition();

    // Sets a random time of day, day (06:00-18:00) is 1.5x more likely than night; returns false without world time
    bool RandomizeTime();

    bool Place();
};
