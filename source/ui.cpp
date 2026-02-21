#include "ui.h"
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <string>

#ifndef APP_VERSION
#define APP_VERSION "1.0.0"
#endif
#ifndef APP_AUTHOR
#define APP_AUTHOR "Insektaure"
#endif

#include <switch.h>
#include <SDL2/SDL_image.h>

// ============================================================================
// Init / Shutdown
// ============================================================================

bool UI::init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0)
        return false;
    if (TTF_Init() < 0)
        return false;
    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);

    window_ = SDL_CreateWindow("Event Raid Injector",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_W, SCREEN_H, SDL_WINDOW_SHOWN);
    if (!window_) return false;

    renderer_ = SDL_CreateRenderer(window_, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) return false;
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);

    // Load system fonts
    PlFontData fontData;
    plInitialize(PlServiceType_User);
    plGetSharedFontByType(&fontData, PlSharedFontType_Standard);
    SDL_RWops* rw18 = SDL_RWFromMem(fontData.address, fontData.size);
    SDL_RWops* rw14 = SDL_RWFromMem(fontData.address, fontData.size);
    SDL_RWops* rw28 = SDL_RWFromMem(fontData.address, fontData.size);
    font_      = TTF_OpenFontRW(rw18, 1, 18);
    fontSmall_ = TTF_OpenFontRW(rw14, 1, 14);
    fontLarge_ = TTF_OpenFontRW(rw28, 1, 28);

    if (!font_ || !fontSmall_ || !fontLarge_)
        return false;

    // Open first gamepad
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            pad_ = SDL_GameControllerOpen(i);
            break;
        }
    }

    return true;
}

void UI::shutdown() {
    for (auto& [k, tex] : gameIconCache_) {
        if (tex) SDL_DestroyTexture(tex);
    }
    gameIconCache_.clear();
    account_.freeTextures();

    if (fontLarge_) TTF_CloseFont(fontLarge_);
    if (fontSmall_) TTF_CloseFont(fontSmall_);
    if (font_)      TTF_CloseFont(font_);
    if (pad_) SDL_GameControllerClose(pad_);
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_) SDL_DestroyWindow(window_);

    IMG_Quit();
    plExit();
    TTF_Quit();
    SDL_Quit();
}

// ============================================================================
// Splash Screen
// ============================================================================

void UI::showSplash() {
    if (!renderer_) return;

#ifdef __SWITCH__
    const char* splashPath = "romfs:/splash.png";
#else
    const char* splashPath = "romfs/splash.png";
#endif

    SDL_Surface* surf = IMG_Load(splashPath);
    if (!surf) return;

    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer_, surf);
    SDL_FreeSurface(surf);
    if (!tex) return;

    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

    int texW, texH;
    SDL_QueryTexture(tex, nullptr, nullptr, &texW, &texH);

    // Scale to fit screen while preserving aspect ratio
    float scale = std::min(static_cast<float>(SCREEN_W) / texW,
                           static_cast<float>(SCREEN_H) / texH);
    int dstW = static_cast<int>(texW * scale);
    int dstH = static_cast<int>(texH * scale);
    SDL_Rect dst = {(SCREEN_W - dstW) / 2, (SCREEN_H - dstH) / 2, dstW, dstH};

    // Hold splash for ~2.5 seconds
    Uint32 holdMs = 2500;
    Uint32 start = SDL_GetTicks();
    while (SDL_GetTicks() - start < holdMs) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                SDL_DestroyTexture(tex);
                return;
            }
        }
        SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
        SDL_RenderClear(renderer_);
        SDL_RenderCopy(renderer_, tex, nullptr, &dst);
        SDL_RenderPresent(renderer_);
        SDL_Delay(16);
    }

    // Fade out over ~0.5 seconds
    Uint32 fadeMs = 500;
    start = SDL_GetTicks();
    while (true) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                SDL_DestroyTexture(tex);
                return;
            }
        }
        Uint32 elapsed = SDL_GetTicks() - start;
        if (elapsed >= fadeMs)
            break;
        int alpha = 255 - static_cast<int>(255 * elapsed / fadeMs);
        SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
        SDL_RenderClear(renderer_);
        SDL_SetTextureAlphaMod(tex, static_cast<Uint8>(alpha));
        SDL_RenderCopy(renderer_, tex, nullptr, &dst);
        SDL_RenderPresent(renderer_);
        SDL_Delay(16);
    }

    SDL_DestroyTexture(tex);
}

// ============================================================================
// Drawing Primitives
// ============================================================================

void UI::drawRect(int x, int y, int w, int h, SDL_Color c) {
    SDL_SetRenderDrawColor(renderer_, c.r, c.g, c.b, c.a);
    SDL_Rect r = {x, y, w, h};
    SDL_RenderFillRect(renderer_, &r);
}

void UI::drawRectOutline(int x, int y, int w, int h, SDL_Color c, int thickness) {
    SDL_SetRenderDrawColor(renderer_, c.r, c.g, c.b, c.a);
    for (int t = 0; t < thickness; t++) {
        SDL_Rect r = {x + t, y + t, w - 2*t, h - 2*t};
        SDL_RenderDrawRect(renderer_, &r);
    }
}

void UI::drawText(TTF_Font* f, const char* text, int x, int y, SDL_Color c) {
    if (!text || !text[0]) return;
    SDL_Surface* surf = TTF_RenderUTF8_Blended(f, text, c);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer_, surf);
    SDL_Rect dst = {x, y, surf->w, surf->h};
    SDL_RenderCopy(renderer_, tex, nullptr, &dst);
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
}

void UI::drawTextCentered(TTF_Font* f, const char* text, int cx, int cy, SDL_Color c) {
    if (!text || !text[0]) return;
    int tw, th;
    TTF_SizeUTF8(f, text, &tw, &th);
    drawText(f, text, cx - tw/2, cy - th/2, c);
}

void UI::drawStatusBar(const char* text, const char* rightText) {
    drawRect(0, SCREEN_H - 35, SCREEN_W, 35, T().statusBarBg);
    drawText(fontSmall_, text, 15, SCREEN_H - 26, T().statusText);
    if (rightText && rightText[0]) {
        int tw = 0, th = 0;
        TTF_SizeUTF8(fontSmall_, rightText, &tw, &th);
        drawText(fontSmall_, rightText, SCREEN_W - tw - 15, SCREEN_H - 26, T().goldLabel);
    }
}

// ============================================================================
// Dialogs
// ============================================================================

void UI::showMessageAndWait(const char* title, const char* body) {
    bool waiting = true;
    while (waiting) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) { waiting = false; break; }
            if (ev.type == SDL_CONTROLLERBUTTONDOWN) {
                auto btn = ev.cbutton.button;
                if (btn == SDL_CONTROLLER_BUTTON_A || btn == SDL_CONTROLLER_BUTTON_B)
                    waiting = false;
            }
            if (ev.type == SDL_KEYDOWN) {
                if (ev.key.keysym.sym == SDLK_RETURN || ev.key.keysym.sym == SDLK_ESCAPE)
                    waiting = false;
            }
        }

        SDL_SetRenderDrawColor(renderer_, T().bg.r, T().bg.g, T().bg.b, 255);
        SDL_RenderClear(renderer_);
        drawRect(0, 0, SCREEN_W, SCREEN_H, T().overlay);

        drawTextCentered(fontLarge_, title, SCREEN_W/2, SCREEN_H/2 - 60, T().red);
        drawTextCentered(font_, body, SCREEN_W/2, SCREEN_H/2, T().text);
        drawTextCentered(fontSmall_, "Press A or B to continue", SCREEN_W/2, SCREEN_H/2 + 50, T().textDim);

        SDL_RenderPresent(renderer_);
        SDL_Delay(16);
    }
}

bool UI::showConfirmDialog(const char* title, const char* body) {
    bool waiting = true;
    int result = 0;
    while (waiting) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) { waiting = false; break; }
            if (ev.type == SDL_CONTROLLERBUTTONDOWN) {
                if (ev.cbutton.button == SDL_CONTROLLER_BUTTON_B) { result = 1; waiting = false; } // A on Switch
                if (ev.cbutton.button == SDL_CONTROLLER_BUTTON_A) { result = 0; waiting = false; } // B on Switch
            }
            if (ev.type == SDL_KEYDOWN) {
                if (ev.key.keysym.sym == SDLK_RETURN || ev.key.keysym.sym == SDLK_a) { result = 1; waiting = false; }
                if (ev.key.keysym.sym == SDLK_ESCAPE || ev.key.keysym.sym == SDLK_b) { result = 0; waiting = false; }
            }
        }

        SDL_SetRenderDrawColor(renderer_, T().bg.r, T().bg.g, T().bg.b, 255);
        SDL_RenderClear(renderer_);
        drawRect(0, 0, SCREEN_W, SCREEN_H, T().overlay);

        drawTextCentered(fontLarge_, title, SCREEN_W/2, SCREEN_H/2 - 60, T().red);
        drawTextCentered(font_, body, SCREEN_W/2, SCREEN_H/2, T().text);
        drawTextCentered(fontSmall_, "A: Confirm   B: Cancel", SCREEN_W/2, SCREEN_H/2 + 50, T().textDim);

        SDL_RenderPresent(renderer_);
        SDL_Delay(16);
    }
    return result == 1;
}

void UI::showWorking(const char* msg) {
    SDL_SetRenderDrawColor(renderer_, T().bg.r, T().bg.g, T().bg.b, 255);
    SDL_RenderClear(renderer_);
    drawRect(0, 0, SCREEN_W, SCREEN_H, T().overlay);

    int pw = 400, ph = 120;
    int px = (SCREEN_W - pw) / 2, py = (SCREEN_H - ph) / 2;
    drawRect(px, py, pw, ph, T().panelBg);
    drawRectOutline(px, py, pw, ph, T().popupBorder, 2);
    drawTextCentered(font_, msg, SCREEN_W/2, SCREEN_H/2, T().text);

    SDL_RenderPresent(renderer_);
}

// ============================================================================
// Joystick
// ============================================================================

void UI::updateStick(int16_t axisX, int16_t axisY) {
    int newDirX = 0, newDirY = 0;
    if (axisX < -STICK_DEADZONE) newDirX = -1;
    else if (axisX > STICK_DEADZONE) newDirX = 1;
    if (axisY < -STICK_DEADZONE) newDirY = -1;
    else if (axisY > STICK_DEADZONE) newDirY = 1;

    if (newDirX != stickDirX_ || newDirY != stickDirY_) {
        stickDirX_ = newDirX;
        stickDirY_ = newDirY;
        stickMoved_ = false;
        stickMoveTime_ = 0;
    }
}

// ============================================================================
// Game Icons
// ============================================================================

void UI::loadGameIcons() {
    std::string cacheDir = basePath_ + "cache/";
    mkdir(cacheDir.c_str(), 0755);

    nsInitialize();

    GameType games[] = { GameType::S, GameType::V };
    for (auto game : games) {
        if (gameIconCache_.count(game))
            continue;

        uint64_t tid = titleIdOf(game);
        char tidStr[32];
        std::snprintf(tidStr, sizeof(tidStr), "%016lX", tid);
        std::string cachePath = cacheDir + tidStr + ".jpg";

        // Try loading from cache
        SDL_Surface* surf = IMG_Load(cachePath.c_str());

        if (!surf) {
            // Fetch from system
            NsApplicationControlData controlData;
            u64 controlSize = 0;
            Result rc = nsGetApplicationControlData(NsApplicationControlSource_Storage,
                tid, &controlData, sizeof(controlData), &controlSize);
            if (R_SUCCEEDED(rc) && controlSize > sizeof(controlData.nacp)) {
                size_t iconSize = controlSize - sizeof(controlData.nacp);
                SDL_RWops* rw = SDL_RWFromMem(controlData.icon, (int)iconSize);
                if (rw) {
                    surf = IMG_Load_RW(rw, 1);
                    // Save to cache
                    if (surf) {
                        FILE* f = std::fopen(cachePath.c_str(), "wb");
                        if (f) {
                            std::fwrite(controlData.icon, 1, iconSize, f);
                            std::fclose(f);
                        }
                    }
                }
            }
        }

        if (surf) {
            gameIconCache_[game] = SDL_CreateTextureFromSurface(renderer_, surf);
            SDL_FreeSurface(surf);
        }
    }

    nsExit();
}

// ============================================================================
// Profile Selector
// ============================================================================

void UI::drawProfileSelectorFrame() {
    SDL_SetRenderDrawColor(renderer_, T().bg.r, T().bg.g, T().bg.b, 255);
    SDL_RenderClear(renderer_);

    drawTextCentered(fontLarge_, "Event Raid Injector", SCREEN_W/2, 40, T().text);
    drawTextCentered(font_, "Select Profile", SCREEN_W/2, 80, T().textDim);

    int count = account_.profileCount();
    if (count == 0) {
        drawTextCentered(font_, "No profiles found", SCREEN_W/2, SCREEN_H/2, T().textDim);
        drawStatusBar("B: Quit");
        return;
    }

    constexpr int CARD_W = 160, CARD_H = 200, CARD_GAP = 20;
    int totalW = count * CARD_W + (count - 1) * CARD_GAP;
    int startX = (SCREEN_W - totalW) / 2;
    int startY = (SCREEN_H - CARD_H) / 2 - 20;

    auto& profiles = account_.profiles();
    for (int i = 0; i < count; i++) {
        int cx = startX + i * (CARD_W + CARD_GAP);
        bool sel = (i == profileSelCursor_);

        drawRect(cx, startY, CARD_W, CARD_H, sel ? T().menuHighlight : T().panelBg);

        // Icon
        if (profiles[i].iconTexture) {
            SDL_Rect dst = {cx + (CARD_W - 128)/2, startY + 10, 128, 128};
            SDL_RenderCopy(renderer_, profiles[i].iconTexture, nullptr, &dst);
        } else {
            drawRect(cx + (CARD_W - 128)/2, startY + 10, 128, 128, T().iconPlaceholder);
            char initial[2] = { profiles[i].nickname.empty() ? '?' : profiles[i].nickname[0], 0 };
            drawTextCentered(fontLarge_, initial, cx + CARD_W/2, startY + 74, T().text);
        }

        // Nickname
        std::string name = profiles[i].nickname;
        if (name.size() > 14) name = name.substr(0, 14) + ".";
        drawTextCentered(fontSmall_, name.c_str(), cx + CARD_W/2, startY + 155, T().text);

        if (sel) drawRectOutline(cx, startY, CARD_W, CARD_H, T().cursor, 3);
    }

    drawStatusBar("A: Select   Y: Theme   -: About   +: Quit");
}

void UI::handleProfileSelectorInput(SDL_Event& e, bool& running) {
    int count = account_.profileCount();

    if (e.type == SDL_CONTROLLERBUTTONDOWN) {
        auto btn = e.cbutton.button;
        if (btn == SDL_CONTROLLER_BUTTON_DPAD_LEFT && count > 0) {
            profileSelCursor_ = (profileSelCursor_ - 1 + count) % count;
        } else if (btn == SDL_CONTROLLER_BUTTON_DPAD_RIGHT && count > 0) {
            profileSelCursor_ = (profileSelCursor_ + 1) % count;
        } else if (btn == SDL_CONTROLLER_BUTTON_B) { // A on Switch
            if (count > 0) selectProfile(profileSelCursor_);
        } else if (btn == SDL_CONTROLLER_BUTTON_X) { // Y on Switch
            showThemeSelector_ = true;
            themeSelCursor_ = themeIndex_;
            themeSelOriginal_ = themeIndex_;
        } else if (btn == SDL_CONTROLLER_BUTTON_BACK) { // - on Switch
            showAbout_ = true;
        } else if (btn == SDL_CONTROLLER_BUTTON_START) { // + on Switch
            running = false;
        }
    }

    if (e.type == SDL_KEYDOWN) {
        if (e.key.keysym.sym == SDLK_LEFT && count > 0)
            profileSelCursor_ = (profileSelCursor_ - 1 + count) % count;
        else if (e.key.keysym.sym == SDLK_RIGHT && count > 0)
            profileSelCursor_ = (profileSelCursor_ + 1) % count;
        else if (e.key.keysym.sym == SDLK_a || e.key.keysym.sym == SDLK_RETURN) {
            if (count > 0) selectProfile(profileSelCursor_);
        } else if (e.key.keysym.sym == SDLK_y) {
            showThemeSelector_ = true;
            themeSelCursor_ = themeIndex_;
            themeSelOriginal_ = themeIndex_;
        } else if (e.key.keysym.sym == SDLK_MINUS) {
            showAbout_ = true;
        } else if (e.key.keysym.sym == SDLK_ESCAPE) {
            running = false;
        }
    }
}

void UI::selectProfile(int index) {
    selectedProfile_ = index;
    availableGames_.clear();

    GameType allGames[] = { GameType::S, GameType::V };
    for (auto g : allGames) {
        if (account_.hasSaveData(index, g))
            availableGames_.push_back(g);
    }

    if (availableGames_.empty()) {
        showMessageAndWait("No Save Data",
            "No Scarlet/Violet save data found for this profile.");
        return;
    }

    gameSelCursor_ = 0;
    screen_ = Screen::GameSelector;
}

// ============================================================================
// Game Selector
// ============================================================================

void UI::drawGameSelectorFrame() {
    SDL_SetRenderDrawColor(renderer_, T().bg.r, T().bg.g, T().bg.b, 255);
    SDL_RenderClear(renderer_);

    drawTextCentered(fontLarge_, "Event Raid Injector", SCREEN_W/2, 40, T().text);
    drawTextCentered(font_, "Select Game", SCREEN_W/2, 80, T().textDim);

    int count = (int)availableGames_.size();
    constexpr int CARD_W = 200, CARD_H = 220, CARD_GAP = 30;
    int totalW = count * CARD_W + (count - 1) * CARD_GAP;
    int startX = (SCREEN_W - totalW) / 2;
    int startY = (SCREEN_H - CARD_H) / 2 - 20;

    for (int i = 0; i < count; i++) {
        int cx = startX + i * (CARD_W + CARD_GAP);
        bool sel = (i == gameSelCursor_);
        GameType game = availableGames_[i];

        drawRect(cx, startY, CARD_W, CARD_H, sel ? T().menuHighlight : T().panelBg);

        // Game icon
        auto it = gameIconCache_.find(game);
        if (it != gameIconCache_.end() && it->second) {
            SDL_Rect dst = {cx + (CARD_W - 128)/2, startY + 15, 128, 128};
            SDL_RenderCopy(renderer_, it->second, nullptr, &dst);
        } else {
            // Colored placeholder
            SDL_Color ph = (game == GameType::S)
                ? SDL_Color{200, 60, 60, 255}
                : SDL_Color{120, 60, 200, 255};
            drawRect(cx + (CARD_W - 128)/2, startY + 15, 128, 128, ph);
            const char* abbrev = (game == GameType::S) ? "S" : "V";
            drawTextCentered(fontLarge_, abbrev, cx + CARD_W/2, startY + 79, T().text);
        }

        // Game name
        drawTextCentered(fontSmall_, gameDisplayNameOf(game), cx + CARD_W/2, startY + 165, T().text);

        if (sel) drawRectOutline(cx, startY, CARD_W, CARD_H, T().cursor, 3);
    }

    // Profile info in status bar (right-aligned)
    std::string profLabel;
    if (selectedProfile_ >= 0 && selectedProfile_ < account_.profileCount()) {
        profLabel = "Profile: " + account_.profiles()[selectedProfile_].nickname;
    }
    drawStatusBar("A: Select   B: Back   Y: Theme   -: About   +: Quit",
                   profLabel.empty() ? nullptr : profLabel.c_str());
}

void UI::handleGameSelectorInput(SDL_Event& e, bool& running) {
    int count = (int)availableGames_.size();

    if (e.type == SDL_CONTROLLERBUTTONDOWN) {
        auto btn = e.cbutton.button;
        if (btn == SDL_CONTROLLER_BUTTON_DPAD_LEFT && count > 0)
            gameSelCursor_ = (gameSelCursor_ - 1 + count) % count;
        else if (btn == SDL_CONTROLLER_BUTTON_DPAD_RIGHT && count > 0)
            gameSelCursor_ = (gameSelCursor_ + 1) % count;
        else if (btn == SDL_CONTROLLER_BUTTON_B) // A on Switch
            selectGame(availableGames_[gameSelCursor_]);
        else if (btn == SDL_CONTROLLER_BUTTON_A) // B on Switch
            screen_ = Screen::ProfileSelector;
        else if (btn == SDL_CONTROLLER_BUTTON_X) { // Y on Switch
            showThemeSelector_ = true;
            themeSelCursor_ = themeIndex_;
            themeSelOriginal_ = themeIndex_;
        } else if (btn == SDL_CONTROLLER_BUTTON_BACK) // - on Switch
            showAbout_ = true;
        else if (btn == SDL_CONTROLLER_BUTTON_START)
            running = false;
    }

    if (e.type == SDL_KEYDOWN) {
        if (e.key.keysym.sym == SDLK_LEFT && count > 0)
            gameSelCursor_ = (gameSelCursor_ - 1 + count) % count;
        else if (e.key.keysym.sym == SDLK_RIGHT && count > 0)
            gameSelCursor_ = (gameSelCursor_ + 1) % count;
        else if (e.key.keysym.sym == SDLK_a || e.key.keysym.sym == SDLK_RETURN)
            selectGame(availableGames_[gameSelCursor_]);
        else if (e.key.keysym.sym == SDLK_b || e.key.keysym.sym == SDLK_ESCAPE)
            screen_ = Screen::ProfileSelector;
        else if (e.key.keysym.sym == SDLK_MINUS)
            showAbout_ = true;
    }
}

void UI::selectGame(GameType game) {
    selectedGame_ = game;
    showWorking("Loading save...");

    // Mount save
    std::string mountPath = account_.mountSave(selectedProfile_, game);
    if (mountPath.empty()) {
        showMessageAndWait("Error", "Failed to mount save data.");
        return;
    }

    savePath_ = mountPath + saveFileNameOf(game);

    // Backup save
    if (!backupDone_) {
        // Calculate save directory size
        size_t saveSize = AccountManager::calculateDirSize(mountPath);

        // Check free space
        struct statvfs vfs;
        if (statvfs("sdmc:/", &vfs) == 0) {
            size_t freeSpace = (size_t)vfs.f_bavail * vfs.f_bsize;
            if (freeSpace < saveSize * 2) {
                bool proceed = showConfirmDialog("Low Space",
                    "Insufficient SD card space for backup. Continue without backup?");
                if (!proceed) {
                    account_.unmountSave();
                    return;
                }
            } else {
                // Create backup
                time_t now = std::time(nullptr);
                struct tm* tm = std::localtime(&now);
                char timestamp[64];
                std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H-%M-%S", tm);

                std::string profileName = (selectedProfile_ >= 0 && selectedProfile_ < account_.profileCount())
                    ? account_.profiles()[selectedProfile_].pathSafeName : "Unknown";

                std::string backupDir = basePath_ + "backups/" + profileName + "/"
                    + gamePathNameOf(game) + "/" + profileName + "_" + timestamp + "/";

                showWorking("Creating backup...");
                AccountManager::backupSaveDir(mountPath, backupDir);
            }
        }
        backupDone_ = true;
    }

    // Load save file
    showWorking("Decrypting save...");
    if (!save_.load(savePath_)) {
        showMessageAndWait("Error", "Failed to load save file.");
        account_.unmountSave();
        return;
    }

    // Verify round-trip
    std::string rtResult = save_.verifyRoundTrip();
    if (rtResult != "OK") {
        showMessageAndWait("Round-Trip Warning", rtResult.c_str());
    }

    saveLoaded_ = true;
    mainMenuCursor_ = 0;
    eventInjected_ = false;
    eventValidated_ = false;
    selectedEventFolder_.clear();
    eventIdentifier_.clear();
    statusMessage_ = "Ready";
    screen_ = Screen::Main;
}

// ============================================================================
// Main Screen
// ============================================================================

void UI::drawMainScreenFrame() {
    SDL_SetRenderDrawColor(renderer_, T().bg.r, T().bg.g, T().bg.b, 255);
    SDL_RenderClear(renderer_);

    // Title
    drawTextCentered(fontLarge_, "Event Raid Injector", SCREEN_W/2, 45, T().text);

    // Info section
    int infoY = 100;
    std::string gameStr = std::string("Game: ") + gameDisplayNameOf(selectedGame_);
    drawText(font_, gameStr.c_str(), 80, infoY, T().text);
    infoY += 30;

    if (selectedProfile_ >= 0 && selectedProfile_ < account_.profileCount()) {
        std::string profStr = "Profile: " + account_.profiles()[selectedProfile_].nickname;
        drawText(font_, profStr.c_str(), 80, infoY, T().text);
    }
    infoY += 30;

    std::string saveStr = std::string("Save: ") + (saveLoaded_ ? "Loaded" : "Not loaded");
    drawText(font_, saveStr.c_str(), 80, infoY, saveLoaded_ ? T().statusText : T().red);

    // Menu
    int menuX = 120, menuY = 220;
    constexpr int MENU_W = 500, ITEM_H = 42;

    const char* labels[MENU_ITEM_COUNT] = {
        "Browse Event Folder",
        "Clear Event (Inject Null)",
        "Inject Event",
        "Save & Exit",
        "Clear Cache & Revalidate Events",
        "Exit Without Saving",
    };

    // Determine which items are enabled
    bool enabled[MENU_ITEM_COUNT] = {
        true,                                   // Browse: always
        true,                                   // Clear: always
        eventValidated_ && !eventInjected_,     // Inject: after browse, before inject
        eventInjected_,                         // Save: after inject
        true,                                   // Revalidate: always
        true,                                   // Exit: always
    };

    drawRect(menuX - 10, menuY - 8, MENU_W, MENU_ITEM_COUNT * ITEM_H + 16, T().panelBg);
    drawRectOutline(menuX - 10, menuY - 8, MENU_W, MENU_ITEM_COUNT * ITEM_H + 16, T().popupBorder, 2);

    for (int i = 0; i < MENU_ITEM_COUNT; i++) {
        int iy = menuY + i * ITEM_H;
        bool sel = (i == mainMenuCursor_);

        if (sel) {
            drawRect(menuX - 5, iy - 2, MENU_W - 10, ITEM_H - 4, T().menuHighlight);
        }

        SDL_Color col = enabled[i] ? (sel ? T().cursor : T().text) : T().textDim;
        std::string label = (sel ? "> " : "  ") + std::string(labels[i]);
        drawText(font_, label.c_str(), menuX, iy + 8, col);
    }

    // Event info
    int eventY = menuY + MENU_ITEM_COUNT * ITEM_H + 40;
    if (!selectedEventFolder_.empty()) {
        // Show just the folder name
        std::string folderName = selectedEventFolder_;
        if (folderName.back() == '/') folderName.pop_back();
        auto pos = folderName.rfind('/');
        if (pos != std::string::npos) folderName = folderName.substr(pos + 1);

        std::string eventStr = "Event folder: " + folderName;
        drawText(font_, eventStr.c_str(), 80, eventY, T().text);
        eventY += 26;

        if (!eventIdentifier_.empty()) {
            std::string idStr = "Identifier: " + eventIdentifier_;
            drawText(font_, idStr.c_str(), 80, eventY, T().goldLabel);
            eventY += 26;
        }
    } else {
        drawText(font_, "Event: [None selected]", 80, eventY, T().textDim);
        eventY += 26;
    }

    // Status
    SDL_Color statusColor = eventInjected_ ? T().statusText : T().textDim;
    if (statusMessage_.find("Error") != std::string::npos ||
        statusMessage_.find("Failed") != std::string::npos)
        statusColor = T().red;
    drawText(font_, ("Status: " + statusMessage_).c_str(), 80, eventY, statusColor);

    drawStatusBar("A: Select   B: Back to Game   -: About   +: Quit");
}

void UI::handleMainScreenInput(SDL_Event& e, bool& running) {
    if (e.type == SDL_CONTROLLERBUTTONDOWN) {
        auto btn = e.cbutton.button;
        if (btn == SDL_CONTROLLER_BUTTON_DPAD_UP) {
            mainMenuCursor_ = (mainMenuCursor_ - 1 + MENU_ITEM_COUNT) % MENU_ITEM_COUNT;
        } else if (btn == SDL_CONTROLLER_BUTTON_DPAD_DOWN) {
            mainMenuCursor_ = (mainMenuCursor_ + 1) % MENU_ITEM_COUNT;
        } else if (btn == SDL_CONTROLLER_BUTTON_B) { // A on Switch
            switch (mainMenuCursor_) {
                case MENU_BROWSE:
                    // Open folder browser
                    {
                        std::string eventsDir = basePath_ + "events/";
                        mkdir(eventsDir.c_str(), 0755);
                        browser_.open(eventsDir);
                    }
                    screen_ = Screen::FolderBrowser;
                    break;
                case MENU_CLEAR:
                    doClearEvent();
                    break;
                case MENU_INJECT:
                    if (eventValidated_ && !eventInjected_) doInject();
                    break;
                case MENU_SAVE:
                    if (eventInjected_) doSaveAndExit(running);
                    break;
                case MENU_REVALIDATE:
                    {
                        std::string eventsDir = basePath_ + "events/";
                        FolderBrowser::deleteCache(eventsDir);
                        statusMessage_ = "Cache cleared. Events will be revalidated on next browse.";
                    }
                    break;
                case MENU_EXIT:
                    if (eventInjected_) {
                        if (showConfirmDialog("Unsaved Changes",
                            "You have injected data that hasn't been saved. Discard?"))
                            running = false;
                    } else {
                        running = false;
                    }
                    break;
            }
        } else if (btn == SDL_CONTROLLER_BUTTON_A) { // B on Switch
            // Go back to game selector (unmount save)
            if (eventInjected_) {
                if (!showConfirmDialog("Unsaved Changes",
                    "You have injected data that hasn't been saved. Discard?"))
                    return;
            }
            account_.unmountSave();
            saveLoaded_ = false;
            backupDone_ = false;
            screen_ = Screen::GameSelector;
        } else if (btn == SDL_CONTROLLER_BUTTON_X) { // Y on Switch
            showThemeSelector_ = true;
            themeSelCursor_ = themeIndex_;
            themeSelOriginal_ = themeIndex_;
        } else if (btn == SDL_CONTROLLER_BUTTON_BACK) { // - on Switch
            showAbout_ = true;
        } else if (btn == SDL_CONTROLLER_BUTTON_START) { // +
            if (eventInjected_) {
                if (showConfirmDialog("Unsaved Changes", "Save before quitting?"))
                    doSaveAndExit(running);
                else
                    running = false;
            } else {
                running = false;
            }
        }
    }

    if (e.type == SDL_KEYDOWN) {
        if (e.key.keysym.sym == SDLK_UP)
            mainMenuCursor_ = (mainMenuCursor_ - 1 + MENU_ITEM_COUNT) % MENU_ITEM_COUNT;
        else if (e.key.keysym.sym == SDLK_DOWN)
            mainMenuCursor_ = (mainMenuCursor_ + 1) % MENU_ITEM_COUNT;
        else if (e.key.keysym.sym == SDLK_a || e.key.keysym.sym == SDLK_RETURN) {
            switch (mainMenuCursor_) {
                case MENU_BROWSE: {
                    std::string eventsDir = basePath_ + "events/";
                    mkdir(eventsDir.c_str(), 0755);
                    browser_.open(eventsDir);
                    screen_ = Screen::FolderBrowser;
                    break;
                }
                case MENU_CLEAR: doClearEvent(); break;
                case MENU_INJECT: if (eventValidated_ && !eventInjected_) doInject(); break;
                case MENU_SAVE: if (eventInjected_) doSaveAndExit(running); break;
                case MENU_REVALIDATE: {
                    std::string eventsDir = basePath_ + "events/";
                    FolderBrowser::deleteCache(eventsDir);
                    statusMessage_ = "Cache cleared. Events will be revalidated on next browse.";
                    break;
                }
                case MENU_EXIT: running = false; break;
            }
        } else if (e.key.keysym.sym == SDLK_MINUS) {
            showAbout_ = true;
        } else if (e.key.keysym.sym == SDLK_b || e.key.keysym.sym == SDLK_ESCAPE) {
            account_.unmountSave();
            saveLoaded_ = false;
            backupDone_ = false;
            screen_ = Screen::GameSelector;
        }
    }
}

void UI::doInject() {
    if (!saveLoaded_ || selectedEventFolder_.empty())
        return;

    showWorking("Injecting event...");
    InjectorResult result = Injector::injectRaidEvent(save_, selectedEventFolder_);

    if (result.success) {
        eventInjected_ = true;
        eventIdentifier_ = result.identifier;
        statusMessage_ = "Injected: " + result.identifier;
        mainMenuCursor_ = MENU_SAVE;
    } else {
        statusMessage_ = "Error: " + result.message;
    }

    showMessageAndWait(result.success ? "Success" : "Error", result.message.c_str());
}

void UI::doClearEvent() {
    if (!saveLoaded_)
        return;

    if (!showConfirmDialog("Clear Event", "Remove all active raid event data?"))
        return;

    showWorking("Clearing event...");
    InjectorResult result = Injector::injectNullEvent(save_);

    if (result.success) {
        eventInjected_ = true;
        eventIdentifier_ = "Null (cleared)";
        selectedEventFolder_.clear();
        eventValidated_ = false;
        statusMessage_ = "Event data cleared";
        mainMenuCursor_ = MENU_SAVE;
    } else {
        statusMessage_ = "Error: " + result.message;
    }

    showMessageAndWait(result.success ? "Success" : "Error", result.message.c_str());
}

void UI::doSaveAndExit(bool& running) {
    showWorking("Saving...");
    if (save_.save(savePath_)) {
        account_.commitSave();
        showMessageAndWait("Saved", "Save file updated successfully.");
        running = false;
    } else {
        showMessageAndWait("Error", "Failed to write save file.");
    }
}

// ============================================================================
// Folder Browser
// ============================================================================

void UI::drawFolderBrowserFrame() {
    SDL_SetRenderDrawColor(renderer_, T().bg.r, T().bg.g, T().bg.b, 255);
    SDL_RenderClear(renderer_);

    drawTextCentered(fontLarge_, "Select Event Folder", SCREEN_W/2, 35, T().text);

    // Separator
    drawRect(40, 70, SCREEN_W - 80, 2, T().popupBorder);

    // Directory listing
    auto& entries = browser_.entries();

    if (entries.empty()) {
        drawTextCentered(font_, "No event folders found.", SCREEN_W/2, SCREEN_H/2 - 20, T().textDim);
        drawTextCentered(fontSmall_, "Place event folders in sdmc:/switch/EventRaidInjector/events/",
                         SCREEN_W/2, SCREEN_H/2 + 20, T().textDim);
        drawStatusBar("B: Back");
        return;
    }

    int listY = 80;
    constexpr int ROW_H = 32;
    int scrollOff = browser_.scrollOffset();

    for (int i = 0; i < FolderBrowser::VISIBLE_ROWS && (scrollOff + i) < (int)entries.size(); i++) {
        int idx = scrollOff + i;
        int iy = listY + i * ROW_H;
        bool isCursor = (idx == browser_.cursor());

        if (isCursor) {
            drawRect(40, iy, SCREEN_W - 80, ROW_H - 2, T().menuHighlight);
        }

        const DirEntry& entry = entries[idx];
        SDL_Color col = isCursor ? T().cursor : T().text;
        std::string display = "  " + entry.name;
        drawText(font_, display.c_str(), 50, iy + 5, col);

        // Show cached validity indicator on the right
        const char* validLabel = entry.isValidEvent ? "Valid" : "Invalid";
        SDL_Color validCol = entry.isValidEvent ? T().statusText : T().textDim;
        int tw = 0, th = 0;
        TTF_SizeUTF8(fontSmall_, validLabel, &tw, &th);
        drawText(fontSmall_, validLabel, SCREEN_W - 60 - tw, iy + 8, validCol);
    }

    // Scroll indicator
    if ((int)entries.size() > FolderBrowser::VISIBLE_ROWS) {
        int totalH = FolderBrowser::VISIBLE_ROWS * ROW_H;
        int scrollBarH = totalH * FolderBrowser::VISIBLE_ROWS / (int)entries.size();
        if (scrollBarH < 20) scrollBarH = 20;
        int scrollBarY = listY + (totalH - scrollBarH) * scrollOff / ((int)entries.size() - FolderBrowser::VISIBLE_ROWS);
        drawRect(SCREEN_W - 35, scrollBarY, 8, scrollBarH, T().textDim);
    }

    drawStatusBar("A: Select   B: Cancel   L/R: Page   -: About");
}

void UI::selectEventFolder() {
    std::string selPath = browser_.selectedPath();
    if (selPath.empty()) return;

    if (Injector::isValidRaidFolder(selPath)) {
        selectedEventFolder_ = selPath;
        eventValidated_ = true;
        eventInjected_ = false;

        // Read identifier for display
        std::string idPath = selPath + "Identifier.txt";
        std::ifstream idFile(idPath);
        if (idFile.is_open()) {
            std::getline(idFile, eventIdentifier_);
        }

        statusMessage_ = "Event selected: " + eventIdentifier_;
        mainMenuCursor_ = MENU_INJECT;
        screen_ = Screen::Main;
    } else {
        showMessageAndWait("Invalid Folder",
            "This folder does not contain valid raid event data.\n"
            "Expected: Identifier.txt + Files/ with binary data.");
    }
}

void UI::handleFolderBrowserInput(SDL_Event& e, bool& running) {
    (void)running;

    if (e.type == SDL_CONTROLLERBUTTONDOWN) {
        auto btn = e.cbutton.button;
        if (btn == SDL_CONTROLLER_BUTTON_DPAD_UP) {
            browser_.moveCursor(-1);
        } else if (btn == SDL_CONTROLLER_BUTTON_DPAD_DOWN) {
            browser_.moveCursor(1);
        } else if (btn == SDL_CONTROLLER_BUTTON_B) { // A on Switch - select folder
            selectEventFolder();
        } else if (btn == SDL_CONTROLLER_BUTTON_A) { // B on Switch - cancel
            screen_ = Screen::Main;
        } else if (btn == SDL_CONTROLLER_BUTTON_LEFTSHOULDER) { // L - page up
            browser_.moveCursor(-FolderBrowser::VISIBLE_ROWS);
        } else if (btn == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) { // R - page down
            browser_.moveCursor(FolderBrowser::VISIBLE_ROWS);
        } else if (btn == SDL_CONTROLLER_BUTTON_BACK) { // - on Switch
            showAbout_ = true;
        }
    }

    if (e.type == SDL_KEYDOWN) {
        if (e.key.keysym.sym == SDLK_UP) browser_.moveCursor(-1);
        else if (e.key.keysym.sym == SDLK_DOWN) browser_.moveCursor(1);
        else if (e.key.keysym.sym == SDLK_a || e.key.keysym.sym == SDLK_RETURN) selectEventFolder();
        else if (e.key.keysym.sym == SDLK_b || e.key.keysym.sym == SDLK_ESCAPE) screen_ = Screen::Main;
        else if (e.key.keysym.sym == SDLK_PAGEUP) browser_.moveCursor(-FolderBrowser::VISIBLE_ROWS);
        else if (e.key.keysym.sym == SDLK_PAGEDOWN) browser_.moveCursor(FolderBrowser::VISIBLE_ROWS);
        else if (e.key.keysym.sym == SDLK_MINUS) showAbout_ = true;
    }
}

// ============================================================================
// Theme Selector
// ============================================================================

void UI::drawThemeSelectorPopup() {
    drawRect(0, 0, SCREEN_W, SCREEN_H, T().overlay);

    int pw = 380, ph = 50 + THEME_COUNT * 36 + 30;
    int px = (SCREEN_W - pw) / 2, py = (SCREEN_H - ph) / 2;

    drawRect(px, py, pw, ph, T().panelBg);
    drawRectOutline(px, py, pw, ph, T().popupBorder, 2);
    drawTextCentered(font_, "Select Theme", SCREEN_W/2, py + 18, T().text);

    for (int i = 0; i < THEME_COUNT; i++) {
        int iy = py + 45 + i * 36;
        bool sel = (i == themeSelCursor_);
        if (sel)
            drawRect(px + 10, iy, pw - 20, 32, T().menuHighlight);

        std::string label = getThemeName(i);
        if (i == themeIndex_) label = "* " + label + " *";
        drawTextCentered(font_, label.c_str(), SCREEN_W/2, iy + 16, sel ? T().cursor : T().text);
    }

    drawTextCentered(fontSmall_, "A: Select   B: Cancel", SCREEN_W/2, py + ph - 18, T().textDim);
}

void UI::handleThemeSelectorInput(SDL_Event& e) {
    if (e.type == SDL_CONTROLLERBUTTONDOWN) {
        auto btn = e.cbutton.button;
        if (btn == SDL_CONTROLLER_BUTTON_DPAD_UP) {
            themeSelCursor_ = (themeSelCursor_ - 1 + THEME_COUNT) % THEME_COUNT;
            // Live preview
            themeIndex_ = themeSelCursor_;
            theme_ = &getTheme(themeIndex_);
        } else if (btn == SDL_CONTROLLER_BUTTON_DPAD_DOWN) {
            themeSelCursor_ = (themeSelCursor_ + 1) % THEME_COUNT;
            themeIndex_ = themeSelCursor_;
            theme_ = &getTheme(themeIndex_);
        } else if (btn == SDL_CONTROLLER_BUTTON_B) { // A on Switch - confirm
            saveThemeIndex(basePath_, themeIndex_);
            showThemeSelector_ = false;
        } else if (btn == SDL_CONTROLLER_BUTTON_A) { // B on Switch - cancel
            themeIndex_ = themeSelOriginal_;
            theme_ = &getTheme(themeIndex_);
            showThemeSelector_ = false;
        }
    }

    if (e.type == SDL_KEYDOWN) {
        if (e.key.keysym.sym == SDLK_UP) {
            themeSelCursor_ = (themeSelCursor_ - 1 + THEME_COUNT) % THEME_COUNT;
            themeIndex_ = themeSelCursor_;
            theme_ = &getTheme(themeIndex_);
        } else if (e.key.keysym.sym == SDLK_DOWN) {
            themeSelCursor_ = (themeSelCursor_ + 1) % THEME_COUNT;
            themeIndex_ = themeSelCursor_;
            theme_ = &getTheme(themeIndex_);
        } else if (e.key.keysym.sym == SDLK_a || e.key.keysym.sym == SDLK_RETURN) {
            saveThemeIndex(basePath_, themeIndex_);
            showThemeSelector_ = false;
        } else if (e.key.keysym.sym == SDLK_b || e.key.keysym.sym == SDLK_ESCAPE) {
            themeIndex_ = themeSelOriginal_;
            theme_ = &getTheme(themeIndex_);
            showThemeSelector_ = false;
        }
    }
}

// ============================================================================
// About
// ============================================================================

void UI::drawAboutPopup() {
    drawRect(0, 0, SCREEN_W, SCREEN_H, T().overlayDark);

    int pw = 700, ph = 440;
    int px = (SCREEN_W - pw) / 2, py = (SCREEN_H - ph) / 2;

    drawRect(px, py, pw, ph, T().panelBg);
    drawRectOutline(px, py, pw, ph, T().popupBorder, 2);

    int ty = py + 25;
    drawTextCentered(fontLarge_, "Event Raid Injector", SCREEN_W/2, ty, T().shiny);
    ty += 38;
    drawTextCentered(fontSmall_, "v" APP_VERSION " by " APP_AUTHOR, SCREEN_W/2, ty, T().textDim);
    ty += 22;
    drawTextCentered(fontSmall_, "github.com/Insektaure", SCREEN_W/2, ty, T().textDim);

    // Divider
    ty += 30;
    drawRect(px + 30, ty, pw - 60, 1, T().popupBorder);

    // Description
    ty += 20;
    drawTextCentered(fontSmall_, "Inject Tera Raid event data into", SCREEN_W/2, ty, T().text);
    ty += 20;
    drawTextCentered(fontSmall_, "Pokemon Scarlet / Violet save files.", SCREEN_W/2, ty, T().text);
    ty += 20;
    drawTextCentered(fontSmall_, "Event data: place folders in sdmc:/switch/EventRaidInjector/events/", SCREEN_W/2, ty, T().textDim);

    // Divider
    ty += 20;
    drawRect(px + 30, ty, pw - 60, 1, T().popupBorder);

    // Credits
    ty += 18;
    drawTextCentered(fontSmall_, "Tera-Finder by Manu098vm - Injection logic & block definitions", SCREEN_W/2, ty, T().creditsText);
    ty += 18;
    drawTextCentered(fontSmall_, "ProjectPokemon Events Gallery - Event data source", SCREEN_W/2, ty, T().creditsText);
    ty += 18;
    drawTextCentered(fontSmall_, "PKHeX by kwsch - SCBlock format & encryption", SCREEN_W/2, ty, T().creditsText);
    ty += 18;
    drawTextCentered(fontSmall_, "JKSV by J-D-K - Save backup & write logic reference", SCREEN_W/2, ty, T().creditsText);
    ty += 18;
    drawTextCentered(fontSmall_, "pkHouse by Insektaure - UI framework", SCREEN_W/2, ty, T().creditsText);
    ty += 18;
    drawTextCentered(fontSmall_, "Built with libnx and SDL2", SCREEN_W/2, ty, T().creditsText);

    // Footer
    drawTextCentered(fontSmall_, "Press - or B to close", SCREEN_W/2, py + ph - 22, T().textDim);
}

void UI::handleAboutInput(SDL_Event& e) {
    if (e.type == SDL_CONTROLLERBUTTONDOWN) {
        auto btn = e.cbutton.button;
        if (btn == SDL_CONTROLLER_BUTTON_BACK || btn == SDL_CONTROLLER_BUTTON_A ||
            btn == SDL_CONTROLLER_BUTTON_B)
            showAbout_ = false;
    }
    if (e.type == SDL_KEYDOWN) {
        if (e.key.keysym.sym == SDLK_MINUS || e.key.keysym.sym == SDLK_ESCAPE ||
            e.key.keysym.sym == SDLK_b)
            showAbout_ = false;
    }
}

// ============================================================================
// Main Loop
// ============================================================================

void UI::run(const std::string& basePath) {
    basePath_ = basePath;

    // Load theme
    themeIndex_ = loadThemeIndex(basePath_);
    theme_ = &getTheme(themeIndex_);

    // Create events directory
    mkdir((basePath_ + "events/").c_str(), 0755);

    // Init account system
    account_.init();
    bool hasProfiles = account_.loadProfiles(renderer_);

    // Load game icons
    loadGameIcons();

    // Applet mode: warn and exit - title override is required for save access
    if (appletMode_) {
        showMessageAndWait("Applet Mode",
            "This app requires title override mode.\n"
            "Please launch via a game title, not the album.");
        return;
    }

    if (!hasProfiles) {
        showMessageAndWait("Error", "No user profiles found.");
        return;
    }

    screen_ = Screen::ProfileSelector;

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                break;
            }

            // Always update stick state regardless of active overlay
            if (event.type == SDL_CONTROLLERAXISMOTION) {
                if (event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX ||
                    event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
                    updateStick(
                        SDL_GameControllerGetAxis(pad_, SDL_CONTROLLER_AXIS_LEFTX),
                        SDL_GameControllerGetAxis(pad_, SDL_CONTROLLER_AXIS_LEFTY));
                }
            }

            // Global overlays get priority
            if (showAbout_) {
                handleAboutInput(event);
                continue;
            }
            if (showThemeSelector_) {
                handleThemeSelectorInput(event);
                continue;
            }

            // Route to current screen
            switch (screen_) {
                case Screen::ProfileSelector:
                    handleProfileSelectorInput(event, running);
                    break;
                case Screen::GameSelector:
                    handleGameSelectorInput(event, running);
                    break;
                case Screen::Main:
                    handleMainScreenInput(event, running);
                    break;
                case Screen::FolderBrowser:
                    handleFolderBrowserInput(event, running);
                    break;
            }
        }

        if (!running) break;

        // Joystick repeat
        if (stickDirX_ != 0 || stickDirY_ != 0) {
            uint32_t now = SDL_GetTicks();
            uint32_t delay = stickMoved_ ? STICK_REPEAT_DELAY : STICK_INITIAL_DELAY;
            if (now - stickMoveTime_ >= delay) {
                bool moved = false;

                // Overlays first
                if (showThemeSelector_) {
                    if (stickDirY_ != 0) {
                        themeSelCursor_ = (themeSelCursor_ + stickDirY_ + THEME_COUNT) % THEME_COUNT;
                        themeIndex_ = themeSelCursor_;
                        theme_ = &getTheme(themeIndex_);
                        moved = true;
                    }
                } else {
                    switch (screen_) {
                        case Screen::ProfileSelector:
                            if (stickDirX_ != 0) {
                                int count = account_.profileCount();
                                if (count > 0) {
                                    profileSelCursor_ = (profileSelCursor_ + stickDirX_ + count) % count;
                                    moved = true;
                                }
                            }
                            break;
                        case Screen::GameSelector:
                            if (stickDirX_ != 0) {
                                int count = (int)availableGames_.size();
                                if (count > 0) {
                                    gameSelCursor_ = (gameSelCursor_ + stickDirX_ + count) % count;
                                    moved = true;
                                }
                            }
                            break;
                        case Screen::Main:
                            if (stickDirY_ != 0) {
                                mainMenuCursor_ = (mainMenuCursor_ + stickDirY_ + MENU_ITEM_COUNT) % MENU_ITEM_COUNT;
                                moved = true;
                            }
                            break;
                        case Screen::FolderBrowser:
                            if (stickDirY_ != 0) {
                                browser_.moveCursor(stickDirY_);
                                moved = true;
                            }
                            break;
                    }
                }

                if (moved) {
                    stickMoveTime_ = now;
                    stickMoved_ = true;
                }
            }
        }

        // Draw current screen
        switch (screen_) {
            case Screen::ProfileSelector: drawProfileSelectorFrame(); break;
            case Screen::GameSelector:    drawGameSelectorFrame(); break;
            case Screen::Main:            drawMainScreenFrame(); break;
            case Screen::FolderBrowser:   drawFolderBrowserFrame(); break;
        }

        // Draw overlays on top
        if (showThemeSelector_) drawThemeSelectorPopup();
        if (showAbout_) drawAboutPopup();

        SDL_RenderPresent(renderer_);
        SDL_Delay(16);
    }

    // Cleanup: unmount save if still mounted
    account_.unmountSave();
    account_.shutdown();
}
