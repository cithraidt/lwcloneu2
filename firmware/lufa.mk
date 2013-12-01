
# --------------------------------------
#             LUFA Library
#     Copyright (C) Dean Camera, 2012.
#
#  dean [at] fourwalledcubicle [dot] com
#           www.lufa-lib.org
#
# --------------------------------------
#         LUFA Project Makefile.
# --------------------------------------

ARCH         = AVR8
BOARD        = USER
F_USB        = $(F_CPU)
OPTIMIZATION = s
LUFA_PATH    = $(PARENT_PATH)/../../../electronic/LUFA/LUFA-130303/LUFA
SRC          =  $(LWCLONE_SRC) $(LUFA_SRC_USB)
CC_FLAGS     = -DUSE_LUFA_CONFIG_HEADER -I$(TARGET_PATH) -I$(PARENT_PATH)
LD_FLAGS     =

# Default target
all:

# Include LUFA build script makefiles
include $(LUFA_PATH)/Build/lufa_core.mk
include $(LUFA_PATH)/Build/lufa_sources.mk
include $(LUFA_PATH)/Build/lufa_build.mk
include $(LUFA_PATH)/Build/lufa_cppcheck.mk
include $(LUFA_PATH)/Build/lufa_doxygen.mk
include $(LUFA_PATH)/Build/lufa_dfu.mk
include $(LUFA_PATH)/Build/lufa_hid.mk
include $(LUFA_PATH)/Build/lufa_avrdude.mk
include $(LUFA_PATH)/Build/lufa_atprogram.mk



