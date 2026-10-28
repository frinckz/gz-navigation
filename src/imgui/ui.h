#pragma once

#include "imgui.h"

#include <Windows.h>


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


    private:
        UI() = default;
        ~UI() = default;

        bool m_initialized = false;

        void RenderGuesserInfo();
        void RenderGuesserControls();
        void RenderGuessResult();
        void RenderGuessOverlay();
        void RenderCompass();
        void RenderTime();
        void RenderCrosshair();
    };

} // namespace gz