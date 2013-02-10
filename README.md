arduino-code
============

This project started out as a common repository for my Arduino
experiments. A TOC follows, with a brief description for every entry.

The sketches include a cli build system based on
https://github.com/mjoldfield/Arduino-Makefile The libraries are
imported into a project simply by simlinking both the library .h and
.cpp files into the sketch directory. For an example see the
Thermostat sketch.

All the code is released under GPLv2.1

LIBRARIES
=========

* Debug - Debugging utilities.

* Debouncers - Provides a pushbutton debouncing event based API. A
  registered callback function will be invoked by the library when the
  corresponding button event is detected by the library. Uses Timers
  (see below) as a dependency. Currently CLICK and HOLD events are
  supported.

* Microtimers - Same as Timers (see below) on a micro-second scale.

* Timers - Provides a Time event based API. A registered callback
function will be invoked by the library when the corresponding time
event is detected by the library. Time resolution is 1/1000th of a
second (aka a millisecond).

SKETCHES
========

* Thermostat - My first Arduino sketch. Implements a standard
thermostat with hysteresis, user interaction is provided by a 2x16 LED
display and a few bush buttons. An extra LED is used for diagnostic. A
relay is used as the main actuator.
