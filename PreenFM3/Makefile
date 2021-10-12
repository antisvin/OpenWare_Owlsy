PROJECT = PreenFM3
BUILDROOT = .
OPENWARE ?= $(BUILDROOT)/..
OPENOCD ?= openocd -f $(OPENWARE)/Hardware/openocd_h7.cfg

include $(OPENWARE)/Hardware/preenfm3.mk

C_SRC = $(wildcard Src/*.c)
CPP_SRC = $(wildcard Src/*.cpp)
C_SRC += $(OPENWARE)/Source/usbd_midi.c
C_SRC += $(OPENWARE)/Source/Font6x10.c
C_SRC += $(OPENWARE)/Source/Font10x16.c
CPP_SRC += $(OPENWARE)/Source/uart.cpp
CPP_SRC += $(OPENWARE)/Source/uart_midi.cpp
#CPP_SRC += $(OPENWARE)/Source/bus.cpp
#CPP_SRC += $(OPENWARE)/Source/DigitalBusHandler.cpp
#CPP_SRC += $(OPENWARE)/Source/DigitalBusReader.cpp
#CPP_SRC += $(OPENWARE)/Source/DigitalBusStreamReader.cpp
CPP_SRC += $(OPENWARE)/Source/ScreenBuffer.cpp
CPP_SRC += $(OPENWARE)/Source/ScreenBufferColour.cpp
CPP_SRC += $(OPENWARE)/Source/Graphics.cpp
CPP_SRC += $(OPENWARE)/Source/MidiStreamReader.cpp

include $(OPENWARE)/Hardware/sources.mk

C_SRC += $(C_SRC_SAI)
C_SRC += $(C_SRC_DAC)
C_SRC += $(C_SRC_USBD)
C_SRC += $(C_SRC_DSP)
C_SRC += $(C_SRC_OS)
C_SRC += $(C_SRC_UART)
C_SRC += $(C_SRC_IWDG)

include $(OPENWARE)/Hardware/h7.mk
