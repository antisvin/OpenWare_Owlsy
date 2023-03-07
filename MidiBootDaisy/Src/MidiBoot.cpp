#include "device.h"
#include "FirmwareHeader.h"
#include "FirmwareLoader.hpp"
#include "MidiController.h"
#include "MidiReader.h"
#include "MidiStatus.h"
#include "OpenWareMidiControl.h"
#include "errorhandlers.h"
#include "midi.h"
#include "qspicontrol.h"

extern char _FIRMWARE_STORAGE_BEGIN, _FIRMWARE_STORAGE_END, _FIRMWARE_STORAGE_SIZE;
extern char _PATCH_STORAGE_BEGIN, _PATCH_STORAGE_END;

static SystemMidiReader midi_rx;
MidiController midi_tx;
FirmwareLoader loader;
ProgramManager program;


MidiHandler::MidiHandler() {}
ProgramManager::ProgramManager() {}
void ProgramManager::exitProgram(bool isr) {}
void setParameterValue(uint8_t ch, int16_t value) {}
void SystemMidiReader::reset() {}

void led_off() {
#ifdef USER_LED_Pin
  HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, USER_LED_INVERTED ? GPIO_PIN_SET : GPIO_PIN_RESET);
#endif
}

void led_on() {
#ifdef USER_LED_Pin
  HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, USER_LED_INVERTED ? GPIO_PIN_RESET : GPIO_PIN_SET);
#endif
}

void led_toggle() {
#ifdef USER_LED_Pin
  HAL_GPIO_TogglePin(USER_LED_GPIO_Port, USER_LED_Pin);
#endif
}

const char *getFirmwareVersion() {
  return (const char *)(HARDWARE_VERSION " " FIRMWARE_VERSION);
}

#define FIRMWARE_SECTOR 0xff

const char *message = NULL;
extern "C" void setMessage(const char *msg) { message = msg; }

void sendMessage() {
  if (getErrorStatus() != NO_ERROR) {
    message = getErrorMessage() == NULL ? "Error" : getErrorMessage();
  }
  if (message != NULL) {
    char buffer[64];
    buffer[0] = SYSEX_PROGRAM_MESSAGE;
    char *p = &buffer[1];
    p = stpncpy(p, message, 62);
    midi_tx.sendSysEx((uint8_t *)buffer, p - buffer);
    message = NULL;
  }
}

void eraseFromFlash(uint32_t sector) {
  if (qspi_init(QSPI_MODE_INDIRECT_POLLING) != MEMORY_OK) {
    error(RUNTIME_ERROR, "Flash write mode error");
  }

  led_on();
  if (sector == 0xff) {
    if (qspi_erase((uint32_t)&_PATCH_STORAGE_BEGIN,
                   (uint32_t)&_PATCH_STORAGE_END) != MEMORY_OK) {
      error(RUNTIME_ERROR, "Storage erase error");
    } else {
      setMessage("Erased patch storage");
      led_off();
    }
  } else {
    if (qspi_erase(
        (uint32_t)&_FIRMWARE_STORAGE_BEGIN,
        (uint32_t)&_FIRMWARE_STORAGE_END) != MEMORY_OK) {
      error(RUNTIME_ERROR, "Flash erase error");
    } else {
      setMessage("Erased flash sector");
      led_off();
    }
  }

#ifdef USE_IWDG
      IWDG1->KR = 0xaaaa; // reset the watchdog timer
#endif  
  if (qspi_init(QSPI_MODE_MEMORY_MAPPED) != MEMORY_OK) {
    error(RUNTIME_ERROR, "Flash read mode error");
  }
}

void saveToFlash(uint32_t address, void *data, uint32_t length) {
  if (length > (uint32_t)&_FIRMWARE_STORAGE_SIZE) {
    error(RUNTIME_ERROR, "Firmware too big");
  }
  else if (qspi_init(QSPI_MODE_INDIRECT_POLLING) != MEMORY_OK) {
    error(RUNTIME_ERROR, "Flash write mode error");
  }
  else if (qspi_erase(address, address + length) != MEMORY_OK) {
    error(RUNTIME_ERROR, "Flash erase error");
  }
  else if (qspi_write_block(address, (uint8_t *)data, length) != MEMORY_OK) {
    error(RUNTIME_ERROR, "Flash firmware write error");
  }
  else if (qspi_init(QSPI_MODE_MEMORY_MAPPED) != MEMORY_OK) {
    error(RUNTIME_ERROR, "Flash read mode error");
  }
  /* TODO: can we have a checksum calculation here? */
}

extern "C" {

volatile int8_t errorcode = NO_ERROR;
static char *errormessage = NULL;
int8_t getErrorStatus() { return errorcode; }

void setErrorStatus(int8_t err) {
  errorcode = err;
  if (err == NO_ERROR) {
    errormessage = NULL;
  //    else
  //      led_red();
#ifdef ERROR_LED_Pin
    HAL_GPIO_WritePin(ERROR_LED_GPIO_Port, ERROR_LED_Pin, ERROR_LED_INVERTED ? GPIO_PIN_SET : GPIO_PIN_RESET);
  }
  else {
    HAL_GPIO_WritePin(ERROR_LED_GPIO_Port, ERROR_LED_Pin, ERROR_LED_INVERTED ? GPIO_PIN_RESET : GPIO_PIN_SET);
#endif
  }
}


FirmwareHeader* getFirmwareHeader(void) {
  return (FirmwareHeader*)&_FIRMWARE_STORAGE_BEGIN;
}

int loadFirmware(void) {
  FirmwareHeader* header = getFirmwareHeader();
  if (header->magic == FIRMWARE_HEADER && header->hardware_id == HARDWARE_ID) { // TODO: possibly check checksum here
    uint32_t* start = &header->section_0_start;
    for (uint32_t i = 0; i < ((header->options >> OPT_NUM_RELOCATIONS_OFFSET) & OPT_NUM_RELOCATIONS_MASK); i++) {
      memcpy((void*)(*start), (void*)(*(start + 2)), *(start + 1) - *start);
      start += 3;
    }
    return 1;
  }
  else {
    return 0;
  }
}

void error(int8_t err, const char *msg) {
  setErrorStatus(err);
  errormessage = (char *)msg;
}

const char *getErrorMessage() { return errormessage; }

bool midi_error(const char *str) {
  error(PROGRAM_ERROR, str);
  return false;
}

bool updateChecksum(uint32_t address, uint32_t checksum){
  bool success = false;
  if (qspi_init(QSPI_MODE_INDIRECT_POLLING) != MEMORY_OK)
    return false;
  else if (qspi_write_block(address, (uint8_t*)&checksum, 4) == MEMORY_OK) {
    success = true;
  }
  if (qspi_init(QSPI_MODE_MEMORY_MAPPED) != MEMORY_OK)
    success = false;
  return success;
}

void setup() {
  // led_on();
  midi_tx.setOutputChannel(MIDI_OUTPUT_CHANNEL);
  midi_rx.setInputChannel(MIDI_INPUT_CHANNEL);
  setMessage("OWL Bootloader Ready");    
}

void loop(void) {
  static int counter = 3 * 1200;
  if (counter) {
    switch (counter-- % 1200) {
    case 600:
      led_off();
      break;
    case 0:
      led_on();
      break;
    default:
      HAL_Delay(1);
      break;
    }
  }
  midi_tx.transmit();
}
}

void MidiHandler::handleFirmwareUploadCommand(uint8_t *data, uint16_t size) {
  led_on();
  int32_t ret = loader.handleFirmwareUpload(data, size);
  if (ret > 0) {
    setMessage("Firmware upload complete");
    // firmware upload complete: wait for run or store
    // setLed(NONE); todo!
    led_off();
  }
  else if (ret == 0) {
    setMessage("Firmware upload in progress");
    led_off();
    // toggleLed(); todo!
  }
  else {
    error(RUNTIME_ERROR, "Firmware upload error");
  }
  // setParameterValue(PARAMETER_A, loader.index*4095/loader.size);
}

void MidiHandler::handleFlashEraseCommand(uint8_t *data, uint16_t size) {
  led_on();
  if (size == 5) {
    uint32_t sector = loader.decodeInt(data);
    eraseFromFlash(sector);
    led_off();
  } else if (size == 0) {
    eraseFromFlash(FIRMWARE_SECTOR);
    led_off();
  } else {
    error(PROGRAM_ERROR, "Invalid FLASH ERASE command");
  }
}

void MidiHandler::handleFirmwareFlashCommand(uint8_t *data, uint16_t size) {
  if (loader.isReady() && size == 5) {
    uint32_t checksum = loader.decodeInt(data);
    if (checksum == loader.getChecksum()) {
      led_on();
      FirmwareHeader* header = (FirmwareHeader*)loader.getData();
      if (header->magic == HEADER_MAGIC && header->checksum == HEADER_CHECKSUM_PLACEHOLDER){
        if (header->hardware_id != HARDWARE_ID) {
          error(PROGRAM_ERROR, "Invalid hardware ID");
        }
        else {
          header->checksum = 0xFFFFFFFF; // Not an actual checksum yet
          checksum = crc32((void*)header, header->header_size - 4, 0);
          saveToFlash((uint32_t)&_FIRMWARE_STORAGE_BEGIN, loader.getData(), loader.getSize());
          loader.clear();

          // Now that data is stored, we can update the checksum field to confirm it's successful
          uint32_t checksum_address = (uint32_t)&(getFirmwareHeader()->checksum);
          if (updateChecksum(checksum_address, checksum)){
            led_off();
          }
          else {
            error(PROGRAM_ERROR, "Error storing checksum");
          }
        }
      }
      else {
        error(PROGRAM_ERROR, "Invalid firmware header");
      }
    } else {
      error(PROGRAM_ERROR, "Invalid FLASH checksum");
    }
  } else {
    error(PROGRAM_ERROR, "Invalid FLASH command");
  }
}

void MidiHandler::handleFirmwareStoreCommand(uint8_t *data, uint16_t size) {
  error(RUNTIME_ERROR, "Invalid STORE command");
}

void device_reset() {
  *OWLBOOT_MAGIC_ADDRESS = 0;
  NVIC_SystemReset();
}

void MidiHandler::handleSysEx(uint8_t *data, uint16_t size) {
  if (size < 5 || data[1] != MIDI_SYSEX_MANUFACTURER)
    return;
  if (data[2] != MIDI_SYSEX_OMNI_DEVICE &&
      data[2] != (MIDI_SYSEX_OWL_DEVICE | channel))
    // not for us
    return;
  switch (data[3]) {
  // case SYSEX_CONFIGURATION_COMMAND:
  //   handleConfigurationCommand(data+4, size-5);
  //   break;
  case SYSEX_DEVICE_RESET_COMMAND:
    device_reset();
    break;
  case SYSEX_BOOTLOADER_COMMAND:
    error(RUNTIME_ERROR, "Bootloader OK");
    break;
  case SYSEX_FIRMWARE_UPLOAD:
    handleFirmwareUploadCommand(data + 1, size - 2);
    break;
  // case SYSEX_FIRMWARE_RUN:
  //   handleFirmwareRunCommand(data+4, size-5);
  //   break;
  case SYSEX_FIRMWARE_STORE:
    handleFirmwareStoreCommand(data + 4, size - 5);
    break;
  case SYSEX_FIRMWARE_FLASH:
    handleFirmwareFlashCommand(data + 4, size - 5);
    break;
  case SYSEX_FLASH_ERASE:
    handleFlashEraseCommand(data + 4, size - 5);
    break;
  default:
    error(PROGRAM_ERROR, "Invalid SysEx Message");
    break;
  }
}

bool SystemMidiReader::readMidiFrame(uint8_t *frame) {
  switch (frame[0] & 0x0f) { // accept any cable number /  port
  case USB_COMMAND_SINGLE_BYTE:
    // Single Byte: in some special cases, an application may prefer not to use
    // parsed MIDI events. Using CIN=0xF, a MIDI data stream may be transferred
    // by placing each individual byte in one 32 Bit USB-MIDI Event Packet. This
    // way, any MIDI data may be transferred without being parsed.
    if (frame[1] == 0xF7 && pos > 2) {
      // suddenly found the end of our sysex as a Single Byte Unparsed
      buffer[pos++] = frame[1];
      handleSysEx(buffer, pos);
      pos = 0;
    } else if (frame[1] & 0x80) {
      // handleSystemRealTime(frame[1]);
    } else if (pos > 2) {
      // we are probably in the middle of a sysex
      buffer[pos++] = frame[1];
    } else {
      return false;
    }
    break;
  case USB_COMMAND_SYSEX_EOX1:
    if (pos < 3 || buffer[0] != SYSEX || frame[1] != SYSEX_EOX) {
      return midi_error("Invalid SysEx");
    } else if (pos >= size) {
      return midi_error("SysEx buffer overflow");
    } else {
      buffer[pos++] = frame[1];
      handleSysEx(buffer, pos);
    }
    pos = 0;
    break;
  case USB_COMMAND_SYSEX_EOX2:
    if (pos < 3 || buffer[0] != SYSEX || frame[2] != SYSEX_EOX) {
      return midi_error("Invalid SysEx");
    } else if (pos + 2 > size) {
      return midi_error("SysEx buffer overflow");
    } else {
      buffer[pos++] = frame[1];
      buffer[pos++] = frame[2];
      handleSysEx(buffer, pos);
    }
    pos = 0;
    break;
  case USB_COMMAND_SYSEX_EOX3:
    if (pos < 3 || buffer[0] != SYSEX || frame[3] != SYSEX_EOX) {
      return midi_error("Invalid SysEx");
    } else if (pos + 3 > size) {
      return midi_error("SysEx buffer overflow");
    } else {
      buffer[pos++] = frame[1];
      buffer[pos++] = frame[2];
      buffer[pos++] = frame[3];
      handleSysEx(buffer, pos);
    }
    pos = 0;
    break;
  case USB_COMMAND_SYSEX:
    if (pos + 3 > size) {
      return midi_error("SysEx buffer overflow");
    } else {
      buffer[pos++] = frame[1];
      buffer[pos++] = frame[2];
      buffer[pos++] = frame[3];
    }
    break;
  case USB_COMMAND_CONTROL_CHANGE:
    if ((frame[1] & 0xf0) != CONTROL_CHANGE)
      return false;
    handleControlChange(frame[1], frame[2], frame[3]);
    break;
  }
  return true;
}

void MidiHandler::handleControlChange(uint8_t status, uint8_t cc,
                                      uint8_t value) {
  // if(channel != MIDI_OMNI_CHANNEL && channel != getChannel(status))
  //   return;
  switch (cc) {
  case REQUEST_SETTINGS:
    switch (value) {
    case 0:
      sendMessage();
    case SYSEX_FIRMWARE_VERSION:
      midi_tx.sendFirmwareVersion();
      break;
    case SYSEX_DEVICE_ID:
      midi_tx.sendDeviceId();
      break;
    case SYSEX_PROGRAM_MESSAGE:
      sendMessage();
      break;
    }
    break;
  }
}

void usbd_midi_rx(uint8_t *buffer, uint32_t length) {
  for (uint16_t i = 0; i < length; i += 4) {
    if (!midi_rx.readMidiFrame(buffer + i))
      midi_rx.reset();
  }
}
