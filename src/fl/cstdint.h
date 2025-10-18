#pragma once

///////////////////////////////////////////////////////////////////////////////
// FastLED C Language Integer Type Definitions
//
// IMPORTANT: This file is for C language compilation ONLY. C++ code should
// use fl/stdint.h which provides the fl:: namespace types.
//
// FastLED has carefully purged <stdint.h> from the include path to avoid
// ~500ms compilation time overhead per object file. Instead, FastLED defines
// its own integer types using primitive types in platform-specific int.h files.
//
// This file provides standard C integer type names (uint8_t, int32_t, etc.)
// as typedefs of platform-specific types, plus stddef types (size_t, ptrdiff_t).
///////////////////////////////////////////////////////////////////////////////

// Include platform-specific type definitions (no namespace for C)
#include "platforms/int.h"

// Define standard integer type names for C code
// 8-bit types use raw primitives to match system headers exactly
typedef unsigned char uint8_t;
typedef signed char int8_t;

// 16-bit types use platform-specific base types
typedef u16 uint16_t;
typedef i16 int16_t;

// 32-bit types use platform-specific base types
typedef u32 uint32_t;
typedef i32 int32_t;

// 64-bit types use platform-specific base types
typedef u64 uint64_t;
typedef i64 int64_t;

// Pointer and size types for C code
// These are defined here directly for C language support (no C++ namespace needed)
#ifndef FL_SIZE_T_DEFINED
#define FL_SIZE_T_DEFINED
typedef __SIZE_TYPE__ size_t;
#endif

#ifndef FL_PTRDIFF_T_DEFINED
#define FL_PTRDIFF_T_DEFINED
#ifdef __PTRDIFF_TYPE__
typedef __PTRDIFF_TYPE__ ptrdiff_t;
#else
// Fallback for compilers that don't define __PTRDIFF_TYPE__ (like older AVR-GCC)
typedef long ptrdiff_t;
#endif
#endif

// uintptr_t and intptr_t from platform-specific types
typedef uptr uintptr_t;
typedef iptr intptr_t;

// stdint.h limit macros
// Guard against redefinition if system headers already defined them
#ifndef INT8_MIN
#define INT8_MIN   (-128)
#endif
#ifndef INT16_MIN
#define INT16_MIN  (-32767-1)
#endif
#ifndef INT32_MIN
#define INT32_MIN  (-2147483647-1)
#endif
#ifndef INT64_MIN
#define INT64_MIN  (-9223372036854775807LL-1)
#endif

#ifndef INT8_MAX
#define INT8_MAX   127
#endif
#ifndef INT16_MAX
#define INT16_MAX  32767
#endif
#ifndef INT32_MAX
#define INT32_MAX  2147483647
#endif
#ifndef INT64_MAX
#define INT64_MAX  9223372036854775807LL
#endif

#ifndef UINT8_MAX
#define UINT8_MAX  0xFF
#endif
#ifndef UINT16_MAX
#define UINT16_MAX 0xFFFF
#endif
#ifndef UINT32_MAX
#define UINT32_MAX 0xFFFFFFFFU
#endif
#ifndef UINT64_MAX
#define UINT64_MAX 0xFFFFFFFFFFFFFFFFULL
#endif
