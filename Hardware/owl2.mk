# Debug / Release
CONFIG ?= Release
# Debug without -O1 runs out of flash space at least on Magus
ifeq ($(CONFIG),Debug)
  CPPFLAGS = -O1 -g3 -Wall -Wcpp -Wunused-function -DDEBUG # -DUSE_FULL_ASSERT
  ASFLAGS  = -g3
  CFLAGS   = -g3
endif
ifeq ($(CONFIG),Release)
  CPPFLAGS = -O2
  ASFLAGS  = -O2
  CFLAGS   = -O2
endif

# compile with semihosting if Debug is selected
ifeq ($(CONFIG),Debug)
  LDLIBS += -lrdimon
  LDFLAGS += -specs=rdimon.specs
else
  CPPFLAGS += -nostdlib -nostartfiles -fno-builtin -ffreestanding
  C_SRC += libnosys_gnu.c
  LDFLAGS += --specs=nano.specs
endif

# Compilation Flags
LDFLAGS += -Wl,--gc-sections
LDSCRIPT ?= $(OPENWARE)/Hardware/owl2.ld
LDLIBS += -lc -lm
# CPPFLAGS += -DEXTERNAL_SRAM -DARM_CORTEX
# CPPFLAGS += -fpic -fpie
CPPFLAGS += -fdata-sections
CPPFLAGS += -ffunction-sections
CPPFLAGS += -fno-builtin -ffreestanding
LDFLAGS += -fno-builtin -ffreestanding
CXXFLAGS = -fno-rtti -fno-exceptions -std=gnu++11
CFLAGS  += -std=gnu99
ARCH_FLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
ARCH_FLAGS += -fsingle-precision-constant
DEF_FLAGS = -DSTM32F427xx -DARM_MATH_CM4
DEF_FLAGS += -D__FPU_PRESENT=1U
S_SRC = startup_stm32f427xx.s
