#pragma once

/// @file spi_hw_4.h
/// @brief Platform-agnostic 4-lane hardware SPI interface
///
/// This file defines the abstract interface that all platform-specific
/// 4-lane (quad-lane) SPI hardware must implement. It enables the generic
/// QuadSPIDevice to work across different platforms (ESP32, RP2040, etc.)
/// without knowing platform-specific implementation details.
///
/// For 8-lane (octal) SPI support, see spi_hw_8.h

#include "fl/namespace.h"
#include "fl/vector.h"
#include "fl/span.h"
#include "fl/stdint.h"

namespace fl {

class SpiHw4;

/// Abstract interface for platform-specific 4-lane hardware SPI
///
/// Platform implementations (ESP32, RP2040, etc.) inherit from this interface
/// and provide concrete implementations of all virtual methods.
///
/// Naming: "SpiHw4" = SPI Hardware 4-lane
class SpiHw4 {
public:
    virtual ~SpiHw4() = default;

    /// Platform-agnostic configuration structure for 4-lane SPI
    struct Config {
        uint8_t bus_num;           ///< SPI bus number (platform-specific numbering)
        uint32_t clock_speed_hz;   ///< Clock frequency in Hz
        int8_t clock_pin;          ///< SCK GPIO pin
        int8_t data0_pin;          ///< D0/MOSI GPIO pin
        int8_t data1_pin;          ///< D1/MISO GPIO pin (-1 = unused)
        int8_t data2_pin;          ///< D2/WP GPIO pin (-1 = unused)
        int8_t data3_pin;          ///< D3/HD GPIO pin (-1 = unused)
        uint32_t max_transfer_sz;  ///< Max bytes per transfer

        Config()
            : bus_num(0)
            , clock_speed_hz(20000000)
            , clock_pin(-1)
            , data0_pin(-1)
            , data1_pin(-1)
            , data2_pin(-1)
            , data3_pin(-1)
            , max_transfer_sz(65536) {}
    };

    /// Initialize SPI peripheral with given configuration
    /// @param config Hardware configuration (up to 4 data pins)
    /// @returns true on success, false on error
    /// @note Implementation should auto-detect 1/2/4-lane mode based on active pins
    /// @note For 8-lane support, use SpiHw8 interface instead
    virtual bool begin(const Config& config) = 0;

    /// Shutdown SPI peripheral and release resources
    /// @note Should wait for any pending transmissions to complete
    virtual void end() = 0;

    /// Queue asynchronous DMA transmission (non-blocking)
    /// @param buffer Data buffer to transmit (platform handles DMA requirements internally)
    /// @returns true if queued successfully, false on error
    /// @note Platform implementations handle DMA buffer allocation/alignment internally
    /// @note Buffer must remain valid until waitComplete() returns
    virtual bool transmitAsync(fl::span<const uint8_t> buffer) = 0;

    /// Wait for current transmission to complete (blocking)
    /// @param timeout_ms Maximum wait time in milliseconds
    /// @returns true if completed, false on timeout
    virtual bool waitComplete(uint32_t timeout_ms = UINT32_MAX) = 0;

    /// Check if a transmission is currently in progress
    /// @returns true if busy, false if idle
    virtual bool isBusy() const = 0;

    /// Get initialization status
    /// @returns true if initialized, false otherwise
    virtual bool isInitialized() const = 0;

    /// Get the SPI bus number/ID for this controller
    /// @returns SPI bus number (e.g., 2 or 3 for ESP32), or -1 if not assigned
    virtual int getBusId() const = 0;

    /// Get the platform-specific peripheral name for this controller
    /// @returns Human-readable peripheral name (e.g., "HSPI", "VSPI", "SPI0")
    /// @note Primarily for debugging, logging, and error messages
    /// @note Returns "Unknown" if not assigned
    virtual const char* getName() const = 0;

    /// Get all available 4-lane hardware SPI devices on this platform
    /// @returns Reference to static vector of available devices
    /// @note Cached - only allocates once on first call
    /// @note Thread-safe via C++11 static local initialization
    /// @note Returns empty vector if platform doesn't support 4-lane SPI
    /// @note Returns bare pointers - instances are alive forever (static lifetime)
    static const fl::vector<SpiHw4*>& getAll() {
        static fl::vector<SpiHw4*> instances = createInstances();
        return instances;
    }

private:
    /// Platform-specific factory implementation (weak linkage)
    /// Each platform overrides this with strong definition
    /// @returns Vector of platform-specific instances
    static fl::vector<SpiHw4*> createInstances();
};

}  // namespace fl
