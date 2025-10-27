// ok no namespace fl
#pragma once

/// @file is_arm.h
/// ARM platform detection header
/// 
/// This header detects ARM-based platforms by checking compiler-defined macros
/// and defines FASTLED_ARM when an ARM platform is detected.
/// 
/// Used by platforms/int.h for platform dispatching and by ARM platform headers
/// for validation that ARM detection has occurred.

/// ARM platform detection with optimized macro grouping
/// This checks for various ARM-based microcontroller families
#ifndef FASTLED_ARM
#if \
    /* ARM Cortex-M0/M0+ (SAM) */ \
    defined(__SAM3X8E__) || \
    /* ARM Cortex-M (STM32F/H7) */ \
    defined(STM32F10X_MD) || defined(__STM32F1__) || defined(STM32F1) || defined(STM32F1xx) || \
    defined(STM32F2XX) || defined(STM32F4) || \
    /* NXP Kinetis (MK20, MK26, IMXRT) */ \
    defined(__MK20DX128__) || defined(__MK20DX256__) || \
    defined(__MKL26Z64__) || defined(__IMXRT1062__) || \
    /* Arduino Renesas UNO R4 */ \
    defined(ARDUINO_ARCH_RENESAS) || \
    /* Arduino STM32H747 (GIGA) */ \
    defined(ARDUINO_GIGA) || defined(ARDUINO_GIGA_M7) || \
    /* Nordic nRF52 */ \
    defined(NRF52_SERIES) || defined(ARDUINO_ARCH_NRF52) || \
    defined(NRF52840_XXAA) || defined(ARDUINO_NRF52840_FEATHER_SENSE) || \
    /* Ambiq Apollo3 */ \
    defined(ARDUINO_ARCH_APOLLO3) || defined(FASTLED_APOLLO3) || \
    /* Raspberry Pi RP2040 */ \
    defined(ARDUINO_ARCH_RP2040) || defined(TARGET_RP2040) || \
    defined(PICO_32BIT) || defined(ARDUINO_RASPBERRY_PI_PICO) || \
    /* Silicon Labs EFM32 */ \
    defined(ARDUINO_ARCH_SILABS) || \
    /* Microchip SAMD21 */ \
    defined(__SAMD21G18A__) || defined(__SAMD21J18A__) || \
    defined(__SAMD21E17A__) || defined(__SAMD21E18A__) || \
    /* Microchip SAMD51/SAME51 */ \
    defined(__SAMD51G19A__) || defined(__SAMD51J19A__) || \
    defined(__SAME51J19A__) || defined(__SAMD51P19A__) || defined(__SAMD51P20A__)
#define FASTLED_ARM
#endif  // ARM platform detection
#endif  // FASTLED_ARM
