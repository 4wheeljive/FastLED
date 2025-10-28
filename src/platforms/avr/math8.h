#pragma once

#include "platforms/math8_config.h"
#include "lib8tion/lib8static.h"
#include "lib8tion/intmap.h"
#include "fl/compiler_control.h"
#include "fl/force_inline.h"

// Select appropriate AVR implementation based on MUL instruction availability
#if defined(LIB8_ATTINY)
// ATtiny platforms without MUL instruction
#include "math8_attiny.h"
#else
// AVR platforms with MUL instruction (ATmega, etc.)
#include "math8_avr.h"
#endif

// All function implementations are now in math8_attiny.h or math8_avr.h
// This file is just a router to select the appropriate implementation
