#pragma once
#include <cstdint>

enum class GameType { S, V };

inline bool isSV(GameType) { return true; }

inline uint64_t titleIdOf(GameType g) {
    switch (g) {
        case GameType::S: return 0x0100A3D008C5C000;
        case GameType::V: return 0x01008F6008C5E000;
    }
    return 0;
}

inline const char* saveFileNameOf(GameType) {
    return "main";
}

inline const char* gameDisplayNameOf(GameType g) {
    switch (g) {
        case GameType::S: return "Pokemon Scarlet";
        case GameType::V: return "Pokemon Violet";
    }
    return "";
}

inline const char* gamePathNameOf(GameType g) {
    switch (g) {
        case GameType::S: return "Scarlet";
        case GameType::V: return "Violet";
    }
    return "Unknown";
}
