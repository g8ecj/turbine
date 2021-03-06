#
# Wizard autogenerated makefile.
# DO NOT EDIT, use the ardmega-turbine_user.mk file instead.
#

# Constants automatically defined by the selected modules
ardmega-turbine_DEBUG = 1

# Our target application
TRG += ardmega-turbine

ardmega-turbine_PREFIX = "/usr/bin/avr-"

ardmega-turbine_SUFFIX = ""

ardmega-turbine_SRC_PATH = ardmega-turbine

ardmega-turbine_HW_PATH = ardmega-turbine

# Files automatically generated by the wizard. DO NOT EDIT, USE ardmega-turbine_USER_CSRC INSTEAD!
ardmega-turbine_WIZARD_CSRC = \
	bertos/algo/crc8.c \
	bertos/cpu/avr/drv/ser_avr.c \
	bertos/cpu/avr/drv/ser_mega.c \
	bertos/cpu/avr/drv/timer_mega.c \
	bertos/cpu/avr/drv/i2c_mega.c \
	bertos/drv/pcf8574.c \
	bertos/drv/lcd_hd44780.c \
	bertos/drv/i2c.c \
	bertos/drv/kbd.c \
	bertos/drv/ow_1wire.c \
	bertos/drv/ow_ds18x20.c \
	bertos/drv/ow_ds2413.c \
	bertos/drv/ow_ds2438.c \
	bertos/drv/sd.c \
	bertos/drv/sd_spi.c \
	bertos/drv/ser.c \
	bertos/drv/term.c \
	bertos/drv/timer.c \
	bertos/io/kblock.c \
	bertos/io/kfile.c \
	bertos/mware/event.c \
	bertos/mware/formatwr.c \
	bertos/mware/hex.c \
	bertos/mware/sprintf.c \
	#

# Files automatically generated by the wizard. DO NOT EDIT, USE ardmega-turbine_USER_PCSRC INSTEAD!
ardmega-turbine_WIZARD_PCSRC = \
	bertos/mware/formatwr.c \
	bertos/mware/sprintf.c \
	#

# Files automatically generated by the wizard. DO NOT EDIT, USE ardmega-turbine_USER_CPPASRC INSTEAD!
ardmega-turbine_WIZARD_CPPASRC = \
	 \
	#

# Files automatically generated by the wizard. DO NOT EDIT, USE ardmega-turbine_USER_CXXSRC INSTEAD!
ardmega-turbine_WIZARD_CXXSRC = \
	 \
	#

# Files automatically generated by the wizard. DO NOT EDIT, USE ardmega-turbine_USER_ASRC INSTEAD!
ardmega-turbine_WIZARD_ASRC = \
	 \
	#

ardmega-turbine_CPPFLAGS = -D'CPU_FREQ=(16000000UL)' -D'ARCH=(ARCH_DEFAULT)' -D'WIZ_AUTOGEN' -I$(ardmega-turbine_HW_PATH) -I$(ardmega-turbine_SRC_PATH) $(ardmega-turbine_CPU_CPPFLAGS) $(ardmega-turbine_USER_CPPFLAGS)

# Automatically generated by the wizard. PLEASE DO NOT EDIT!
ardmega-turbine_LDFLAGS = $(ardmega-turbine_CPU_LDFLAGS) $(ardmega-turbine_WIZARD_LDFLAGS) $(ardmega-turbine_USER_LDFLAGS)

# Automatically generated by the wizard. PLEASE DO NOT EDIT!
ardmega-turbine_CPPAFLAGS = $(ardmega-turbine_CPU_CPPAFLAGS) $(ardmega-turbine_WIZARD_CPPAFLAGS) $(ardmega-turbine_USER_CPPAFLAGS)

# Automatically generated by the wizard. PLEASE DO NOT EDIT!
ardmega-turbine_CSRC = $(ardmega-turbine_CPU_CSRC) $(ardmega-turbine_WIZARD_CSRC) $(ardmega-turbine_USER_CSRC)

# Automatically generated by the wizard. PLEASE DO NOT EDIT!
ardmega-turbine_PCSRC = $(ardmega-turbine_CPU_PCSRC) $(ardmega-turbine_WIZARD_PCSRC) $(ardmega-turbine_USER_PCSRC)

# Automatically generated by the wizard. PLEASE DO NOT EDIT!
ardmega-turbine_CPPASRC = $(ardmega-turbine_CPU_CPPASRC) $(ardmega-turbine_WIZARD_CPPASRC) $(ardmega-turbine_USER_CPPASRC)

# Automatically generated by the wizard. PLEASE DO NOT EDIT!
ardmega-turbine_CXXSRC = $(ardmega-turbine_CPU_CXXSRC) $(ardmega-turbine_WIZARD_CXXSRC) $(ardmega-turbine_USER_CXXSRC)

# Automatically generated by the wizard. PLEASE DO NOT EDIT!
ardmega-turbine_ASRC = $(ardmega-turbine_CPU_ASRC) $(ardmega-turbine_WIZARD_ASRC) $(ardmega-turbine_USER_ASRC)

# CPU specific flags and options, defined in the CPU definition files.
# Automatically generated by the wizard. PLEASE DO NOT EDIT!
ardmega-turbine_MCU = atmega2560
ardmega-turbine_CPU_CPPFLAGS = -Os -Ibertos/cpu/avr/
ardmega-turbine_PROGRAMMER_CPU = atmega2560
ardmega-turbine_STOPFLASH_SCRIPT = bertos/prg_scripts/avr/stopflash.sh
ardmega-turbine_STOPDEBUG_SCRIPT = bertos/prg_scripts/none.sh
ardmega-turbine_DEBUG_SCRIPT = bertos/prg_scripts/nodebug.sh
ardmega-turbine_FLASH_SCRIPT = bertos/prg_scripts/avr/flash.sh

include $(ardmega-turbine_SRC_PATH)/ardmega-turbine_user.mk
