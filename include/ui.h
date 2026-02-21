#pragma once
#include "account.h"
#include "save_file.h"
#include "folder_browser.h"
#include "theme.h"
#include "injector.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <map>

class UI {
public:
    bool init();
    void shutdown();
    void showSplash();
    void run(const std::string& basePath);
    void setAppletMode(bool applet) { appletMode_ = applet; }

private:
    // Screen dimensions (Switch native)
    static constexpr int SCREEN_W = 1280;
    static constexpr int SCREEN_H = 720;

    // Screens
    enum class Screen { ProfileSelector, GameSelector, Main, FolderBrowser };
    Screen screen_ = Screen::ProfileSelector;

    // SDL
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    SDL_GameController* pad_ = nullptr;
    TTF_Font* font_ = nullptr;
    TTF_Font* fontSmall_ = nullptr;
    TTF_Font* fontLarge_ = nullptr;

    // Theme
    int themeIndex_ = 0;
    const Theme* theme_ = nullptr;
    const Theme& T() const { return *theme_; }
    bool showThemeSelector_ = false;
    int themeSelCursor_ = 0;
    int themeSelOriginal_ = 0;

    // Account & Save
    AccountManager account_;
    SaveFile save_;
    std::string basePath_;
    std::string savePath_;
    bool appletMode_ = false;
    bool saveLoaded_ = false;
    bool backupDone_ = false;

    // Profile selector
    int profileSelCursor_ = 0;
    int selectedProfile_ = -1;

    // Game selector
    int gameSelCursor_ = 0;
    GameType selectedGame_ = GameType::S;
    std::vector<GameType> availableGames_;
    std::map<GameType, SDL_Texture*> gameIconCache_;

    // Main screen
    int mainMenuCursor_ = 0;
    static constexpr int MENU_BROWSE     = 0;
    static constexpr int MENU_CLEAR      = 1;
    static constexpr int MENU_INJECT     = 2;
    static constexpr int MENU_SAVE       = 3;
    static constexpr int MENU_REVALIDATE = 4;
    static constexpr int MENU_EXIT       = 5;
    static constexpr int MENU_ITEM_COUNT = 6;

    // Injection state
    std::string selectedEventFolder_;
    bool eventValidated_ = false;
    bool eventInjected_ = false;
    std::string eventIdentifier_;
    std::string statusMessage_;

    // Folder browser
    FolderBrowser browser_;

    // Joystick repeat
    static constexpr int16_t STICK_DEADZONE = 16000;
    static constexpr uint32_t STICK_INITIAL_DELAY = 400;
    static constexpr uint32_t STICK_REPEAT_DELAY  = 150;
    int stickDirX_ = 0, stickDirY_ = 0;
    uint32_t stickMoveTime_ = 0;
    bool stickMoved_ = false;

    // About dialog
    bool showAbout_ = false;

    // --- Rendering ---
    void drawRect(int x, int y, int w, int h, SDL_Color c);
    void drawRectOutline(int x, int y, int w, int h, SDL_Color c, int thickness = 1);
    void drawText(TTF_Font* f, const char* text, int x, int y, SDL_Color c);
    void drawTextCentered(TTF_Font* f, const char* text, int cx, int cy, SDL_Color c);
    void drawStatusBar(const char* text, const char* rightText = nullptr);

    void drawProfileSelectorFrame();
    void drawGameSelectorFrame();
    void drawMainScreenFrame();
    void drawFolderBrowserFrame();
    void drawThemeSelectorPopup();
    void drawAboutPopup();

    // --- Input ---
    void updateStick(int16_t axisX, int16_t axisY);
    void handleProfileSelectorInput(SDL_Event& e, bool& running);
    void handleGameSelectorInput(SDL_Event& e, bool& running);
    void handleMainScreenInput(SDL_Event& e, bool& running);
    void handleFolderBrowserInput(SDL_Event& e, bool& running);
    void handleThemeSelectorInput(SDL_Event& e);
    void handleAboutInput(SDL_Event& e);

    // --- Actions ---
    void selectProfile(int index);
    void selectGame(GameType game);
    void doInject();
    void doClearEvent();
    void selectEventFolder();
    void doSaveAndExit(bool& running);
    void loadGameIcons();

    // --- Dialogs ---
    void showMessageAndWait(const char* title, const char* body);
    bool showConfirmDialog(const char* title, const char* body);
    void showWorking(const char* msg);
};
