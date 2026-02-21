#include "injector.h"
#include <fstream>
#include <sys/stat.h>
#include <cstring>

namespace Injector {

// Version suffixes to try, in order (newest first)
static const char* VERSION_SUFFIXES[] = { "_3_0_0", "_2_0_0", "_1_3_0", "" };
static constexpr int NUM_SUFFIXES = 4;

static bool fileExists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

// Try to find a file with version fallback. Returns full path or empty string.
static std::string findVersionedFile(const std::string& dir, const char* baseName) {
    for (int i = 0; i < NUM_SUFFIXES; i++) {
        std::string path = dir + baseName + VERSION_SUFFIXES[i];
        if (fileExists(path))
            return path;
    }
    return "";
}

static std::vector<uint8_t> readBinaryFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return {};

    auto size = file.tellg();
    file.seekg(0);

    std::vector<uint8_t> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);
    return data;
}

static std::string readTextFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
        return "";

    std::string content;
    std::getline(file, content);
    // Trim trailing whitespace/newlines
    while (!content.empty() && (content.back() == '\r' || content.back() == '\n' || content.back() == ' '))
        content.pop_back();
    return content;
}

bool isValidRaidFolder(const std::string& path) {
    // Check Identifier.txt
    if (!fileExists(path + "Identifier.txt"))
        return false;

    std::string filesDir = path + "Files/";

    // Check all 5 required binary files (with version fallback)
    if (findVersionedFile(filesDir, "event_raid_identifier").empty())
        return false;
    if (findVersionedFile(filesDir, "raid_enemy_array").empty())
        return false;
    if (findVersionedFile(filesDir, "fixed_reward_item_array").empty())
        return false;
    if (findVersionedFile(filesDir, "lottery_reward_item_array").empty())
        return false;
    if (findVersionedFile(filesDir, "raid_priority_array").empty())
        return false;

    return true;
}

InjectorResult injectRaidEvent(SaveFile& save, const std::string& folderPath) {
    InjectorResult result{};

    if (!save.isLoaded()) {
        result.message = "Save file not loaded";
        return result;
    }

    // Ensure trailing slash
    std::string path = folderPath;
    if (!path.empty() && path.back() != '/')
        path += '/';

    // Read identifier
    result.identifier = readTextFile(path + "Identifier.txt");
    if (result.identifier.empty()) {
        result.message = "Failed to read Identifier.txt";
        return result;
    }

    std::string filesDir = path + "Files/";

    // Find and read all binary files with version fallback
    struct FileSpec {
        const char* baseName;
        uint32_t blockKey;
    };
    static const FileSpec specs[] = {
        { "event_raid_identifier",    KEY_RAID_IDENTIFIER },
        { "raid_enemy_array",         KEY_RAID_ENEMY_ARRAY },
        { "fixed_reward_item_array",  KEY_FIXED_REWARD_ARRAY },
        { "lottery_reward_item_array", KEY_LOTTERY_REWARD_ARRAY },
        { "raid_priority_array",      KEY_RAID_PRIORITY_ARRAY },
    };

    for (const auto& spec : specs) {
        std::string filePath = findVersionedFile(filesDir, spec.baseName);
        if (filePath.empty()) {
            result.message = std::string("Missing file: ") + spec.baseName;
            return result;
        }

        std::vector<uint8_t> data = readBinaryFile(filePath);
        if (data.empty()) {
            result.message = std::string("Failed to read: ") + spec.baseName;
            return result;
        }

        if (!save.replaceBlockData(spec.blockKey, data)) {
            result.message = std::string("Failed to inject block: ") + spec.baseName;
            return result;
        }
    }

    result.success = true;
    result.message = "Successfully imported event [" + result.identifier + "]";
    return result;
}

InjectorResult injectNullEvent(SaveFile& save) {
    InjectorResult result{};

    if (!save.isLoaded()) {
        result.message = "Save file not loaded";
        return result;
    }

    struct NullSpec {
        uint32_t key;
        size_t size;
    };
    static const NullSpec specs[] = {
        { KEY_RAID_IDENTIFIER,     SIZE_RAID_IDENTIFIER },
        { KEY_RAID_ENEMY_ARRAY,    SIZE_RAID_ENEMY_ARRAY },
        { KEY_FIXED_REWARD_ARRAY,  SIZE_FIXED_REWARD_ARRAY },
        { KEY_LOTTERY_REWARD_ARRAY, SIZE_LOTTERY_REWARD_ARRAY },
        { KEY_RAID_PRIORITY_ARRAY, SIZE_RAID_PRIORITY_ARRAY },
    };

    for (const auto& spec : specs) {
        std::vector<uint8_t> nullData(spec.size, 0);
        if (!save.replaceBlockData(spec.key, nullData)) {
            result.message = "Failed to clear raid block";
            return result;
        }
    }

    result.success = true;
    result.identifier = "Null";
    result.message = "Successfully cleared raid event data";
    return result;
}

} // namespace Injector
