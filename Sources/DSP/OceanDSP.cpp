#include "OceanDSP.hpp"
#include <algorithm>
#include <cmath>

void OceanDSP::init(double sampleRate, int /*maxFrames*/) noexcept {
    sr = sampleRate;

    brownL.reset();
    brownR.reset();

    lpL.setLowpass(sr, 9000.0f, 0.7f);
    lpR.setLowpass(sr, 9000.0f, 0.7f);

    waves.init(sr);
    waves.setIntensity(0.5f);

    crashes.init(sr);

    limiterL.init();
    limiterR.init();
}

void OceanDSP::render(float* L, float* R, int frameCount) noexcept {
    // Read atomics once per block, then smooth with one-pole filter
    float targetWI      = paramWaveIntensity.load(std::memory_order_relaxed);
    float targetSW      = paramStereoWidth.load(std::memory_order_relaxed);
    float targetGain    = paramMasterGain.load(std::memory_order_relaxed);
    bool  crashEnabled  = paramCrashEnabled.load(std::memory_order_relaxed);
    float targetCR      = paramCrashRate.load(std::memory_order_relaxed);

    smoothWaveIntensity += SMOOTH_COEFF * (targetWI   - smoothWaveIntensity);
    smoothStereoWidth   += SMOOTH_COEFF * (targetSW   - smoothStereoWidth);
    smoothMasterGain    += SMOOTH_COEFF * (targetGain - smoothMasterGain);
    smoothCrashRate     += SMOOTH_COEFF * (targetCR   - smoothCrashRate);

    waves.setIntensity(smoothWaveIntensity);
    crashes.setEnabled(crashEnabled);
    crashes.setRate(smoothCrashRate);

    const float masterGain = smoothMasterGain;
    const float width      = smoothStereoWidth * 0.3f;
    // Wave volume goes from 0 (silent) to WAVE_GAIN (full) across the slider
    const float waveGain   = WAVE_GAIN * smoothWaveIntensity;

    for (int i = 0; i < frameCount; ++i) {
        // ── 1. Ocean body ─────────────────────────────────────────────────────
        float bodyL = lpL.tick(brownL.tick(rng.nextFloat())) * BODY_GAIN;
        float bodyR = lpR.tick(brownR.tick(rng.nextFloat())) * BODY_GAIN;

        // ── 2. Wave pool ──────────────────────────────────────────────────────
        float waveL, waveR;
        waves.tick(rng, waveL, waveR);
        waveL *= waveGain;
        waveR *= waveGain;

        // ── 3. Crash pool ─────────────────────────────────────────────────────
        float crashL, crashR;
        crashes.tick(rng, crashL, crashR);
        crashL *= CRASH_GAIN;
        crashR *= CRASH_GAIN;

        // ── 4. Mix ────────────────────────────────────────────────────────────
        float mixL = bodyL + waveL + crashL;
        float mixR = bodyR + waveR + crashR;

        // ── 5. Stereo width (mid-side) ────────────────────────────────────────
        float mid   = (mixL + mixR) * 0.5f;
        float sideL = mixL - mid;
        float sideR = mixR - mid;
        float outL  = mid + sideL * (1.0f + width);
        float outR  = mid + sideR * (1.0f + width);

        // ── 6. Master gain ────────────────────────────────────────────────────
        outL *= masterGain;
        outR *= masterGain;

        // ── 7. Soft saturation ────────────────────────────────────────────────
        outL = softSat(outL);
        outR = softSat(outR);

        // ── 8. Lookahead limiter ──────────────────────────────────────────────
        outL = limiterL.tick(outL);
        outR = limiterR.tick(outR);

        // ── 9. Final hard clamp ───────────────────────────────────────────────
        L[i] = std::clamp(outL, -1.0f, 1.0f);
        R[i] = std::clamp(outR, -1.0f, 1.0f);
    }
}
