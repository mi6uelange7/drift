#include "OceanBridge.h"
#include "OceanDSP.hpp"

struct OceanDSPHandle { OceanDSP engine; };

OceanDSPHandle* ocean_create(double sr, int maxFrames) {
    auto* h = new OceanDSPHandle();
    h->engine.init(sr, maxFrames);
    return h;
}
void ocean_destroy(OceanDSPHandle* h) { delete h; }
void ocean_render(OceanDSPHandle* h, float* L, float* R, int n) { h->engine.render(L, R, n); }

void ocean_set_wave_intensity(OceanDSPHandle* h, float v) { h->engine.setWaveIntensity(v); }
void ocean_set_stereo_width(OceanDSPHandle* h, float v)   { h->engine.setStereoWidth(v); }
void ocean_set_master_gain(OceanDSPHandle* h, float v)    { h->engine.setMasterGain(v); }
void ocean_set_crash_enabled(OceanDSPHandle* h, bool v)   { h->engine.setCrashEnabled(v); }
void ocean_set_crash_rate(OceanDSPHandle* h, float v)     { h->engine.setCrashRate(v); }
