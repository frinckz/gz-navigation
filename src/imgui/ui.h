#pragma once

#include "imgui.h"

#include <Windows.h>

#include "imgui_internal.h"
#include "game/player_network_manager.h"

namespace gz
{

    class UI
    {
    public:
        static UI* Get()
        {
            static UI instance;
            return &instance;
        }

        void Initialize();
        void SetupUiLook();
        void Shutdown();
        void Render();

        bool IsVisible() const { return m_visible; }
        void SetVisible(bool visible);
        void ToggleVisible() { SetVisible(!m_visible); }

    private:
        UI() = default;
        ~UI() = default;

        bool m_visible = false;
        bool m_initialized = false;
    };

} // namespace gz