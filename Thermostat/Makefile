# Simple Makefile to build Arduino sketches
# Markus Wolf, 2013

# Standard build parameters, no need to change those
ARDUINO_DIR  = /usr/share/arduino
ARDUINO_PORT = /dev/ttyACM0
BOARD_TAG    = uno

# Project-specific parameters
ARDUINO_LIBS = SoftwareSerial
TARGET       = Thermostat

# Let arduino-mk play its magic :-)
include /usr/share/arduino/Arduino.mk

# Extra stuff
tags:
	@ctags -Re .
