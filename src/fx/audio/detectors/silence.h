#pragma once

#include "fl/audio/audio_detector.h"
#include "fl/audio/audio_context.h"
#include "fl/function.h"
#include "fl/vector.h"

namespace fl {

class SilenceDetector : public AudioDetector {
public:
    SilenceDetector();
    ~SilenceDetector() override;

    void update(shared_ptr<AudioContext> context) override;
    bool needsFFT() const override { return false; }  // Uses RMS from AudioSample
    const char* getName() const override { return "SilenceDetector"; }
    void reset() override;

    // Callbacks
    function<void(bool silent)> onSilenceChange;
    function<void()> onSilenceStart;
    function<void()> onSilenceEnd;
    function<void(u32 durationMs)> onSilenceDuration;

    // State access
    bool isSilent() const { return mIsSilent; }
    u32 getSilenceDuration() const;
    float getSilenceThreshold() const { return mSilenceThreshold; }
    float getCurrentRMS() const { return mCurrentRMS; }

    // Configuration
    void setSilenceThreshold(float threshold) { mSilenceThreshold = threshold; }
    void setMinSilenceDuration(u32 durationMs) { mMinSilenceDuration = durationMs; }
    void setMaxSilenceDuration(u32 durationMs) { mMaxSilenceDuration = durationMs; }
    void setHysteresis(float hysteresis) { mHysteresis = hysteresis; }

private:
    bool mIsSilent;
    bool mPreviousSilent;
    float mCurrentRMS;
    float mSilenceThreshold;
    float mHysteresis;

    u32 mSilenceStartTime;
    u32 mSilenceEndTime;
    u32 mMinSilenceDuration;
    u32 mMaxSilenceDuration;
    u32 mLastUpdateTime;

    // History for smoothing
    vector<float> mRMSHistory;
    int mHistorySize;
    int mHistoryIndex;

    static constexpr u32 DEFAULT_MIN_SILENCE_MS = 500;
    static constexpr u32 DEFAULT_MAX_SILENCE_MS = 60000;  // 1 minute
    static constexpr float DEFAULT_SILENCE_THRESHOLD = 0.01f;
    static constexpr float DEFAULT_HYSTERESIS = 0.2f;
    static constexpr int DEFAULT_HISTORY_SIZE = 5;

    float getSmoothedRMS();
    bool checkSilenceCondition(float smoothedRMS);
};

} // namespace fl
