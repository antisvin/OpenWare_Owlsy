PROJECT = MagusEncoder
BUILDROOT = .
OPENWARE ?= $(BUILDROOT)/..
OPENOCD ?= openocd -f $(BUILDROOT)/openocd.cfg

include $(OPENWARE)/Hardware/STM32F051K4.mk

C_SRC = $(wildcard Src/*.c)
CPP_SRC = $(wildcard Src/*.cpp)

#OBJS = $(C_SRC:%.c=$(BUILDROOT)/%.o)
#OBJS += $(CPP_SRC:%.cpp=$(BUILDROOT)/%.o)
#OBJS += $(S_SRC:%.s=$(BUILDROOT)/%.o)

include $(OPENWARE)/Hardware/f0.mk


upload:
	$(OPENOCD) -c "program Build/${PROJECT}.elf verify reset exit"
