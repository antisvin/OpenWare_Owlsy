#ifndef __ParameterController_hpp__
#define __ParameterController_hpp__

#include "Owl.h"
#include "ProgramVector.h"
#include "ScreenBuffer.h"
#include "VersionToken.h"
#include "device.h"
#include "errorhandlers.h"
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

extern VersionToken *bootloader_token;

extern uint32_t tftDirtyBits;

extern void pfmDefaultDrawCallback(uint8_t *pixels, uint16_t width,
                                   uint16_t height);

enum Menu : uint8_t {
  MENU_MAIN,
  MENU_PARAMS,
  MENU_SENSITIVITY,
  MENU_PATCHES,
  MENU_DATA,
};

template <uint8_t SIZE> class ControllerState {
public:
  char patch_name[12] = "";
  uint8_t active_param_id;
  uint8_t active_param_value;
  uint8_t params[SIZE];
  uint16_t cpu_load;
  uint16_t mem_load;
  bool is_drawn;
};

/* shows a single parameter selected and controlled with a single encoder
 */
template <uint8_t SIZE> class ParameterController {
public:
  char title[11] = "PreenFM3";
  int16_t parameters[SIZE];
  char names[SIZE][12];
  int8_t selected = 0;
  ControllerState<SIZE> states[2];
  ControllerState<SIZE> *current_state;
  ControllerState<SIZE> *prev_state;
  ParameterController() { reset(); }
  void setTitle(const char *str) { strncpy(title, str, 10); }
  void reset() {
    drawCallback = pfmDefaultDrawCallback;
    for (int i = 0; i < SIZE; ++i) {
      strcpy(names[i], "Parameter  ");
      names[i][10] = 'A' + i;
      parameters[i] = 0;
    }
  }
  void draw(uint8_t *pixels, uint16_t width, uint16_t height) {
    ScreenBuffer screen(width, height);
    screen.setBuffer(pixels);
    draw(screen);
  }

  void draw(ScreenBuffer &screen) {
    tftDirtyBits |= 0b1111;
    screen.clear();
    screen.setTextColour(GREEN);
    if (sw1()) {
      drawStats(screen);
      // todo: show patch name and next/previous patch
    } else if (sw2()) {
      screen.fill(BLACK);
      drawGlobalParameterNames(42, screen);
    } else if (getErrorStatus() != NO_ERROR && getErrorMessage() != NULL) {
      screen.setTextSize(1);
      screen.print(2, 20, getErrorMessage());
    } else {
      drawCallback(screen.getBuffer() + SCREEN_BUFFER_OFFSET, screen.getWidth(),
                   screen.getHeight() - SCREEN_HEIGHT_OFFSET);

      drawParameter(screen);
      drawParameterValues(screen);
      drawButtonValues(screen);
    }
  }

  void drawParameter(ScreenBuffer &screen) {
    // Active parameter
    screen.setTextSize(1);
    screen.setCursor(1, 260);
    screen.print(names[selected]);
    screen.print(": ");
    screen.print((int)parameters[selected] / 41);
    screen.drawRectangle(1, 262, 118, 16, WHITE);
    screen.fillRectangle(2, 263, 100, 14, BLUE);
    // screen.drawRectangle(0, 280, 120 * parameters[selected] / 4095, 20,
    // BLUE);
  }

  void drawParameterValues(ScreenBuffer &screen) {
    // Draw 6x4 levels on bottom 40px row
    //    int y = 63-7;
    int block = 2;
    int x = 0;
    for (int i = 0; i < 6; i++) {
      int y = 320 - 40;
      for (int j = 0; j < 4; j++) {
        screen.drawRectangle(x + 2, y + 2, 36, 6, WHITE);
        if ((i & 0x3) == j)
          screen.fillRectangle(x + 3, y + 3, j * 6 + i, 4, BLUE);
        else
          screen.fillRectangle(x + 3, y + 3, j * 6 + i, 4, WHITE);
        y += 10;
        // if(i == selectedPid[block])
        // screen.fillRectangle(x + 2, y , max(1, min(16, parameters[i]/255)),
        // 4, WHITE); else screen.drawRectangle(x, y, max(1, min(16,
        // parameters[i]/255)), 4, WHITE);
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

  void drawButtonValues(ScreenBuffer& screen) {
    int x = 120;
    screen.setCursor(120 + 20, 260);
    screen.print("Buttons:");
    for (int i = 0; i < 3; i++) {
      int y = 320 - 60;
      screen.setCursor(x + 3, y + 9);
      screen.print((int)i * 2 + 1);
      screen.drawRectangle(x + 11, y + 2, 28, 7, WHITE);
      y += 11;
      screen.setCursor(x + 3, y + 8);
      screen.print((int)i * 2 + 2);
      screen.drawRectangle(x + 11, y, 28, 7, WHITE);
      x += 40;
    }
  }

  void drawParameterNames(int y, int pid, const char names[][12], int size,
                          ScreenBuffer &screen) {
    screen.setTextSize(1);
    // int selected = selectedPid[pid];
    if (selected > 2)
      screen.print(1, y - 30, names[selected - 3]);
    if (selected > 1)
      screen.print(1, y - 20, names[selected - 2]);
    if (selected > 0)
      screen.print(1, y - 10, names[selected - 1]);
    screen.print(1, y, names[selected]);
    if (selected < size - 1)
      screen.print(1, y + 10, names[selected + 1]);
    if (selected < size - 2)
      screen.print(1, y + 20, names[selected + 2]);
    if (selected < size - 3)
      screen.print(1, y + 30, names[selected + 3]);
    screen.invert(0, y - 9, screen.getWidth(), 10);
  }

  void drawGlobalParameterNames(int y, ScreenBuffer &screen) {
    drawParameterNames(y, 0, names, SIZE, screen);
  }

  void drawStats(ScreenBuffer &screen) {
    screen.setTextSize(1);
    ProgramVector *pv = getProgramVector();
    if (pv->message != NULL)
      screen.print(2, 16, pv->message);
    screen.print(2, 26, "cpu/mem: ");
    float percent = (pv->cycles_per_block / pv->audio_blocksize) /
                    (float)ARM_CYCLES_PER_SAMPLE;
    screen.print((int)(percent * 100));
    screen.print("% ");
    screen.print((int)(pv->heap_bytes_used) / 1024);
    screen.print("kB");

    // draw firmware version
    screen.print(1, 36, getFirmwareVersion());
    if (bootloader_token->magic == BOOTLOADER_MAGIC) {
      screen.print(" bt.");
      screen.print(getBootloaderVersion());
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

  void encoderChanged(uint8_t encoder, int32_t delta) {
    if (encoder == 0) {
      if (sw2()) {
        if (delta > 1)
          selected = min(SIZE - 1, selected + 1);
        else if (delta < 1)
          selected = max(0, selected - 1);
      } else {
        parameters[selected] += delta * 10;
        parameters[selected] = min(4095, max(0, parameters[selected]));
      }
    } // todo: change patch with enc1/sw1
  }
  void setName(uint8_t pid, const char *name) {
    if (pid < SIZE)
      strncpy(names[pid], name, 11);
  }
  uint8_t getSize() { return SIZE; }
  void setValue(uint8_t ch, int16_t value) { parameters[ch] = value; }

  void drawMessage(int16_t y, ScreenBuffer &screen) {
    ProgramVector *pv = getProgramVector();
    if (pv->message != NULL) {
      screen.setTextSize(1);
      screen.setTextWrap(true);
      screen.print(0, y, pv->message);
      screen.setTextWrap(false);
    }
  }

  void drawTitle(ScreenBuffer &screen) { drawTitle(title, screen); }

  void drawTitle(const char *title, ScreenBuffer &screen) {
    // draw title
    screen.setTextSize(2);
    screen.print(1, 17, title);
  }

  void setCallback(void *callback) {
    if (callback == NULL)
      drawCallback = pfmDefaultDrawCallback;
    else
      drawCallback = (void (*)(uint8_t *, uint16_t, uint16_t))callback;
  }

private:
  void (*drawCallback)(uint8_t *, uint16_t, uint16_t);
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
