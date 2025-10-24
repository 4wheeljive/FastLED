/// @file spi_dual_stub.h
/// @brief Stub/Mock Dual-SPI interface for testing
///
/// This header exposes the SPIDualStub class for tests to access
/// test-specific inspection methods.

#pragma once

#include "platforms/shared/spi_hw_2.h"

#ifdef FASTLED_TESTING

namespace fl {

/// Mock Dual-SPI driver for testing without real hardware
/// Implements SpiHw2 interface with data capture for validation
class SpiHw2Stub : public SpiHw2 {
public:
    explicit SpiHw2Stub(int bus_id = -1, const char* name = "MockSPI");
    ~SpiHw2Stub() override = default;

    bool begin(const SpiHw2::Config& config) override;
    void end() override;
    bool transmit(fl::span<const uint8_t> buffer, TransmitMode mode = TransmitMode::ASYNC) override;
    bool waitComplete(uint32_t timeout_ms = UINT32_MAX) override;
    bool isBusy() const override;
    bool isInitialized() const override;
    int getBusId() const override;
    const char* getName() const override;

    // Test inspection methods
    const fl::vector<uint8_t>& getLastTransmission() const;
    uint32_t getTransmissionCount() const;
    uint32_t getClockSpeed() const;
    bool isTransmissionActive() const;
    void reset();

    // De-interleave transmitted data to extract per-lane data (for testing)
    fl::vector<fl::vector<uint8_t>> extractLanes(uint8_t num_lanes, size_t bytes_per_lane) const;

private:
    int mBusId;
    const char* mName;
    bool mInitialized;
    bool mBusy;
    uint32_t mClockSpeed;
    uint32_t mTransmitCount;
    fl::vector<uint8_t> mLastBuffer;
};

/// Cast SpiHw2* to SpiHw2Stub* for test inspection
/// @param driver SpiHw2 pointer (must be from test environment)
/// @returns SpiHw2Stub pointer, or nullptr if cast fails
inline SpiHw2Stub* toStub(SpiHw2* driver) {
    return static_cast<SpiHw2Stub*>(driver);
}

}  // namespace fl

#endif  // FASTLED_TESTING
