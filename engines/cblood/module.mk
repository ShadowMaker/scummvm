MODULE := engines/cblood

MODULE_OBJS := \
	cblood.o \
	metaengine.o \
	game_state.o \
	resources.o \
	renderer.o \
	sound.o \
	screen_controller.o \
	cockpit_panel.o \
	cockpit_screen.o \
	planet_screen.o \
	planet_destruction.o \
	startup_screen.o

MODULE_DIRS += \
	engines/cblood

# This module can be built as a plugin
ifeq ($(ENABLE_CBLOOD), DYNAMIC_PLUGIN)
PLUGIN := 1
endif

# Include common rules
include $(srcdir)/rules.mk

# Detection objects
DETECT_OBJS += $(MODULE)/detection.o
