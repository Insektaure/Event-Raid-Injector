#pragma once
#include "swish_crypto.h"
#include <vector>
#include <string>

class SaveFile {
public:
    bool load(const std::string& path);
    bool save(const std::string& path);

    bool isLoaded() const { return loaded_; }

    // Debug: verify encrypt(decrypt(file)) == file. Call right after load().
    std::string verifyRoundTrip();

    // Find a block by key. Returns nullptr if not found.
    SCBlock* findBlock(uint32_t key);

    // Replace block data for a given key. Creates block if not found.
    // Returns true on success.
    bool replaceBlockData(uint32_t key, const std::vector<uint8_t>& data);

private:
    std::vector<SCBlock> blocks_;
    std::string filePath_;
    bool loaded_ = false;
    std::vector<uint8_t> originalFileData_;
};
