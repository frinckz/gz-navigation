#pragma once

#include <cstdint>
#include "addresses.h"
#include "hook_helpers.h"

#pragma pack(push, 1)
namespace gz
{
    class CNotificationPopup;

    // type, param (e.g. location id), two const refs (usually an empty string), extra int
    using QueueNotificationFunc = void(*)(CNotificationPopup*, int32_t, int32_t, const void*, const void*, int32_t);
    inline QueueNotificationFunc g_origQueueNotification = nullptr;

    inline bool g_hideLocationPopups = true;
    inline bool g_hideXPPopups = true;

    struct NotificationType
    {
        static constexpr int32_t XP_REWARD = 0;             // "ShowXPReward"
        static constexpr int32_t LOCATION_DISCOVERED = 7;   // "ShowLocationDiscovered"
        static constexpr int32_t LOCATION_SUMMARY = 13;     // "ShowLocationSummary" (entering a location, shows collectibles)
    };

    class CNotificationPopup
    {
    public:
        static void HookedQueueNotification(CNotificationPopup* popup, int32_t type, int32_t param, const void* a, const void* b, int32_t c)
        {
            if (g_hideLocationPopups &&
                (type == NotificationType::LOCATION_DISCOVERED || type == NotificationType::LOCATION_SUMMARY))
                return;

            if (g_hideXPPopups && type == NotificationType::XP_REWARD)
                return;

            g_origQueueNotification(popup, type, param, a, b, c);
        }

        static bool SetupQueueNotificationHook()
        {
            if (g_origQueueNotification != nullptr) {
                return true;
            }
            return MH_CreateHookGZ(NOTIFICATION_POPUP_QUEUE, &HookedQueueNotification, &g_origQueueNotification);
        }
    };
} // namespace gz
#pragma pack(pop)
