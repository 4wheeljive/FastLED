#pragma once

#include "fl/int.h"  // for FastLED integer types
#include "fl/cstring.h"  // for memcopy (FastLED equivalent)

// Guard against PROGMEM redefinition on platforms that have their own definition
#if !defined(PROGMEM) && !defined(__IMXRT1062__) && !defined(__MK20DX128__) && !defined(__MK20DX256__) && !defined(__MK66FX1M0__) && !defined(__MK64FX512__) && !defined(__MKL26Z64__)
#define PROGMEM
#endif

#if !defined(FL_PROGMEM)
// Ensure PROGMEM is available before using it
#if defined(PROGMEM)
#define FL_PROGMEM PROGMEM
#else
// Fallback for platforms without PROGMEM - just use empty attribute
#define FL_PROGMEM
#endif
#endif

// Private helper function for safe memory copying to avoid strict aliasing issues
// This uses fl::memcpy (FastLED equivalent) instead of standard memcpy
template<typename T>
static inline T fl_progmem_safe_read(const void* addr) {
    T result;
    fl::memcpy(&result, addr, sizeof(T));
    return result;
}

// Safe memory access macros to avoid strict aliasing issues
// These use the private helper function which wraps memcpy safely
static inline fl::u8 fl_pgm_read_byte_near_safe(const void* addr) {
    return fl_progmem_safe_read<fl::u8>(addr);
}

static inline fl::u16 fl_pgm_read_word_near_safe(const void* addr) {
    return fl_progmem_safe_read<fl::u16>(addr);
}

static inline fl::u32 fl_pgm_read_dword_near_safe(const void* addr) {
    return fl_progmem_safe_read<fl::u32>(addr);
}

#define FL_PGM_READ_BYTE_NEAR(x) (fl_pgm_read_byte_near_safe(x))
#define FL_PGM_READ_WORD_NEAR(x) (fl_pgm_read_word_near_safe(x))
#define FL_PGM_READ_DWORD_NEAR(x) (fl_pgm_read_dword_near_safe(x))
#define FL_ALIGN_PROGMEM

#define FL_PROGMEM_USES_NULL 1
