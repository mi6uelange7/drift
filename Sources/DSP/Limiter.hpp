#pragma once
#include <array>
#include <cmath>
#include <algorithm>

// 64-sample lookahead limiter.
// Fast attack (~1ms), slow release (~200ms), smooth gain changes.
// Stack-only — no heap allocation.
class Limiter {
public:
    static constexpr int LOOKAHEAD = 64;

    void init() noexcept {
        delayBuf.fill(0.0f);
        pos = 0;
        envelope = 0.0f;
        gain = 1.0f;
    }

    float tick(float in) noexcept {
        // Write incoming sample into delay line
        delayBuf[pos] = in;

        // Peak envelope follower on the lookahead signal
        float peak = std::abs(in);
        if (peak > envelope)
            envelope = ATTACK_COEFF  * envelope + (1.0f - ATTACK_COEFF)  * peak;
        else
            envelope = RELEASE_COEFF * envelope;

        // Gain computation — only reduce, never boost
        float targetGain = (envelope > THRESHOLD)
            ? (THRESHOLD / envelope)
            : 1.0f;

        // Smooth to avoid zipper noise
        gain = GAIN_SMOOTH * gain + (1.0f - GAIN_SMOOTH) * targetGain;

        // Read from the head of the delay (lookahead position)
        int readPos = (pos + 1) % LOOKAHEAD;
        float out   = delayBuf[readPos] * gain;

        pos = (pos + 1) % LOOKAHEAD;
        return out;
    }

private:
    std::array<float, LOOKAHEAD> delayBuf {};
    int   pos      = 0;
    float envelope = 0.0f;
    float gain     = 1.0f;

    // Tuned for 48kHz
    static constexpr float THRESHOLD     = 0.93f;
    static constexpr float ATTACK_COEFF  = 0.9953f;  // ~4ms attack
    static constexpr float RELEASE_COEFF = 0.9995f;  // ~42ms release (was 208ms — caused pumping)
    static constexpr float GAIN_SMOOTH   = 0.9970f;  // ~7ms smoothing (was 20ms — gain lagged peaks)
};
