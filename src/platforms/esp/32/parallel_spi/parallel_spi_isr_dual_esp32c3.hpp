// parallel_spi_isr_dual_esp32c3.hpp — 2-way Dual-SPI ISR wrapper for ESP32-C3/C2
#pragma once

#include "fl/namespace.h"
#include "fl/stdint.h"
#include <stddef.h>
#include "fl_parallel_spi_isr_rv.h"

namespace fl {

/**
 * DualSPI_ISR_ESP32C3 - 2-way parallel soft-SPI ISR driver for ESP32-C3/C2
 *
 * This class provides a simplified 2-pin variant of the parallel SPI ISR driver,
 * specifically designed to match hardware Dual-SPI architecture (2 data + 1 clock).
 *
 * Key Differences from 4-way QuadSPI and 8-way ParallelSPI:
 * - Only 2 data pins (instead of 4 or 8)
 * - Simplified LUT initialization (only 4 unique states)
 * - Direct mapping to hardware Dual-SPI topology
 * - Ideal for testing hardware Dual-SPI implementations
 * - Useful for platforms with limited GPIO availability
 *
 * Architecture:
 * - Reuses the same ISR code (fl_parallel_spi_isr_rv.h/cpp)
 * - 256-entry LUT maps byte values to 2-pin GPIO masks
 * - Only uses lower 2 bits of byte value (upper 6 bits ignored)
 * - ISR operates at highest priority for minimal jitter
 *
 * Typical Usage:
 *   DualSPI_ISR_ESP32C3 spi;
 *   spi.setPinMapping(gpio_d0, gpio_d1, gpio_clk);
 *   spi.setupISR(1600000);  // 1.6MHz timer = 800kHz SPI
 *   spi.loadBuffer(data, len);
 *   spi.arm();
 *   while(spi.isBusy()) { }
 *   spi.stopISR();
 *
 * Test Patterns:
 * - 0x00: Both pins low (00)
 * - 0x01: D0 high, D1 low (01)
 * - 0x02: D0 low, D1 high (10)
 * - 0x03: Both pins high (11)
 */
class DualSPI_ISR_ESP32C3 {
public:
    /// Status bit definitions
    static constexpr uint32_t STATUS_BUSY = 1u;
    static constexpr uint32_t STATUS_DONE = 2u;

    /// Maximum pins per lane (dual = 2)
    static constexpr int NUM_DATA_PINS = 2;

    DualSPI_ISR_ESP32C3() = default;
    ~DualSPI_ISR_ESP32C3() = default;

    /**
     * Configure pin mapping for 2 data pins + 1 clock
     * @param d0 GPIO number for data bit 0 (LSB)
     * @param d1 GPIO number for data bit 1 (MSB)
     * @param clk GPIO number for clock pin
     *
     * Automatically initializes the 256-entry LUT to map byte values
     * to GPIO masks for the 2 specified data pins.
     */
    void setPinMapping(uint8_t d0, uint8_t d1, uint8_t clk) {
        // Store clock mask
        fl_spi_set_clock_mask(1u << clk);

        // Build pin masks array
        uint32_t dataPinMasks[NUM_DATA_PINS] = {
            1u << d0,  // Bit 0
            1u << d1   // Bit 1
        };

        // Initialize 256-entry LUT
        // For each possible byte value (0-255):
        // - Extract lower 2 bits
        // - Map each bit to corresponding GPIO pin
        // - Generate set_mask (pins to set high) and clear_mask (pins to clear low)
        PinMaskEntry* lut = fl_spi_get_lut_array();

        for (int byteValue = 0; byteValue < 256; byteValue++) {
            uint32_t setMask = 0;
            uint32_t clearMask = 0;

            // Only process lower 2 bits (upper 6 bits ignored)
            for (int bitPos = 0; bitPos < NUM_DATA_PINS; bitPos++) {
                if (byteValue & (1 << bitPos)) {
                    setMask |= dataPinMasks[bitPos];
                } else {
                    clearMask |= dataPinMasks[bitPos];
                }
            }

            lut[byteValue].set_mask = setMask;
            lut[byteValue].clear_mask = clearMask;
        }
    }

    /**
     * Alternative: Configure pin mapping using clock mask directly
     * @param d0-d1 GPIO numbers for data pins
     * @param clockMask Pre-computed GPIO mask for clock pin (e.g., 1 << 8)
     */
    void setPinMappingWithMask(uint8_t d0, uint8_t d1, uint32_t clockMask) {
        fl_spi_set_clock_mask(clockMask);

        uint32_t dataPinMasks[NUM_DATA_PINS] = {
            1u << d0, 1u << d1
        };

        PinMaskEntry* lut = fl_spi_get_lut_array();
        for (int v = 0; v < 256; v++) {
            uint32_t setMask = 0;
            uint32_t clearMask = 0;

            for (int b = 0; b < NUM_DATA_PINS; b++) {
                if (v & (1 << b)) {
                    setMask |= dataPinMasks[b];
                } else {
                    clearMask |= dataPinMasks[b];
                }
            }

            lut[v].set_mask = setMask;
            lut[v].clear_mask = clearMask;
        }
    }

    /**
     * Bulk load data buffer
     * @param data Pointer to data bytes
     * @param n Number of bytes (max 256)
     *
     * Each byte in the buffer represents 2 parallel bits to output.
     * Only the lower 2 bits of each byte are used.
     */
    void loadBuffer(const uint8_t* data, uint16_t n) {
        if (!data) return;
        if (n > 256) n = 256;

        uint8_t* dest = fl_spi_get_data_array();
        for (uint16_t i = 0; i < n; ++i) {
            dest[i] = data[i];
        }
        fl_spi_set_total_bytes(n);
    }

    /**
     * Setup ISR and timer
     * @param timer_hz Timer frequency in Hz (should be 2× target SPI bit rate)
     * @return 0 on success, error code on failure
     *
     * Example: For 800kHz SPI output, use 1600000 Hz timer
     */
    int setupISR(uint32_t timer_hz) {
        return fl_spi_platform_isr_start(timer_hz);
    }

    /**
     * Stop ISR and timer
     */
    void stopISR() {
        fl_spi_platform_isr_stop();
    }

    /**
     * Arm a transfer (caller must ensure visibility delay first)
     * Increments doorbell counter to trigger ISR edge detection
     */
    void arm() {
        fl_spi_arm();
    }

    /**
     * Check if ISR is currently transmitting
     */
    bool isBusy() const {
        return (fl_spi_status_flags() & STATUS_BUSY) != 0;
    }

    /**
     * Get raw status flags
     */
    uint32_t statusFlags() const {
        return fl_spi_status_flags();
    }

    /**
     * Acknowledge DONE flag (clears it)
     */
    void ackDone() {
        fl_spi_ack_done();
    }

    /**
     * Visibility delay (ensures memory writes are visible to ISR)
     * Typical value: 10 microseconds
     */
    static void visibilityDelayUs(uint32_t us) {
        fl_spi_visibility_delay_us(us);
    }

    /**
     * Reset ISR state (between runs)
     */
    static void resetState() {
        fl_spi_reset_state();
    }

    /**
     * Get mutable reference to LUT array (256 entries)
     * For advanced users who want direct LUT control
     */
    static PinMaskEntry* getLUTArray() {
        return fl_spi_get_lut_array();
    }

    /**
     * Get mutable reference to data buffer array (256 bytes)
     * For advanced users who want direct buffer access
     */
    static uint8_t* getDataArray() {
        return fl_spi_get_data_array();
    }

#ifdef FL_SPI_ISR_VALIDATE
    /**
     * Get GPIO event log (only available when FL_SPI_ISR_VALIDATE is defined)
     * Returns pointer to array of GPIO events captured during ISR execution
     */
    static const FastLED_GPIO_Event* getValidationEvents() {
        return fl_spi_get_validation_events();
    }

    /**
     * Get number of GPIO events captured
     */
    static uint16_t getValidationEventCount() {
        return fl_spi_get_validation_event_count();
    }

    // C++ wrapper types for GPIO events
    enum class GPIOEventType : uint8_t {
        StateStart  = FASTLED_GPIO_EVENT_STATE_START,
        StateDone   = FASTLED_GPIO_EVENT_STATE_DONE,
        SetBits     = FASTLED_GPIO_EVENT_SET_BITS,
        ClearBits   = FASTLED_GPIO_EVENT_CLEAR_BITS,
        ClockLow    = FASTLED_GPIO_EVENT_CLOCK_LOW,
        ClockHigh   = FASTLED_GPIO_EVENT_CLOCK_HIGH
    };

    struct GPIOEvent {
        uint8_t  event_type;
        uint8_t  padding[3];
        union {
            uint32_t gpio_mask;
            uint32_t state_info;
        } payload;

        GPIOEventType type() const { return static_cast<GPIOEventType>(event_type); }
    };

    static const GPIOEvent* getValidationEventsTyped() {
        return reinterpret_cast<const GPIOEvent*>(fl_spi_get_validation_events());
    }
#endif
};

}  // namespace fl
