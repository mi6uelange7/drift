#pragma once

// Paul Kellet pink noise algorithm + leaky integrator for brown (1/f²) spectrum.
// Zero allocation, stack-only state.
class BrownNoiseGen {
public:
    // white: sample from Xorshift32::nextFloat() — range [-1, 1]
    float tick(float white) noexcept {
        b0 = 0.99886f * b0 + white * 0.0555179f;
        b1 = 0.99332f * b1 + white * 0.0750759f;
        b2 = 0.96900f * b2 + white * 0.1538520f;
        b3 = 0.86650f * b3 + white * 0.3104856f;
        b4 = 0.55000f * b4 + white * 0.5329522f;
        b5 = -0.7616f * b5 - white * 0.0168980f;
        float pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f;
        b6 = white * 0.115926f;
        pink *= 0.115926f; // normalize pink to ~[-1, 1]

        // Leaky integrator: pink → brown (steeper roll-off)
        // 0.9997 leak prevents DC buildup
        integ = integ * 0.9997f + pink * 0.0003f;
        return integ * 8.0f; // empirical scale to restore RMS ~0.1
    }

    void reset() noexcept {
        b0=b1=b2=b3=b4=b5=b6=integ = 0.0f;
    }

private:
    float b0=0,b1=0,b2=0,b3=0,b4=0,b5=0,b6=0;
    float integ = 0.0f;
};
