#pragma once

#include "addresses.h"
#include "hook_helpers.h"
#include "game/animal.h"
#include "log.h"

#include <unordered_set>

namespace gz::NoEnemies
{
    using SpawnAnimalFunc = void(*)(void* spawnSystem, CAnimal* animal, void* networkComponent);

    inline SpawnAnimalFunc g_origSpawnAnimal = nullptr;
    inline bool g_blockSpawns = false;

    // animals created by the mod (Machine tab etc.); the game also sets flag 0x02 on its own scripted machines (seen: 0x12), so flags can't tell them apart
    inline std::unordered_set<const CAnimal*> g_modSpawned;

    inline void RegisterModSpawn(const CAnimal* animal)
    {
        if (animal) g_modSpawned.insert(animal);
    }

    inline void HookedSpawnAnimal(void* spawnSystem, CAnimal* animal, void* networkComponent)
    {
        // let machines spawned by the mod through
        const bool modSpawned = animal && g_modSpawned.count(animal) > 0;
        static int s_logged = 0;
        if (s_logged++ < 50) {
            Log("[NO ENEMIES] SpawnAnimal animal=%p flags=0x%02x block=%d%s", (void*)animal,
                animal ? (unsigned)(uint8_t)animal->m_Flags : 0u, g_blockSpawns ? 1 : 0,
                (g_blockSpawns && !modSpawned) ? " -> blocked" : "");
        }
        if (g_blockSpawns && !modSpawned) {
            return;
        }
        if (modSpawned) {
            g_modSpawned.erase(animal); // spawned once, don't keep stale pointers around
        }
        if (g_origSpawnAnimal) {
            g_origSpawnAnimal(spawnSystem, animal, networkComponent);
        }
    }

    inline bool SetupSpawnAnimalHook()
    {
        if (g_origSpawnAnimal != nullptr) {
            return true;
        }
        return MH_CreateHookGZ(SPAWN_ANIMAL, &HookedSpawnAnimal, &g_origSpawnAnimal);
    }

    inline void SetBlockSpawns(bool v) { g_blockSpawns = v; }
}
