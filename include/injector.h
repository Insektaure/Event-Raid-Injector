#pragma once
#include "save_file.h"
#include <string>

struct InjectorResult {
    bool success;
    std::string message;
    std::string identifier;
};

namespace Injector {

// BCAT block keys from Tera-Finder DataBlocks.cs
constexpr uint32_t KEY_RAID_IDENTIFIER     = 0x37B99B4D;
constexpr uint32_t KEY_RAID_ENEMY_ARRAY    = 0x0520A1B0;
constexpr uint32_t KEY_FIXED_REWARD_ARRAY  = 0x7D6C2B82;
constexpr uint32_t KEY_LOTTERY_REWARD_ARRAY = 0xA52B4811;
constexpr uint32_t KEY_RAID_PRIORITY_ARRAY = 0x095451E4;

// Expected block sizes
constexpr size_t SIZE_RAID_IDENTIFIER     = 0x04;
constexpr size_t SIZE_RAID_ENEMY_ARRAY    = 0x7530;
constexpr size_t SIZE_FIXED_REWARD_ARRAY  = 0x6B40;
constexpr size_t SIZE_LOTTERY_REWARD_ARRAY = 0xD0D8;
constexpr size_t SIZE_RAID_PRIORITY_ARRAY = 0x58;

// Validate that a folder contains valid raid event files.
// Expects: {path}/Identifier.txt and {path}/Files/ with binary data files.
bool isValidRaidFolder(const std::string& path);

// Inject raid event data from folder into save file.
InjectorResult injectRaidEvent(SaveFile& save, const std::string& folderPath);

// Inject null/empty event data to clear existing raid events.
InjectorResult injectNullEvent(SaveFile& save);

} // namespace Injector
