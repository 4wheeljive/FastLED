/// @file spi_single_stub.h
/// @brief Stub/Mock Single-SPI interface for testing
///
/// This header exposes the SPISingleStub class for tests to access
/// test-specific inspection methods.

#pragma once

#include "platforms/shared/spi_hw_1.h"

#ifdef FASTLED_TESTING

namespace fl {

/// Mock Single-SPI driver for testing without real hardware
/// Implements SpiHw1 interface with data capture for validation
class SpiHw1Stub : public SpiHw1 {
public:
    explicit SpiHw1Stub(int bus_id = -1, const char* name = "MockSPI");
    ~SpiHw1Stub() override = default;

    bool begin(const SpiHw1::Config& config) override;
    void end() override;
    bool transmitAsync(fl::span<const uint8_t> buffer) override;
    bool waitComplete(uint32_t timeout_ms = UINT32_MAX) override;
    bool isBusy() const override;
    bool isInitialized() const override;
    int getBusId() const override;
    const char* getName() const override;

    // Test inspection methods
    const fl::vector<uint8_t>& getLastTransmission() const;
    uint32_t getTransmissionCount() const;
    uint32_t getClockSpeed() const;
    void reset();

private:
    int mBusId;
    const char* mName;
    bool mInitialized;
    uint32_t mClockSpeed;
    uint32_t mTransmitCount;
    fl::vector<uint8_t> mLastBuffer;
};

/// Cast SpiHw1* to SpiHw1Stub* for test inspection
/// @param driver SpiHw1 pointer (must be from test environment)
/// @returns SpiHw1Stub pointer, or nullptr if cast fails
inline SpiHw1Stub* toStub(SpiHw1* driver) {
    return static_cast<SpiHw1Stub*>(driver);
}

}  // namespace fl

#endif  // FASTLED_TESTING
