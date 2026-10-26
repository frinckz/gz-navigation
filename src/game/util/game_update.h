#pragma once

#include <chrono>
#include "game/damageable.h"
#include "game/game_state.h"
#include "game/animal_network.h"
#include "game/player_network_manager.h"
#include "game/character_tasks.h"
#include "game/player_spawn_manager.h"
#include "game/vehicle.h"
#include "game/vehicle_manager.h"
#include "game/world_time.h"
#include "game/custom/preset_override.h"
#include "imgui/ui.h"
#include "placer/gz_nav.h"

#pragma pack(push, 1)
namespace gz::Utils
{
    /// <summary>
    /// Function that runs every game update tick.\n
    /// Any logic that needs to be updated on each tick should go here.
    /// </summary>
    static void OnGameUpdate()
    {
        // custom logic that needs to run every game update
        if (GameState::IsInGame())
        {
            if (CNetworkPlayerManager* networkPlayerManager = CNetworkPlayerManager::instance())
            {
                CCharacter* playerCharacter = networkPlayerManager->GetCharacter();

                if (playerCharacter)
                {
                    // update player invulnerability status
                    CDamageable* playerDamageable = playerCharacter->GetDamageable();
                    if (playerInvulnerable && !playerDamageable->IsInvulnerable())
                    {
                        playerDamageable->SetInvulnerable(true);
                    }

                    // third person stuff
                    if (TpState::g_toggleRequested.exchange(false, std::memory_order_relaxed) && TpState::g_resourcesPrimed)
                    {
                        playerCharacter->SetThirdPerson(!CCharacter::IsThirdPersonActive());
                    }
                    if (TpState::g_animationsActive)
                    {
                        playerCharacter->SetThirdPersonBodyVisible(true);
                    }
                    if (!TpState::g_resourcesPrimed)
                    {
                        PrimeThirdPersonResources();
                    }
                    CharacterTasks::g_jumpSeededThisFrame = false;
                    if (ThirdPersonCamera::NeedsRepush())
                        ThirdPersonCamera::Reapply();
                }
            }

            ProcessPendingHacks();

            PresetOverride::OnGameUpdate();

            update_gz_nav();
        }
    }
} // namespace gz::Utils
#pragma pack(pop)