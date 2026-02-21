#include "save_file.h"
#include <fstream>
#include <cstdio>
#include <cstring>

bool SaveFile::load(const std::string& path) {
    filePath_ = path;
    loaded_ = false;

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return false;

    auto fileSize = file.tellg();
    file.seekg(0);

    std::vector<uint8_t> fileData(fileSize);
    file.read(reinterpret_cast<char*>(fileData.data()), fileSize);
    file.close();

    // Save original file data for round-trip verification
    originalFileData_ = fileData;

    // Decrypt into SCBlocks (modifies fileData in-place)
    blocks_ = SwishCrypto::decrypt(fileData.data(), fileData.size());

    loaded_ = true;
    return true;
}

bool SaveFile::save(const std::string& path) {
    if (!loaded_)
        return false;

    std::vector<uint8_t> encrypted = SwishCrypto::encrypt(blocks_);

    // Open for in-place writing (r+b) to avoid truncating the file.
    // The Switch save filesystem journal can break if we truncate + rewrite.
    FILE* f = std::fopen(path.c_str(), "r+b");
    if (!f)
        f = std::fopen(path.c_str(), "wb");
    if (!f)
        return false;

    size_t written = std::fwrite(encrypted.data(), 1, encrypted.size(), f);
    std::fclose(f);
    return written == encrypted.size();
}

SCBlock* SaveFile::findBlock(uint32_t key) {
    return SwishCrypto::findBlock(blocks_, key);
}

bool SaveFile::replaceBlockData(uint32_t key, const std::vector<uint8_t>& data) {
    SCBlock* block = SwishCrypto::findBlock(blocks_, key);
    if (block) {
        // Block exists - replace its data
        if (block->type == SCTypeCode::None) {
            // Dummy/uninitialized block - set type to Object
            block->type = SCTypeCode::Object;
        }
        block->data = data;
        return true;
    }

    // Block not found - create a new one and append
    SCBlock newBlock{};
    newBlock.key = key;
    newBlock.type = SCTypeCode::Object;
    newBlock.data = data;
    blocks_.push_back(std::move(newBlock));
    return true;
}

std::string SaveFile::verifyRoundTrip() {
    if (originalFileData_.empty())
        return "No original data";

    std::vector<uint8_t> encrypted = SwishCrypto::encrypt(blocks_);

    std::string result;

    if (encrypted.size() != originalFileData_.size()) {
        result = "SIZE MISMATCH: encrypted=" + std::to_string(encrypted.size())
               + " original=" + std::to_string(originalFileData_.size());
    } else {
        size_t diffCount = 0;
        size_t firstDiff = 0;
        for (size_t i = 0; i < encrypted.size(); i++) {
            if (encrypted[i] != originalFileData_[i]) {
                if (diffCount == 0)
                    firstDiff = i;
                diffCount++;
            }
        }

        if (diffCount == 0) {
            result = "OK";
        } else {
            size_t hashStart = originalFileData_.size() - 32;
            bool onlyHashDiffers = true;
            for (size_t i = 0; i < hashStart; i++) {
                if (encrypted[i] != originalFileData_[i]) {
                    onlyHashDiffers = false;
                    break;
                }
            }

            char buf[256];
            if (onlyHashDiffers) {
                std::snprintf(buf, sizeof(buf),
                    "HASH ONLY: payload matches but hash differs");
            } else {
                std::snprintf(buf, sizeof(buf),
                    "DIFF: %zu bytes differ, first at 0x%zX",
                    diffCount, firstDiff);
            }
            result = buf;
        }
    }

    originalFileData_.clear();
    originalFileData_.shrink_to_fit();
    return result;
}
