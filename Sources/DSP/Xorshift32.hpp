#pragma once
#include <cstdint>

// Minimal, zero-allocation, real-time-safe PRNG.
// Period: 2^32 - 1 (~4 billion samples, ~25 hours at 48kHz).
class Xorshift32 {
public:
    explicit Xorshift32(uint32_t seed = 2463534242u) noexcept : state(seed) {}

    uint32_t next() noexcept {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }

    // Returns float in [-1.0, 1.0)
    float nextFloat() noexcept {
        // Map uint32 to [0, 1) then shift to [-1, 1)
        return static_cast<float>(next()) * (2.0f / 4294967296.0f) - 1.0f;
    }

    // Returns float in [0.0, 1.0)
    float nextUFloat() noexcept {
        return static_cast<float>(next()) * (1.0f / 4294967296.0f);
    }

private:
    uint32_t state;
};
