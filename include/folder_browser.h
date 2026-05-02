#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

struct DirEntry {
    std::string name;
    bool isDirectory;
    bool isValidEvent;
};

class FolderBrowser {
public:
    using Validator = std::function<bool(const std::string&)>;

    // Open the folder. Validator is called for any uncached subfolder to check
    // if it contains a valid event. The cache file lives inside startPath, so
    // raid and outbreak folders keep separate caches naturally.
    void open(const std::string& startPath, Validator validator = {});

    const std::string& currentPath() const { return currentPath_; }
    const std::vector<DirEntry>& entries() const { return entries_; }
    int cursor() const { return cursor_; }
    int scrollOffset() const { return scrollOffset_; }

    void moveCursor(int delta);
    std::string selectedPath() const;  // Full path of cursor item
    std::string selectedName() const;  // Name of cursor item

    static void deleteCache(const std::string& eventsDir);

    static constexpr int VISIBLE_ROWS = 16;

private:
    std::string currentPath_;
    std::vector<DirEntry> entries_;
    int cursor_ = 0;
    int scrollOffset_ = 0;
    Validator validator_;

    static constexpr const char* CACHE_FILENAME = ".cache";

    void refresh();
    static std::unordered_map<std::string, bool> loadCache(const std::string& dir);
    static void saveCache(const std::string& dir, const std::unordered_map<std::string, bool>& cache);
};
