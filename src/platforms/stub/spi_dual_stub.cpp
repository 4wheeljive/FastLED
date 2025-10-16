/// @file spi_dual_stub.cpp
/// @brief Stub/Mock Dual-SPI implementation for testing

#ifdef FASTLED_TESTING

#include "spi_dual_stub.h"

namespace fl {

SpiHw2Stub::SpiHw2Stub(int bus_id, const char* name)
    : mBusId(bus_id)
    , mName(name)
    , mInitialized(false)
    , mBusy(false)
    , mClockSpeed(0)
    , mTransmitCount(0) {
}

bool SpiHw2Stub::begin(const SpiHw2::Config& config) {
    if (mInitialized) {
        return true;  // Already initialized
    }

    // Validate bus_num against mBusId if driver has pre-assigned ID
    if (mBusId != -1 && config.bus_num != static_cast<uint8_t>(mBusId)) {
        return false;  // Mismatch
    }

    mClockSpeed = config.clock_speed_hz;
    mInitialized = true;
    return true;
}

void SpiHw2Stub::end() {
    mInitialized = false;
    mBusy = false;
    mLastBuffer.clear();
}

bool SpiHw2Stub::transmitAsync(fl::span<const uint8_t> buffer) {
    if (!mInitialized) {
        return false;
    }

    if (buffer.empty()) {
        return true;  // Nothing to transmit
    }

    // Capture data for inspection
    mLastBuffer.clear();
    mLastBuffer.reserve(buffer.size());
    for (size_t i = 0; i < buffer.size(); ++i) {
        mLastBuffer.push_back(buffer[i]);
    }

    mTransmitCount++;
    mBusy = true;

    return true;
}

bool SpiHw2Stub::waitComplete(uint32_t timeout_ms) {
    (void)timeout_ms;  // Unused in stub
    mBusy = false;
    return true;
}

bool SpiHw2Stub::isBusy() const {
    return mBusy;
}

bool SpiHw2Stub::isInitialized() const {
    return mInitialized;
}

int SpiHw2Stub::getBusId() const {
    return mBusId;
}

const char* SpiHw2Stub::getName() const {
    return mName;
}

const fl::vector<uint8_t>& SpiHw2Stub::getLastTransmission() const {
    return mLastBuffer;
}

uint32_t SpiHw2Stub::getTransmissionCount() const {
    return mTransmitCount;
}

uint32_t SpiHw2Stub::getClockSpeed() const {
    return mClockSpeed;
}

bool SpiHw2Stub::isTransmissionActive() const {
    return mBusy;
}

void SpiHw2Stub::reset() {
    mLastBuffer.clear();
    mTransmitCount = 0;
    mBusy = false;
}

fl::vector<fl::vector<uint8_t>> SpiHw2Stub::extractLanes(uint8_t num_lanes, size_t bytes_per_lane) const {
    fl::vector<fl::vector<uint8_t>> lanes(num_lanes);

    if (num_lanes != 2) {
        return lanes;  // Only support 2 lanes for dual-SPI
    }

    // Initialize lane vectors
    for (uint8_t i = 0; i < num_lanes; i++) {
        lanes[i].resize(bytes_per_lane);
    }

    // De-interleave: reverse the interleaving process
    // Each 2 input bytes produce 1 byte per lane
    for (size_t byte_idx = 0; byte_idx < bytes_per_lane; byte_idx++) {
        size_t input_offset = byte_idx * 2;
        if (input_offset + 1 >= mLastBuffer.size()) {
            break;  // Incomplete data
        }

        // Extract nibbles from interleaved data
        uint8_t byte0 = mLastBuffer[input_offset];
        uint8_t byte1 = mLastBuffer[input_offset + 1];

        // Reconstruct lane bytes
        // byte0 format: [b_hi a_hi] (4 bits each)
        // byte1 format: [b_lo a_lo] (4 bits each)
        lanes[0][byte_idx] = ((byte0 & 0x0F) << 4) | (byte1 & 0x0F);
        lanes[1][byte_idx] = (byte0 & 0xF0) | ((byte1 >> 4) & 0x0F);
    }

    return lanes;
}

// ============================================================================
// Factory Implementation (Weak Linkage for Testing)
// ============================================================================

fl::vector<SpiHw2*> SpiHw2::createInstances() {
    fl::vector<SpiHw2*> controllers;

    // Create two mock controllers for testing
    static SpiHw2Stub controller0(0, "MockDual0");
    static SpiHw2Stub controller1(1, "MockDual1");

    controllers.push_back(&controller0);
    controllers.push_back(&controller1);

    return controllers;
}

}  // namespace fl

#endif  // FASTLED_TESTING
