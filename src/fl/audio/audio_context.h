#pragma once

#include "fl/audio.h"
#include "fl/fft.h"
#include "fl/vector.h"
#include "fl/ptr.h"
#include "fl/span.h"

namespace fl {

class AudioContext {
public:
    explicit AudioContext(const AudioSample& sample);
    ~AudioContext();

    // ----- Basic Sample Access -----
    const AudioSample& getSample() const { return mSample; }
    span<const int16_t> getPCM() const { return mSample.pcm(); }
    float getRMS() const { return mSample.rms(); }
    float getZCF() const { return mSample.zcf(); }
    u32 getTimestamp() const { return mSample.timestamp(); }

    // ----- Lazy FFT Computation (with caching) -----
    const FFTBins& getFFT(
        int bands = 16,
        float fmin = FFT_Args::DefaultMinFrequency(),
        float fmax = FFT_Args::DefaultMaxFrequency()
    );
    bool hasFFT() const { return mFFTComputed; }

    // ----- FFT History (for temporal analysis) -----
    const vector<FFTBins>& getFFTHistory(int depth = 4);
    bool hasFFTHistory() const { return mFFTHistoryDepth > 0; }
    const FFTBins* getHistoricalFFT(int framesBack) const;

    // ----- Update & Reset -----
    void setSample(const AudioSample& sample);
    void clearCache();

private:
    AudioSample mSample;
    mutable FFTBins mFFT;
    mutable bool mFFTComputed;
    mutable FFT_Args mFFTArgs;
    vector<FFTBins> mFFTHistory;
    int mFFTHistoryDepth;
    int mFFTHistoryIndex;
};

} // namespace fl
