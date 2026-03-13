import Cocoa

// MARK: - AppDelegate

final class AppDelegate: NSObject, NSApplicationDelegate {
    private let ocean = OceanEngine()
    private var statusItem: NSStatusItem?
    private var popover: NSPopover?
    // Persist the VC so sleep timer state survives popover close/reopen
    private var contentVC: DriftViewController?

    func applicationDidFinishLaunching(_ notification: Notification) {
        setupMenu()
        setupStatusItem()
        do { try ocean.start() }
        catch { print("[Drift] start failed: \(error)") }
    }

    func applicationWillTerminate(_ notification: Notification) { ocean.stop() }

    private func setupMenu() {
        let appMenu = NSMenu()
        appMenu.addItem(NSMenuItem(title: "Quit Drift",
                                   action: #selector(NSApplication.terminate(_:)),
                                   keyEquivalent: "q"))
        let appMenuItem = NSMenuItem()
        appMenuItem.submenu = appMenu
        let mainMenu = NSMenu()
        mainMenu.addItem(appMenuItem)
        NSApp.mainMenu = mainMenu
    }

    private func setupStatusItem() {
        statusItem = NSStatusBar.system.statusItem(withLength: NSStatusItem.squareLength)
        guard let btn = statusItem?.button else { return }
        let cfg = NSImage.SymbolConfiguration(pointSize: 13, weight: .regular)
        let img = NSImage(systemSymbolName: "water.waves", accessibilityDescription: "Drift")?
            .withSymbolConfiguration(cfg)
        img?.isTemplate = true
        btn.image  = img
        btn.target = self
        btn.action = #selector(togglePopover)
    }

    @objc private func togglePopover(_ sender: Any?) {
        guard let btn = statusItem?.button else { return }
        if let p = popover, p.isShown { p.performClose(nil); return }
        if contentVC == nil { contentVC = DriftViewController(ocean: ocean) }
        let p  = NSPopover()
        p.contentViewController = contentVC
        p.behavior = .transient
        p.animates = true
        p.show(relativeTo: btn.bounds, of: btn, preferredEdge: .minY)
        popover = p
    }
}

// MARK: - Popover view controller

private final class DriftViewController: NSViewController {
    private let ocean: OceanEngine

    private var waveSlider:  NSSlider!
    private var waveVal:     NSTextField!
    private var crashSw:     NSSwitch!
    private var rateSlider:  NSSlider!
    private var rateVal:     NSTextField!
    private var sleepPopup:  NSPopUpButton!
    private var sleepTimer:  Timer?

    private let W:    CGFloat = 240
    private let padX: CGFloat = 16

    init(ocean: OceanEngine) {
        self.ocean = ocean
        super.init(nibName: nil, bundle: nil)
    }
    required init?(coder: NSCoder) { fatalError() }

    override func loadView() {
        let fx = NSVisualEffectView(frame: .zero)
        fx.blendingMode = .behindWindow
        fx.material     = .popover
        fx.state        = .active
        view = fx
    }

    override func viewDidLoad() {
        super.viewDidLoad()
        layoutUI()
    }

    private func layoutUI() {
        var y: CGFloat = 10

        // — Quit —
        let quitBtn = NSButton(title: "Quit", target: NSApp,
                               action: #selector(NSApplication.terminate(_:)))
        quitBtn.bezelStyle  = .inline
        quitBtn.controlSize = .small
        quitBtn.font = .systemFont(ofSize: 12)
        quitBtn.sizeToFit()
        quitBtn.frame.origin = CGPoint(x: W - padX - quitBtn.frame.width, y: y + 5)
        view.addSubview(quitBtn)
        y += 26

        addSep(y: y + 4); y += 14

        // — Sleep timer —
        let sleepRow = NSView(frame: NSRect(x: 0, y: y, width: W, height: 30))
        let sleepLbl = makeLabel("Sleep", size: 13)
        sleepLbl.frame = NSRect(x: padX, y: 7, width: 60, height: 18)
        sleepRow.addSubview(sleepLbl)

        let popup = NSPopUpButton(frame: .zero, pullsDown: false)
        popup.addItems(withTitles: ["Off", "15 min", "30 min", "1 hr", "2 hr"])
        popup.controlSize = .small
        popup.font = .systemFont(ofSize: 12)
        popup.sizeToFit()
        popup.frame.origin = CGPoint(x: W - padX - popup.frame.width, y: 4)
        popup.target = self
        popup.action = #selector(sleepChanged)
        sleepRow.addSubview(popup)
        sleepPopup = popup
        view.addSubview(sleepRow); y += 30

        addSep(y: y + 4); y += 14

        // — Crash rate —
        let (rs, rv, rRow) = makeSliderRow("Rate", value: 0.5, action: #selector(rateChanged))
        rs.isEnabled = false
        rv.stringValue = "—"
        rateSlider = rs; rateVal = rv
        rRow.frame.origin = CGPoint(x: 0, y: y)
        view.addSubview(rRow); y += 30

        // — Crashes toggle —
        let cRow = NSView(frame: NSRect(x: 0, y: y, width: W, height: 32))
        let cLbl = makeLabel("Crashes", size: 13)
        cLbl.frame = NSRect(x: padX, y: 7, width: 80, height: 18)
        cRow.addSubview(cLbl)

        let sw = NSSwitch(frame: .zero)
        sw.sizeToFit()
        sw.frame.origin = CGPoint(x: W - padX - sw.frame.width, y: 5)
        sw.state = .off; sw.target = self; sw.action = #selector(crashToggled)
        cRow.addSubview(sw)
        crashSw = sw
        view.addSubview(cRow); y += 32

        addSep(y: y + 4); y += 14

        // — Waves —
        let (ws, wv, wRow) = makeSliderRow("Waves", value: 0.5, action: #selector(waveChanged))
        waveSlider = ws; waveVal = wv
        wRow.frame.origin = CGPoint(x: 0, y: y)
        view.addSubview(wRow); y += 30

        addSep(y: y + 4); y += 14

        // — Title —
        let titleRow = NSView(frame: NSRect(x: 0, y: y, width: W, height: 36))
        let iconV = NSImageView(frame: NSRect(x: padX, y: 10, width: 15, height: 15))
        iconV.image = NSImage(systemSymbolName: "water.waves", accessibilityDescription: nil)?
            .withSymbolConfiguration(.init(pointSize: 11, weight: .regular))
        iconV.contentTintColor = .secondaryLabelColor
        titleRow.addSubview(iconV)
        let titleLbl = makeLabel("Drift", size: 14, weight: .semibold)
        titleLbl.frame = NSRect(x: padX + 22, y: 10, width: 80, height: 18)
        titleRow.addSubview(titleLbl)
        view.addSubview(titleRow); y += 36

        let totalH = y + 10
        view.frame = NSRect(x: 0, y: 0, width: W, height: totalH)
        preferredContentSize = view.frame.size
    }

    // MARK: Layout helpers

    private func makeSliderRow(_ text: String, value: Double, action: Selector)
        -> (NSSlider, NSTextField, NSView)
    {
        let row = NSView(frame: NSRect(x: 0, y: 0, width: W, height: 30))

        let lbl = makeLabel(text, size: 13)
        lbl.frame = NSRect(x: padX, y: 6, width: 54, height: 18)
        row.addSubview(lbl)

        let valW: CGFloat = 36
        let slX = padX + 56
        let slW = W - slX - valW - padX - 4
        let sl  = NSSlider(value: value, minValue: 0, maxValue: 1,
                           target: self, action: action)
        sl.frame = NSRect(x: slX, y: 5, width: slW, height: 20)
        row.addSubview(sl)

        let vl = NSTextField(labelWithString: pct(value))
        vl.frame     = NSRect(x: W - padX - valW, y: 7, width: valW, height: 16)
        vl.font      = .monospacedDigitSystemFont(ofSize: 11, weight: .regular)
        vl.textColor = .tertiaryLabelColor
        vl.alignment = .right
        row.addSubview(vl)

        return (sl, vl, row)
    }

    private func addSep(y: CGFloat) {
        let box = NSBox(frame: NSRect(x: padX, y: y, width: W - padX * 2, height: 1))
        box.boxType = .separator
        view.addSubview(box)
    }

    private func makeLabel(_ s: String, size: CGFloat,
                            weight: NSFont.Weight = .regular) -> NSTextField {
        let f = NSTextField(labelWithString: s)
        f.font      = .systemFont(ofSize: size, weight: weight)
        f.textColor = .labelColor
        return f
    }

    private func pct(_ v: Double) -> String { "\(Int(v * 100))%" }

    // MARK: Actions

    @objc private func waveChanged(_ s: NSSlider) {
        waveVal.stringValue = pct(s.doubleValue)
        ocean.setWaveIntensity(s.floatValue)
    }

    @objc private func crashToggled() {
        let on = crashSw.state == .on
        ocean.setCrashEnabled(on)
        rateSlider.isEnabled = on
        rateVal.stringValue  = on ? pct(rateSlider.doubleValue) : "—"
    }

    @objc private func rateChanged(_ s: NSSlider) {
        rateVal.stringValue = pct(s.doubleValue)
        ocean.setCrashRate(s.floatValue)
    }

    @objc private func sleepChanged() {
        sleepTimer?.invalidate()
        sleepTimer = nil

        let minutes: Double? = [nil, 15, 30, 60, 120][sleepPopup.indexOfSelectedItem]
        guard let mins = minutes else { return }
        sleepTimer = Timer.scheduledTimer(withTimeInterval: mins * 60,
                                          repeats: false) { _ in NSApp.terminate(nil) }
    }
}
