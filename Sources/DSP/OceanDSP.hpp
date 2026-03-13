#pragma once
#include <atomic>
#include "BrownNoise.hpp"
#include "WavePool.hpp"
#include "CrashPool.hpp"
#include "BiquadFilter.hpp"
#include "Limiter.hpp"
#include "Xorshift32.hpp"

class OceanDSP {
public:
    void init(double sampleRate, int maxFrames) noexcept;
    void render(float* L, float* R, int frameCount) noexcept;

    // Thread-safe setters — called from Swift main thread
    void setWaveIntensity(float v) noexcept { paramWaveIntensity.store(v, std::memory_order_relaxed); }
    void setStereoWidth(float v)   noexcept { paramStereoWidth.store(v,   std::memory_order_relaxed); }
    void setMasterGain(float v)    noexcept { paramMasterGain.store(v,    std::memory_order_relaxed); }
    void setCrashEnabled(bool v)   noexcept { paramCrashEnabled.store(v,  std::memory_order_relaxed); }
    void setCrashRate(float v)     noexcept { paramCrashRate.store(v,     std::memory_order_relaxed); }

private:
    double sr = 48000.0;

    Xorshift32    rng { 2463534242u };

    BrownNoiseGen brownL, brownR;
    BiquadFilter  lpL, lpR;

    WavePool      waves;
    CrashPool     crashes;

    Limiter       limiterL, limiterR;

    std::atomic<float> paramWaveIntensity { 0.5f };
    std::atomic<float> paramStereoWidth   { 0.4f };
    std::atomic<float> paramMasterGain    { 1.0f };
    std::atomic<bool>  paramCrashEnabled  { false };
    std::atomic<float> paramCrashRate     { 0.5f };

    float smoothWaveIntensity = 0.5f;
    float smoothStereoWidth   = 0.4f;
    float smoothMasterGain    = 1.0f;
    float smoothCrashRate     = 0.5f;

    // ~70ms at ~94 blocks/sec (512 frames @ 48kHz)
    static constexpr float SMOOTH_COEFF = 0.15f;

    static constexpr float BODY_GAIN   = 0.45f;
    static constexpr float WAVE_GAIN   = 0.55f;
    static constexpr float CRASH_GAIN  = 0.85f;
    static constexpr float DRIVE      = 1.4f;
    static constexpr float INV_DRIVE  = 1.0f / DRIVE;

    inline float softSat(float x) const noexcept {
        return tanhf(x * DRIVE) * INV_DRIVE;
    }
};
