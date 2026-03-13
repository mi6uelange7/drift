#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct OceanDSPHandle OceanDSPHandle;

OceanDSPHandle* ocean_create(double sampleRate, int maxFrames);
void            ocean_destroy(OceanDSPHandle* handle);
void            ocean_render(OceanDSPHandle* handle, float* left, float* right, int frameCount);

void ocean_set_wave_intensity(OceanDSPHandle* handle, float value); // 0.0–1.0
void ocean_set_stereo_width(OceanDSPHandle* handle, float value);   // 0.0–1.0
void ocean_set_master_gain(OceanDSPHandle* handle, float value);    // 0.0–1.0
void ocean_set_crash_enabled(OceanDSPHandle* handle, bool enabled);
void ocean_set_crash_rate(OceanDSPHandle* handle, float value);     // 0.0–1.0

#ifdef __cplusplus
}
#endif
