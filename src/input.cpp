#include <Windows.h>
#include <sstream>

#include "graphics.h"
#include "input.h"

#include "game/camera_collision_modifier.h"
#include "game/game_state.h"
#include "game/game_world.h"
#include "game/input_manager.h"
#include "game/map.h"
#include "game/ui_manager.h"
#include "imgui/ui.h"
#include "log.h"
#include "placer/guesser.h"
#include "placer/placer.h"
#include "placer/gz_nav.h"

static constexpr int KEY_TELEPORT_AIM  = VK_F2;
static constexpr int KEY_HIDE_HUD      = VK_F3;
static constexpr int KEY_THIRD_PERSON  = VK_F4;
static constexpr int KEY_SAVE_POSITION = VK_F5;
static constexpr int KEY_PLACE_POSITION = VK_F6;

void Input::Draw(ID3D11DeviceContext* context)
{
    if (!context) return;

    if (!gz::GameState::IsInGame()) {
        // don't draw anything if we are not in-game
        return;
    }
}

bool Input::FeedEvent(uint32_t message, WPARAM wParam, LPARAM lParam)
{
    // track active input device
    if (message == WM_KEYDOWN || message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN)
        gz::NInput::g_isUsingGamepad = false;

    // global hotkeys
    if (message == WM_KEYDOWN && (lParam >> 30) == 0)
    {
        // shortcut to teleport to aim position
        if (wParam == KEY_TELEPORT_AIM) {
            if (auto* gameWorld = gz::CGameWorld::instance())
            {
                gameWorld->TeleportToAimPosition();
                return true;
            }
        }

        // shortcut to hide/show HUD
        if (wParam == KEY_HIDE_HUD)
        {
            gz::CUIManager::SetHideHud(!gz::CUIManager::IsHudHidden());
            return true;
        }

        if (wParam == KEY_THIRD_PERSON)
        {
            gz::TpState::g_toggleRequested.store(true, std::memory_order_relaxed);
            return true;
        }

        // shortcut to save the current player position
        if (wParam == KEY_SAVE_POSITION)
        {
            auto* mgr = gz::CNetworkPlayerManager::instance();
            if (mgr && mgr->GetPlayer())
            {
                Placer::instance()->SavePosition(mgr->GetPlayer()->GetPositionVector(),
                                                 mgr->GetPlayer()->GetAimPosition());
                return true;
            }
        }

        // shortcut to guess / start the next round
        if (wParam == KEY_PLACE_POSITION)
        {
            // same as the map buttons: the user's map marker is the guess, a guessed round advances to the next one
            auto* guesser = GuesserGameMode::instance();
            if (guesser->HasGuessed()) {
                guesser->AdvanceToNextRound();
            } else {
                guesser->Guess();
            }
            return true;
        }
    }

    if (message == WM_MOUSEWHEEL)
    {
        auto* mgr = gz::CNetworkPlayerManager::instance();
        if (mgr && mgr->GetPlayer())
        {
            auto* character = mgr->GetPlayer()->GetCharacter();
            if (character && character->IsControllingEntity())
            {
                float delta = GET_WHEEL_DELTA_WPARAM(wParam);
                float normalizedDelta = delta / WHEEL_DELTA;
                gz::CCameraCollisionModifier::ChangeThirdPersonCameraDistance(-normalizedDelta * 0.05f);
                return true; // consume the event
            }
        }
    }

    // let other input pass through
    return false;
}