#include "ui.h"
#include "log.h"

#include <imgui.h>
#include "imgui_internal.h"
#include "game/custom/tp_state.h"
#include "game/game_state.h"
#include "game/input_manager.h"
#include "game/map.h"
#include "placer/guesser.h"
#include "placer/gz_nav.h"

namespace gz
{

void UI::Initialize()
{
    if (m_initialized) {
        return;
    }

    Log("Initializing ImGui...");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    io.IniFilename = nullptr; // no ini persistence

    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.MouseDrawCursor = false;

    SetupUiLook();

    m_initialized = true;
    Log("ImGui initialized successfully");
}

void UI::Shutdown()
{
    if (!m_initialized) {
        return;
    }

    Log("Shutting down ImGui");
    ImGui::DestroyContext();
    m_initialized = false;
}

void UI::SetupUiLook()
{
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();

    // query DPI from the game window (or fallback to desktop DPI)
    float dpiScale = 1.0f;
    dpiScale = GetDeviceCaps(GetDC(nullptr), LOGPIXELSX) / 96.0f;
    style.FontSizeBase = std::round(16.0f * dpiScale); // default font, e.g. 32px on 4K

    // colours
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_WindowBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.89f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.12f, 0.12f, 0.12f, 0.45f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.98f, 0.26f, 0.26f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.98f, 0.26f, 0.26f, 0.67f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.396f, 0.012f, 0.012f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.98f, 0.26f, 0.26f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.88f, 0.24f, 0.24f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.98f, 0.26f, 0.26f, 0.40f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.98f, 0.26f, 0.26f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.98f, 0.06f, 0.06f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(1.00f, 0.33f, 0.33f, 0.17f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.98f, 0.26f, 0.26f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.98f, 0.26f, 0.26f, 1.00f);
    colors[ImGuiCol_Separator]              = ImVec4(0.50f, 0.43f, 0.43f, 0.50f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.75f, 0.10f, 0.10f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.75f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.98f, 0.26f, 0.26f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.98f, 0.26f, 0.26f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.98f, 0.26f, 0.26f, 0.95f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.98f, 0.26f, 0.26f, 0.80f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.58f, 0.18f, 0.18f, 0.86f);
    colors[ImGuiCol_TabSelected]            = ImVec4(0.68f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_TabSelectedOverline]    = ImVec4(0.98f, 0.26f, 0.26f, 1.00f);
    colors[ImGuiCol_TabDimmed]              = ImVec4(0.15f, 0.07f, 0.07f, 0.97f);
    colors[ImGuiCol_TabDimmedSelected]      = ImVec4(0.42f, 0.14f, 0.14f, 1.00f);

    // rounding
    style.FrameRounding = 4.0f;
    style.WindowRounding = 6.0f;

    style.WindowPadding     = ImVec2(14.0f, 12.0f);
    style.FramePadding      = ImVec2( 8.0f,  5.0f);
    style.ItemSpacing       = ImVec2(10.0f,  7.0f);
    style.ItemInnerSpacing  = ImVec2( 7.0f,  5.0f);
    style.CellPadding       = ImVec2( 6.0f,  4.0f);
    style.IndentSpacing     = 20.0f;
    style.ScrollbarSize     = 12.0f;
    style.GrabMinSize       = 12.0f;

    style.ScaleAllSizes(0.75f * dpiScale); // scales padding, grab sizes, etc. proportionally
    style.MouseCursorScale = dpiScale;
}

void UI::Render()
{
    if (!m_initialized) {
        return;
    }

    // --- THIRD PERSON CONTROLLER LOGIC ---
    static float s_lastR3PressTime = -999.f;

    if (ImGui::IsKeyPressed(ImGuiKey_GamepadR3)) {
        const float now = ImGui::GetTime();
        if (now - s_lastR3PressTime < 0.35f) {
            TpState::g_toggleRequested.store(true, std::memory_order_relaxed);
        }
        s_lastR3PressTime = now;
    }
    // -------------------------------------

    // log the raw map state on every change so the open/close detection can be verified in-game
    static uint32_t s_lastMapState = UINT32_MAX;
    if (const auto* map = CMap::instance(); map && map->GetMapState() != s_lastMapState) {
        s_lastMapState = map->GetMapState();
        Log("UI: map state changed to %u", s_lastMapState);
    }

    if (!GameState::IsInGame() || !CMap::IsOpen()) {
        return;
    }

    if (g_gzNavSettings.mode == GzNavGameMode::Guesser) {
        RenderGuesserInfo();
        RenderGuesserControls();
        if (GuesserGameMode::instance()->HasGuessed()) {
            RenderGuessResult();
        }
    }
}

// borderless layer without background, only the widgets are visible
static constexpr ImGuiWindowFlags OVERLAY_FLAGS = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground
    | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing
    | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove;

// round and score, top left corner of the map
void UI::RenderGuesserInfo()
{
    const ImGuiStyle& style = ImGui::GetStyle();
    ImGui::SetNextWindowPos(ImVec2(style.WindowPadding.x, style.WindowPadding.y));

    if (ImGui::Begin("##guesser_info", nullptr, OVERLAY_FLAGS | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav)) {
        const auto* guesser = GuesserGameMode::instance();
        ImGui::Text("ROUND #%ld", guesser->GetRound() + 1);
        ImGui::Text("SCORE %.0f POINTS", guesser->GetScore());
    }
    ImGui::End();
}

// sharp cornered buttons, black text on white
struct ButtonStyle
{
    ButtonStyle()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.85f, 0.85f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.70f, 0.70f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    }
    ~ButtonStyle()
    {
        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar(2);
    }
};

// bottom left corner of the map: go back to the target and guess, after a guess only the next round button
// gamepad: X resets to the target, Y guesses / starts the next round
void UI::RenderGuesserControls()
{
    auto* guesser = GuesserGameMode::instance();
    const ImGuiIO& io = ImGui::GetIO();
    const ImGuiStyle& style = ImGui::GetStyle();
    ImGui::SetNextWindowPos(ImVec2(style.WindowPadding.x, io.DisplaySize.y - style.WindowPadding.y),
                            ImGuiCond_Always, ImVec2(0.0f, 1.0f));

    if (ImGui::Begin("##guesser_controls", nullptr, OVERLAY_FLAGS)) {
        const bool usingGamepad = NInput::g_isUsingGamepad;
        bool guessHovered = false;

        {
            ButtonStyle buttonStyle;

            if (guesser->HasGuessed()) {
                if (ImGui::Button(usingGamepad ? "NEXT ROUND [Y]" : "NEXT ROUND")
                    || ImGui::IsKeyPressed(ImGuiKey_GamepadFaceUp, false)) {
                    guesser->AdvanceToNextRound();
                }
            } else {
                if (ImGui::Button(usingGamepad ? "BACK TO TARGET [X]" : "BACK TO TARGET")
                    || ImGui::IsKeyPressed(ImGuiKey_GamepadFaceLeft, false)) {
                    guesser->ResetToTarget();
                }
                ImGui::SameLine();
                ImGui::BeginDisabled(!guesser->CanGuess());
                if (ImGui::Button(usingGamepad ? "GUESS [Y]" : "GUESS")
                    || (guesser->CanGuess() && ImGui::IsKeyPressed(ImGuiKey_GamepadFaceUp, false))) {
                    guesser->Guess();
                }
                ImGui::EndDisabled();
                guessHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);
            }
        }

        // outside of the button style so the tooltip keeps its default colors
        if (guessHovered && !guesser->CanGuess()) {
            ImGui::SetTooltip("PLACE A MAP MARKER TO GUESS");
        }
    }
    ImGui::End();
}

// score of the guessed round, centered
void UI::RenderGuessResult()
{
    const auto* guesser = GuesserGameMode::instance();
    const ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always,
                            ImVec2(0.5f, 0.5f));

    constexpr ImGuiWindowFlags flags = (OVERLAY_FLAGS & ~ImGuiWindowFlags_NoBackground)
        | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav;
    if (ImGui::Begin("##guess_result", nullptr, flags)) {
        ImGui::Text("ROUND #%ld", guesser->GetRound() + 1);
        ImGui::Text("%.0f M AWAY", guesser->GetRoundDistance());
        ImGui::Text("+%.0f POINTS", guesser->GetRoundScore());
    }
    ImGui::End();
}

} // namespace gz
