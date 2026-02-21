#include "theme.h"
#include <cstdio>

static const Theme themes[THEME_COUNT] = {
    // ===== 0: Default (current dark theme) =====
    {
        .name            = "Default",
        .bg              = {30, 30, 40, 255},
        .panelBg         = {45, 45, 60, 255},
        .statusBarBg     = {20, 20, 30, 255},
        .slotEmpty       = {60, 60, 80, 255},
        .slotFull        = {70, 90, 120, 255},
        .slotEgg         = {90, 80, 70, 255},
        .cursor          = {255, 220, 50, 255},
        .selected        = {100, 200, 220, 255},
        .selectedPos     = {120, 200, 120, 255},
        .text            = {240, 240, 240, 255},
        .textDim         = {160, 160, 170, 255},
        .textOnBadge     = {0, 0, 0, 255},
        .boxName         = {200, 200, 220, 255},
        .arrow           = {180, 180, 200, 255},
        .statusText      = {140, 200, 140, 255},
        .red             = {220, 60, 60, 255},
        .shiny           = {255, 215, 0, 255},
        .goldLabel       = {255, 215, 0, 255},
        .genderMale      = {100, 150, 255, 255},
        .genderFemale    = {255, 130, 150, 255},
        .overlay         = {0, 0, 0, 160},
        .overlayDark     = {0, 0, 0, 187},
        .menuHighlight   = {60, 60, 80, 255},
        .iconPlaceholder = {80, 80, 120, 255},
        .popupBorder     = {0x30, 0x30, 0x55, 255},
        .creditsText     = {0x88, 0x88, 0x88, 255},
        .boxPreviewBg    = {40, 40, 55, 230},
        .miniCellEmpty   = {55, 55, 70, 180},
        .miniCellFull    = {55, 70, 90, 200},
        .textFieldBg     = {30, 30, 40, 255},
    },

    // ===== 1: HOME (Pokemon HOME-inspired light pastel theme) =====
    {
        .name            = "HOME",
        .bg              = {235, 240, 245, 255},
        .panelBg         = {248, 248, 252, 255},
        .statusBarBg     = {225, 230, 238, 255},
        .slotEmpty       = {230, 235, 242, 255},
        .slotFull        = {190, 225, 220, 255},
        .slotEgg         = {245, 225, 210, 255},
        .cursor          = {240, 128, 118, 255},
        .selected        = {90, 195, 190, 255},
        .selectedPos     = {130, 200, 140, 255},
        .text            = {50, 55, 65, 255},
        .textDim         = {120, 130, 145, 255},
        .textOnBadge     = {255, 255, 255, 255},
        .boxName         = {70, 80, 95, 255},
        .arrow           = {140, 150, 170, 255},
        .statusText      = {60, 160, 130, 255},
        .red             = {230, 80, 80, 255},
        .shiny           = {220, 170, 40, 255},
        .goldLabel       = {210, 150, 50, 255},
        .genderMale      = {70, 130, 230, 255},
        .genderFemale    = {230, 110, 140, 255},
        .overlay         = {0, 0, 0, 120},
        .overlayDark     = {0, 0, 0, 150},
        .menuHighlight   = {220, 235, 240, 255},
        .iconPlaceholder = {200, 210, 225, 255},
        .popupBorder     = {180, 190, 210, 255},
        .creditsText     = {140, 150, 165, 255},
        .boxPreviewBg    = {240, 243, 248, 240},
        .miniCellEmpty   = {225, 230, 240, 200},
        .miniCellFull    = {190, 220, 218, 220},
        .textFieldBg     = {245, 245, 250, 255},
    },
};

const Theme& getTheme(int index) {
    if (index < 0 || index >= THEME_COUNT)
        return themes[0];
    return themes[index];
}

const char* getThemeName(int index) {
    if (index < 0 || index >= THEME_COUNT)
        return themes[0].name;
    return themes[index].name;
}

int loadThemeIndex(const std::string& basePath) {
    std::string path = basePath + "theme.cfg";
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return 0;
    uint8_t idx = 0;
    std::fread(&idx, 1, 1, f);
    std::fclose(f);
    if (idx >= THEME_COUNT) idx = 0;
    return idx;
}

void saveThemeIndex(const std::string& basePath, int index) {
    std::string path = basePath + "theme.cfg";
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return;
    uint8_t idx = static_cast<uint8_t>(index);
    std::fwrite(&idx, 1, 1, f);
    std::fclose(f);
}
