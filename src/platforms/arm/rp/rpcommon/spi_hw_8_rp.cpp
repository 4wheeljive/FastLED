/// @file spi_hw_8_rp.cpp
/// @brief RP2040/RP2350 implementation of Octal-SPI using PIO
///
/// This file provides the SpiHw8RP2040 class and factory for all Raspberry Pi Pico platforms.
/// Uses PIO (Programmable I/O) to implement true octal-lane SPI with DMA support.
/// All class definition and implementation is contained in this single file.

#if defined(PICO_RP2040) || defined(PICO_RP2350) || defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_RP2350)

#include "platforms/shared/spi_hw_8.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "pio_asm.h"
#include "fl/warn.h"
#include <cstring> // ok include
#include "fl/memfill.h"

namespace fl {

// ============================================================================
// PIO Program for Octal-SPI
// ============================================================================

/// PIO program for octal-lane SPI transmission
///
/// The program outputs synchronized data on 8 data pins (D0-D7) with a clock signal.
/// Data is fed from DMA into the PIO TX FIFO as 32-bit words.
///
/// Pin mapping:
/// - Base pin: D0 (data bit 0)
/// - Base+1:   D1 (data bit 1)
/// - Base+2:   D2 (data bit 2)
/// - Base+3:   D3 (data bit 3)
/// - Base+4:   D4 (data bit 4)
/// - Base+5:   D5 (data bit 5)
/// - Base+6:   D6 (data bit 6)
/// - Base+7:   D7 (data bit 7)
/// - Sideset:  SCK (clock)
///
/// Data format:
/// Each 32-bit word contains 4 bytes to transmit, split across 8 lanes:
/// - Bits are output 8 at a time (one per lane) on each clock cycle
/// - 4 clock cycles per 32-bit word (4 bits × 8 lanes = 32 bits throughput)
///
/// Timing:
/// - Clock high on output
/// - Clock low on output
/// - Repeat for all 4 bits in the word

#define SPI_OCTAL_PIO_SIDESET_COUNT 1  // Clock on sideset pin

static inline int add_spi_octal_pio_program(PIO pio) {
    // PIO program for octal-SPI:
    // Loop 4 times (output 4 bits × 8 lanes = 32 bits total per word)
    //   out pins, 8 side 1  ; Output 8 bits (D0-D7) with clock high
    //   nop         side 0  ; Clock low

    pio_instr spi_octal_pio_instr[] = {
        // Set loop counter Y = 3 (for 4 iterations)
        // This runs once at start via exec from C code, not part of looped program

        // wrap_target (address 0)
        // out pins, 8 side 1  ; Output 8 bits to pins D0-D7 with clock high
        (pio_instr)(PIO_INSTR_OUT | PIO_OUT_DST_PINS | PIO_OUT_CNT(8) | PIO_SIDESET(1, SPI_OCTAL_PIO_SIDESET_COUNT)),
        // jmp y-- side 0      ; Decrement Y, loop if Y != 0, clock low
        (pio_instr)(PIO_INSTR_JMP | PIO_JMP_CND_Y_DEC | PIO_JMP_ADR(0) | PIO_SIDESET(0, SPI_OCTAL_PIO_SIDESET_COUNT)),
        // set y, 3 side 0     ; Reset counter for next word, clock low
        (pio_instr)(PIO_INSTR_SET | PIO_SET_DST_Y | PIO_SET_DATA(3) | PIO_SIDESET(0, SPI_OCTAL_PIO_SIDESET_COUNT)),
        // wrap (back to address 0)
    };

    struct pio_program spi_octal_pio_program = {
        .instructions = spi_octal_pio_instr,
        .length = sizeof(spi_octal_pio_instr) / sizeof(spi_octal_pio_instr[0]),
        .origin = -1,
    };

    if (!pio_can_add_program(pio, &spi_octal_pio_program))
        return -1;

    return (int)pio_add_program(pio, &spi_octal_pio_program);
}

static inline pio_sm_config spi_octal_pio_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + 0, offset + 2);  // Wrap from instruction 2 back to 0
    sm_config_set_sideset(&c, SPI_OCTAL_PIO_SIDESET_COUNT, false, false);
    return c;
}

// ============================================================================
// SpiHw8RP2040 Class Definition
// ============================================================================

/// RP2040/RP2350 hardware driver for Octal-SPI DMA transmission using PIO
///
/// Implements SpiHw8 interface for Raspberry Pi Pico platforms using:
/// - PIO (Programmable I/O) for synchronized octal-lane output
/// - DMA for non-blocking asynchronous transfers
/// - Configurable clock frequency up to 25 MHz
/// - Full 8-bit parallel output (one byte per clock cycle)
///
/// @note Each instance allocates one PIO state machine and one DMA channel
/// @note All 8 data pins must be consecutive GPIO numbers (D0-D7)
/// @note Highest throughput mode - outputs full bytes in parallel
class SpiHw8RP2040 : public SpiHw8 {
public:
    /// @brief Construct a new SpiHw8RP2040 controller
    /// @param bus_id Logical bus identifier (0 or 1)
    /// @param name Human-readable name for this controller
    explicit SpiHw8RP2040(int bus_id = -1, const char* name = "Unknown");

    /// @brief Destroy the controller and release all resources
    ~SpiHw8RP2040();

    /// @brief Initialize the SPI controller with specified configuration
    /// @param config Configuration including pins, clock speed, and bus number
    /// @return true if initialization successful, false on error
    /// @note Validates that all 8 data pins are consecutive
    /// @note Allocates PIO/DMA resources
    bool begin(const SpiHw8::Config& config) override;

    /// @brief Deinitialize the controller and release resources
    void end() override;

    /// @brief Start non-blocking transmission of data buffer
    /// @param buffer Data to transmit (packed into octal-lane format internally)
    /// @return true if transfer started successfully, false on error
    /// @note Waits for previous transaction to complete if still active
    /// @note Returns immediately - use waitComplete() to block until done
    /// @note Each byte is transmitted as one full octal word (8 bits parallel)
    bool transmitAsync(fl::span<const uint8_t> buffer) override;

    /// @brief Wait for current transmission to complete
    /// @param timeout_ms Maximum time to wait in milliseconds (UINT32_MAX = infinite)
    /// @return true if transmission completed, false on timeout
    bool waitComplete(uint32_t timeout_ms = UINT32_MAX) override;

    /// @brief Check if transmission is currently in progress
    /// @return true if busy, false if idle
    bool isBusy() const override;

    /// @brief Check if controller has been initialized
    /// @return true if initialized, false otherwise
    bool isInitialized() const override;

    /// @brief Get the bus identifier for this controller
    /// @return Bus ID (0 or 1)
    int getBusId() const override;

    /// @brief Get the human-readable name for this controller
    /// @return Controller name string
    const char* getName() const override;

private:
    /// @brief Release all allocated resources (PIO, DMA, buffers)
    void cleanup();

    /// @brief Allocate or resize internal DMA buffer
    /// @param required_size Size needed in bytes
    /// @return true if buffer allocated successfully
    bool allocateDMABuffer(size_t required_size);

    int mBusId;  ///< Logical bus identifier
    const char* mName;  ///< Controller name

    // PIO resources
    PIO mPIO;
    int mStateMachine;
    int mPIOOffset;

    // DMA resources
    int mDMAChannel;
    void* mDMABuffer;
    size_t mDMABufferSize;

    // State
    bool mTransactionActive;
    bool mInitialized;

    // Configuration
    uint8_t mClockPin;
    uint8_t mDataPins[8];  // All 8 data pins

    SpiHw8RP2040(const SpiHw8RP2040&) = delete;
    SpiHw8RP2040& operator=(const SpiHw8RP2040&) = delete;
};

// ============================================================================
// SpiHw8RP2040 Implementation
// ============================================================================

SpiHw8RP2040::SpiHw8RP2040(int bus_id, const char* name)
    : mBusId(bus_id)
    , mName(name)
    , mPIO(nullptr)
    , mStateMachine(-1)
    , mPIOOffset(-1)
    , mDMAChannel(-1)
    , mDMABuffer(nullptr)
    , mDMABufferSize(0)
    , mTransactionActive(false)
    , mInitialized(false)
    , mClockPin(0) {
    // Initialize all data pins to 0
    for (int i = 0; i < 8; ++i) {
        mDataPins[i] = 0;
    }
}

SpiHw8RP2040::~SpiHw8RP2040() {
    cleanup();
}

bool SpiHw8RP2040::begin(const SpiHw8::Config& config) {
    if (mInitialized) {
        return true;  // Already initialized
    }

    // Validate bus_num against mBusId if driver has pre-assigned ID
    if (mBusId != -1 && config.bus_num != static_cast<uint8_t>(mBusId)) {
        FL_WARN("SpiHw8RP2040: Bus ID mismatch");
        return false;
    }

    // Validate pin assignments - all 8 data pins and clock must be set
    if (config.clock_pin < 0 || config.data0_pin < 0 || config.data1_pin < 0 ||
        config.data2_pin < 0 || config.data3_pin < 0 || config.data4_pin < 0 ||
        config.data5_pin < 0 || config.data6_pin < 0 || config.data7_pin < 0) {
        FL_WARN("SpiHw8RP2040: Invalid pin configuration (all 8 data pins + clock required)");
        return false;
    }

    // Store pin configuration
    mClockPin = config.clock_pin;
    mDataPins[0] = config.data0_pin;
    mDataPins[1] = config.data1_pin;
    mDataPins[2] = config.data2_pin;
    mDataPins[3] = config.data3_pin;
    mDataPins[4] = config.data4_pin;
    mDataPins[5] = config.data5_pin;
    mDataPins[6] = config.data6_pin;
    mDataPins[7] = config.data7_pin;

    // Validate that all 8 data pins are consecutive
    for (int i = 1; i < 8; ++i) {
        if (mDataPins[i] != mDataPins[0] + i) {
            FL_WARN("SpiHw8RP2040: Data pins must be consecutive (D0, D0+1, ..., D0+7)");
            return false;
        }
    }

    // Find available PIO instance and state machine
#if defined(PICO_RP2040)
    const PIO pios[NUM_PIOS] = { pio0, pio1 };
#elif defined(PICO_RP2350) || defined(ARDUINO_ARCH_RP2350)
    const PIO pios[NUM_PIOS] = { pio0, pio1, pio2 };
#endif

    mPIO = nullptr;
    mStateMachine = -1;
    mPIOOffset = -1;

    for (unsigned int i = 0; i < NUM_PIOS; i++) {
        PIO pio = pios[i];
        int sm = pio_claim_unused_sm(pio, false);
        if (sm == -1) continue;

        int offset = add_spi_octal_pio_program(pio);
        if (offset == -1) {
            pio_sm_unclaim(pio, sm);
            continue;
        }

        mPIO = pio;
        mStateMachine = sm;
        mPIOOffset = offset;
        break;
    }

    if (mPIO == nullptr || mStateMachine == -1 || mPIOOffset == -1) {
        FL_WARN("SpiHw8RP2040: No available PIO resources");
        return false;
    }

    // Claim DMA channel
    mDMAChannel = dma_claim_unused_channel(false);
    if (mDMAChannel == -1) {
        FL_WARN("SpiHw8RP2040: No available DMA channel");
        pio_sm_unclaim(mPIO, mStateMachine);
        return false;
    }

    // Configure GPIO pins for PIO - all 8 data pins
    for (int i = 0; i < 8; ++i) {
        pio_gpio_init(mPIO, mDataPins[i]);
    }
    pio_sm_set_consecutive_pindirs(mPIO, mStateMachine, mDataPins[0], 8, true);  // D0-D7 as outputs

    // Initialize clock pin
    pio_gpio_init(mPIO, mClockPin);
    pio_sm_set_consecutive_pindirs(mPIO, mStateMachine, mClockPin, 1, true);  // CLK as output

    // Configure PIO state machine
    pio_sm_config c = spi_octal_pio_program_get_default_config(mPIOOffset);
    sm_config_set_out_pins(&c, mDataPins[0], 8);  // 8 output pins (D0-D7)
    sm_config_set_sideset_pins(&c, mClockPin);    // Clock on sideset
    sm_config_set_out_shift(&c, false, true, 32); // Shift left, autopull at 32 bits

    // Calculate clock divider for requested frequency
    // PIO clock runs at 2x SPI clock (high + low cycles)
    float div = clock_get_hz(clk_sys) / (2.0f * config.clock_speed_hz);
    sm_config_set_clkdiv(&c, div);

    // Initialize and start state machine
    pio_sm_init(mPIO, mStateMachine, mPIOOffset, &c);

    // Initialize Y register to 3 (for 4 iterations per word)
    pio_sm_exec(mPIO, mStateMachine,
                (pio_instr)(PIO_INSTR_SET | PIO_SET_DST_Y | PIO_SET_DATA(3)));

    pio_sm_set_enabled(mPIO, mStateMachine, true);

    // Configure DMA channel
    dma_channel_config dma_config = dma_channel_get_default_config(mDMAChannel);
    channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_32);  // 32-bit transfers
    channel_config_set_dreq(&dma_config, pio_get_dreq(mPIO, mStateMachine, true));  // TX DREQ
    channel_config_set_read_increment(&dma_config, true);   // Increment read address
    channel_config_set_write_increment(&dma_config, false); // Fixed write to PIO FIFO

    dma_channel_configure(mDMAChannel,
                         &dma_config,
                         &mPIO->txf[mStateMachine],  // Write to PIO TX FIFO
                         nullptr,  // Source set in transmitAsync
                         0,        // Count set in transmitAsync
                         false);   // Don't start yet

    mInitialized = true;
    mTransactionActive = false;

    return true;
}

void SpiHw8RP2040::end() {
    cleanup();
}

bool SpiHw8RP2040::allocateDMABuffer(size_t required_size) {
    if (mDMABufferSize >= required_size) {
        return true;  // Buffer is already large enough
    }

    // Free old buffer if exists
    if (mDMABuffer != nullptr) {
        free(mDMABuffer);
        mDMABuffer = nullptr;
        mDMABufferSize = 0;
    }

    // Allocate new buffer (32-bit aligned for DMA)
    mDMABuffer = malloc(required_size);
    if (mDMABuffer == nullptr) {
        FL_WARN("SpiHw8RP2040: Failed to allocate DMA buffer");
        return false;
    }

    mDMABufferSize = required_size;
    return true;
}

bool SpiHw8RP2040::transmitAsync(fl::span<const uint8_t> buffer) {
    if (!mInitialized) {
        return false;
    }

    // Wait for previous transaction if still active
    if (mTransactionActive) {
        waitComplete();
    }

    if (buffer.empty()) {
        return true;  // Nothing to transmit
    }

    // For octal-SPI: Each byte is transmitted directly as one 8-bit word
    // Each 32-bit DMA word contains 4 bytes that will be output sequentially
    // Clock cycle 0: byte0[7:0] on D0-D7
    // Clock cycle 1: byte1[7:0] on D0-D7
    // Clock cycle 2: byte2[7:0] on D0-D7
    // Clock cycle 3: byte3[7:0] on D0-D7

    size_t byte_count = buffer.size();

    // Calculate words needed: ceil(byte_count / 4) since each word holds 4 bytes
    size_t word_count = (byte_count + 3) / 4;
    size_t buffer_size_bytes = word_count * 4;  // 4 bytes per 32-bit word

    if (!allocateDMABuffer(buffer_size_bytes)) {
        return false;
    }

    // Pack bytes into octal-lane format
    uint32_t* word_buffer = (uint32_t*)mDMABuffer;
    fl::memfill(word_buffer, 0, buffer_size_bytes);  // Clear buffer

    for (size_t i = 0; i < byte_count; i += 4) {
        uint8_t byte0 = buffer[i];
        uint8_t byte1 = (i + 1 < byte_count) ? buffer[i + 1] : 0;
        uint8_t byte2 = (i + 2 < byte_count) ? buffer[i + 2] : 0;
        uint8_t byte3 = (i + 3 < byte_count) ? buffer[i + 3] : 0;

        // Pack 4 bytes into one 32-bit word, MSB first
        // The PIO will output these 4 bytes sequentially, one per clock cycle
        uint32_t word = (static_cast<uint32_t>(byte0) << 24) |
                        (static_cast<uint32_t>(byte1) << 16) |
                        (static_cast<uint32_t>(byte2) << 8) |
                        static_cast<uint32_t>(byte3);

        word_buffer[i / 4] = word;
    }

    // Start DMA transfer
    dma_channel_set_read_addr(mDMAChannel, mDMABuffer, false);
    dma_channel_set_trans_count(mDMAChannel, word_count, true);  // Start transfer

    mTransactionActive = true;
    return true;
}

bool SpiHw8RP2040::waitComplete(uint32_t timeout_ms) {
    if (!mTransactionActive) {
        return true;  // Nothing to wait for
    }

    // Simple polling implementation (timeout support can be added later)
    if (timeout_ms == UINT32_MAX) {
        // Infinite timeout - just wait
        dma_channel_wait_for_finish_blocking(mDMAChannel);
    } else {
        // Timeout-based polling
        // Note: This is a simple implementation, could be improved with timestamp checking
        while (dma_channel_is_busy(mDMAChannel)) {
            // TODO: Add actual timeout checking with timestamp
            // For now, just poll (safe but not ideal)
        }
    }

    mTransactionActive = false;
    return true;
}

bool SpiHw8RP2040::isBusy() const {
    if (!mInitialized) {
        return false;
    }
    return mTransactionActive || dma_channel_is_busy(mDMAChannel);
}

bool SpiHw8RP2040::isInitialized() const {
    return mInitialized;
}

int SpiHw8RP2040::getBusId() const {
    return mBusId;
}

const char* SpiHw8RP2040::getName() const {
    return mName;
}

void SpiHw8RP2040::cleanup() {
    if (mInitialized) {
        // Wait for any pending transmission
        if (mTransactionActive) {
            waitComplete();
        }

        // Disable and unclaim PIO state machine
        if (mPIO != nullptr && mStateMachine != -1) {
            pio_sm_set_enabled(mPIO, mStateMachine, false);
            pio_sm_unclaim(mPIO, mStateMachine);
        }

        // Release DMA channel
        if (mDMAChannel != -1) {
            dma_channel_unclaim(mDMAChannel);
            mDMAChannel = -1;
        }

        // Free DMA buffer
        if (mDMABuffer != nullptr) {
            free(mDMABuffer);
            mDMABuffer = nullptr;
            mDMABufferSize = 0;
        }

        mInitialized = false;
    }
}

// ============================================================================
// Factory Implementation
// ============================================================================

/// RP2040/RP2350 factory override - returns available SPI bus instances
/// Strong definition overrides weak default
fl::vector<SpiHw8*> SpiHw8::createInstances() {
    fl::vector<SpiHw8*> controllers;

    // Create 2 logical SPI buses (each uses separate PIO state machine)
    static SpiHw8RP2040 controller0(0, "SPI0");
    controllers.push_back(&controller0);

    static SpiHw8RP2040 controller1(1, "SPI1");
    controllers.push_back(&controller1);

    return controllers;
}

}  // namespace fl

#endif  // PICO_RP2040 || PICO_RP2350 || ARDUINO_ARCH_RP2040 || ARDUINO_ARCH_RP2350
