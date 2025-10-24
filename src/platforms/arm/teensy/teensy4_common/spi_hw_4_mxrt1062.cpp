/// @file spi_hw_4_mxrt1062.cpp
/// @brief Teensy 4.x (IMXRT1062) implementation of 4-lane (Quad) SPI
///
/// This file provides the SpiHw4MXRT1062 class and factory for Teensy 4.x platforms.
/// All class definition and implementation is contained in this single file.
///
/// The IMXRT1062's LPSPI peripheral supports quad-mode transfers by configuring
/// the WIDTH field in the transmit command register (TCR).
///
/// IMPORTANT PIN REQUIREMENT:
/// Quad-SPI requires data2 and data3 pins which correspond to PCS2 and PCS3
/// signals. These pins are NOT exposed on standard Teensy 4.0/4.1 boards but
/// can be accessed via:
/// - Custom PCB designs
/// - Breakout adapters that expose the full pin set
/// - Advanced users who can solder to the processor pads
///
/// Pin mappings for quad mode:
/// - data0 (D0): MOSI pin (SDO)
/// - data1 (D1): MISO pin (SDI)
/// - data2 (D2): PCS2 / WP pin
/// - data3 (D3): PCS3 / HOLD pin

#if defined(__IMXRT1062__) && defined(ARM_HARDWARE_SPI)

#include "platforms/shared/spi_hw_4.h"
#include "fl/warn.h"
#include <SPI.h>
#include <imxrt.h>
#include <cstring> // ok include

namespace fl {

// ============================================================================
// SpiHw4MXRT1062 Class Definition
// ============================================================================

/// Teensy 4.x hardware for 4-lane (Quad) SPI transmission
/// Implements SpiHw4 interface for LPSPI peripheral (1-4 lanes)
class SpiHw4MXRT1062 : public SpiHw4 {
public:
    explicit SpiHw4MXRT1062(int bus_id = -1, const char* name = "Unknown");
    ~SpiHw4MXRT1062();

    bool begin(const SpiHw4::Config& config) override;
    void end() override;
    bool transmit(fl::span<const uint8_t> buffer, TransmitMode mode = TransmitMode::ASYNC) override;
    bool waitComplete(uint32_t timeout_ms = UINT32_MAX) override;
    bool isBusy() const override;
    bool isInitialized() const override;
    int getBusId() const override;
    const char* getName() const override;

private:
    void cleanup();
    IMXRT_LPSPI_t* getPort();

    int mBusId;
    const char* mName;
    SPIClass* mSPI;
    bool mTransactionActive;
    bool mInitialized;
    uint32_t mClockSpeed;
    uint8_t mActiveLanes;  // Number of active data lanes (1-4)

    // Pin configuration
    int8_t mClockPin;
    int8_t mData0Pin;
    int8_t mData1Pin;
    int8_t mData2Pin;
    int8_t mData3Pin;

    SpiHw4MXRT1062(const SpiHw4MXRT1062&) = delete;
    SpiHw4MXRT1062& operator=(const SpiHw4MXRT1062&) = delete;
};

// ============================================================================
// SpiHw4MXRT1062 Implementation
// ============================================================================

SpiHw4MXRT1062::SpiHw4MXRT1062(int bus_id, const char* name)
    : mBusId(bus_id)
    , mName(name)
    , mSPI(nullptr)
    , mTransactionActive(false)
    , mInitialized(false)
    , mClockSpeed(20000000)  // Default 20 MHz
    , mActiveLanes(1)
    , mClockPin(-1)
    , mData0Pin(-1)
    , mData1Pin(-1)
    , mData2Pin(-1)
    , mData3Pin(-1)
{
}

SpiHw4MXRT1062::~SpiHw4MXRT1062() {
    cleanup();
}

IMXRT_LPSPI_t* SpiHw4MXRT1062::getPort() {
    // Map bus_id to LPSPI port
    // SPI  (index 0) -> LPSPI4
    // SPI1 (index 1) -> LPSPI3
    // SPI2 (index 2) -> LPSPI1
    switch (mBusId) {
        case 0:
            return &IMXRT_LPSPI4_S;
        case 1:
            return &IMXRT_LPSPI3_S;
        case 2:
            return &IMXRT_LPSPI1_S;
        default:
            return nullptr;
    }
}

bool SpiHw4MXRT1062::begin(const SpiHw4::Config& config) {
    if (mInitialized) {
        return true;  // Already initialized
    }

    // Validate bus_num against mBusId if driver has pre-assigned ID
    if (mBusId != -1 && config.bus_num != static_cast<uint8_t>(mBusId)) {
        FL_WARN("SpiHw4MXRT1062: Bus mismatch - expected " << mBusId << ", got " << static_cast<int>(config.bus_num));
        return false;
    }

    // Select SPI object based on bus_num
    uint8_t bus_num = (mBusId != -1) ? static_cast<uint8_t>(mBusId) : config.bus_num;
    switch (bus_num) {
        case 0:
            mSPI = &SPI;
            mBusId = 0;
            break;
        case 1:
            mSPI = &SPI1;
            mBusId = 1;
            break;
        case 2:
            mSPI = &SPI2;
            mBusId = 2;
            break;
        default:
            FL_WARN("SpiHw4MXRT1062: Invalid bus number " << static_cast<int>(bus_num));
            return false;
    }

    // Count active data pins to determine SPI mode (1-4 lanes)
    mActiveLanes = 1;  // data0 always present
    if (config.data1_pin >= 0) mActiveLanes++;
    if (config.data2_pin >= 0) mActiveLanes++;
    if (config.data3_pin >= 0) mActiveLanes++;

    // Store configuration
    mClockSpeed = config.clock_speed_hz;
    mClockPin = config.clock_pin;
    mData0Pin = config.data0_pin;
    mData1Pin = config.data1_pin;
    mData2Pin = config.data2_pin;
    mData3Pin = config.data3_pin;

    // Warn if quad mode requested but pins aren't available on standard boards
    if (mActiveLanes == 4) {
        FL_WARN("SpiHw4MXRT1062: Quad-SPI mode enabled with 4 lanes");
        FL_WARN("  Note: data2/data3 pins (PCS2/PCS3) are not exposed on standard Teensy 4.0/4.1 boards");
        FL_WARN("  This requires custom hardware or breakout adapters");
    }

    // Initialize the SPI peripheral
    mSPI->begin();

    FL_WARN("SpiHw4MXRT1062: Initialized on bus " << mBusId
            << " with " << static_cast<int>(mActiveLanes) << " lanes");

    mInitialized = true;
    mTransactionActive = false;

    return true;
}

void SpiHw4MXRT1062::end() {
    cleanup();
}

bool SpiHw4MXRT1062::transmit(fl::span<const uint8_t> buffer, TransmitMode mode) {
    if (!mInitialized || !mSPI) {
        return false;
    }

    // Wait for previous transaction if still active
    if (mTransactionActive) {
        waitComplete();
    }

    if (buffer.empty()) {
        return true;  // Nothing to transmit
    }

    // Begin SPI transaction with configured clock speed
    mSPI->beginTransaction(SPISettings(mClockSpeed, MSBFIRST, SPI_MODE0));

    IMXRT_LPSPI_t* port = getPort();
    if (!port) {
        mSPI->endTransaction();
        return false;
    }

    // Save current TCR
    uint32_t old_tcr = port->TCR;

    // Configure for appropriate mode based on active lanes
    // TCR.WIDTH field is bits 17:16
    // 0b00 = 1-bit (standard SPI)
    // 0b01 = 2-bit (dual SPI)
    // 0b10 = 4-bit (quad SPI)
    uint32_t width_bits = 0;
    if (mActiveLanes >= 4) {
        width_bits = 0x2;  // Quad mode
    } else if (mActiveLanes >= 2) {
        width_bits = 0x1;  // Dual mode
    } else {
        width_bits = 0x0;  // Standard SPI
    }

    uint32_t new_tcr = (old_tcr & ~(0x3 << 16)) | (width_bits << 16);
    port->TCR = new_tcr;

    // Transmit data
    // In quad mode, each byte is transmitted with 2 bits per data line
    // The transposer has already prepared the data in interleaved format
    for (size_t i = 0; i < buffer.size(); ++i) {
        // Wait for transmit FIFO to have space
        while (!(port->SR & LPSPI_SR_TDF)) ;

        port->TDR = buffer[i];
    }

    // Wait for transmission to complete
    while (port->SR & LPSPI_SR_MBF) ;  // Module Busy Flag

    // Restore original TCR
    port->TCR = old_tcr;

    mSPI->endTransaction();
    mTransactionActive = false;

    return true;
}

bool SpiHw4MXRT1062::waitComplete(uint32_t timeout_ms) {
    if (!mTransactionActive) {
        return true;  // Nothing to wait for
    }

    // For synchronous implementation, transmission is already complete
    mTransactionActive = false;
    return true;
}

bool SpiHw4MXRT1062::isBusy() const {
    return mTransactionActive;
}

bool SpiHw4MXRT1062::isInitialized() const {
    return mInitialized;
}

int SpiHw4MXRT1062::getBusId() const {
    return mBusId;
}

const char* SpiHw4MXRT1062::getName() const {
    return mName;
}

void SpiHw4MXRT1062::cleanup() {
    if (mInitialized && mSPI) {
        // Wait for any pending transmission
        if (mTransactionActive) {
            waitComplete();
        }

        mSPI->end();
        mSPI = nullptr;
        mInitialized = false;
    }
}

// ============================================================================
// Factory Implementation
// ============================================================================

/// Teensy 4.x factory override - returns available 4-lane SPI bus instances
/// Strong definition overrides weak default
fl::vector<SpiHw4*> SpiHw4::createInstances() {
    fl::vector<SpiHw4*> controllers;

    // Teensy 4.x has 3 LPSPI peripherals available
    // SPI (bus 0), SPI1 (bus 1), SPI2 (bus 2)
    static SpiHw4MXRT1062 controller0(0, "SPI");
    static SpiHw4MXRT1062 controller1(1, "SPI1");
    static SpiHw4MXRT1062 controller2(2, "SPI2");

    controllers.push_back(&controller0);
    controllers.push_back(&controller1);
    controllers.push_back(&controller2);

    return controllers;
}

}  // namespace fl

#endif  // __IMXRT1062__ && ARM_HARDWARE_SPI
