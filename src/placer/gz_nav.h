#pragma once
#include "guesser.h"
#include "log.h"
#include "game/game_state.h"
#include "game/custom/no_enemies.h"
#include "game/player_network_manager.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <libloaderapi.h>
#include <minwindef.h>
#include <sstream>
#include <string>

inline std::string GetSettingsPath() {
    char modulePath[MAX_PATH];
    GetModuleFileNameA(nullptr, modulePath, MAX_PATH);
    return (std::filesystem::path(modulePath).parent_path() / "gz_nav" / "settings.ini").string();
}

enum class GzNavGameMode {
    Normal,
    Guesser,
    Extraction
};

struct gz_nav_settings {
    GzNavGameMode mode;
    bool showCompass = true;
    bool showTime = true;
    bool showCrosshair = true;
    // -1 = auto (off in normal mode, on in guesser mode), 0 = off, 1 = on
    int noEnemiesSetting = -1;
    bool noEnemies = false;
    // -1 = auto (off in normal mode, on in guesser mode), 0 = off, 1 = on
    int unnoticedSetting = -1;
    bool unnoticed = false;
};

inline gz_nav_settings g_gzNavSettings{};
inline bool g_gzNavStarted = false;

// Runs while the DLL loads: only reads the settings, the game world doesn't exist yet
inline void init_gz_nav() {
    gz::Log("GZ_NAV: Initializing...");
    gz_nav_settings& settings = g_gzNavSettings;

    std::ifstream file(GetSettingsPath());
    std::string line;
    size_t lineNumber = 0;
    while (std::getline(file, line)) {
        lineNumber++;
        // everything after '#' is a comment
        line = line.substr(0, line.find('#'));
        if (line.find_first_not_of(" \t\r") == std::string::npos) {
            continue;
        }
        std::replace_if(line.begin(), line.end(), [](char c) { return c == '='; }, ' ');
        std::istringstream stream(line);
        std::string key;
        std::string value;
        if (stream >> key >> value) {
            if (key == "mode") {
                int modeRaw = atoi(value.c_str());
                switch (modeRaw) {
                    case static_cast<int>(GzNavGameMode::Guesser): {
                        settings.mode = GzNavGameMode::Guesser;
                        break;
                    }
                    case static_cast<int>(GzNavGameMode::Extraction): {
                        settings.mode = GzNavGameMode::Extraction;
                        break;
                    }
                    default: {
                        settings.mode = GzNavGameMode::Normal;
                        break;
                    }
                }
            } else if (key == "show-compass") {
                settings.showCompass = atoi(value.c_str()) != 0;
            } else if (key == "show-time") {
                settings.showTime = atoi(value.c_str()) != 0;
            } else if (key == "show-crosshair") {
                settings.showCrosshair = atoi(value.c_str()) != 0;
            } else if (key == "enemies-ignore-player") {
                settings.unnoticedSetting = (value == "auto") ? -1 : (atoi(value.c_str()) != 0 ? 1 : 0);
            } else if (key == "no-enemies") {
                settings.noEnemiesSetting = (value == "auto") ? -1 : (atoi(value.c_str()) != 0 ? 1 : 0);
            }
        } else {
            gz::Log("GZ_NAV: ignoring invalid line %zu in %s", lineNumber, GetSettingsPath().c_str());
        }
    }

    // mode is only known after the whole file is read
    settings.noEnemies = settings.noEnemiesSetting >= 0 ? settings.noEnemiesSetting == 1
                                                        : settings.mode == GzNavGameMode::Guesser;
    gz::NoEnemies::SetBlockSpawns(settings.noEnemies);
    settings.unnoticed = settings.unnoticedSetting >= 0 ? settings.unnoticedSetting == 1
                                                        : settings.mode == GzNavGameMode::Guesser;

    gz::Log("GZ_NAV: INITIALISED! (mode %d)", settings.mode);
}

// Runs every game update tick: starts the configured mode once the player is in the world
inline void update_gz_nav() {
    if (!gz::GameState::IsInGame()) {
        return;
    }
    auto* playerManager = gz::CNetworkPlayerManager::instance();
    if (!playerManager || !playerManager->GetPlayer() || !playerManager->GetCharacter()) {
        return;
    }

    // machines ignore the player; the game resets this on respawn or reload, so it is applied every tick
    if (g_gzNavSettings.unnoticed) {
        gz::CCharacter* character = playerManager->GetCharacter();
        if (character->IsDetectable()) {
            character->SetDetectable(false);
        }
    }
    if (g_gzNavSettings.mode == GzNavGameMode::Guesser) {
        // nothing else to update
    } else if (g_gzNavSettings.mode == GzNavGameMode::Extraction) {
        // TODO: implement update
    }

    if (g_gzNavStarted) {
        return;
    }
    // only once per game launch, loading another save keeps the running mode
    g_gzNavStarted = true;

    switch (g_gzNavSettings.mode) {
        case GzNavGameMode::Guesser: {
            gz::Log("GZ_NAV: Starting guesser game mode");
            GuesserGameMode::instance()->Start();
            break;
        }
        case GzNavGameMode::Extraction: {
            gz::Log("GZ_NAV: Extraction not implemented");
            break;
        }
        default: {
            break;
        }
    }
}