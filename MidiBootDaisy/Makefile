PROJECT = MidiBootDaisy
BUILDROOT = .
OPENWARE ?= $(BUILDROOT)/..

OPENOCD ?= openocd -f $(OPENWARE)/Hardware/openocd_h7.cfg

LDSCRIPT = STM32H750IBKx-boot.ld
include $(OPENWARE)/Hardware/daisy.mk

C_SRC = $(notdir $(wildcard Src/*.c))
CPP_SRC = $(notdir $(wildcard Src/*.cpp))

C_SRC += usbd_midi.c
C_SRC += usbd_midi_if.c
C_SRC += qspicontrol.c
C_SRC += sdram_daisy.c
C_SRC += sysex.c
C_SRC += crc32.c
#C_SRC += eepromcontrol.c
C_SRC += message.c
CPP_SRC += MidiWriter.cpp
# S_SRC = startup_openboot.s

ifndef PLATFORM
PLATFORM = Daisy
endif

#C_SRC += usbd_desc.c usbd_conf.c

ifeq ($(PLATFORM),Daisy)
  CPPFLAGS += -Iusbd-hs -DDAISY -DUSE_USBD_FS
  vpath %.c usbd-hs
endif

# CPP_SRC += MidiReader.cpp
CPP_SRC += MidiController.cpp

OBJS = $(C_SRC:%.c=Build/%.o)
OBJS += $(CPP_SRC:%.cpp=Build/%.o)
OBJS += $(S_SRC:%.s=Build/%.o)

OBJS += $(OBJS_SDRAM)
OBJS += $(OBJS_USBD)
OBJS += $(OBJS_UART)
OBJS += $(OBJS_IWDG)

include $(OPENWARE)/Hardware/h7.mk

.PHONY: clean deploy unlock erase upload lock

info:
	$(OPENOCD) -c "init" -c "flash info 0" -c "exit"

unlock:
	$(OPENOCD) -c "init" -c "reset halt" -c "stm32h7x unlock 0" -c "exit"
	$(OPENOCD) -c "init" -c "reset halt" -c "flash protect 0 0 0 off" -c "flash info 0" -c "exit"

erase:
	$(OPENOCD) -c "init" -c "halt" -c "stm32h7x mass_erase 0" -c "flash info 0" -c "exit"

#erase-boot: # flash erase_sector num first last
#	$(OPENOCD) -c "init" -c "halt" -c "flash erase_sector 0 0 0" -c "exit"

#erase-storage: # flash erase_sector num first last
#	$(OPENOCD) -c "init" -c "halt" -c "flash erase_sector 0 7 11" -c "exit"

dump: # flash read_bank num filename [offset [length]]
	$(OPENOCD) -c "init" -c "halt" -c "flash read_bank 0 dump.bin" -c "exit"

upload:
	$(OPENOCD) -c "program Build/MidiBootDaisy.elf verify reset exit"

#lock:
#	$(OPENOCD) -c "init" -c "halt" -c "flash protect 0 0 1 on" -c "flash info 0" -c "exit"

deploy: upload lock
