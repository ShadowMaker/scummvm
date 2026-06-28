MODULE := engines/cblood

MODULE_OBJS := \
	cblood.o \
	metaengine.o \
	music.o

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
