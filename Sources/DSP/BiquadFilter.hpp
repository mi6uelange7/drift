#pragma once
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Direct Form II biquad. Supports lowpass and bandpass modes.
// Coefficients computed at init time — no allocation in render path.
class BiquadFilter {
public:
    void setLowpass(double sampleRate, float cutHz, float q) noexcept {
        double w0    = 2.0 * M_PI * cutHz / sampleRate;
        double cosW0 = std::cos(w0);
        double sinW0 = std::sin(w0);
        double alpha = sinW0 / (2.0 * q);

        double b0 =  (1.0 - cosW0) * 0.5;
        double b1 =   1.0 - cosW0;
        double b2 =  (1.0 - cosW0) * 0.5;
        double a0 =   1.0 + alpha;
        double a1 =  -2.0 * cosW0;
        double a2 =   1.0 - alpha;

        setCoeffs(b0, b1, b2, a0, a1, a2);
    }

    void setBandpass(double sampleRate, float centerHz, float q) noexcept {
        double w0    = 2.0 * M_PI * centerHz / sampleRate;
        double cosW0 = std::cos(w0);
        double sinW0 = std::sin(w0);
        double alpha = sinW0 / (2.0 * q);

        // Constant peak gain (skirt gain = Q)
        double b0 =  sinW0 * 0.5;
        double b1 =  0.0;
        double b2 = -sinW0 * 0.5;
        double a0 =  1.0 + alpha;
        double a1 = -2.0 * cosW0;
        double a2 =  1.0 - alpha;

        setCoeffs(b0, b1, b2, a0, a1, a2);
    }

    float tick(float x) noexcept {
        float w = x - a1 * z1 - a2 * z2;
        float y = b0 * w + b1 * z1 + b2 * z2;
        z2 = z1;
        z1 = w;
        return y;
    }

    void reset() noexcept { z1 = z2 = 0.0f; }

private:
    float b0=1, b1=0, b2=0;
    float a1=0, a2=0;
    float z1=0, z2=0;

    void setCoeffs(double b0_, double b1_, double b2_,
                   double a0_, double a1_, double a2_) noexcept {
        double inv = 1.0 / a0_;
        b0 = (float)(b0_ * inv);
        b1 = (float)(b1_ * inv);
        b2 = (float)(b2_ * inv);
        a1 = (float)(a1_ * inv);
        a2 = (float)(a2_ * inv);
    }
};
