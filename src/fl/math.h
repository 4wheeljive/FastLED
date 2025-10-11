#pragma once

#include "fl/clamp.h"
#include "fl/math_macros.h"

namespace fl {

// Forward declarations of implementation functions defined in math.cpp
float floor_impl(float value);
double floor_impl(double value);
float ceil_impl(float value);
double ceil_impl(double value);
float exp_impl(float value);
double exp_impl(double value);
float sqrt_impl(float value);
double sqrt_impl(double value);
float sin_impl(float value);
double sin_impl(double value);
float cos_impl(float value);
double cos_impl(double value);

template <typename T> inline T floor(T value) {
    if (value >= 0) {
        return static_cast<T>(static_cast<int>(value));
    }
    return static_cast<T>(floor_impl(static_cast<float>(value)));
}

template <typename T> inline T ceil(T value) {
    if (value <= 0) {
        return static_cast<T>(static_cast<int>(value));
    }
    return static_cast<T>(ceil_impl(static_cast<float>(value)));
}

// Exponential function using custom implementation
template <typename T> inline T exp(T value) {
    return static_cast<T>(exp_impl(static_cast<double>(value)));
}

// Square root using Newton-Raphson method
template <typename T> inline T sqrt(T value) {
    return static_cast<T>(sqrt_impl(static_cast<float>(value)));
}

// Floating point modulo operation: fmod(x, y) = x - floor(x/y) * y
// This is compatible with platforms that don't have fmodf() in their math library
template <typename T> inline T fmod(T x, T y) {
    if (y == 0) {
        return static_cast<T>(0);  // Avoid division by zero
    }
    return x - floor(x / y) * y;
}

// Trigonometric functions
template <typename T> inline T sin(T value) {
    return static_cast<T>(sin_impl(static_cast<float>(value)));
}

template <typename T> inline T cos(T value) {
    return static_cast<T>(cos_impl(static_cast<float>(value)));
}

// Constexpr version for compile-time evaluation (compatible with older C++
// standards)
constexpr int ceil_constexpr(float value) {
    return static_cast<int>((value > static_cast<float>(static_cast<int>(value)))
                                ? static_cast<int>(value) + 1
                                : static_cast<int>(value));
}

// Arduino will define this in the global namespace as macros, so we can't
// define them ourselves.
// template <typename T>
// inline T abs(T value) {
//     return (value < 0) ? -value : value;
// }

// template <typename T>
// inline T min(T a, T b) {
//     return (a < b) ? a : b;
// }

// template <typename T>
// inline T max(T a, T b) {
//     return (a > b) ? a : b;
// }

} // namespace fl

// Include map_range.h at the end to avoid circular dependency issues
// geometry.h (included by map_range.h) uses fl::sqrt which must be defined first
#include "fl/map_range.h"
