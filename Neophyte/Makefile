PROJECT = Neophyte
BUILDROOT = .
OPENWARE ?= $(BUILDROOT)/..

include $(OPENWARE)/Hardware/neophyte.mk

C_SRC = $(filter-out Src/test.c, $(wildcard Src/*.c))
CPP_SRC = $(wildcard Src/*.cpp)
C_SRC += $(OPENWARE)/Source/HAL_MAX11300.c
C_SRC += $(OPENWARE)/Source/usbd_midi.c
#CPP_SRC += $(OPENWARE)/Source/ScreenBuffer.cpp
#CPP_SRC += $(OPENWARE)/Source/ScreenBufferColour.cpp
#CPP_SRC += $(OPENWARE)/Source/Graphics.cpp
CPP_SRC += $(OPENWARE)/Source/MidiStreamReader.cpp
CPP_SRC += $(OPENWARE)/Source/FirmwareHeader.cpp

include $(OPENWARE)/Hardware/sources.mk

C_SRC += $(C_SRC_USBD)
C_SRC += $(C_SRC_DSP)
C_SRC += $(C_SRC_OS)
C_SRC += $(C_SRC_UART)
C_SRC += $(C_SRC_I2C)
C_SRC += $(C_SRC_IWDG)

include $(OPENWARE)/Hardware/l4.mk
