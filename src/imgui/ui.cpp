#include "ui.h"
#include "log.h"

#include <algorithm>
#include <cmath>
#include <imgui.h>
#include "imgui_internal.h"
#include "game/camera_manager.h"
#include "game/custom/tp_state.h"
#include "game/game_state.h"
#include "game/input_manager.h"
#include "game/map.h"
#include "game/ui_static_handler.h"
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

    if (GameState::IsInGame() && !CMap::IsOpen()) {
        RenderCompass();
    }

    if (!GameState::IsInGame() || !CMap::IsOpen()) {
        return;
    }

    if (g_gzNavSettings.mode == GzNavGameMode::Guesser) {
        RenderGuesserInfo();
        RenderGuesserControls();
        if (GuesserGameMode::instance()->HasGuessed()) {
            RenderGuessOverlay();
            RenderGuessResult();
        }
    }
}

// The map is laid out on a 1280x720 stage (verified in-game at 1920x1080, where the stage is 1.5x the window).
// Whether the stage is letterboxed or stretched on other aspect ratios is unverified, letterboxed is assumed.
static constexpr float STAGE_W = 1280.0f;
static constexpr float STAGE_H = 720.0f;

// world position -> window pixels; map units are world / 32 (x, z), the stage position is pan + mapPos * scale
static ImVec2 WorldToWindow(const CMap& map, const CVector3f& world)
{
    const float stageX = map.panX + world.x / 32.0f * map.mapScale2;
    const float stageY = map.panY + world.z / 32.0f * map.mapScale2;
    const ImVec2 size = ImGui::GetIO().DisplaySize;
    const float k = std::min(size.x / STAGE_W, size.y / STAGE_H);
    return ImVec2((size.x - STAGE_W * k) * 0.5f + stageX * k, (size.y - STAGE_H * k) * 0.5f + stageY * k);
}

// gz_nav/assets/Target.svg (viewBox 0 0 20 20) with the group transforms applied: a flag and a pole (the SVG fill is
// rgb(235,235,235), the overlay uses green for the target). The flag polygon is concave, so it is drawn as two convex pieces.
static constexpr ImVec2 TARGET_FLAG_RIGHT[] = {{11.5446f, 5.0f}, {16.25f, 3.75f}, {16.25f, 11.5625f}, {11.5446f, 12.8125f}};
static constexpr ImVec2 TARGET_FLAG_LEFT[] = {{11.5446f, 5.0f}, {11.5446f, 12.8125f}, {6.5625f, 9.6875f}, {6.5625f, 1.875f}};
static constexpr ImVec2 TARGET_POLE_MIN{5.0f, 1.875f};
static constexpr ImVec2 TARGET_POLE_MAX{6.5625f, 18.125f};
static constexpr ImVec2 TARGET_ANCHOR{5.78125f, 18.125f}; // foot of the pole, sits on the target position

// the flag with its pole foot at anchorPos, size is the edge length of the 20x20 icon box in pixels
static void DrawTargetIcon(ImDrawList* dl, const ImVec2& anchorPos, float size, ImU32 fillColor)
{
    const float k = size / 20.0f;
    const ImVec2 origin(anchorPos.x - TARGET_ANCHOR.x * k, anchorPos.y - TARGET_ANCHOR.y * k);

    // the first pass is a dark drop shadow so the light flag stays readable on the map
    for (const ImVec2 offset : {ImVec2(1.5f, 1.5f), ImVec2(0.0f, 0.0f)}) {
        const bool shadow = offset.x != 0.0f;
        const ImU32 color = shadow ? IM_COL32(0, 0, 0, 170) : fillColor;
        const auto toWindow = [&](const ImVec2& p) {
            return ImVec2(origin.x + offset.x + p.x * k, origin.y + offset.y + p.y * k);
        };
        const auto fillPiece = [&](const ImVec2 (&piece)[4]) {
            const ImVec2 points[] = {toWindow(piece[0]), toWindow(piece[1]), toWindow(piece[2]), toWindow(piece[3])};
            dl->AddConvexPolyFilled(points, 4, color);
        };

        fillPiece(TARGET_FLAG_RIGHT);
        fillPiece(TARGET_FLAG_LEFT);
        dl->AddRectFilled(toWindow(TARGET_POLE_MIN), toWindow(TARGET_POLE_MAX), color);
    }
}

// after a guess: the guessed position as our own marker, the target as a rect and a line between them, below the
// ImGui windows. The guess is drawn here because the user's marker can be moved after guessing.
void UI::RenderGuessOverlay()
{
    const auto* map = CMap::instance();
    if (!map) {
        return;
    }

    const auto* guesser = GuesserGameMode::instance();
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    const ImVec2 markerPos = WorldToWindow(*map, guesser->GetGuessPosition());
    const ImVec2 targetPos = WorldToWindow(*map, guesser->GetTargetPosition());

    dl->AddLine(markerPos, targetPos, IM_COL32(0, 0, 0, 200), 5.0f);
    dl->AddLine(markerPos, targetPos, IM_COL32(255, 255, 255, 255), 2.0f);

    // white guess (dark outline for contrast), green target
    dl->AddCircleFilled(markerPos, 8.0f, IM_COL32(0, 0, 0, 200));
    dl->AddCircleFilled(markerPos, 6.0f, IM_COL32(255, 255, 255, 255));

    DrawTargetIcon(dl, targetPos, 28.0f, IM_COL32(60, 220, 90, 255));
}

// Heading of the render camera in degrees, 0 = north, clockwise. North is -z (the map y axis is z / 32 and grows
// southwards). The camera looks along its transform's -z axis (verified in-game: +z gave a heading
// that was off by 180 degrees).
static bool GetCameraHeading(float& headingDeg)
{
    static constexpr float CAMERA_FORWARD_SIGN = -1.0f;

    const auto* manager = CCameraManager::instance();
    if (!manager || !manager->m_RenderCamera) {
        return false;
    }
    const CVector4f& forward = manager->m_RenderCamera->m_TransformF.m[2];
    const float fx = forward.x * CAMERA_FORWARD_SIGN;
    const float fz = forward.z * CAMERA_FORWARD_SIGN;
    if (fx * fx + fz * fz < 1e-6f) {
        return false;
    }
    headingDeg = std::atan2(fx, -fz) * (180.0f / IM_PI);
    return true;
}

// Replacement for the game's compass strip (it disappears with the hidden HUD): heading band at the top center,
// without markers. Drawn only while the HUD is hidden, otherwise the game's own compass is visible.
void UI::RenderCompass()
{
    float heading = 0.0f;
    if (!g_gzNavSettings.showCompass || !g_hideHud || !GetCameraHeading(heading)) {
        return;
    }

    static constexpr const char* DIRECTIONS[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    static constexpr float VISIBLE_SPAN = 300.0f; // degrees across the band

    const ImVec2 display = ImGui::GetIO().DisplaySize;
    if (display.x <= 0.0f || display.y <= 0.0f) {
        return; // minimized / tabbed out: a zero font size trips an ImGui assert
    }
    const float scale = display.y / 1080.0f;
    const float width = display.x * 0.3f;
    const float height = 26.0f * scale;
    const float top = 14.0f * scale;
    const float centerX = display.x * 0.5f;
    const float pixelsPerDeg = width / VISIBLE_SPAN;
    const ImVec2 bandMin(centerX - width * 0.5f, top);
    const ImVec2 bandMax(centerX + width * 0.5f, top + height);

    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    // background fades out towards both ends
    const ImU32 bandColor = IM_COL32(0, 0, 0, 45);
    const ImU32 bandClear = IM_COL32(0, 0, 0, 0);
    dl->AddRectFilledMultiColor(bandMin, ImVec2(centerX, bandMax.y), bandClear, bandColor, bandColor, bandClear);
    dl->AddRectFilledMultiColor(ImVec2(centerX, bandMin.y), bandMax, bandColor, bandClear, bandClear, bandColor);
    dl->PushClipRect(bandMin, bandMax, true);

    ImFont* font = ImGui::GetFont();
    const float baseFontSize = ImGui::GetFontSize() * scale * 1.2f;

    // The band scrolls in whole pixels and the labels keep a fixed rounded spacing, so they move in lockstep. Placing
    // each label at heading * pixelsPerDeg directly makes labels near a pixel boundary flip between two positions.
    const float scrollPx = std::round(heading * pixelsPerDeg);

    // one label every 45 degrees
    const int firstStep = static_cast<int>(std::floor((heading - VISIBLE_SPAN * 0.5f) / 45.0f));
    const int lastStep = static_cast<int>(std::ceil((heading + VISIBLE_SPAN * 0.5f) / 45.0f));
    for (int step = firstStep; step <= lastStep; ++step) {
        const float x = std::round(centerX + step * 45.0f * pixelsPerDeg - scrollPx);

        // fade out towards the edges of the band
        const float edge = 1.0f - std::fabs(x - centerX) / (width * 0.5f);
        const float linearFade = std::clamp(edge * 1.5f, 0.0f, 1.0f);
        const float fade = linearFade * linearFade;

        const int index = (step % 8 + 8) % 8;
        const bool cardinal = index % 2 == 0;
        const float fontSize = cardinal ? baseFontSize * 1.15f : baseFontSize * 0.85f;
        const ImU32 color = IM_COL32(205, 205, 205, static_cast<int>((cardinal ? 255 : 150) * fade));
        const ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, DIRECTIONS[index]);
        dl->AddText(font, fontSize, ImVec2(std::round(x - textSize.x * 0.5f), std::round(top + (height - textSize.y) * 0.5f)), color,
                    DIRECTIONS[index]);
    }
    dl->PopClipRect();

    // current heading
    dl->AddLine(ImVec2(centerX, bandMin.y), ImVec2(centerX, bandMax.y), IM_COL32(205, 205, 205, 200), 1.5f * scale);
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
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    if (ImGui::Begin("##guess_result", nullptr, flags)) {
        ImGui::Text("ROUND #%ld", guesser->GetRound() + 1);
        ImGui::Text("%.0f M AWAY", guesser->GetRoundDistance());
        ImGui::Text("+%.0f POINTS", guesser->GetRoundScore());
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

} // namespace gz
