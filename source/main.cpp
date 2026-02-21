#include "ui.h"
#include <switch.h>
#include <string>
#include <sys/stat.h>

int main(int argc, char* argv[]) {
    romfsInit();

    // Determine base path from NRO location
    std::string basePath;
    if (argc > 0 && argv[0]) {
        basePath = argv[0];
        auto lastSlash = basePath.rfind('/');
        if (lastSlash != std::string::npos)
            basePath = basePath.substr(0, lastSlash + 1);
        else
            basePath = "sdmc:/switch/EventRaidInjector/";
    } else {
        basePath = "sdmc:/switch/EventRaidInjector/";
    }

    // Ensure base directory exists
    mkdir(basePath.c_str(), 0755);

    // Initialize UI
    UI ui;
    if (!ui.init()) {
        romfsExit();
        return 1;
    }

    // Show splash screen
    ui.showSplash();

    // Detect applet mode - this app requires title override for save access
    AppletType at = appletGetAppletType();
    if (at != AppletType_Application && at != AppletType_SystemApplication)
        ui.setAppletMode(true);

    // Run main loop
    ui.run(basePath);

    // Cleanup
    ui.shutdown();
    romfsExit();
    return 0;
}
