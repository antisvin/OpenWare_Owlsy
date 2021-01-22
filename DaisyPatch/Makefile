PROJECT = DaisyPatch
BUILDROOT = .
OPENWARE ?= $(BUILDROOT)/..

OPENOCD ?= openocd -f $(OPENWARE)/Hardware/openocd_h7.cfg

LDSCRIPT = $(OPENWARE)/Hardware/daisy.ld

include $(OPENWARE)/Hardware/daisy.mk

C_SRC = $(wildcard Src/*.c)
CPP_SRC = $(wildcard Src/*.cpp)
C_SRC += $(OPENWARE)/Source/usbd_audio.c
C_SRC += $(OPENWARE)/Source/sdram_daisy.c
C_SRC += $(OPENWARE)/Source/ssd1309.c
C_SRC += $(OPENWARE)/Source/ak4556.c
CPP_SRC += $(OPENWARE)/Source/FirmwareHeader.cpp
CPP_SRC += $(OPENWARE)/Source/uart.cpp
CPP_SRC += $(OPENWARE)/Source/MidiStreamReader.cpp
CPP_SRC += $(OPENWARE)/Source/ScreenBuffer.cpp
CPP_SRC += $(OPENWARE)/Source/ScreenBufferMono.cpp
CPP_SRC += $(OPENWARE)/Source/Graphics.cpp

ifndef PLATFORM
	PLATFORM = Daisy
endif

ifeq ($(PLATFORM),Daisy)
	CPPFLAGS += -DDAISY
#  vpath %.c usbd-hs
endif

include $(OPENWARE)/Hardware/sources.mk

C_SRC += $(C_SRC_DAC)
C_SRC += $(C_SRC_SDRAM)
C_SRC += $(C_SRC_SAI)
C_SRC += $(C_SRC_DSP)
C_SRC += $(C_SRC_OS)
C_SRC += $(C_SRC_USBD)
C_SRC += $(C_SRC_UART)
C_SRC += $(C_SRC_IWDG)
C_SRC += $(C_SRC_QSPI)

include $(OPENWARE)/Hardware/h7.mk

.PHONY: upload dfu

upload:
	$(OPENOCD) -c "program Build/DaisyPatch.elf verify reset exit"

dfu:
	sudo dfu-util -a 0 -s 0x08000000:leave -D Build/DaisyPatch.bin -d 0483:df11
