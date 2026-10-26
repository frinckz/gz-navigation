#include "ui.h"
#include "log.h"

#include <imgui.h>
#include "imgui_internal.h"
#include "game/custom/tp_state.h"
#include "game/game_state.h"
#include "game/input_manager.h"

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
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
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

void UI::SetVisible(bool visible)
{
    m_visible = visible;

    auto& io = ImGui::GetIO();
    if (visible)
        io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange; // let ImGui change cursor shapes
    else
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;  // hands cursor back to game

    if (auto* inputMgr = NInput::CManager::instance())
    {
        if (visible) inputMgr->LoseFocus();
        else inputMgr->GainFocus();
    }
}

void UI::Render()
{
    if (!m_initialized) {
        return;
    }

    // --- CONTROLLER TOGGLE LOGIC ---
    // check for L3 + R3 (Left Stick Click + Right Stick Click)
    static bool s_wasComboPressed = false;
    const bool isComboPressed = ImGui::IsKeyDown(ImGuiKey_GamepadL3) && ImGui::IsKeyDown(ImGuiKey_GamepadR3);

    // only trigger once per press to prevent flickering open / closed
    if (isComboPressed && !s_wasComboPressed) {
        SetVisible(!m_visible);
    }
    s_wasComboPressed = isComboPressed;
    // -------------------------------

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

    if (!m_visible) {
        return;
    }

    if (!GameState::IsInGame()) {
        SetVisible(false);
        return;
    }

    ImGui::Begin("Generation Zero Console Thingy");

    ImGui::End();
}

} // namespace gz
