#ifdef FASTLED_STUB_IMPL

#define FASTLED_INTERNAL
#include "fl/fastled.h"

#include "platforms/stub/fastspi_stub_generic.h"

extern uint8_t get_brightness();

namespace fl {

StubSPIOutput::StubSPIOutput() { EngineEvents::addListener(this); }
StubSPIOutput::~StubSPIOutput() { EngineEvents::removeListener(this); }

void StubSPIOutput::onEndShowLeds() {
    // Simply push the captured RGB data through the tracker
    // The mRgb buffer is populated by writeByte() calls during LED transmission
    mTracker.update(fl::span<const uint8_t>(mRgb.data(), mRgb.size()));
}

void StubSPIOutput::select() { mRgb.clear(); }

void StubSPIOutput::init() { mRgb.clear(); }

void StubSPIOutput::waitFully() {}

void StubSPIOutput::release() {}

void StubSPIOutput::writeByte(uint8_t byte) {
    mRgb.push_back(byte);
}

void StubSPIOutput::writeWord(uint16_t word) {
    writeByte(word >> 8);
    writeByte(word & 0xFF);
}

} // namespace fl

#endif // FASTLED_STUB_IMPL
