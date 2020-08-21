PROJECT = DaisyPatch
BUILDROOT = .
OPENWARE ?= $(BUILDROOT)/..

OPENOCD ?= openocd -f $(OPENWARE)/Hardware/openocd_h7.cfg

C_SRC = $(notdir $(wildcard Src/*.c))
CPP_SRC = $(notdir $(wildcard Src/*.cpp))

LDSCRIPT = $(OPENWARE)/Hardware/daisy_noflash.ld

include $(OPENWARE)/Hardware/daisy.mk

C_SRC += usbd_audio.c
# C_SRC += usbd_midi.c
# C_SRC += usbd_midi_if.c
C_SRC += sdram_daisy.c
C_SRC += ssd1309.c
C_SRC += ak4556.c
CPP_SRC += uart.cpp MidiStreamReader.cpp
CPP_SRC += ScreenBuffer.cpp ScreenBufferMono.cpp Graphics.cpp

ifndef PLATFORM
PLATFORM = Daisy
endif

ifeq ($(PLATFORM),Daisy)
  CPPFLAGS += -DDAISY
#  vpath %.c usbd-hs
endif

include $(OPENWARE)/Hardware/sources.mk

OBJS = $(C_SRC:%.c=Build/%.o)
OBJS += $(CPP_SRC:%.cpp=Build/%.o)
OBJS += $(S_SRC:%.s=Build/%.o)

OBJS += $(OBJS_DAC)
OBJS += $(OBJS_SDRAM)
OBJS += $(OBJS_SAI)
OBJS += $(OBJS_DSP)
OBJS += $(OBJS_OS)
OBJS += $(OBJS_USBD)
OBJS += $(OBJS_UART)
OBJS += $(OBJS_IWDG)

include $(OPENWARE)/Hardware/h7.mk

