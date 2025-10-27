#pragma once

#define FASTLED_CLOCKLESS_STUB_DEFINED

#include "eorder.h"
#include "fl/unused.h"
#include "fl/vector.h"
#include "fl/singleton.h"
#include "fl/chipsets/timing_traits.h"
#include "pixel_iterator.h"
#include "platforms/shared/active_strip_data/active_strip_data.h"
#include "platforms/shared/strip_id_map/strip_id_map.h"
#include "platforms/shared/active_strip_tracker/active_strip_tracker.h"

namespace fl {
#define FASTLED_HAS_CLOCKLESS 1

// Template for type-based timing API (TIMING_WS2812_800KHZ, etc.)
// This is the primary template with LED capture support
template <int DATA_PIN, typename TIMING, EOrder RGB_ORDER = RGB, int XTRA0 = 0, bool FLIP = false, int WAIT_TIME = 0>
class ClocklessController : public CPixelLEDController<RGB_ORDER> {
public:
	virtual void init() { }

protected:
	virtual void showPixels(PixelController<RGB_ORDER> & pixels) {
		// Capture LED data for simulation/testing
		mRgb.clear();
		PixelController<RGB> pixels_rgb = pixels; // Converts to RGB pixels
		pixels_rgb.disableColorAdjustment();
		auto iterator = pixels_rgb.as_iterator(RgbwInvalid());
		mRgb.resize(iterator.size() * 3);
		auto out_iterator = mRgb.data();
		while (iterator.has(1)) {
			uint8_t r, g, b;
			iterator.loadAndScaleRGB(&r, &g, &b);
			out_iterator[0] = r;
			out_iterator[1] = g;
			out_iterator[2] = b;
			out_iterator += 3;
			iterator.advanceData();
		}
		mTracker.update(fl::span<const uint8_t>(mRgb.data(), mRgb.size()));
	}

private:
	ActiveStripTracker mTracker;
	fl::vector<uint8_t> mRgb;
};

// Adapter struct that accepts timing-like objects via duck typing
// TIMING_LIKE can be ChipsetTiming& or any type that quacks like a timing object
// Preserves all platform-specific parameters: XTRA0, FLIP, WAIT_TIME
template <int DATA_PIN, typename TIMING_LIKE, EOrder RGB_ORDER = RGB, int XTRA0 = 0, bool FLIP = false, int WAIT_TIME = 0>
struct ClocklessControllerAdapter : public CPixelLEDController<RGB_ORDER> {
	virtual void init() { }

protected:
	virtual void showPixels(PixelController<RGB_ORDER> & pixels) {
		// Capture LED data for simulation/testing
		mRgb.clear();
		PixelController<RGB> pixels_rgb = pixels; // Converts to RGB pixels
		pixels_rgb.disableColorAdjustment();
		auto iterator = pixels_rgb.as_iterator(RgbwInvalid());
		mRgb.resize(iterator.size() * 3);
		auto out_iterator = mRgb.data();
		while (iterator.has(1)) {
			uint8_t r, g, b;
			iterator.loadAndScaleRGB(&r, &g, &b);
			out_iterator[0] = r;
			out_iterator[1] = g;
			out_iterator[2] = b;
			out_iterator += 3;
			iterator.advanceData();
		}
		mTracker.update(fl::span<const uint8_t>(mRgb.data(), mRgb.size()));
	}

private:
	ActiveStripTracker mTracker;
	fl::vector<uint8_t> mRgb;
};

// ClocklessBlockController for type-based timing (TIMING_WS2812_800KHZ, etc.)
// This is used when chipsets.h instantiates controllers with timing structs
// This overrides the generic ClocklessBlockController to add LED capture support
template <int DATA_PIN, typename TIMING, EOrder RGB_ORDER = RGB, int XTRA0 = 0, bool FLIP = false, int WAIT_TIME = 0>
class ClocklessBlockController : public CPixelLEDController<RGB_ORDER> {
public:
	virtual void init() { }

protected:
	virtual void showPixels(PixelController<RGB_ORDER> & pixels) {
		// Capture LED data for simulation/testing
		mRgb.clear();
		PixelController<RGB> pixels_rgb = pixels; // Converts to RGB pixels
		pixels_rgb.disableColorAdjustment();
		auto iterator = pixels_rgb.as_iterator(RgbwInvalid());
		mRgb.resize(iterator.size() * 3);
		auto out_iterator = mRgb.data();
		while (iterator.has(1)) {
			uint8_t r, g, b;
			iterator.loadAndScaleRGB(&r, &g, &b);
			out_iterator[0] = r;
			out_iterator[1] = g;
			out_iterator[2] = b;
			out_iterator += 3;
			iterator.advanceData();
		}
		mTracker.update(fl::span<const uint8_t>(mRgb.data(), mRgb.size()));
	}

private:
	ActiveStripTracker mTracker;
	fl::vector<uint8_t> mRgb;
};
}  // namespace fl
