#ifndef __PreenFM3ParameterController_hpp__
#define __PreenFM3ParameterController_hpp__

#include <algorithm>
#include "EncodersListener.h"
#include "Storage.h"
#include "Owl.h"
#include "OpenWareMidiControl.h"
#include "ProgramVector.h"
#include "PatchRegistry.h"
#include "ProgramManager.h"
#include "Graphics.h"
#include "ScreenBuffer.h"
#include "VersionToken.h"
#include "device.h"
#include "errorhandlers.h"
#include "message.h"
#include "StorageTasks.hpp"
#include <stdint.h>
#include <string.h>
#ifdef USE_DIGITALBUS
#include "DigitalBusReader.h"
extern DigitalBusReader bus;
#endif

#ifndef clamp
#define clamp(x, a, b) (((x) > (b)) ? (b) : (((x) < (a)) ? (a) : (x)))
#endif
#define ENC_MULTIPLIER 6 // shift left by this many steps

#define MAX_MESSAGE 256

extern VersionToken* bootloader_token;

extern uint32_t tftDirtyBits;

extern void pfmDefaultDrawCallback(uint8_t* pixels, uint16_t width, uint16_t height);

enum Menu : uint8_t {
    MENU_MAIN,
    MENU_PARAMS,
    MENU_PATCHES,
    MENU_RESOURCES,
    MENU_SYSTEM,
};

template <uint8_t SIZE>
class ControllerState {
public:
    // Section 1
    char title[17] = "PreenFM3";
    // Section 2
    uint8_t cpu_load = 0;
    uint8_t mem_kbytes_used = 0;
    uint16_t flash_used = 0;
    uint8_t fps = 0;
    // Section 3
    char message[MAX_MESSAGE] = {0};
    // Section 4
    Menu menu = MENU_MAIN;
    uint8_t menu_key = 0;
    uint8_t custom_callback = 0;
    // Section 5
    uint8_t active_buttons = 0;
    uint8_t active_param_id = 0;
    char active_param_name[12];
    uint8_t active_param_value = 0;
    uint16_t active_param_value_pct = 0;
    // Section 6
    uint8_t params[SIZE] = {};
    uint8_t blocks[NOF_ENCODERS] = {};

    void update() {
        ProgramVector* pv = getProgramVector();
        float cpu_load_percent = (pv->cycles_per_block / pv->audio_blocksize) /
            (float)ARM_CYCLES_PER_SAMPLE * 100.f;
        cpu_load = cpu_load_percent;
        mem_kbytes_used = (int)(pv->heap_bytes_used) / 1024;
        flash_used = storage.getUsedSize() / 1024;
        //message = pv->message;
        strncpy(message, pv->message, MAX_MESSAGE);
    }
};

/* shows a single parameter selected and controlled with a single encoder
 */
class PreenFM3ParameterController : public EncodersListener, public ParameterController {
public:
    //int16_t parameters[NOF_PARAMETERS];
    //char names[NOF_PARAMETERS][12];
    int8_t selected = 0;
    ControllerState<NOF_VISIBLE_PARAMETERS> current_state;
    ControllerState<NOF_VISIBLE_PARAMETERS> prev_state;
    uint32_t dirty;

    const char system_names[SYS_LAST + 1][16] = {
        "Bootloader",
        "Erase patches",
        "Reboot",
    };

    enum EncoderSensitivity {
        SENS_SUPER_FINE = 0,
        SENS_FINE = (ENC_MULTIPLIER / 2),
        SENS_STANDARD = ENC_MULTIPLIER,
        SENS_COARSE = (3 * ENC_MULTIPLIER / 2),
        SENS_SUPER_COARSE = (ENC_MULTIPLIER * 2)
    };
    EncoderSensitivity encoderSensitivity = SENS_STANDARD;
    // Sensitivity is currently not stored in settings, but it's not reset either.
    bool sensitivitySelected;

    PreenFM3ParameterController() {
        nextListener = 0;
        dirty = 0;
        reset();
        debugMessage("Das also war des Pudels Kern!");
    }
    void setTitle(const char* str) {
        strncpy(current_state.title, str, 16);
    }
    void reset() {
        dirty = 0b111111;
        drawCallback = pfmDefaultDrawCallback;
        for (int i = 0; i < NOF_PARAMETERS; ++i) {
            strncpy(names[i], "Parameter  ", 12);
            parameters[i] = 0;
            if (i < 8)
                names[i][10] = 'A' + i;
            else {
                names[i][9] = 'A' - 1 + (i / 8);
                names[i][10] = 'A' + (i & 0x7);
            }
        }
        current_state.active_param_id = 0;
        updateActiveParameter(0, 0);
    }

    void findChanges() {
        if (strncmp(current_state.title, prev_state.title, 12) != 0)
            dirty |= 1;
        if (memcmp((void*)(&current_state.cpu_load),
                (void*)(&prev_state.cpu_load), 5) != 0)
            dirty |= 2;
        if (strncmp(current_state.message, prev_state.message, MAX_MESSAGE) != 0)
            // if (message != other.message)
            dirty |= 4;
        if (current_state.custom_callback ||
            current_state.menu_key != prev_state.menu_key ||
            current_state.menu != prev_state.menu)
            dirty |= 8;
        if (memcmp((void*)&current_state.active_buttons,
                (void*)&prev_state.active_buttons, 17) != 0)
            dirty |= 16;
        if (memcmp((void*)current_state.params, (void*)prev_state.params,
                sizeof(current_state.params) + sizeof(current_state.blocks)) != 0)
            dirty |= 32;
    }

    void updateState() {
        findChanges();
        prev_state = current_state;
        // memcpy((void*)&prev_state, (void*)&current_state,
        //    sizeof(ControllerState<SIZE>));
        current_state.update();
    }

    void draw(uint8_t* pixels, uint16_t width, uint16_t height) {
        ScreenBuffer screen(width, height);
        screen.setBuffer(pixels);
        draw(screen);
    }

    void draw(ScreenBuffer& screen) {
        if (dirty & 1) {
            drawTitle(screen);
            tftDirtyBits |= 1;
            dirty &= ~1;
        }
        else if (dirty & 2) {
            drawStats(screen);
            tftDirtyBits |= 2;
            dirty &= ~2;
        }
        else if (dirty & 4) {
            drawMessage(screen);
            tftDirtyBits |= 4;
            dirty &= ~4;
        }
        else if (dirty & 8) { // 8 or 16
            screen.fillRectangle(0, 0, 240, 214, BLACK);
            switch (current_state.menu) {
            case MENU_MAIN:
                /*
                    screen.setTextSize(2);
                    screen.setCursor(0, 20);
                    screen.setTextColour(RED);
                    screen.print("RED");
                    screen.setCursor(0, 40);
                    screen.setTextColour(GREEN);
                    screen.print("GREEN");
                    screen.setCursor(0, 60);
                    screen.setTextColour(BLUE);
                    screen.print("BLUE");
                    screen.setCursor(0, 80);
                    screen.setTextColour(CYAN);
                    screen.print("CYAN");
                    screen.setCursor(0, 100);
                    screen.setTextColour(MAGENTA);
                    screen.print("MAGENTA");
                    screen.setCursor(0, 120);
                    screen.setTextColour(YELLOW);
                    screen.print("YELLOW");
                    screen.setCursor(0, 140);
                    screen.setTextColour(WHITE);
                    screen.print("WHITE");
                    screen.setCursor(0, 160);
                    screen.setTextColour(BLACK);
                    screen.print("BLACK");
                    screen.setTextSize(1);
                */
                drawCallback(screen.getBuffer(), 240, 214);
                break;
            case MENU_PARAMS:
                drawParamsMenu(screen);
                break;
            case MENU_PATCHES:
                drawPatchesMenu(screen);
                break;
            case MENU_RESOURCES:
                drawResourcesMenu(screen);
                break;
            case MENU_SYSTEM:
                drawSystemMenu(screen);
                break;
            }
            tftDirtyBits |= 8;
            dirty &= ~8;
        }
        else if (dirty & 16) {
            drawParameter(screen);
            tftDirtyBits |= 16;
            dirty &= ~16;
        }
        else if (dirty & 32) {
            drawParameterValues(screen);
            tftDirtyBits |= 32;
            dirty &= ~32;
        }
    }

    void drawTitle(ScreenBuffer& screen) {
        // clear
        screen.fillRectangle(0, 0, 240, 16, BLACK);

        // draw title
        screen.setTextColour(CYAN);
        screen.setTextSize(2);
        screen.print(1, 16, current_state.title);

        // draw firmware version
        screen.setTextSize(1);
        screen.setCursor(120, 12);
        screen.print(FIRMWARE_VERSION);

        // bootloader string
        if (bootloader_token->magic == BOOTLOADER_MAGIC) {
            screen.print(" bt.");
            screen.print(getBootloaderVersion());
        }
    }

    void drawStats(ScreenBuffer& screen) {
        // clear
        screen.fillRectangle(0, 0, 240, 10, BLACK);

        // cpu / mem stats
        screen.setTextSize(1);
        screen.setTextColour(GREEN);
        screen.print(2, 10, "cpu:");
        screen.print((int)current_state.cpu_load);
        screen.print("%");
        screen.print(50, 10, "mem:");
        screen.print((int)current_state.mem_kbytes_used);
        screen.print("kB");

        // draw flash usage
        int flash_used = current_state.flash_used;
        int flash_total = storage.getTotalCapacity() / 1024;
        screen.print(110, 10, "flash:");
        screen.print(flash_used * 100 / flash_total);
        screen.print("% ");
        if (flash_used > 999) {
            screen.print(flash_used / 1024);
            screen.print(".");
            screen.print((int)((flash_used % 1024) * 10 / 1024));
            screen.print("M");
        }
        else {
            screen.print(flash_used);
            screen.print("k");
        }

        screen.print(200, 10, "FR:");
        screen.print(current_state.fps);

#ifdef USE_DIGITALBUS
        screen.print(1, 56, "Bus: ");
        screen.print(bus.getStatusString());
        screen.print(" ");
        extern uint32_t bus_tx_packets, bus_rx_packets;
        screen.print((int)bus_tx_packets);
        screen.print("/");
        screen.print((int)bus_rx_packets);
        if (bus.getStatus() == BUS_STATUS_CONNECTED) {
            screen.print(", ");
            screen.print(bus.getPeers());
            screen.print(" peers");
        }
#endif
    }

    void drawMessage(ScreenBuffer& screen) {
        // clear
        screen.fillRectangle(0, 0, 240, 10, BLACK);

        // draw message if necessary
        ProgramVector* pv = getProgramVector();
        if (pv->message != 0) {
            screen.setTextSize(1);
            if (getErrorStatus() == NO_ERROR) {
                screen.setTextColour(GREEN);
            }
            else {
                screen.setTextColour(RED);
            }
            // screen.setTextWrap(true);
            // TODO: add scrolling here?
            screen.print(2, 10, pv->message);
            // screen.setTextWrap(false);
        }
    }

    void drawParameter(ScreenBuffer& screen) {
        // clear
        screen.fillRectangle(0, 0, 240, 30, BLACK);

        // Active parameter
        screen.setTextColour(MAGENTA);
        screen.setTextSize(1);
        screen.setCursor(1, 10);
        screen.print(current_state.active_param_name);
        screen.print(": 0.");
        int current_value = current_state.active_param_value_pct;
        if (current_value > 0) {
            if (current_value < 10) {
                screen.print("00");
            }
            else if (current_value < 100) {
                screen.print("0");
            }
        }
        screen.print(current_value);
        screen.fillRectangle(2, 12, 116, 16, YELLOW);
        screen.fillRectangle(3 + current_state.active_param_value, 13,
            114 - current_state.active_param_value, 14, BLACK);

        // buttons
        int x = 120;
        screen.setCursor(120 + 11, 10);
        screen.print("Buttons:");
        for (uint8_t i = 0; i < 3; i++) {
            int y = 10;
            screen.setCursor(x + 3, y + 11);
            screen.print((int)i * 2 + 1);
            if (current_state.active_buttons & (1 << i)) {
                screen.fillRectangle(x + 11, y + 2, 28, 7, YELLOW);
            }
            else {
                screen.drawRectangle(x + 11, y + 2, 28, 7, YELLOW);
            }
            y += 11;
            screen.setCursor(x + 3, y + 10);
            screen.print((int)i * 2 + 2);
            if (current_state.active_buttons & (1 << (i + 3))) {
                screen.fillRectangle(x + 11, y, 28, 7, YELLOW);
            }
            else {
                screen.drawRectangle(x + 11, y, 28, 7, YELLOW);
            }
            x += 40;
        }
    }

    void drawParameterValues(ScreenBuffer& screen) {
        // clear
        screen.fillRectangle(0, 0, 240, 40, BLACK);
        // draw blocks
        int block = 2;
        int x = 0;
        for (int i = 0; i < 6; i++) {
            int y = 0;
            uint8_t active = current_state.blocks[i];
            for (int j = 0; j < 4; j++) {
                int val = current_state.params[j * 6 + i];
                if (active == j) {
                    screen.fillRectangle(x + 1, y + 1, 38, 8, YELLOW);
                    screen.fillRectangle(x + 3 + val, y + 3, 34 - val, 4, BLACK);
                }
                else {
                    screen.fillRectangle(x + 2, y + 2, 36, 6, WHITE);
                    screen.fillRectangle(x + 3 + val, y + 3, 34 - val, 4, BLACK);
                }
                y += 10;
            }
            x += 40;
        }
    }

    void drawParamsMenu(ScreenBuffer& screen) {
        // clear
        screen.setTextSize(2);
        // header
        screen.setTextColour(WHITE);
        screen.print(20, 16, "Patch Parameters:");
        // draw menu
        screen.setTextColour(CYAN);
        uint8_t first_item, last_item, highlight_pos;
        if (current_state.menu_key < 11) {
            highlight_pos = current_state.menu_key;
            first_item = 0;
            last_item = 10;
        }
        else if (current_state.menu_key < NOF_PARAMETERS - 2) {
            highlight_pos = 10;
            first_item = current_state.menu_key - 9;
            last_item = current_state.menu_key;
        }
        else if (current_state.menu_key < NOF_PARAMETERS - 1) {
            highlight_pos = 10;
            first_item = current_state.menu_key - 9;
            last_item = current_state.menu_key + 1;
        }
        else { // Last param
            highlight_pos = 11;
            first_item = current_state.menu_key - 10;
            last_item = current_state.menu_key;
        }
        screen.fillRectangle(0, 16 * highlight_pos + 16, 240, 16, MAGENTA);

        screen.print(180, 16 * highlight_pos + 32, "0.");
        int current_value = current_state.active_param_value_pct;
        if (current_value > 0) {
            if (current_value < 8) {
                screen.print("00");
            }
            else if (current_value < 100) {
                screen.print("0");
            }
        }
        screen.print(current_value);

        uint8_t y = first_item == 0 ? 32 : 48;
        if (current_state.menu_key > 10)
            screen.print(1, 32, " ...");
        if (current_state.menu_key < NOF_PARAMETERS - 2)
            screen.print(1, 16 * 13, " ...");
        for (uint8_t i = first_item; i <= last_item; i++) {
            screen.print(1, y, names[i]);
            y += 16;
        }
    }

    void drawPatchesMenu(ScreenBuffer& screen) {
        screen.setTextSize(2);

        screen.setTextColour(WHITE);
        screen.print(20, 16, "Patches:");

        screen.setTextColour(CYAN);

        uint32_t size = registry.getNumberOfPatches();

        uint8_t first_item, last_item, highlight_pos;
        if (size > 12) {
            if (current_state.menu_key < 11) {
                highlight_pos = current_state.menu_key;
                first_item = 0;
                last_item = std::min<uint8_t>(size - 1, 10);
            }
            else if (current_state.menu_key + 2 < size) {
                highlight_pos = 10;
                first_item = current_state.menu_key - 9;
                last_item = current_state.menu_key;
            }
            else if (current_state.menu_key + 1 < size) {
                highlight_pos = 10;
                first_item = current_state.menu_key - 9;
                last_item = current_state.menu_key + 1;
            }
            else {
                highlight_pos = 11;
                first_item = current_state.menu_key - 10;
                last_item = current_state.menu_key;
            }
            if (current_state.menu_key > 10)
                screen.print(1, 32, " ...");
            if (current_state.menu_key + 2 < size)
                screen.print(1, 16 * 13, " ...");
        }
        else {
            first_item = 0;
            last_item = size > 0 ? (size - 1) : 0;
            highlight_pos = current_state.menu_key;
        }
        screen.fillRectangle(0, 16 * highlight_pos + 16, 240, 16, MAGENTA);

        screen.print(180, 16 * highlight_pos + 32, "Load");

        uint8_t y = first_item == 0 ? 32 : 48;

        for (uint8_t i = first_item; i <= last_item; i++) {
            screen.setCursor(0, y);
            screen.print((int)i);
            screen.print(" ");
            screen.print(registry.getPatchName(i));
            y += 16;
        }
    }

    void drawResourcesMenu(ScreenBuffer& screen) {
        screen.setTextSize(2);

        screen.setTextColour(WHITE);
        screen.print(20, 16, "Resources:");

        screen.setTextColour(CYAN);

        uint32_t size = registry.getNumberOfResources();

        uint8_t first_item, last_item, highlight_pos;
        if (size > 12) {
            if (current_state.menu_key < 11) {
                highlight_pos = current_state.menu_key;
                first_item = 0;
                last_item = std::min<uint8_t>(size - 1, 10);
            }
            else if (current_state.menu_key + 2 < size) {
                highlight_pos = 10;
                first_item = current_state.menu_key - 9;
                last_item = current_state.menu_key;
            }
            else if (current_state.menu_key + 1 < size) {
                highlight_pos = 10;
                first_item = current_state.menu_key - 9;
                last_item = current_state.menu_key + 1;
            }
            else {
                highlight_pos = 11;
                first_item = current_state.menu_key - 10;
                last_item = current_state.menu_key;
            }
            if (current_state.menu_key > 10)
                screen.print(1, 32, " ...");
            if (current_state.menu_key + 2 < size)
                screen.print(1, 16 * 13, " ...");
        }
        else {
            first_item = 0;
            last_item = size > 0 ? (size - 1) : 0;
            highlight_pos = current_state.menu_key;
        }
        screen.fillRectangle(0, 16 * highlight_pos + 16, 240, 16, MAGENTA);

        screen.print(180, 16 * highlight_pos + 32, "Load");

        uint8_t y = first_item == 0 ? 32 : 48;

        for (uint8_t i = first_item; i <= last_item; i++) {
            screen.setCursor(0, y);
            screen.print((int)i);
            screen.print(" ");
            screen.print(registry.getResourceName(i));
            y += 16;
        }
    }

    void drawSystemMenu(ScreenBuffer& screen) {
        screen.setTextSize(2);

        screen.setTextColour(WHITE);
        screen.print(20, 16, "System:");

        screen.setTextColour(CYAN);

        screen.fillRectangle(0, 16 * current_state.menu_key + 16, 240, 16, MAGENTA);

        for (uint8_t i = 0; i <= SYS_LAST; i++) {
            screen.setCursor(0, 16 * i + 32);
            screen.print(system_names[i]);            
        }
    }

    void setName(uint8_t pid, const char* name) {
        if (pid < NOF_PARAMETERS)
            strncpy(names[pid], name, 11);
    }
    uint8_t getSize() {
        return NOF_PARAMETERS;
    }

    void drawMessage(int16_t y, ScreenBuffer& screen) {
        drawMessage(screen);
    }

    void setValue(uint8_t ch, int16_t value) {
        parameters[ch] = value;
        current_state.params[ch] = clamp(value / 120, 0, 34);
        if (ch == current_state.active_param_id) {
            updateActiveParameter(ch, value);
        }
    }

    void updateActiveParameter(uint8_t ch, int16_t value) {
        strncpy(current_state.active_param_name,
            names[current_state.active_param_id], 12);
        current_state.active_param_value = clamp(parameters[ch] / 35, 0, 114);
        current_state.active_param_value_pct =
            int(clamp(parameters[ch], 0, 4095) * 1000 / 4096);
        // if (current_state.menu == MENU_PARAMS && ch == current_state.menu_key)
        //    dirty |= 8;
    }

    void setCallback(void* callback) {
        if (callback == NULL || callback == pfmDefaultDrawCallback) {
            drawCallback = pfmDefaultDrawCallback;
            current_state.custom_callback = false;
        }
        else {
            drawCallback = (void (*)(uint8_t*, uint16_t, uint16_t))callback;
            current_state.custom_callback = true;
        }
    }

    void encoderPressed(int encoder) override {
        uint8_t active = current_state.blocks[encoder] * 6 + encoder;
        current_state.active_param_id = active;
        updateActiveParameter(active, parameters[active]);
    }

    void encoderTurned(int encoder, int ticks) override {
        uint8_t pid = current_state.blocks[encoder] * 6 + encoder;
        current_state.active_param_id = pid;
        setValue(pid, parameters[pid] + ticks * encoderSensitivity);
    };

    void encoderTurnedWhileButtonPressed(int encoder, int ticks, int button) override {
        uint8_t active = current_state.blocks[encoder] + ((ticks > 0) ? 1 : -1);
        active &= 0b11;
        current_state.blocks[encoder] = active;
        active = active * 6 + encoder;
        current_state.active_param_id = active;
        updateActiveParameter(active, parameters[active]);
    };

    void buttonPressStarted(int button) override {
        if (button < PFM_MENU_BUTTON) {
            current_state.active_buttons |= 1 << uint8_t(button);
            setButtonValue(BUTTON_A + button, 1);
            if (button == 0)
                setButtonValue(PUSHBUTTON, 1);
        }
    };

    void buttonPressed(int button) override {
        if (button < PFM_MENU_BUTTON) {
            current_state.active_buttons &= ~(1 << uint8_t(button));
            setButtonValue(BUTTON_A + button, 0);
            if (button == 0)
                setButtonValue(PUSHBUTTON, 0);
        }
        else {
            switch (current_state.menu) {
            case MENU_MAIN:
                switch (button) {
                case PFM_MINUS_BUTTON:
                    setValue(current_state.active_param_id,
                        parameters[current_state.active_param_id] - encoderSensitivity);
                    break;
                case PFM_PLUS_BUTTON:
                    setValue(current_state.active_param_id,
                        parameters[current_state.active_param_id] + encoderSensitivity);
                    break;
                case PFM_MENU_BUTTON:
                    current_state.menu = MENU_PARAMS;
                    current_state.menu_key = current_state.active_param_id;
                    // dirty |= 8;
                    break;
                case PFM_PATCHES_BUTTON:
                    current_state.menu = MENU_PATCHES;
                    current_state.menu_key = program.getProgramIndex();
                    // dirty |= 8;
                    break;
                case PFM_RESOURCES_BUTTON:
                    current_state.menu = MENU_RESOURCES;
                    current_state.menu_key = 0;
                    // dirty |= 8;
                    break;
                case PFM_SYSTEM_BUTTON:
                    current_state.menu = MENU_SYSTEM;
                    current_state.menu_key = 0;
                    // dirty |= 8;
                    break;
                default:
                    break;
                }
                break;
            case MENU_PARAMS:
                switch (button) {
                case PFM_MENU_BUTTON:
                    current_state.menu = MENU_MAIN;
                    current_state.active_param_id = current_state.menu_key;
                    // dirty |= 8;
                    break;
                case PFM_MINUS_BUTTON:
                    setValue(current_state.active_param_id,
                        parameters[current_state.active_param_id] - encoderSensitivity);
                    // dirty |= 8;
                    break;
                case PFM_PLUS_BUTTON:
                    setValue(current_state.active_param_id,
                        parameters[current_state.active_param_id] + encoderSensitivity);
                    break;
                case PFM_PATCHES_BUTTON:
                    current_state.menu_key--;
                    if (current_state.menu_key >= 40)
                        current_state.menu_key = 39;
                    current_state.active_param_id = current_state.menu_key;
                    updateActiveParameter(current_state.menu_key,
                        parameters[current_state.menu_key]);
                    break;
                case PFM_SYSTEM_BUTTON:
                    current_state.menu_key++;
                    if (current_state.menu_key >= 40)
                        current_state.menu_key = 0;
                    current_state.active_param_id = current_state.menu_key;
                    updateActiveParameter(current_state.menu_key,
                        parameters[current_state.menu_key]);
                    break;
                default:
                    break;
                }
                break;
            case MENU_PATCHES:
                switch (button) {
                case PFM_MENU_BUTTON:
                    current_state.menu = MENU_MAIN;
                    // dirty |= 8;
                    break;
                case PFM_MINUS_BUTTON:
                case PFM_PATCHES_BUTTON: {
                    current_state.menu_key--;
                    uint32_t num_patches = registry.getNumberOfPatches();
                    if (current_state.menu_key >= num_patches)
                        current_state.menu_key = num_patches - 1;
                    // dirty |= 8;
                    break;
                }
                case PFM_RESOURCES_BUTTON:
                    program.loadProgram(current_state.menu_key);
                    program.resetProgram(false);
                    current_state.menu = MENU_MAIN;
                    // dirty |= 8;
                    break;
                case PFM_PLUS_BUTTON:
                case PFM_SYSTEM_BUTTON: {
                    current_state.menu_key++;
                    uint32_t num_patches = registry.getNumberOfPatches();
                    if (current_state.menu_key >= num_patches)
                        current_state.menu_key = 0;
                    // dirty |= 8;
                    break;
                }
                default:
                    break;
                }
                break;
            case MENU_RESOURCES:
                switch (button) {
                case PFM_MENU_BUTTON:
                    current_state.menu = MENU_MAIN;
                    // dirty |= 8;
                    break;
                case PFM_MINUS_BUTTON:
                case PFM_PATCHES_BUTTON: {
                    current_state.menu_key--;
                    uint32_t num_resources = registry.getNumberOfResources();
                    if (current_state.menu_key >= num_resources)
                        current_state.menu_key = num_resources;
                    // dirty |= 8;
                    break;
                }
                case PFM_RESOURCES_BUTTON:
                    storage.eraseResource(current_state.menu_key);
                    current_state.menu = MENU_MAIN;
                    // dirty |= 8;
                    break;
                case PFM_PLUS_BUTTON:
                case PFM_SYSTEM_BUTTON: {
                    current_state.menu_key++;
                    uint32_t num_resources = registry.getNumberOfResources();
                    if (current_state.menu_key >= num_resources)
                        current_state.menu_key = 0;
                    // dirty |= 8;
                    break;
                }
                default:
                    break;
                }
                break;
            case MENU_SYSTEM:
                switch (button) {
                case PFM_MENU_BUTTON:
                    current_state.menu = MENU_MAIN;
                    // dirty |= 8;
                    break;
                case PFM_MINUS_BUTTON:
                case PFM_PATCHES_BUTTON: {
                    current_state.menu_key--;
                    if (current_state.menu_key > SYS_LAST)
                        current_state.menu_key = SYS_LAST;
                    // dirty |= 8;
                    break;
                }
                case PFM_RESOURCES_BUTTON:
                    switch (current_state.menu_key) {
                        case SYS_BOOTLOADER:
                            jump_to_bootloader();
                            break;
                        case SYS_ERASE:
                            owl.setBackgroundTask(&erase_task);
                            break;
                        case SYS_REBOOT:
                            device_reset();
                            break;
                    }
                    // dirty |= 8;
                    break;
                case PFM_PLUS_BUTTON:
                case PFM_SYSTEM_BUTTON: {
                    current_state.menu_key++;
                    if (current_state.menu_key > SYS_LAST)
                        current_state.menu_key = 0;
                    // dirty |= 8;
                    break;
                }
                default:
                    break;
                }
                break;
            default:
                break;
            }
        }
    }

    void buttonLongPressed(int button) override {
        if (button < PFM_MENU_BUTTON) {
            current_state.active_buttons &= ~(1 << uint8_t(button));
        }
    };

    void twoButtonsPressed(int button1, int button2) override {
        if (button1 < PFM_MENU_BUTTON) {
            current_state.active_buttons |= 1 << uint8_t(button1);
        }
        if (button2 < PFM_MENU_BUTTON) {
            current_state.active_buttons |= 1 << uint8_t(button2);
        }
    };

    void buttonTick(int button) override {
        switch (current_state.menu) {
        case MENU_MAIN:
            switch (button) {
            case PFM_MINUS_BUTTON:
                setValue(current_state.active_param_id,
                    parameters[current_state.active_param_id] - encoderSensitivity);
                break;
            case PFM_PLUS_BUTTON:
                setValue(current_state.active_param_id,
                    parameters[current_state.active_param_id] + encoderSensitivity);
                break;
            default:
                break;
            }
            break;
        case MENU_PARAMS:
            switch (button) {
            case PFM_PLUS_BUTTON:
                setValue(current_state.active_param_id,
                    parameters[current_state.active_param_id] + encoderSensitivity);
                break;
            case PFM_MINUS_BUTTON:
                setValue(current_state.active_param_id,
                    parameters[current_state.active_param_id] - encoderSensitivity);
                break;
            default:
                break;
            }
            break;
        case MENU_PATCHES:
            break;
        case MENU_RESOURCES:
            break;
        case MENU_SYSTEM:
            break;
        }
    }

    void updateEncoders(int16_t* data, uint8_t size) override {}

private:
    void (*drawCallback)(uint8_t*, uint16_t, uint16_t);
};

#endif // __ParameterController_hpp__
