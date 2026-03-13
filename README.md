# drift

> procedural ocean engine for macOS. no samples. no loops. no bs.

![platform](https://img.shields.io/badge/platform-macOS%2013%2B-black?style=flat-square)
![arch](https://img.shields.io/badge/arch-arm64-black?style=flat-square)
![license](https://img.shields.io/badge/license-MIT-black?style=flat-square)

---

drift is a menu bar app that synthesizes ocean sound in real time using a custom DSP engine written in C++17. every wave, crash, and swell is computed from scratch — no audio files anywhere in the binary.

---

## features

- **procedural DSP** — brown noise, bandpass-filtered wave bursts, multi-layer crash synthesis
- **real-time safe** — zero heap allocation in the render loop, lock-free atomics throughout
- **wave pool** — up to 8 simultaneous wave bursts with independent ADSR envelopes
- **crash engine** — 3-layer crashes (rumble / body / foam) with randomized timing
- **native UI** — popover interface, SF Symbols, `NSSwitch`, live parameter readouts
- **no Xcode** — single `build.sh`, clang++ + swiftc direct

---

## signal flow

```
brown noise (×2 L/R)
  └── lowpass 9kHz
          ↓
  wave pool (8 bursts, bandpass filtered)
          ↓
  crash pool (3 slots, 3-layer)
          ↓
    mid-side stereo width
          ↓
       master gain
          ↓
    tanh soft saturation
          ↓
  64-sample lookahead limiter
          ↓
        output
```

---

## build

requires Xcode command line tools.

```bash
git clone https://github.com/mi6uelange7/drift
cd drift
./build.sh run
```

output: `build/Drift.app`

---

## controls

| control | description |
|---|---|
| Waves | intensity and spawn rate of wave bursts |
| Crashes | enable/disable crash events |
| Rate | how frequently crashes occur |

---

## requirements

- macOS 13.0+
- Apple Silicon (arm64)
