#ifndef __ParameterController_hpp__
#define __ParameterController_hpp__

#include "EncodersListener.h"
#include "FlashStorage.h"
#include "Owl.h"
#include "ProgramVector.h"
#include "ScreenBuffer.h"
#include "VersionToken.h"
#include "device.h"
#include "errorhandlers.h"
#include "message.h"
#include <stdint.h>
#include <string.h>
#ifdef USE_DIGITALBUS
#include "DigitalBusReader.h"
extern DigitalBusReader bus;
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
#ifndef clamp
#define clamp(x, a, b) min(b, max(a, x))
#endif
#define ENC_MULTIPLIER 6 // shift left by this many steps

extern VersionToken* bootloader_token;

extern uint32_t tftDirtyBits;

extern void pfmDefaultDrawCallback(uint8_t* pixels, uint16_t width, uint16_t height);

enum Menu : uint8_t {
    MENU_MAIN,
    MENU_PATCHES,
    MENU_DATA,
    MENU_SETTINGS,
};

// SIZE = 24
// BLOCKS ==
template <uint8_t SIZE>
class ControllerState {
public:
    // Section 1
    char title[13] = "PreenFM3";
    // Section 2
    uint8_t cpu_load = 0;
    uint8_t mem_kbytes_used = 0;
    uint16_t flash_used = 0;
    // Section 3
    const char* message = 0;
    // Section 4 + 5
    Menu menu = MENU_MAIN;
    uint8_t menu_dirty;
    uint8_t custom_callback = 0;
    // Section 6
    uint8_t active_buttons = 0;
    uint8_t active_param_id = 0;
    char active_param_name[12];
    uint8_t active_param_value = 0;
    uint16_t active_param_value_pct = 0;
    // Section 7
    uint8_t params[SIZE] = {};
    uint8_t blocks[NOF_ENCODERS] = {};

    void update() {
        ProgramVector* pv = getProgramVector();
        float cpu_load_percent = (pv->cycles_per_block / pv->audio_blocksize) /
            (float)ARM_CYCLES_PER_SAMPLE * 100.f;
        cpu_load = cpu_load_percent;
        mem_kbytes_used = (int)(pv->heap_bytes_used) / 1024;
        flash_used = storage.getWrittenSize() / 1024;
        message = pv->message;
    }
};

/* shows a single parameter selected and controlled with a single encoder
 */
template <uint8_t SIZE>
class ParameterController : public EncodersListener {
public:
    int16_t parameters[SIZE];
    char names[SIZE][12];
    int8_t selected = 0;
    ControllerState<NOF_VISIBLE_PARAMETERS> current_state;
    ControllerState<NOF_VISIBLE_PARAMETERS> prev_state;
    uint32_t dirty;

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

    ParameterController() {
        nextListener = 0;
        dirty = 0;
        reset();
        debugMessage(
            "To debug or not to debug? That's not\neven the question.");
    }
    void setTitle(const char* str) {
        strncpy(current_state.title, str, 12);
    }
    void reset() {
        dirty = 0b1111111;
        drawCallback = pfmDefaultDrawCallback;
        for (int i = 0; i < SIZE; ++i) {
            strncpy(names[i], "Parameter  ", 12);
            names[i][10] = 'A' + i;
            parameters[i] = 0;
        }
    }

    void findChanges() {
        if (strncmp(current_state.title, prev_state.title, 12) != 0)
            dirty |= 1;
        if ( // Compare 4 bytes at once
            (uint32_t*)(&current_state.cpu_load) != (uint32_t*)(&prev_state.cpu_load))
            dirty |= 2;
        if (strncmp(current_state.message, prev_state.message, 100) != 0)
        //if (message != other.message)
            dirty |= 4;
        if (current_state.custom_callback || current_state.menu_dirty || current_state.menu != prev_state.menu)
            dirty |= 24;
        if (memcmp((void*)&current_state.active_buttons, (void*)&prev_state.active_buttons, 17) != 0)
            dirty |= 32;
        if (memcmp((void*)current_state.params, (void*)prev_state.params, sizeof(current_state.params) + sizeof(current_state.blocks)) != 0)
            dirty |= 64;
    }

    void updateState() {
        findChanges();
        prev_state = current_state;
        //memcpy((void*)&prev_state, (void*)&current_state,
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
        else if (dirty & 24) { // 8 or 16
            switch (current_state.menu) {
            case MENU_MAIN:
                screen.fillRectangle(0, 0, 240, 210, BLACK);
                screen.setTextSize(2);
                screen.setCursor(0, 20);
                screen.setTextColour(RED);
                screen.print("RED");
                screen.setCursor(0, 50);
                screen.setTextColour(GREEN);
                screen.print("GREEN");
                screen.setCursor(0, 80);
                screen.setTextColour(BLUE);
                screen.print("BLUE");
                screen.setCursor(0, 110);
                screen.setTextColour(CYAN);
                screen.print("CYAN");
                screen.setCursor(0, 140);
                screen.setTextColour(MAGENTA);
                screen.print("MAGENTA");
                screen.setCursor(0, 170);
                screen.setTextColour(YELLOW);
                screen.print("YELLOW");
                screen.setCursor(0, 200);
                screen.setTextColour(WHITE);
                screen.print("WHITE");
                screen.setTextSize(1);
                break;
            case MENU_PATCHES:
                break;
            case MENU_DATA:
                break;
            case MENU_SETTINGS:
                break;
            }
            tftDirtyBits |= 24;
            dirty &= ~24;
        }
        else if (dirty & 32) {
            drawParameter(screen);
            tftDirtyBits |= 32;
            dirty &= ~32;
        }
        else if (dirty & 64) {
            drawParameterValues(screen);
            tftDirtyBits |= 64;
            dirty &= ~64;
        }
    }

    void drawTitle(ScreenBuffer& screen) {
        // clear
        screen.fillRectangle(0, 0, 240, 16, BLACK);

        // draw title
        screen.setTextColour(CYAN);
        screen.setTextSize(2);
        screen.print(1, 17, current_state.title);

        // draw firmware version
        //screen.setTextColour(CYAN);
        screen.setTextSize(1);
        screen.setCursor(160, 10);
        screen.print(FIRMWARE_VERSION);

        // bootloader string
        if (bootloader_token->magic == BOOTLOADER_MAGIC) {
            screen.print(" bt.");
            screen.print(getBootloaderVersion());
        }
    }

    void drawStats(ScreenBuffer& screen) {
        // clear
        screen.fillRectangle(0, 0, 240, 8, BLACK);

        // cpu / mem stats
        screen.setTextSize(1);
        screen.setTextColour(GREEN);
        screen.print(2, 8, "cpu: ");
        screen.print((int)current_state.cpu_load);
        screen.print("%");
        screen.print(60, 8, "mem: ");
        screen.print((int)current_state.mem_kbytes_used);
        screen.print("kB");

        // draw flash usage
        int flash_used = current_state.flash_used;
        int flash_total = storage.getTotalAllocatedSize() / 1024;
        screen.print(120, 8, "flash: ");
        screen.print(flash_used * 100 / flash_total);
        screen.print("% ");
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
        screen.fillRectangle(0, 0, 240, 16, BLACK);

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
            screen.setTextSize(1);
            screen.setTextWrap(true);

            screen.print(2, 8, pv->message);
            screen.setTextWrap(false);
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
        screen.drawRectangle(1, 12, 116, 16, WHITE);
        screen.fillRectangle(2, 13, current_state.active_param_value, 14, BLUE);

        // buttons
        int x = 120;
        screen.setCursor(120 + 11, 10);
        screen.print("Buttons:");
        for (uint8_t i = 0; i < 3; i++) {
            int y = 10;
            screen.setCursor(x + 3, y + 9);
            screen.print((int)i * 2 + 1);
            if (current_state.active_buttons & (1 << i)) {
                screen.fillRectangle(x + 11, y + 2, 28, 7, YELLOW);
            }
            else {
                screen.drawRectangle(x + 11, y + 2, 28, 7, YELLOW);
            }
            y += 11;
            screen.setCursor(x + 3, y + 8);
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

        //screen.setTextSize(1);
        //screen.setTextColour(MAGENTA);

        // Draw 6x4 levels on bottom 40px row
        //    int y = 63-7;
        int block = 2;
        int x = 0;
        for (int i = 0; i < 6; i++) {
            int y = 0;
            uint8_t active = current_state.blocks[i];
            for (int j = 0; j < 4; j++) {
                screen.drawRectangle(x + 2, y + 2, 36, 6, WHITE);
                screen.fillRectangle(x + 3, y + 3, current_state.params[j * 6 + i],
                    4, (active == j) ? BLUE : WHITE);
                y += 10;
                // if(i == selectedPid[block])
                // screen.fillRectangle(x + 2, y , max(1, min(16,
                // parameters[i]/255)), 4, WHITE); else screen.drawRectangle(x,
                // y, max(1, min(16, parameters[i]/255)), 4, WHITE);
            }
            x += 40;
            /*
            if(i & 0x01)
              block++;
            if(i == 7){
              x = 0;
              y += 3;
              block = 0;
            }
            */
        }
    }

    void encoderChanged(uint8_t encoder, int32_t delta) {
        if (encoder == 0) {
            if (sw2()) {
                if (delta > 1)
                    selected = min(SIZE - 1, selected + 1);
                else if (delta < 1)
                    selected = max(0, selected - 1);
            }
            else {
                parameters[selected] += delta * 10;
                parameters[selected] = min(4095, max(0, parameters[selected]));
            }
        } // todo: change patch with enc1/sw1
    }
    void setName(uint8_t pid, const char* name) {
        if (pid < SIZE)
            strncpy(names[pid], name, 11);
    }
    uint8_t getSize() {
        return SIZE;
    }

    void drawMessage(int16_t y, ScreenBuffer& screen) {
      drawMessage(screen);
    }

    void setValue(uint8_t ch, int16_t value) {
        parameters[ch] = value;
        current_state.params[ch] = clamp(value / 120, 0, 33);
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
    }

    void setCallback(void* callback) {
        if (callback == NULL)
            drawCallback = pfmDefaultDrawCallback;
        else
            drawCallback = (void (*)(uint8_t*, uint16_t, uint16_t))callback;
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

    void buttonPressed(int button) override {
        current_state.active_buttons |= 1 << uint8_t(button);
    };

    void buttonLongPressed(int button) override {};

    void twoButtonsPressed(int button1, int button2) override {};

private:
    void (*drawCallback)(uint8_t*, uint16_t, uint16_t);
    bool sw1() {
        return false;
        //    return HAL_GPIO_ReadPin(ENC1_SW_GPIO_Port, ENC1_SW_Pin) !=
        //    GPIO_PIN_SET;
    }
    bool sw2() {
        return false;
        //    return HAL_GPIO_ReadPin(ENC2_SW_GPIO_Port, ENC2_SW_Pin) !=
        //    GPIO_PIN_SET;
    }
};

#endif // __ParameterController_hpp__
