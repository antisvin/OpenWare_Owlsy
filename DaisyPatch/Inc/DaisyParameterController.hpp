#ifndef __ParameterController_hpp__
#define __ParameterController_hpp__

#include "ApplicationSettings.h"
#include "Codec.h"
#include "FileBrowser.hpp"
#include "OpenWareMidiControl.h"
#include "Owl.h"
#include "PatchRegistry.h"
#include "ProgramManager.h"
#include "ProgramVector.h"
#include "Scope.hpp"
#include "StorageTasks.hpp"
#include "VersionToken.h"
#include "errorhandlers.h"
#include "message.h"
#include <cstring>

#ifdef USE_DIGITALBUS
#include "bus.h"
#endif

#ifdef USE_FATFS
#include "fatfs.h"
#endif

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef abs
#define abs(x) ((x) > 0 ? (x) : -(x))
#endif

#define HYSTERESIS 64
//#define CPU_SMOOTH

extern VersionToken* bootloader_token;
extern uint8_t num_channels;

void defaultDrawCallback(uint8_t* pixels, uint16_t width, uint16_t height);

#define ENC_MULTIPLIER 6 // shift left by this many steps

static EraseStorageTask erase_task;
static DefragStorageTask defrag_task;

using Browser = FileBrowser<3>;
extern Browser sd_browser;

/* shows a single parameter selected and controlled with a single encoder
 */
template <uint8_t SIZE>
class ParameterController {
public:
    char title[13] = "  Owlsy     ";

    enum EncoderSensitivity {
        SENS_SUPER_FINE = 0,
        SENS_FINE = (ENC_MULTIPLIER / 2),
        SENS_STANDARD = ENC_MULTIPLIER,
        SENS_COARSE = (3 * ENC_MULTIPLIER / 2),
        SENS_SUPER_COARSE = (ENC_MULTIPLIER * 2)
    };

    enum DisplayMode {
        STANDARD,
        SELECTGLOBALPARAMETER,
        CONTROL,
        ERROR,
    };

    enum ControlMode {
        SETUP,
        STATUS,
        PATCH,
        DATA,
        SD_CARD,
        GATES,
        SCOPE,
        SYSTEM,
        EXIT,
        NOF_CONTROL_MODES, // Not an actual mode
    };

    enum SystemAction {
        SYS_BOOTLOADER,
        SYS_RESET,
        SYS_ERASE,
        SYS_DEFRAG,
        SYS_EXIT,
        SYS_LAST,
    };
    const char system_names[SYS_LAST][16] = {
        "Bootloader",
        "Reset",
        "Erase patches",
        "Defragmentation",
        "Exit",
    };
    bool systemPressed;
    static constexpr float value_mul = 1.0 / 4096;

    float cpu_used;
#ifdef CPU_SMOOTH
    static constexpr float cpu_smooth_lambda = 0.8;
#endif

    int16_t parameters[SIZE];
    int16_t display_parameters[SIZE];
    int16_t encoders[NOF_ENCODERS]; // last seen encoder values
    int16_t offsets[NOF_ENCODERS]; // last seen encoder values
    int16_t user[SIZE] CACHE_ALIGNED; // user set values (ie by encoder or MIDI)
    char names[SIZE][16];
    uint8_t selectedPid[NOF_ENCODERS];
    uint8_t lastSelectedPid, lastChannel, lastSelectedResource;
    // Use first param after ADC as default to avoid confusion from accidental
    // turns
    bool saveSettings;
    uint8_t activeEncoder;

    bool resourceDelete;
    bool resourceDeletePressed; // This is used to ensure that we don't delete
                                // current resourse on menu enter
    bool sdSelected;

    EncoderSensitivity encoderSensitivity = SENS_STANDARD;

    ControlMode controlMode;
    const char controlModeNames[NOF_CONTROL_MODES][11] = {
        "  Setup  >",
        "< Status >",
        "< Patch  >",
        "< Data   >",
        "< SDCard >",
#ifdef USE_DIGITALBUS
        "< Bus    >",
#endif
        "< Gates  >",
        "< Scope  >",
        "< System >",
        "< Exit    ",
    };

    DisplayMode displayMode;

    Scope<AUDIO_CHANNELS, AUDIO_CHANNELS> scope;

    ParameterController() {
        reset();
        sd_browser.reset();
    }

    void reset() {
        saveSettings = false;
        resourceDelete = false;
        drawCallback = defaultDrawCallback;
        for (int i = 0; i < SIZE; ++i) {
            memset(names[i], 0, 16);
            strcpy(names[i], "Param  ");
            if (i < 8) {
                names[i][6] = 'A' + i;
            }
            else {
                names[i][6] = 'A' - 1 + i / 8;
                names[i][7] = 'A' + i % 8;
            }

            parameters[i] = 0;
            user[i] = 0;
        }

        for (int i = 0; i < NOF_ENCODERS; ++i) {
            // encoders[i] = 0;
            offsets[i] = 0;
        }
        selectedPid[0] = PARAMETER_A;
        selectedPid[1] = 0;
        displayMode = STANDARD;
        controlMode = SETUP;
        activeEncoder = 1;
        sdSelected = false;
    }

    void draw(uint8_t* pixels, uint16_t width, uint16_t height) {
        ScreenBuffer screen(width, height);
        screen.setBuffer(pixels);
        draw(screen);
    }

    void drawSetup(ScreenBuffer& screen) {
        int offset = 16;
        screen.setTextSize(1);
        if (activeEncoder == 1) {
            screen.drawRectangle(10, offset + 3, 108, 24, WHITE);
        }
        screen.print(1, offset + 36, " Encoder sensitivity");

        screen.drawCircle(24, offset + 15, 8, WHITE);
        screen.drawCircle(44, offset + 15, 8, WHITE);
        screen.drawCircle(64, offset + 15, 8, WHITE);
        screen.drawCircle(84, offset + 15, 8, WHITE);
        screen.drawCircle(104, offset + 15, 8, WHITE);

        screen.fillCircle(24, offset + 15, 6, WHITE);
        if (encoderSensitivity >= SENS_FINE)
            screen.fillCircle(44, offset + 15, 6, WHITE);
        if (encoderSensitivity >= SENS_STANDARD)
            screen.fillCircle(64, offset + 15, 6, WHITE);
        if (encoderSensitivity >= SENS_COARSE)
            screen.fillCircle(84, offset + 15, 6, WHITE);
        if (encoderSensitivity >= SENS_SUPER_COARSE)
            screen.fillCircle(104, offset + 15, 6, WHITE);
    }

    void drawGates(uint8_t selected, ScreenBuffer& screen) {
        screen.setTextSize(1);
        int offset = 16;

        screen.drawCircle(15, offset + 15, 12, WHITE);
        if (getButtonValue(PUSHBUTTON)) {
            screen.fillCircle(15, offset + 15, 9, WHITE);
        }
        else {
            screen.drawCircle(15, offset + 15, 9, WHITE);
        }
        screen.print(2, offset + 38, "PUSH");

        screen.drawCircle(47, offset + 15, 12, WHITE);
        if (getButtonValue(BUTTON_A)) {
            screen.fillCircle(47, offset + 15, 9, WHITE);
        }
        else {
            screen.drawCircle(47, offset + 15, 9, WHITE);
        }
        screen.print(32, offset + 38, "A:IN1");

        screen.drawCircle(79, offset + 15, 12, WHITE);
        if (getButtonValue(BUTTON_B)) {
            screen.fillCircle(79, offset + 15, 9, WHITE);
        }
        else {
            screen.drawCircle(79, offset + 15, 9, WHITE);
        }
        screen.print(64, offset + 38, "B:IN2");

        screen.drawCircle(111, offset + 15, 12, WHITE);
        if (getButtonValue(BUTTON_C)) {
            screen.fillCircle(111, offset + 15, 9, WHITE);
        }
        else {
            screen.drawCircle(111, offset + 15, 9, WHITE);
        }
        screen.print(96, offset + 38, "C:OUT");

        if (selectedPid[1])
            screen.drawRectangle(2, offset + 2, 27, 27, WHITE);
        screen.drawRectangle(3, offset + 3, 25, 25, WHITE);
    }

    void drawScope(uint8_t selected, ScreenBuffer& screen) {
        scope.update();
        // scope.resync();
        uint8_t offset = 36;
        uint8_t y_prev = int16_t(scope.getBufferData()) * 32 / 128 + offset;
        // uint16_t(data[0]) * 36U / 255U;
        uint16_t step = 4;
        for (int x = step; x < screen.getWidth(); x += step) {
            uint8_t y = int16_t(scope.getBufferData()) * 32 / 128 + offset;
            // uint8_t y = uint16_t(data[x]) * 36 / 255 + offset;
            screen.drawLine(x - step, y_prev, x, y, WHITE);
            y_prev = y;
        }

        // Channel selection overlay
        if (activeEncoder == 1) {
            screen.setTextSize(1);
            offset = 26;
            for (int i = 0; i < num_channels; i++) {
                screen.print(7 + i * 30, offset, "IN ");
                screen.print(i + 1);
            }
            for (int i = 0; i < num_channels; i++) {
                screen.print(7 + i * 30, offset + 10, "OUT");
                screen.print(i + 1);
            }
            if (selectedPid[1] < num_channels) {
                screen.drawRectangle(
                    4 + selectedPid[1] * 30, offset - 10, 30, 10, WHITE);
            }
            else {
                screen.drawRectangle(4 + (selectedPid[1] - num_channels) * 30,
                    offset, 30, 10, WHITE);
            }
        }
    }

    void drawSystem(uint8_t selected, ScreenBuffer& screen) {
        screen.setTextSize(1);
        if (activeEncoder == 1)
            selected = min(selected, uint8_t(SYS_LAST));
        if (activeEncoder == 1 && selected < 1)
            screen.print(18, 24, "Caveat emptor!");

        if (selected > 0) {
            screen.setCursor(1, 24);
            screen.print(system_names[selected - 1]);
        };
        screen.setCursor(1, 24 + 10);
        screen.print(system_names[selected]);
        if (selected + 1 < uint8_t(SYS_LAST)) {
            screen.setCursor(1, 24 + 20);
            screen.print(system_names[selected + 1]);
        }
        if (activeEncoder == 0)
            screen.invert(0, 25, 128, 10);
        else
            screen.drawRectangle(0, 25, 128, 10, WHITE);
    }

    void drawParameterValues(ScreenBuffer& screen) {
        // draw 4x2x2 levels on bottom 8px row
        int x = 0;
        int y = 63 - 7;
        for (int i = 0; i < 16; ++i) {
            // 4px high by up to 16px long rectangle, filled if selected
            if (i == lastSelectedPid)
                screen.fillRectangle(x, y,
                    max(1, min(16, display_parameters[i] / 255)), 4, WHITE);
            else
                screen.drawRectangle(x, y,
                    max(1, min(16, display_parameters[i] / 255)), 4, WHITE);
            x += 16;
            if (i == 7) {
                x = 0;
                y += 3;
            }
        }
    }

    void drawStatus(ScreenBuffer& screen) {
        int offset = 16;
        screen.setTextSize(1);
        // single row
        screen.print(1, offset + 8, "mem ");
        ProgramVector* pv = getProgramVector();
        int mem = (int)(pv->heap_bytes_used) / 1024;
        if (mem > 999) {
            screen.print(mem / 1024);
            screen.print("M");
        }
        else {
            screen.print(mem);
            screen.print("k");
        }

        // draw flash load
        int flash_used = storage.getWrittenSize() / 1024;
        int flash_total = storage.getTotalAllocatedSize() / 1024;
        screen.print(64, offset + 8, "flash ");
        screen.print(flash_used * 100 / flash_total);
        screen.print("%");

        screen.setCursor(64, offset + 17);
        if (flash_used > 999) {
            screen.print(flash_used / 1024);
            screen.print(".");
            screen.print((int)((flash_used % 1024) * 10 / 1024));
            screen.print("M/");
        }
        else {
            screen.print(flash_used);
            screen.print("k/");
        }
        if (flash_total > 999) {
            screen.print(flash_total / 1024);
            screen.print(".");
            screen.print((int)((flash_total % 1024) * 10 / 1024));
            screen.print("M");
        }
        else {
            screen.print(flash_total);
            screen.print("k");
        }

        // draw CPU load
        screen.print(1, offset + 17, "cpu ");
#ifdef CPU_SMOOTH
        float new_cpu_used = (pv->cycles_per_block) / pv->audio_blocksize / 100;
        cpu_used = cpu_used * cpu_smooth_lambda + new_cpu_used -
            new_cpu_used * cpu_smooth_lambda;
#else
        cpu_used = (pv->cycles_per_block) / pv->audio_blocksize / 100;
#endif
        screen.print((int)cpu_used);
        screen.print("%");
        // draw firmware version
        screen.print(1, offset + 26, getFirmwareVersion());
        if (bootloader_token->magic == BOOTLOADER_MAGIC) {
            screen.print(" (bt.");
            screen.print(getBootloaderVersion());
            screen.print(")");
        }
    }

#ifdef USE_DIGITALBUS
    void drawBusStats(int y, ScreenBuffer& screen) {
        extern int busstatus;
        extern uint32_t bus_tx_packets;
        extern uint32_t bus_rx_packets;
        screen.setTextSize(1);
        screen.setCursor(2, y);
        switch (busstatus) {
        case BUS_STATUS_IDLE:
            screen.print("idle");
            break;
        case BUS_STATUS_DISCOVER:
            screen.print("disco");
            break;
        case BUS_STATUS_CONNECTED:
            screen.print("conn");
            break;
        case BUS_STATUS_ERROR:
            screen.print("err");
            break;
        }
        y += 10;
        screen.setCursor(2, y);
        screen.print("rx");
        screen.setCursor(16, y);
        screen.print(int(bus_rx_packets));
        y += 10;
        screen.setCursor(2, y);
        screen.print("tx");
        screen.setCursor(16, y);
        screen.print(int(bus_tx_packets));
    }
#endif

    void drawControlMode(ScreenBuffer& screen) {
        switch (controlMode) {
        case SETUP:
            drawTitle(controlModeNames[controlMode], screen);
            drawSetup(screen);
            break;
        case STATUS:
            drawTitle(controlModeNames[controlMode], screen);
            drawStatus(screen);
            drawMessage(screen, 55);
            break;
        case PATCH:
            drawTitle(controlModeNames[controlMode], screen);
            drawPatchNames(selectedPid[1], screen);
            break;
        case DATA:
            drawTitle(controlModeNames[controlMode], screen);
            drawResourceNames(selectedPid[1], screen);
            break;
        case SD_CARD:
            drawTitle(controlModeNames[controlMode], screen);
            drawSdCard(selectedPid[1], screen);
            break;
#ifdef USE_DIGITALBUS
        case BUS:
            drawTitle(controlModeNames[controlMode], screen);
            drawBusStats();
            break;
#endif
        case GATES:
            drawTitle(controlModeNames[controlMode], screen);
            drawGates(selectedPid[1], screen);
            break;
        case SCOPE:
            drawTitle(controlModeNames[controlMode], screen);
            drawScope(selectedPid[1], screen);
            break;
        case SYSTEM:
            drawTitle(controlModeNames[controlMode], screen);
            drawSystem(selectedPid[1], screen);
            break;
        case EXIT:
            drawTitle("done", screen);
            break;
        default:
            break;
        }
        // todo!
        // select: VU Meter, Patch Stats, Set Gain, Show MIDI, Reset Patch
    }

    void drawGlobalParameterNames(ScreenBuffer& screen) {
        screen.setTextSize(1);
        // Footer
        screen.setCursor(66, 24 + 20);
        screen.print("Shift");
        screen.setCursor(98, 24 + 20);
        screen.print("Value");
        char name_buf[11] = {};
        // First row
        if (selectedPid[0] > 0) {
            strncpy(name_buf, names[selectedPid[0] - 1], 10);
            screen.print(1, 24, name_buf);
        }
        // Second row
        strncpy(name_buf, names[selectedPid[0]], 10);
        screen.print(1, 24 + 10, name_buf);
        // Third row
        if (selectedPid[0] < SIZE - 1) {
            strncpy(name_buf, names[selectedPid[0] + 1], 10);
            screen.print(1, 24 + 20, name_buf);
        }
        // Offset and param value
        screen.setCursor(66, 24 + 10);
        char buf[6] = { 0, 0, 0 };
        strncpy(buf, msg_ftoa(float(user[selectedPid[0]]) * value_mul, 10), 5);
        if (user[selectedPid[0]] >= 0)
            buf[4] = 0;
        screen.print(buf);
        screen.setCursor(100, 24 + 10);
        buf[4] = 0;
        strncpy(buf, msg_ftoa(float(parameters[selectedPid[0]]) * value_mul, 10), 4);
        screen.print(buf);
        // Frame around value
        screen.invert(0, 25, 98, 10);
    }

    void drawParameterNames(ScreenBuffer& screen) {
        int y = 29;
        screen.setTextSize(1);
        int i = selectedPid[0];
        screen.print(1, y, names[i]);
        if (selectedPid[0] == i)
            screen.invert(0, y - 10, 64, 10);
        i += 1;
        screen.print(65, y, names[i]);
        if (selectedPid[0] == i)
            screen.invert(64, y - 10, 64, 10);
        y += 12;
        i += 7;
        screen.print(1, y, names[i]);
        if (selectedPid[0] == i)
            screen.invert(0, y - 10, 64, 10);
        i += 1;
        screen.print(65, y, names[i]);
        if (selectedPid[0] == i)
            screen.invert(64, y - 10, 64, 10);
    }

    void draw(ScreenBuffer& screen) {
        screen.clear();
        screen.setTextWrap(false);
        switch (owl.getOperationMode()) {
        case LOAD_MODE:
            drawTitle(screen);
            drawProgress(user[LOAD_INDICATOR_PARAMETER] * 127 / 4095, screen,
                "Uploading...");
            break;
        case ERASE_MODE:
            drawTitle(screen);
            drawProgress(user[LOAD_INDICATOR_PARAMETER] * 127 / 4095, screen,
                "Erasing...");
            break;
        case DEFRAG_MODE:
            drawTitle(screen);
            drawProgress(user[LOAD_INDICATOR_PARAMETER] * 127 / 4095, screen,
                "Defrag...");
            break;
        default:
            switch (displayMode) {
            case STANDARD:
                // draw most recently changed parameter
                // drawParameter(selectedPid[selectedBlock], 44, screen);
                drawParameter(selectedPid[0], 54, screen);
                // use callback to draw title and message
                drawCallback(
                    screen.getBuffer(), screen.getWidth(), screen.getHeight());
                break;
            case SELECTGLOBALPARAMETER:
                drawTitle(screen);
                drawGlobalParameterNames(screen);
                break;
            case CONTROL:
                drawControlMode(screen);
                break;
            case ERROR:
                drawTitle("ERROR", screen);
                drawError(screen);
                drawStats(screen);
                break;
            }
            drawParameters(screen);
        }
    }

    void drawPatchNames(int selected, ScreenBuffer& screen) {
        screen.setTextSize(1);
        selected = min(uint8_t(selected), registry.getNumberOfPatches() - 1);
        if (selected > 1) {
            screen.setCursor(1, 24);
            screen.print((int)selected - 1);
            screen.print(".");
            screen.print(registry.getPatchName(selected - 1));
        };
        screen.setCursor(1, 24 + 10);
        screen.print((int)selected);
        screen.print(".");
        screen.print(registry.getPatchName(selected));
        if (selected + 1 < (int)registry.getNumberOfPatches()) {
            screen.setCursor(1, 24 + 20);
            screen.print((int)selected + 1);
            screen.print(".");
            screen.print(registry.getPatchName(selected + 1));
        }
        if (activeEncoder == 0)
            screen.invert(0, 25, 128, 10);
        else
            screen.drawRectangle(0, 25, 128, 10, WHITE);
    }

    void drawResourceNames(int selected, ScreenBuffer& screen) {
        screen.setTextSize(1);
        if (resourceDelete)
            selected = min(uint8_t(selected), registry.getNumberOfResources());
        else
            selected = min(uint8_t(selected), registry.getNumberOfResources() - 1);
        if (resourceDelete && selected == 0)
            screen.print(18, 24, "Delete:");
        if (selected > 0 && registry.getNumberOfResources() > 0) {
            screen.setCursor(1, 24);
            screen.print((int)selected + MAX_NUMBER_OF_PATCHES);
            screen.print(".");
            screen.print(registry.getResourceName(MAX_NUMBER_OF_PATCHES + selected));
        };
        if (selected < (int)registry.getNumberOfResources()) {
            screen.setCursor(1, 24 + 10);
            screen.print((int)selected + 1 + MAX_NUMBER_OF_PATCHES);
            screen.print(".");
            screen.print(
                registry.getResourceName(MAX_NUMBER_OF_PATCHES + 1 + selected));
        }
        else if (resourceDelete) {
            screen.print(18, 24 + 10, "Exit");
        }
        if (selected + 1 < (int)registry.getNumberOfResources()) {
            screen.setCursor(1, 24 + 20);
            screen.print((int)selected + 2 + MAX_NUMBER_OF_PATCHES);
            screen.print(".");
            screen.print(
                registry.getResourceName(MAX_NUMBER_OF_PATCHES + 2 + selected));
        }
        else if (resourceDelete && selected < (int)registry.getNumberOfResources()) {
            screen.print(18, 24 + 20, "Exit");
        }

        if (resourceDelete)
            screen.drawRectangle(0, 25, 128, 10, WHITE);
        else
            screen.invert(0, 25, 128, 10);
    }

    void drawSdCard(int selected, ScreenBuffer& screen) {
        uint16_t offset = 24;
        screen.setTextSize(1);
        if (sd_initialized) {
            if (sd_browser.getState() == Browser::State::FF_RESET) {
                // Browser opened for first time
                sd_browser.openDir();
            }
            auto state = sd_browser.getState();
            if (state == Browser::State::FF_NO_CARD) {
                // No card inserted
                screen.print(28, offset + 10, "Insert card");
                screen.print(28, offset + 20, "and reboot");
            }
            else {
                if (state == Browser::State::FF_OPEN) {
                    if (sd_browser.isRoot()) {
                        // Root directory opened
                        screen.print(1, offset, "/..");
                    }
                    else {
                        // Subdirectory
                        screen.print(1, offset, "/");
                        screen.print(sd_browser.getDirName());
                        screen.print("/..");
                    }
                }
                // List files
                uint32_t sd_index = sd_browser.getIndex();
                offset += 10;
                bool end_reached = sd_browser.isEndReached();
                uint32_t num_files = sd_browser.getNumFiles();
                int last = end_reached ? min(sd_index + 3, num_files) : (sd_index + 3);
                for (int i = sd_index; i < last; i++) {
                    screen.setCursor(1, offset);
                    if (!end_reached || i < num_files) {
                        screen.print(i + 1);
                        screen.print(".");
                        screen.print(sd_browser.getFileName(i));
                        if (sd_browser.isDir(i)) {
                            screen.print("/..");
                        }
                    }
                    offset += 10;
                }
            }
            if (sdSelected)
                screen.drawRectangle(0, 25, 128, 10, WHITE);
            else
                screen.invert(0, 25, 128, 10);            
        }
        else {
            screen.print(28, offset + 10, "Insert card");
            screen.print(28, offset + 20, "and reboot");
        }
    }

    void drawProgress(uint8_t progress, ScreenBuffer& screen, const char* message) {
        // progress should be 0 - 127
        screen.drawRectangle(0, 26, 128, 20, WHITE);
        screen.setCursor(32, 36);
        screen.setTextSize(1);
        screen.print(message);
        screen.fillRectangle(0, 40, progress, 5, WHITE);
        ProgramVector* pv = getProgramVector();
        if (pv->message != NULL)
            screen.print(0, 55, pv->message);
    }

    void drawParameter(int pid, int y, ScreenBuffer& screen) {
        int x = 0;
        screen.setTextSize(1);
        screen.print(x, y, names[pid]);
        // 6px high by up to 96px long rectangle
        y -= 7;
        x += 64;
        screen.drawRectangle(
            x, y, max(1, min(64, display_parameters[pid] / 64)), 6, WHITE);
    }

    void drawParameters(ScreenBuffer& screen) {
        drawParameterValues(screen);
        int x = 0;
        // int y = 63-8;
        for (int i = 2; i < NOF_ENCODERS; ++i) {
            // screen.print(x+1, y, blocknames[i-1]);
            // if(selectedBlock == i)
            //  screen.drawHorizontalLine(x, y, 32, WHITE);
            // screen.invert(x, 63-8, 32, 8);
            // screen.invert(x, y-10, 32, 10);
            x += 32;
        }
    }

    void drawStats(ScreenBuffer& screen) {
        int offset = 0;
        screen.setTextSize(1);
        // screen.clear(86, 0, 128-86, 16);
        // draw memory use

        // two columns
        screen.print(80, offset + 8, "mem");
        ProgramVector* pv = getProgramVector();
        screen.setCursor(80, offset + 17);
        int mem = (int)(pv->heap_bytes_used) / 1024;
        if (mem > 999) {
            screen.print(mem / 1024);
            screen.print("M");
        }
        else {
            screen.print(mem);
            screen.print("k");
        }
        // draw CPU load
        screen.print(110, offset + 8, "cpu");
        screen.setCursor(110, offset + 17);
        screen.print((int)((pv->cycles_per_block) / pv->audio_blocksize) / 83);
        screen.print("%");
    }

    void drawError(ScreenBuffer& screen) {
        if (getErrorMessage() != NULL) {
            screen.setTextSize(1);
            screen.setTextWrap(true);
            screen.print(0, 26, getErrorMessage());
            screen.setTextWrap(false);
        }
    }

    int16_t getEncoderValue(uint8_t eid) const {
        return (encoders[eid] - offsets[eid]) << encoderSensitivity;
    }

    void setEncoderValue(uint8_t eid, int16_t value) {
        offsets[eid] = encoders[eid] - (value >> encoderSensitivity);
    }

    void setName(uint8_t pid, const char* name) {
        if (pid < SIZE)
            strncpy(names[pid], name, 15);
    }

    void setTitle(const char* str) {
        strncpy(title, str, 12);
    }

    uint8_t getSize() const {
        return SIZE;
    }

    void drawMessage(ScreenBuffer& screen, int y = 24) {
        ProgramVector* pv = getProgramVector();
        if (pv->message != NULL) {
            screen.setTextSize(1);
            screen.setTextWrap(true);
            screen.print(0, y, pv->message);
            screen.setTextWrap(false);
        }
    }

    void drawTitle(ScreenBuffer& screen) {
        drawTitle(title, screen);
    }

    void drawTitle(const char* title, ScreenBuffer& screen) {
        // draw title
        screen.setTextSize(2);
        screen.print(1, 17, title);
    }

    void setCallback(void* callback) {
        if (callback == NULL)
            drawCallback = defaultDrawCallback;
        else
            drawCallback = (void (*)(uint8_t*, uint16_t, uint16_t))callback;
    }

    // called by MIDI cc and/or from patch
    void setValue(uint8_t pid, int16_t value) {
        if (pid >= NOF_ADC_VALUES)
            user[pid] = value;
        // reset encoder value if associated through selectedPid to avoid skipping
        for (int i = 0; i < NOF_ENCODERS; ++i)
            if (selectedPid[i] == pid)
                setEncoderValue(i, value);
        // TODO: store values set from patch somewhere and multiply with user[]
        // value for outputs graphics.params.updateOutput(i, getOutputValue(i));
    }

    // @param value is the modulation ADC value
    void updateValue(uint8_t pid, int16_t value) {
        // smoothing at apprx 50Hz
        parameters[pid] =
            max(0, min(4095, (parameters[pid] + user[pid] + value) >> 1));
        if (abs(parameters[pid] - display_parameters[pid]) > HYSTERESIS)
            display_parameters[pid] = parameters[pid];
    }

    void updateOutput(uint8_t pid, int16_t value) {
        parameters[pid] = max(0,
            min(4095, (((parameters[pid] + (user[pid] * value)) >> 12) >> 1)));
    }

    /*
    void updateValues(uint16_t* data, uint8_t size){
      for(int i=0; i<16; ++i)
        parameters[i] = data[i];
    }
    */

    void setControlModeValue(uint8_t value) {
        switch (controlMode) {
        case SETUP:
            value = max((uint8_t)SENS_SUPER_FINE,
                min((uint8_t)SENS_SUPER_COARSE, value));
            if (value > (uint8_t)encoderSensitivity) {
                encoderSensitivity = (EncoderSensitivity)((uint8_t)encoderSensitivity +
                    ENC_MULTIPLIER / 2);
                value = (uint8_t)encoderSensitivity;
            }
            else if (value < (uint8_t)encoderSensitivity) {
                encoderSensitivity = (EncoderSensitivity)((uint8_t)encoderSensitivity -
                    ENC_MULTIPLIER / 2);
                value = (uint8_t)encoderSensitivity;
            }
            selectedPid[1] = value;
            break;
        case PATCH:
            selectedPid[1] = max(1, min(registry.getNumberOfPatches() - 1, value));
            break;
        case DATA:
            if (resourceDelete)
                selectedPid[1] = min(registry.getNumberOfResources(), value);
            else
                selectedPid[1] = min(registry.getNumberOfResources() - 1, value);
            break;
        case SD_CARD:
            if (sdSelected) {
                if (value > selectedPid[1]) {
                    sd_browser.next();
                }
                else if (value < selectedPid[1]) {
                    sd_browser.prev();
                }
                selectedPid[1] = value;
            }
            break;
        case GATES:
            break;
        case SCOPE:
            selectedPid[1] = min(num_channels * 2 - 1, value);
            lastChannel = selectedPid[1];
            if (AUDIO_CHANNELS == num_channels)
                scope.setChannel(lastChannel);
            else {
                scope.setChannel((lastChannel >= num_channels)
                        ? (AUDIO_CHANNELS - num_channels + lastChannel)
                        : lastChannel);
            }
            break;
        case SYSTEM:
            selectedPid[1] = min(int(SYS_LAST) - 1, value);
            break;
        default:
            break;
        }
    }

    void updateEncoders(int16_t* data, uint8_t size) {
        uint16_t pressed = data[0];
        int16_t value = data[1];
        bool isPress = pressed & 0x02;
        bool isLongPress = pressed & 0x04;

        switch (displayMode) {
        case STANDARD:
        case ERROR:
            if (displayMode != ERROR && getErrorStatus() && getErrorMessage() != NULL)
                displayMode = ERROR;
            if (isLongPress) {
                // Long press switches to global parameter selection
                displayMode = SELECTGLOBALPARAMETER;
                activeEncoder = 0;
                // selectedPid[0] = lastSelectedPid;
            }
            else if (isPress) {
                // Go to control mode
                displayMode = CONTROL;
                controlMode = SETUP;
                activeEncoder = 0;
                selectedPid[1] = encoderSensitivity;
            }
            else {
                if (encoders[0] != value) {
                    encoders[0] = value; // min(4095, max(0, value));
                    // We must update encoder value before calculating user
                    // value, otherwise previous value would be displayed
                    user[selectedPid[0]] = getEncoderValue(0);
                }
            }
            break;
        case CONTROL: {
            selectControlMode(value, pressed); // action if encoder is pressed
            if (isLongPress) {
                // Long press exits menu
                controlMode = EXIT;
            }
            break;
        }
        case SELECTGLOBALPARAMETER:
            if (!isLongPress) {
                displayMode = STANDARD;
            }
            else {
                // Selected another parameter
                int16_t delta = value - encoders[0];
                if (delta < 0) {
                    encoders[0] = value;
                    selectGlobalParameter(selectedPid[0] - 1);
                }
                else if (delta > 0) {
                    encoders[0] = value;
                    selectGlobalParameter(selectedPid[0] + 1);
                }
            }
            break;
        }
    }

    void encoderChanged(uint8_t encoder, int32_t delta) const {
    }

    void selectGlobalParameter(int8_t pid) {
        lastSelectedPid = pid;
        selectedPid[0] = max(0, min(SIZE - 1, pid));
        setEncoderValue(0, user[selectedPid[0]]);
    }

    void setControlMode(uint8_t value) {
        controlMode = (ControlMode)value;
        switch (controlMode) {
        case SETUP:
        case STATUS:
        case GATES:
            break;
        case PATCH:
            selectedPid[1] = settings.program_index;
            break;
        case DATA:
            selectedPid[1] = lastSelectedResource;
            resourceDelete = false;
            break;
        case SD_CARD:
            selectedPid[1] = sd_browser.getIndex();
            sdSelected = false;
            break;
        case SCOPE:
            selectedPid[1] = lastChannel;
        case SYSTEM:
            systemPressed = false;
        default:
            break;
        }
    }

    void selectControlMode(int16_t value, uint8_t pressed) {
        if (pressed & 0x04) {
            // Long press end
            if (controlMode == GATES) {
                setButtonValue(PUSHBUTTON, 0);
                selectedPid[1] = 0;
            }
            controlMode = EXIT;
        }
        else if (pressed & 0x02) {
            // Short press end
            switch (controlMode) {
            case SETUP:
            case SCOPE:
                activeEncoder = !activeEncoder;
                break;
            case GATES:
                // Invert output gate on click
                setButtonValue(PUSHBUTTON, 0);
                selectedPid[1] = 0;
                break;
            case STATUS:
                setErrorStatus(NO_ERROR);
                break;
            case SYSTEM: {
                if (!systemPressed && activeEncoder == 1) {
                    systemPressed = true;
                    handleSystem(SystemAction(selectedPid[1]));
                }
                else {
                    activeEncoder = 1;
                }
                break;
            }
            case PATCH:
                // load patch
                if (activeEncoder == 1) {
                    settings.program_index = selectedPid[1];
                    program.loadProgram(settings.program_index);
                    program.resetProgram(false);
                    controlMode = EXIT;
                }
                else {
                    activeEncoder = 1;
                }
                break;
            case DATA:
                if (resourceDelete) {
                    if (selectedPid[1] == registry.getNumberOfResources()) {
                        // Exit on last menu item (exit link after resources list)
                        resourceDelete = false;
                        resourceDeletePressed = false;
                        controlMode = EXIT;
                        activeEncoder = 0;
                    }
                    else if (!resourceDeletePressed) {
                        // Delete resource unless it's protected by "__" prefix
                        resourceDeletePressed = true;
                        ResourceHeader* res = registry.getResource(
                            selectedPid[1] + MAX_NUMBER_OF_PATCHES + 1);
                        if (res != NULL) {
                            if (res->name[0] == '_' && res->name[1] == '_') {
                                debugMessage("Resource protected");
                            }
                            else {
                                program.exitProgram(false);
                                registry.setDeleted(
                                    selectedPid[1] + MAX_NUMBER_OF_PATCHES + 1);
                                program.startProgram(false);
                            }
                        }
                    }
                }
                else {
                    resourceDelete = true;
                    resourceDeletePressed = true;
                    activeEncoder = 1;
                }
                break;
            case SD_CARD:
                if (sdSelected) {
                    if (sd_browser.isRoot()) {
                        if (sd_browser.isDir(sd_browser.getIndex())) {
                            // Directory clicked
                            sd_browser.openDir(false);
                            break;
                        }
                    }
                    else {
                        sd_browser.openDir(true);
                    }
                    sdSelected = false;
                    activeEncoder = 0;
                }
                else {
                    sdSelected = true;
                    activeEncoder = 1;
                }
            default:
                break;
            }
        }
        else if (pressed & 0x01) {
            if (controlMode == GATES) {
                setButtonValue(PUSHBUTTON, 1);
                selectedPid[1] = 1;
            }
        }
        if (controlMode == EXIT) {
            displayMode = STANDARD;
            activeEncoder = 1;
            selectedPid[0] = lastSelectedPid;
            selectedPid[1] = parameters[lastSelectedPid];
            if (saveSettings)
                settings.saveToFlash();
        }
        else {
            int16_t delta = value - encoders[activeEncoder];
            if (activeEncoder == 0) {
                if (delta > 0 && controlMode + 1 < NOF_CONTROL_MODES) {
                    setControlMode(controlMode + 1);
                }
                else if (delta < 0 && controlMode > 0) {
                    setControlMode(controlMode - 1);
                }
            }
            else {
                if (delta > 0 && selectedPid[1] < 127) {
                    setControlModeValue(selectedPid[1] + 1);
                }
                else if (delta < 0 && selectedPid[1] > 0) {
                    setControlModeValue(selectedPid[1] - 1);
                }
                if (controlMode == DATA && resourceDeletePressed) {
                    resourceDeletePressed = false;
                }
            }
            encoders[0] = value;
            encoders[1] = value;
        }
    }

    void handleSystem(SystemAction action) {
        controlMode = EXIT;
        switch (action) {
        case SYS_BOOTLOADER:
            jump_to_bootloader();
            break;
        case SYS_RESET:
            device_reset();
            break;
        case SYS_ERASE:
            owl.setBackgroundTask(&erase_task);
            break;
        case SYS_DEFRAG:
            owl.setBackgroundTask(&defrag_task);
            break;
        case SYS_EXIT:
            break;
        default:
            break;
        }
    }

private:
    void (*drawCallback)(uint8_t*, uint16_t, uint16_t);
};

#endif // __ParameterController_hpp__
