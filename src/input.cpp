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

static constexpr float FONT_SIZE_INPUT = 0.0155f;

static constexpr int KEY_TOGGLE_UI     = VK_F1;
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

    const auto ui = gz::UI::Get();

    if (!ui->IsVisible()) {
        // draw input hint
        const char* hint = gz::NInput::g_isUsingGamepad ? "LS + RS" : "F1";

        Graphics::Get()->DrawString(
            context, hint, 0.0078f, 0.965f, FONT_SIZE_INPUT, 0x5EC9C9C9
        );
    }

    if (!ui->IsVisible() && g_gzNavSettings.mode == GzNavGameMode::Guesser) {
        auto guesser = GuesserGameMode::instance();
        auto round = guesser->GetRound();
        auto score = guesser->GetScore();

        std::stringstream infoStream = std::stringstream("");
        infoStream << "Round #" << round + 1 << ", Score " << score << " points";

        Graphics::Get()->DrawString(
            context, infoStream.str(), 0.0078f, 0.0078f, FONT_SIZE_INPUT, 0x5EF9F9F9
        );
    }
}

bool Input::FeedEvent(uint32_t message, WPARAM wParam, LPARAM lParam)
{
    // track active input device
    if (message == WM_KEYDOWN || message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN)
        gz::NInput::g_isUsingGamepad = false;

    gz::UI* ui = gz::UI::Get();

    // if UI is visible, block ALL game input except UI toggle
    if (ui->IsVisible()) {
        if (message == WM_KEYDOWN && (lParam >> 30) == 0)
        {
            // toggle UI off
            if (wParam == KEY_TOGGLE_UI) {
                ui->ToggleVisible();
                return true;
            }
        }
        // block everything else from reaching the game
        return true;
    }

    // ui is not visible, handle global hotkeys
    if (message == WM_KEYDOWN && (lParam >> 30) == 0)
    {
        // toggling UI
        if (wParam == KEY_TOGGLE_UI) {
            ui->ToggleVisible();
            return true;
        }

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

        // shortcut to teleport to a random saved position
        if (wParam == KEY_PLACE_POSITION)
        {
            // the user's custom map marker is the guess; without one the round is only advanced
            CVector3f marker{};
            if (gz::CMap::GetLocalUserWaypoint(marker)) {
                GuesserGameMode::instance()->Guess(marker);
                GuesserGameMode::instance()->AdvanceToNextRound();
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