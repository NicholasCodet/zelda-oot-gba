# Minimal devkitPro/libgba Makefile for a single-file GBA project.
# It builds zelda_ocarina_of_time.gba from main.c.

# Default devkitPro installation path for this machine.
DEVKITPRO ?= /opt/devkitpro

# devkitARM toolchain location.
DEVKITARM ?= $(DEVKITPRO)/devkitARM

# Explicit libgba include and library paths.
LIBGBA_INC := $(DEVKITPRO)/libgba/include
LIBGBA_LIB := $(DEVKITPRO)/libgba/lib

# Fail early if required toolchain paths are not present.
ifeq ($(wildcard $(DEVKITARM)),)
$(error "devkitARM not found at $(DEVKITARM)")
endif
ifeq ($(wildcard $(LIBGBA_INC)),)
$(error "libgba headers not found at $(LIBGBA_INC)")
endif
ifeq ($(wildcard $(LIBGBA_LIB)/libgba.a),)
$(error "libgba library not found at $(LIBGBA_LIB)/libgba.a")
endif

# Project output and build directory.
TARGET := roms/zelda_ocarina_of_time
BUILD := build

# Keep the project minimal: compile sources only from repository root.
SOURCES := .

# GBA CPU flags.
ARCH := -mthumb -mthumb-interwork

# Compile flags with explicit libgba include path.
CFLAGS := -g -Wall -O2 $(ARCH) -I$(LIBGBA_INC)
CXXFLAGS := $(CFLAGS)
LDFLAGS := -g $(ARCH)

# Use GCC as the linker driver (required by gba.specs and GBA flags).
# If LD falls back to system ld, linking fails with unknown option errors.
LD := $(DEVKITARM)/bin/arm-none-eabi-gcc

# Link flags with explicit libgba library path.
LIBS := -L$(LIBGBA_LIB) -lgba

# Standard devkitPro GBA build rules.
include $(DEVKITARM)/gba_rules

# -----------------------------------------------------------------------------
# Standard recursive build layout from devkitPro examples.
# Top-level make enters $(BUILD), then build happens inside that directory.
# -----------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))

# Output path (without extension).
export OUTPUT := $(CURDIR)/$(TARGET)

# Source search path.
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))

# Dependency output directory.
export DEPSDIR := $(CURDIR)/$(BUILD)

# Find C/C++/ASM source files.
CFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))

# Convert source names to object file names.
export OFILES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

# Main targets.
.PHONY: all clean

all: $(BUILD)

$(BUILD):
	@mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).gba

else

# Link objects into the final .gba ROM.
$(OUTPUT).gba: $(OFILES)

endif
