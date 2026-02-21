#include "folder_browser.h"
#include "injector.h"
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>
#include <fstream>
#include <cstdio>

std::unordered_map<std::string, bool> FolderBrowser::loadCache(const std::string& dir) {
    std::unordered_map<std::string, bool> cache;
    std::string path = dir + CACHE_FILENAME;
    std::ifstream file(path);
    if (!file.is_open())
        return cache;

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        auto tab = line.find('\t');
        if (tab == std::string::npos || tab == 0 || tab + 1 >= line.size())
            continue;

        std::string name = line.substr(0, tab);
        bool valid = (line[tab + 1] == '1');
        cache[name] = valid;
    }
    return cache;
}

void FolderBrowser::saveCache(const std::string& dir,
                              const std::unordered_map<std::string, bool>& cache) {
    std::string path = dir + CACHE_FILENAME;
    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open())
        return;

    for (const auto& [name, valid] : cache) {
        file << name << '\t' << (valid ? '1' : '0') << '\n';
    }
}

void FolderBrowser::deleteCache(const std::string& eventsDir) {
    std::string path = eventsDir;
    if (!path.empty() && path.back() != '/')
        path += '/';
    path += CACHE_FILENAME;
    std::remove(path.c_str());
}

void FolderBrowser::open(const std::string& startPath) {
    currentPath_ = startPath;
    if (!currentPath_.empty() && currentPath_.back() != '/')
        currentPath_ += '/';
    cursor_ = 0;
    scrollOffset_ = 0;
    refresh();
}

void FolderBrowser::refresh() {
    entries_.clear();

    DIR* dir = opendir(currentPath_.c_str());
    if (!dir)
        return;

    auto cache = loadCache(currentPath_);
    std::unordered_map<std::string, bool> newCache;
    bool cacheChanged = false;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_name[0] == '.')
            continue;

        std::string fullPath = currentPath_ + entry->d_name;
        struct stat st;
        if (stat(fullPath.c_str(), &st) != 0)
            continue;

        if (!S_ISDIR(st.st_mode))
            continue;

        DirEntry de;
        de.name = entry->d_name;
        de.isDirectory = true;

        auto it = cache.find(de.name);
        if (it != cache.end()) {
            de.isValidEvent = it->second;
        } else {
            de.isValidEvent = Injector::isValidRaidFolder(fullPath + "/");
            cacheChanged = true;
        }

        newCache[de.name] = de.isValidEvent;
        entries_.push_back(std::move(de));
    }
    closedir(dir);

    if (!cacheChanged && cache.size() != newCache.size())
        cacheChanged = true;

    if (cacheChanged)
        saveCache(currentPath_, newCache);

    std::sort(entries_.begin(), entries_.end(), [](const DirEntry& a, const DirEntry& b) {
        return a.name < b.name;
    });
}

void FolderBrowser::moveCursor(int delta) {
    if (entries_.empty())
        return;

    cursor_ += delta;
    if (cursor_ < 0)
        cursor_ = 0;
    if (cursor_ >= static_cast<int>(entries_.size()))
        cursor_ = static_cast<int>(entries_.size()) - 1;

    if (cursor_ < scrollOffset_)
        scrollOffset_ = cursor_;
    if (cursor_ >= scrollOffset_ + VISIBLE_ROWS)
        scrollOffset_ = cursor_ - VISIBLE_ROWS + 1;
}

std::string FolderBrowser::selectedPath() const {
    if (cursor_ < 0 || cursor_ >= static_cast<int>(entries_.size()))
        return "";
    return currentPath_ + entries_[cursor_].name + "/";
}

std::string FolderBrowser::selectedName() const {
    if (cursor_ < 0 || cursor_ >= static_cast<int>(entries_.size()))
        return "";
    return entries_[cursor_].name;
}
