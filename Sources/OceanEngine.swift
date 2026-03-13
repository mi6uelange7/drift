import AVFoundation

final class OceanEngine {

    private let engine = AVAudioEngine()
    private var sourceNode: AVAudioSourceNode?
    private var dspHandle: OpaquePointer?

    private let sampleRate: Double = 48_000
    private let maxFrames:  Int32  = 512

    private var format: AVAudioFormat {
        AVAudioFormat(standardFormatWithSampleRate: sampleRate, channels: 2)!
    }

    func start() throws {
        dspHandle = ocean_create(sampleRate, maxFrames)
        let dsp = dspHandle

        sourceNode = AVAudioSourceNode(format: format) { _, _, frameCount, audioBufferList -> OSStatus in
            let abl = UnsafeMutableAudioBufferListPointer(audioBufferList)
            let n   = Int(frameCount)
            let L   = abl[0].mData!.assumingMemoryBound(to: Float.self)
            let R   = abl[1].mData!.assumingMemoryBound(to: Float.self)
            ocean_render(dsp, L, R, Int32(n))
            return noErr
        }

        guard let sourceNode else { return }
        engine.attach(sourceNode)
        engine.connect(sourceNode, to: engine.mainMixerNode, format: format)
        try engine.start()
        print("[OceanEngine] Started — \(sampleRate) Hz stereo")
    }

    func stop() {
        engine.stop()
        if let dsp = dspHandle { ocean_destroy(dsp); dspHandle = nil }
        sourceNode = nil
    }

    func setWaveIntensity(_ v: Float) {
        guard let dsp = dspHandle else { return }
        ocean_set_wave_intensity(dsp, v.clamped01())
    }

    func setStereoWidth(_ v: Float) {
        guard let dsp = dspHandle else { return }
        ocean_set_stereo_width(dsp, v.clamped01())
    }

    func setMasterGain(_ v: Float) {
        guard let dsp = dspHandle else { return }
        ocean_set_master_gain(dsp, v.clamped01())
    }

    func setCrashEnabled(_ enabled: Bool) {
        guard let dsp = dspHandle else { return }
        ocean_set_crash_enabled(dsp, enabled)
    }

    func setCrashRate(_ v: Float) {
        guard let dsp = dspHandle else { return }
        ocean_set_crash_rate(dsp, v.clamped01())
    }
}

private extension Float {
    func clamped01() -> Float { Swift.min(Swift.max(self, 0), 1) }
}
