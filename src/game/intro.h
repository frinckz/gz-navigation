#pragma once

#include "addresses.h"
#include "hook_helpers.h"

#pragma pack(push, 1)
namespace gz::Intro
{
    using introCompleteFunc = bool(*)(void* cTitleUi);
    inline introCompleteFunc g_introComplete = nullptr;

    inline bool HookedIntroComplete(void* cTitleUi)
    {
#ifdef GZ_SKIP_INTRO // defined for Debug configs only (see CMakeLists.txt)
        return true;
#else
        return g_introComplete(cTitleUi);
#endif
    }

    inline bool SetupIntroComplete()
    {
        if (g_introComplete != nullptr) {
            return true; // already set up
        }
        return MH_CreateHookGZ(FUNC_INTRO_COMPLETE, &HookedIntroComplete, &g_introComplete);
    }
} // namespace gz::Intro
#pragma pack(pop)