/// @file spi_quad_stub.h
/// @brief Stub/Mock Quad-SPI interface for testing
///
/// This header exposes the SPIQuadStub class for tests to access
/// test-specific inspection methods.

#pragma once

#include "platforms/shared/spi_hw_4.h"

#ifdef FASTLED_TESTING

namespace fl {

/// Mock Quad-SPI driver for testing without real hardware
/// Implements SpiHw4 interface with data capture for validation
class SpiHw4Stub : public SpiHw4 {
public:
    explicit SpiHw4Stub(int bus_id = -1, const char* name = "MockSPI");
    ~SpiHw4Stub() override = default;

    bool begin(const SpiHw4::Config& config) override;
    void end() override;
    DMABuffer acquireDMABuffer(size_t bytes_per_lane) override;
    bool transmit(TransmitMode mode = TransmitMode::ASYNC) override;
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

    // DMA buffer management
    DMABuffer mCurrentBuffer;            // Current active buffer
    bool mBufferAcquired;
};

/// Cast SpiHw4* to SpiHw4Stub* for test inspection
/// @param driver SpiHw4 pointer (must be from test environment)
/// @returns SpiHw4Stub pointer, or nullptr if cast fails
inline SpiHw4Stub* toStub(SpiHw4* driver) {
    return static_cast<SpiHw4Stub*>(driver);
}

}  // namespace fl

#endif  // FASTLED_TESTING
