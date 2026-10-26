#pragma once

#include <cstddef>

#include "addresses.h"
#include "data_types.h"
#include "hook_helpers.h"
#include "network_manager.h"
#include "ui_base.h"

#pragma pack(push, 1)
namespace gz
{
    // POI types (SMapIcon::m_Type)
    inline constexpr uint32_t POI_TYPE_LOCAL_PLAYER  = 0x62;
    inline constexpr uint32_t POI_TYPE_REMOTE_PLAYER = 0x63;

    struct SMapIcon
    {
        char        _pad0[0x18];
        CVector3f   m_Position;     // 0x18 world position
        CVector2f   m_MapPosition;  // 0x24 map position (world x/z / 32), sent to the UI by InitPOIs
        char        _pad1[0x04];    // 0x2C -> 0x30
        int64_t     m_IdHash;       // 0x30
        char        _pad2[0x10];    // 0x38 -> 0x48
        uint32_t    m_Type;         // 0x48 POI_TYPE_*
        char        _pad3[0x01];    // 0x4C
        uint8_t     m_TrackedByUser;   // 0x4D
        uint8_t     m_CanFastTravelTo; // 0x4E
        uint8_t     m_IsActive;     // 0x4F (UpdatePOI and cursor selection skip inactive icons)
        char        _pad4[0x08];    // 0x50 -> 0x58
    };
    static_assert(sizeof(SMapIcon) == 0x58);
    static_assert(offsetof(SMapIcon, m_Position) == 0x18);
    static_assert(offsetof(SMapIcon, m_MapPosition) == 0x24);
    static_assert(offsetof(SMapIcon, m_IdHash) == 0x30);
    static_assert(offsetof(SMapIcon, m_Type) == 0x48);
    static_assert(offsetof(SMapIcon, m_CanFastTravelTo) == 0x4E);
    static_assert(offsetof(SMapIcon, m_IsActive) == 0x4F);

    // user waypoint (custom map marker), filled by CMap::SetUserWaypoint
    struct SUserWaypointDesc
    {
        CVector3f   m_Position;     // 0x00 world position (y raised above the terrain)
        char        _pad0[0x44];    // 0x0C -> 0x50 (title + player name strings)
        int64_t     m_Id;           // 0x50
        char        _pad1[0x18];    // 0x58 -> 0x70
    };
    static_assert(sizeof(SUserWaypointDesc) == 0x70);
    static_assert(offsetof(SUserWaypointDesc, m_Id) == 0x50);

    // MSVC std::map<uint8_t peerId, SUserWaypointDesc> node
    struct SUserWaypointNode
    {
        SUserWaypointNode*  m_Left;     // 0x00
        SUserWaypointNode*  m_Parent;   // 0x08
        SUserWaypointNode*  m_Right;    // 0x10
        char                m_Color;    // 0x18
        char                m_IsNil;    // 0x19
        char                _pad0[0x6]; // 0x1A -> 0x20
        uint8_t             m_PeerId;   // 0x20 key
        char                _pad1[0x7]; // 0x21 -> 0x28
        SUserWaypointDesc   m_Desc;     // 0x28 value
    };
    static_assert(offsetof(SUserWaypointNode, m_IsNil) == 0x19);
    static_assert(offsetof(SUserWaypointNode, m_PeerId) == 0x20);
    static_assert(offsetof(SUserWaypointNode, m_Desc) == 0x28);

    class CMap;

    inline bool g_fastTravelAnywhereEnabled = true;
    inline bool g_hidePlayerMapIcon = true;
    inline bool g_disableMapFocusOnPlayer = true;

    using FocusOnLocalPlayerFunc_t = void(*)(CMap*, bool);
    inline FocusOnLocalPlayerFunc_t g_origFocusOnLocalPlayer = nullptr;
    using UpdatePOIFunc_t = void(*)(CMap*, uint32_t, CVector3f*, float, int64_t);
    inline UpdatePOIFunc_t g_origUpdatePOI = nullptr;
    using InitPOIsFunc_t = void(*)(CMap*);
    inline InitPOIsFunc_t g_origInitPOIs = nullptr;

    using IsAllowedInRegionFunc_t = bool(*)(CMap*, CVector3f*);
    inline IsAllowedInRegionFunc_t g_origIsAllowedInRegion = nullptr;
    using FastTravelFunc_t = void(*)(CMap*, void*);
    inline FastTravelFunc_t g_origFastTravel = nullptr;
    using OnManageInputIconsFunc_t = void(*)(CMap*, uint8_t*, void*);
    inline OnManageInputIconsFunc_t g_origOnManageInputIcons = nullptr;

    class CMap
    {
    public:
        void* vtable;           // 0x00
        char _pad0[8];          // 0x08
        void* vtable2;          // 0x10 (second vtable pointer)
        char _pad1[0x48];       // 0x18

        char _pad2[0x10];       // 0x60
        uint64_t field_70;      // 0x70
        uint32_t field_78;      // 0x78

        float mapCenterX;       // 0x7C
        float mapCenterY;       // 0x80
        float mapCenterZ;       // 0x84
        char _pad3[4];          // 0x88

        float cursorScreenX;    // 0x8C - screen X (0,0 at top-left corner)
        float cursorScreenY;    // 0x90 - screen Y (0,0 at top-left corner)
        char _pad4[8];          // 0x98

        float mapScale;         // 0x9C (initialized to 24.0f)
        float mapScale2;        // 0xA0 (initialized to 24.0f)

        char        _pad5a[0x7C];       // 0xA4 -> 0x120
        SMapIcon*   m_IconArrayBegin;   // 0x120
        SMapIcon*   m_IconArrayEnd;     // 0x128
        char        _pad5b[0xC8];       // 0x130 -> 0x1F8
        SUserWaypointNode* m_UserWaypointsHead; // 0x1F8 std::map<uint8_t peerId, SUserWaypointDesc> head
        size_t      m_UserWaypointsSize;        // 0x200
        char        _pad5c[0x4F0];      // 0x208 -> 0x6F8
        // total size: 0x6f8

        static CMap* instance()
        {
            return *(CMap**)(GetAddress(INST_MAP));
        }

        // reads the user waypoint (custom map marker) of the given player from the map's waypoint tree
        bool GetUserWaypoint(uint8_t peerId, CVector3f& out) const
        {
            if (!m_UserWaypointsHead) return false;
            const SUserWaypointNode* node = m_UserWaypointsHead->m_Parent; // root
            while (node && !node->m_IsNil)
            {
                if (peerId == node->m_PeerId)
                {
                    out = node->m_Desc.m_Position;
                    return true;
                }
                node = peerId < node->m_PeerId ? node->m_Left : node->m_Right;
            }
            return false;
        }

        // the local player's user waypoint; false if none is placed
        static bool GetLocalUserWaypoint(CVector3f& out)
        {
            auto* map = instance();
            auto* networkManager = CBaseNetworkManager::instance();
            if (!map || !networkManager) return false;
            return map->GetUserWaypoint(networkManager->GetLocalPeerID(), out);
        }


        // Map scene state (low dword of field_70). Observed in-game: 0 = closed, 1 = scene initialised after the game loaded,
        // 2 -> 3 within ~20 ms when the map is opened (3 = open), back to 0 when it is closed.
        static constexpr uint32_t MAP_STATE_OPEN = 3;

        [[nodiscard]] uint32_t GetMapState() const
        {
            return static_cast<uint32_t>(field_70 & 0xFFFFFFFFu);
        }

        // true while the map screen is open
        [[nodiscard]] static bool IsOpen()
        {
            const auto* map = instance();
            return map && map->GetMapState() == MAP_STATE_OPEN;
        }

        static CVector2f WorldToMapPosition(float worldX, float worldZ)
        {
            CVector2f mapPos{};
            meow_hook::func_call<void>(GetAddress(WORLD_TO_MAP_COORDS), worldX, worldZ, &mapPos);
            return mapPos;
        }


        // -- patch dlc map check
        static bool HookedIsAllowedInRegion(CMap* thisPtr, CVector3f* position)
        {
            return true;
        }

        static bool SetupIsAllowedInRegionHook()
        {
            if (g_origIsAllowedInRegion) return true;
            return MH_CreateHookGZ(MAP_ALLOWED_IN_REGION, &HookedIsAllowedInRegion, &g_origIsAllowedInRegion);
        }


        // -- patch map icon fast travel
        static void HookedOnManageInputIcons(CMap* thisPtr, uint8_t* iconInfo, void* param3)
        {
            if (g_fastTravelAnywhereEnabled && iconInfo)
            {
                iconInfo[8] = 1; // force "CanBeFastTravelledTo" true before the real function reads it
            }
            g_origOnManageInputIcons(thisPtr, iconInfo, param3);
        }

        static bool SetupOnManageInputIconsHook()
        {
            if (g_origOnManageInputIcons) return true;
            return MH_CreateHookGZ(MAP_ON_MANAGE_INPUT_ICONS, &HookedOnManageInputIcons, &g_origOnManageInputIcons);
        }


        // -- patch fast travel check
        static void HookedFastTravel(CMap* thisPtr, void* destination)
        {
            uint8_t* patchedFlag = nullptr;
            uint8_t savedValue = 0;

            if (g_fastTravelAnywhereEnabled)
            {
                int64_t targetHash = ((CUIBase*)destination)->GetUint64Field();
                if (targetHash != -1)
                {
                    for (auto* icon = thisPtr->m_IconArrayBegin; icon != thisPtr->m_IconArrayEnd; icon++)
                    {
                        if (icon->m_IdHash == targetHash)
                        {
                            patchedFlag = &icon->m_CanFastTravelTo;
                            savedValue = *patchedFlag;
                            *patchedFlag = 1;
                            break;
                        }
                    }
                }
            }

            g_origFastTravel(thisPtr, destination);

            if (patchedFlag)
            {
                *patchedFlag = savedValue; // restore -> don't permanently alter the icon's real state
            }
        }

        static bool SetupFastTravelHook()
        {
            if (g_origFastTravel) return true;
            return MH_CreateHookGZ(MAP_FAST_TRAVEL, &HookedFastTravel, &g_origFastTravel);
        }


        // -- stop centering the map on the player (map open, map reload and the [R] key)
        static void HookedFocusOnLocalPlayer(CMap* thisPtr, bool resetZoom)
        {
            if (g_disableMapFocusOnPlayer) return;
            g_origFocusOnLocalPlayer(thisPtr, resetZoom);
        }

        static bool SetupFocusOnLocalPlayerHook()
        {
            if (g_origFocusOnLocalPlayer) return true;
            return MH_CreateHookGZ(MAP_FOCUS_ON_LOCAL_PLAYER, &HookedFocusOnLocalPlayer, &g_origFocusOnLocalPlayer);
        }


        // -- hide the player icon by keeping it far outside the map
        static constexpr float OFF_MAP_WORLD = -100000.0f;

        // per frame: the player POI is moved to the player position
        static void HookedUpdatePOI(CMap* thisPtr, uint32_t type, CVector3f* worldPos, float rotation, int64_t id)
        {
            if (g_hidePlayerMapIcon && type == POI_TYPE_LOCAL_PLAYER)
            {
                CVector3f offMap{OFF_MAP_WORLD, worldPos ? worldPos->y : 0.0f, OFF_MAP_WORLD};
                g_origUpdatePOI(thisPtr, type, &offMap, rotation, id);
                return;
            }
            g_origUpdatePOI(thisPtr, type, worldPos, rotation, id);
        }

        static bool SetupUpdatePOIHook()
        {
            if (g_origUpdatePOI) return true;
            return MH_CreateHookGZ(MAP_UPDATE_POI, &HookedUpdatePOI, &g_origUpdatePOI);
        }

        // map open / zoom: all icons are sent to the UI with their stored map position
        static void HookedInitPOIs(CMap* thisPtr)
        {
            if (g_hidePlayerMapIcon)
            {
                for (auto* icon = thisPtr->m_IconArrayBegin; icon != thisPtr->m_IconArrayEnd; icon++)
                {
                    if (icon->m_Type != POI_TYPE_LOCAL_PLAYER) continue;
                    icon->m_Position.x = OFF_MAP_WORLD;
                    icon->m_Position.z = OFF_MAP_WORLD;
                    icon->m_MapPosition = {OFF_MAP_WORLD / 32.0f, OFF_MAP_WORLD / 32.0f};
                }
            }
            g_origInitPOIs(thisPtr);
        }

        static bool SetupInitPOIsHook()
        {
            if (g_origInitPOIs) return true;
            return MH_CreateHookGZ(MAP_INIT_POIS, &HookedInitPOIs, &g_origInitPOIs);
        }
    };
    static_assert(sizeof(CMap) == 0x6F8, "CMap size mismatch!");
    static_assert(offsetof(CMap, m_UserWaypointsHead) == 0x1F8, "m_UserWaypointsHead offset mismatch!");
    static_assert(offsetof(CMap, cursorScreenX) == 0x8C, "cursorWorldX offset mismatch!");
    static_assert(offsetof(CMap, cursorScreenY) == 0x90, "cursorWorldY offset mismatch!");
    static_assert(offsetof(CMap, mapScale) == 0x9C, "mapScale offset mismatch!");
} // namespace gz
#pragma pack(pop)