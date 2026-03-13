#pragma once
#include <array>
#include <algorithm>
#include "BiquadFilter.hpp"
#include "BrownNoise.hpp"
#include "Xorshift32.hpp"

// A crash event: three layered noise bands with independent envelopes.
//   Rumble  — 60–160 Hz, slow build, long decay  (the approach)
//   Body    — 200–900 Hz, main crash mass
//   Foam    — 1500–4000 Hz, fast decay           (the wash)
struct CrashBurst {
    enum class State { Idle, Attack, Sustain, Decay };

    // Per-layer noise + filter (L and R independent for width)
    BrownNoiseGen rumbleNL, rumbleNR;
    BiquadFilter  rumbleBPL, rumbleBPR;

    BrownNoiseGen bodyNL, bodyNR;
    BiquadFilter  bodyBPL, bodyBPR;

    BrownNoiseGen foamNL, foamNR;
    BiquadFilter  foamBPL, foamBPR;

    State state      = State::Idle;
    float amplitude  = 0.0f;

    // Main envelope (rumble + body track this)
    float env        = 0.0f;
    float attackRate = 0.0f;
    float decayRate  = 0.0f;
    float sustainSamp = 0.0f;

    // Foam envelope — shares attack, but decays faster
    float foamEnv      = 0.0f;
    float foamDecayRate = 0.0f;

    bool isIdle() const noexcept { return state == State::Idle; }

    void spawn(double sr, Xorshift32& rng) noexcept {
        amplitude = 0.55f + rng.nextUFloat() * 0.35f;   // 0.55–0.90

        float atk = 1.8f + rng.nextUFloat() * 2.0f;     // 1.8–3.8s slow build
        float sus = 0.3f + rng.nextUFloat() * 0.8f;     // 0.3–1.1s peak
        float dec = 4.0f + rng.nextUFloat() * 5.0f;     // 4–9s long wash

        attackRate    = 1.0f / (atk * (float)sr);
        decayRate     = 1.0f / (dec * (float)sr);
        foamDecayRate = 1.0f / ((dec * 0.3f) * (float)sr); // foam ~3× faster
        sustainSamp   = sus * (float)sr;

        // Rumble: 60–160 Hz, wide (low Q)
        float rFreq = 60.0f + rng.nextUFloat() * 100.0f;
        float rQ    = 0.35f + rng.nextUFloat() * 0.25f;
        rumbleBPL.setBandpass(sr, rFreq, rQ);
        rumbleBPR.setBandpass(sr, rFreq * (1.0f + rng.nextUFloat() * 0.02f), rQ);

        // Body: 200–900 Hz
        float bFreq = 200.0f + rng.nextUFloat() * 700.0f;
        float bQ    = 0.5f  + rng.nextUFloat() * 0.5f;
        bodyBPL.setBandpass(sr, bFreq, bQ);
        bodyBPR.setBandpass(sr, bFreq * (1.0f + rng.nextUFloat() * 0.025f), bQ);

        // Foam: 1500–4000 Hz
        float fFreq = 1500.0f + rng.nextUFloat() * 2500.0f;
        float fQ    = 1.0f   + rng.nextUFloat() * 0.8f;
        foamBPL.setBandpass(sr, fFreq, fQ);
        foamBPR.setBandpass(sr, fFreq * (1.0f + rng.nextUFloat() * 0.02f), fQ);

        rumbleNL.reset(); rumbleNR.reset(); rumbleBPL.reset(); rumbleBPR.reset();
        bodyNL.reset();   bodyNR.reset();   bodyBPL.reset();   bodyBPR.reset();
        foamNL.reset();   foamNR.reset();   foamBPL.reset();   foamBPR.reset();

        env     = 0.0f;
        foamEnv = 0.0f;
        state   = State::Attack;
    }

    void tick(Xorshift32& rng, float& outL, float& outR) noexcept {
        if (state == State::Idle) { outL = outR = 0.0f; return; }

        switch (state) {
            case State::Attack:
                env     += attackRate;
                foamEnv  = env;
                if (env >= 1.0f) { env = 1.0f; foamEnv = 1.0f; state = State::Sustain; }
                break;
            case State::Sustain:
                sustainSamp -= 1.0f;
                if (sustainSamp <= 0.0f) state = State::Decay;
                break;
            case State::Decay:
                env     -= decayRate;
                foamEnv -= foamDecayRate;
                foamEnv  = std::max(foamEnv, 0.0f);
                if (env <= 0.0f) { env = 0.0f; state = State::Idle; outL = outR = 0.0f; return; }
                break;
            default: break;
        }

        // Rumble + body share the main envelope; foam uses foamEnv
        float wL1 = rng.nextFloat(), wR1 = rng.nextFloat();
        float wL2 = rng.nextFloat(), wR2 = rng.nextFloat();
        float wL3 = rng.nextFloat(), wR3 = rng.nextFloat();

        float rL = rumbleBPL.tick(rumbleNL.tick(wL1)) * 0.5f; // rumble a bit quieter
        float rR = rumbleBPR.tick(rumbleNR.tick(wR1)) * 0.5f;
        float bL = bodyBPL.tick(bodyNL.tick(wL2));
        float bR = bodyBPR.tick(bodyNR.tick(wR2));
        float fL = foamBPL.tick(foamNL.tick(wL3)) * foamEnv * 0.35f;
        float fR = foamBPR.tick(foamNR.tick(wR3)) * foamEnv * 0.35f;

        outL = ((rL + bL) * env + fL) * amplitude;
        outR = ((rR + bR) * env + fR) * amplitude;
    }
};

class CrashPool {
public:
    static constexpr int MAX_CRASHES = 3;

    void init(double sampleRate) noexcept {
        sr = sampleRate;
        triggerCountdown = (float)sr * 6.0f; // first crash ~6s in
    }

    void setEnabled(bool en) noexcept { enabled = en; }
    void setRate(float v)    noexcept { rate = std::clamp(v, 0.0f, 1.0f); }

    void tick(Xorshift32& rng, float& outL, float& outR) noexcept {
        outL = outR = 0.0f;
        if (!enabled) return;

        triggerCountdown -= 1.0f;
        if (triggerCountdown <= 0.0f) trySpawn(rng);

        for (auto& c : pool) {
            float l, r;
            c.tick(rng, l, r);
            outL += l;
            outR += r;
        }
    }

private:
    std::array<CrashBurst, MAX_CRASHES> pool;
    double sr       = 48000.0;
    bool   enabled  = false;
    float  rate     = 0.5f;
    float  triggerCountdown = 0.0f;

    int countActive() const noexcept {
        int n = 0;
        for (const auto& c : pool) if (!c.isIdle()) ++n;
        return n;
    }

    void trySpawn(Xorshift32& rng) noexcept {
        // Rate 0 = crash every ~45s, rate 1 = every ~8s
        float base   = 45.0f - rate * 37.0f;            // 8–45s
        float jitter = rng.nextUFloat() * base * 0.45f;
        triggerCountdown = std::max((base + jitter) * (float)sr, (float)sr * 5.0f);

        if (countActive() < MAX_CRASHES) {
            for (auto& c : pool) {
                if (c.isIdle()) { c.spawn(sr, rng); return; }
            }
        }
    }
};
