#pragma once
#include <array>
#include <algorithm>
#include "BiquadFilter.hpp"
#include "BrownNoise.hpp"
#include "Xorshift32.hpp"

// A single ocean wave burst: bandpass-filtered noise with smooth ADSR envelope.
struct WaveBurst {
    enum class State { Idle, Attack, Sustain, Decay };

    BrownNoiseGen noiseL, noiseR;
    BiquadFilter  bpL, bpR;

    State state       = State::Idle;
    float env         = 0.0f;
    float amplitude   = 0.0f;
    float attackRate  = 0.0f;
    float decayRate   = 0.0f;
    float sustainSamp = 0.0f;

    bool isIdle()    const noexcept { return state == State::Idle; }
    bool isHealthy() const noexcept { return state == State::Attack || state == State::Sustain; }

    void spawn(double sr, Xorshift32& rng) noexcept {
        amplitude    = 0.40f + rng.nextUFloat() * 0.15f;          // 0.40–0.55
        float atk    = 0.8f  + rng.nextUFloat() * 1.2f;           // 0.8–2.0s
        float sus    = 1.5f  + rng.nextUFloat() * 3.0f;           // 1.5–4.5s
        float dec    = 1.2f  + rng.nextUFloat() * 2.0f;           // 1.2–3.2s

        attackRate   = 1.0f / (atk * (float)sr);
        decayRate    = 1.0f / (dec * (float)sr);
        sustainSamp  = sus * (float)sr;

        float freq   = 150.0f + rng.nextUFloat() * 450.0f;        // 150–600 Hz
        float q      = 0.6f   + rng.nextUFloat() * 0.8f;          // Q 0.6–1.4
        bpL.setBandpass(sr, freq, q);
        bpR.setBandpass(sr, freq * (1.0f + rng.nextUFloat() * 0.03f), q);

        noiseL.reset(); noiseR.reset();
        bpL.reset();    bpR.reset();
        env   = 0.0f;
        state = State::Attack;
    }

    void tick(Xorshift32& rng, float& outL, float& outR) noexcept {
        if (state == State::Idle) { outL = outR = 0.0f; return; }

        switch (state) {
            case State::Attack:
                env += attackRate;
                if (env >= 1.0f) { env = 1.0f; state = State::Sustain; }
                break;
            case State::Sustain:
                sustainSamp -= 1.0f;
                if (sustainSamp <= 0.0f) state = State::Decay;
                break;
            case State::Decay:
                env -= decayRate;
                if (env <= 0.0f) { env = 0.0f; state = State::Idle; outL = outR = 0.0f; return; }
                break;
            default: break;
        }

        // Smoothstep shaping: zero derivative at env=0 and env=1 — no audible knee at
        // Attack→Sustain or Sustain→Decay transitions
        float shaped = env * env * (3.0f - 2.0f * env);

        float wL = rng.nextFloat();
        float wR = rng.nextFloat();
        outL = bpL.tick(noiseL.tick(wL)) * shaped * amplitude;
        outR = bpR.tick(noiseR.tick(wR)) * shaped * amplitude;
    }
};

// Pool of up to 8 concurrent wave bursts.
//
// Key design: fill trigger watches countHealthy() (Attack+Sustain only), not countActive().
// This means a replacement spawns the moment a wave enters Decay — it overlaps the fade-out
// rather than starting after silence. fillDelay staggers startup spawns so they aren't
// synchronized; 0.1–0.25s is enough to desync envelopes without causing audible gaps.
class WavePool {
public:
    static constexpr int MAX_BURSTS = 8;
    static constexpr int MIN_HEALTHY = 3;  // always keep 3 waves in Attack or Sustain

    void init(double sampleRate) noexcept {
        sr               = sampleRate;
        triggerCountdown = 0.0f;
        fillDelay        = 0.0f;
    }

    void setIntensity(float v) noexcept { intensity = std::clamp(v, 0.0f, 1.0f); }

    void tick(Xorshift32& rng, float& outL, float& outR) noexcept {
        outL = outR = 0.0f;

        triggerCountdown -= 1.0f;
        fillDelay        -= 1.0f;

        if (triggerCountdown <= 0.0f) {
            scheduleNext(rng);
            spawnOne(rng);
            // Stagger the next fill so this timed wave gets its own phase
            if (fillDelay < 0.0f)
                fillDelay = (float)sr * (0.1f + rng.nextUFloat() * 0.15f);
        } else if (countHealthy() < MIN_HEALTHY && fillDelay <= 0.0f) {
            // A wave entered Decay — spawn its replacement NOW so it overlaps the fade-out.
            // Short delay (0.1–0.25s) just prevents consecutive-sample sync at startup.
            spawnOne(rng);
            fillDelay = (float)sr * (0.1f + rng.nextUFloat() * 0.15f);
        }

        for (auto& b : pool) {
            float l, r;
            b.tick(rng, l, r);
            outL += l;
            outR += r;
        }
    }

private:
    std::array<WaveBurst, MAX_BURSTS> pool;
    double sr               = 48000.0;
    float  intensity        = 0.5f;
    float  triggerCountdown = 0.0f;
    float  fillDelay        = 0.0f;

    int countHealthy() const noexcept {
        int n = 0;
        for (const auto& b : pool) if (b.isHealthy()) ++n;
        return n;
    }

    void scheduleNext(Xorshift32& rng) noexcept {
        float base   = 10.0f - intensity * 8.5f;          // 1.5s–10s
        float jitter = rng.nextUFloat() * base * 0.4f;
        triggerCountdown = std::max((base + jitter) * (float)sr, (float)sr * 1.0f);
    }

    void spawnOne(Xorshift32& rng) noexcept {
        for (auto& b : pool) {
            if (b.isIdle()) { b.spawn(sr, rng); return; }
        }
    }
};
