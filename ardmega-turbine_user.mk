#
# User makefile.
# Edit this file to change compiler options and related stuff.
#
GIT_VERSION := $(shell cd $(ardmega-turbine_SRC_PATH); git describe --abbrev=4 --dirty --always; cd ..)

# Programmer interface configuration, see http://dev.bertos.org/wiki/ProgrammerInterface for help
ardmega-turbine_PROGRAMMER_TYPE = none
ardmega-turbine_PROGRAMMER_PORT = none

# Files included by the user.
ardmega-turbine_USER_CSRC = \
	$(ardmega-turbine_SRC_PATH)/main.c \
	$(ardmega-turbine_SRC_PATH)/control.c \
	$(ardmega-turbine_SRC_PATH)/measure.c \
	$(ardmega-turbine_SRC_PATH)/rpm.c \
	$(ardmega-turbine_SRC_PATH)/tlog.c \
	$(ardmega-turbine_SRC_PATH)/rtc.c \
	$(ardmega-turbine_SRC_PATH)/ui.c \
	$(ardmega-turbine_SRC_PATH)/byteordering.c \
	$(ardmega-turbine_SRC_PATH)/fat.c \
	$(ardmega-turbine_SRC_PATH)/partition.c \
	$(ardmega-turbine_SRC_PATH)/sd_raw.c \
	$(ardmega-turbine_SRC_PATH)/median.c \
	$(ardmega-turbine_SRC_PATH)/eeprommap.c \
	$(ardmega-turbine_SRC_PATH)/minmax.c \
	#

# Files included by the user.
ardmega-turbine_USER_PCSRC = \
	#

# Files included by the user.
ardmega-turbine_USER_CPPASRC = \
	#

# Files included by the user.
ardmega-turbine_USER_CXXSRC = \
	#

# Files included by the user.
ardmega-turbine_USER_ASRC = \
	#

# Flags included by the user.
ardmega-turbine_USER_LDFLAGS = \
	#

# Flags included by the user.
ardmega-turbine_USER_CPPAFLAGS = \
	#

# Flags included by the user.
ardmega-turbine_USER_CPPFLAGS = \
	-fno-strict-aliasing \
	-fwrapv \
	-DVERSION=\"$(GIT_VERSION)\" \
	#
