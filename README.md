# FlexASIO GUI Tester

*A fork of [FlexASIO][upstream] by [Etienne Dechamps][] that adds a graphical
settings editor, a latency visualizer, and — the main point — an **audio dropout
tester** that lets you find a stable buffer size without touching your DAW.*

*ASIO is a trademark and software of Steinberg Media Technologies GmbH*

## Why this fork exists

[FlexASIO][upstream] is an excellent universal ASIO driver, but tuning it is
tedious. Its settings live in a hand-edited [`FlexASIO.toml`][CONFIGURATION] file,
and finding the lowest buffer size that doesn't crackle means repeating this loop:

1. Close the DAW that's holding the audio device
2. Edit `FlexASIO.toml` by hand
3. Reopen the DAW and load a project
4. Play something and listen for crackling
5. Guess a new buffer size, go to step 1

This fork collapses that loop into a single window. The tester plays audio through
**PortAudio** — the same library FlexASIO itself uses — with the same backend,
device, and buffer size you've configured. So a buffer size that tests clean here
behaves the same way when FlexASIO uses it, and you never have to open your DAW to
find out.

## What's added

Everything upstream does is unchanged. This fork adds one new component,
`FlexASIOGUI.exe`, with three tabs:

### Settings

A visual editor for `FlexASIO.toml`. Pick your backend and devices from dropdowns
populated by live PortAudio device enumeration, so the device names always match
what FlexASIO expects. Covers buffer size, channel counts, sample types, suggested
latency, and the WASAPI-specific options (exclusive mode, auto-convert, explicit
sample format), which appear only when the WASAPI backend is selected.

Settings that FlexASIO treats as "auto" when absent — sample type and suggested
latency — keep an explicit *Auto* option, so opening the GUI and saving won't
silently replace auto-detection with a fixed value.

### Latency

A breakdown of where your latency actually comes from: the ASIO buffer, PortAudio's
own buffering, and the Windows audio engine (which WASAPI Exclusive and WDM-KS
bypass entirely). Alongside it, a comparison table of common buffer sizes with
their latency and a rough stability-risk rating, so you can see the tradeoff before
committing to a number.

These are *estimates* derived from FlexASIO's own latency model, not measurements.
Use the Tester tab to find out what actually works.

### Tester

The reason this fork exists.

- Plays a **440 Hz sine**, **pink noise**, or a **20 Hz–20 kHz sweep** through the
  configured device
- Detects dropouts using PortAudio's `paOutputUnderflow` flag — the authoritative
  signal, raised inside PortAudio's own high-priority audio thread when the
  hardware needed samples that weren't ready
- Shows a live **green / yellow / red** verdict, dropout count, and glitch rate
- Shows callback-measured **stereo output peak meters** alongside the stability verdict
- Lets you **drag the buffer size while audio is playing** and hear/see the effect
  within a couple hundred milliseconds
- **Auto-Tune** binary-searches common buffer sizes (32 … 4096), testing each for a
  few seconds, and reports the smallest one that produced zero dropouts
- **Apply to FlexASIO.toml** writes the result straight into your config

Because FlexASIO watches its config file and resets when it changes, an ASIO host
that's already running will generally pick up the new buffer size on its own.

## Status

Working and usable, built and smoke-tested on Windows 11 with MSVC 2022 and Qt 6.8.1.
The GUI includes config-file change watching, a custom dark arcade-inspired theme, and
an application icon. The build deploys the required Qt runtime files beside the executable.

Input/recording testing isn't implemented; the tester is output-only, which covers the
playback-crackling case that motivated this fork. A packaged installer is also not yet
provided; build from source using the instructions below.

## Building

You'll need Visual Studio 2022 Build Tools with the **Desktop development with C++**
workload *and* the **ATL** component (`Microsoft.VisualStudio.Component.VC.ATL` —
upstream's COM code needs `atlbase.h`, and it isn't in the default workload), plus
CMake and Qt 6 for MSVC 2022.

The quickest way to get Qt is prebuilt binaries via [`aqtinstall`][aqtinstall]:

```
pip install aqtinstall
python -m aqt install-qt windows desktop 6.8.1 win64_msvc2022_64 -O C:/Qt
```

Then clone with submodules and build:

```
git clone --recursive https://github.com/Pasapeb-cell/FlexAsio-GUI-Tester.git
cd FlexAsio-GUI-Tester
set FLEXASIOGUI_QT_DIR=C:/Qt/6.8.1/msvc2022_64
cmake -G "Visual Studio 17 2022" -A x64 -S src -B build
cmake --build build --config Release
```

`FLEXASIOGUI_QT_DIR` is how the superbuild passes `CMAKE_PREFIX_PATH` down to the
GUI so `find_package(Qt6)` can find it. The result lands in
`build/install/bin/FlexASIOGUI.exe`, with the Qt runtime deployed alongside it.

## Upstream documentation

The original FlexASIO docs still apply to the driver itself and remain the best
reference:

- [Configuration reference][CONFIGURATION] — every `FlexASIO.toml` option
- [Backends][BACKENDS] — how MME, DirectSound, WASAPI, and WDM-KS differ, and
  which to pick
- [FAQ][] — low-latency operation, bit-perfect streaming, troubleshooting

Upstream also ships `PortAudioDevices.exe` (dumps everything PortAudio knows about
your devices) and `FlexASIOTest.exe` (a console ASIO self-test); both are built by
this fork too. Creating an empty `FlexASIO.log` in your user directory enables
detailed driver logging — remember to delete it afterward, since logging itself can
cause glitches.

## Credits and license

FlexASIO is the work of [Etienne Dechamps][] and its contributors — all the hard
parts (the ASIO implementation, the PortAudio integration, the configuration
system) are theirs. This fork only adds a GUI on top. If FlexASIO is useful to you,
please [support the upstream project][upstream].

There is also a separate, longer-established third-party configuration GUI,
flipswitchingmonkey's [FlexASIO GUI][FlexASIO_GUI], which upstream recommends. It
is a settings editor rather than a dropout tester; if configuration is all you
need, it's more mature than this fork.

Licensed under the MIT License, same as upstream — see [LICENSE.txt][]. The ASIO
trademark and SDK are subject to Steinberg's own license terms.

**Bugs in the GUI belong [here][fork issues]. Bugs in the driver itself belong
[upstream][upstream issues]** — please don't send FlexASIO's maintainer issues
caused by this fork.

---

![ASIO logo](ASIO.jpg)

[aqtinstall]: https://github.com/miurahr/aqtinstall
[BACKENDS]: BACKENDS.md
[CONFIGURATION]: CONFIGURATION.md
[Etienne Dechamps]: mailto:etienne@edechamps.fr
[FAQ]: FAQ.md
[FlexASIO_GUI]: https://github.com/flipswitchingmonkey/FlexASIO_GUI
[fork issues]: https://github.com/Pasapeb-cell/FlexAsio-GUI-Tester/issues
[LICENSE.txt]: LICENSE.txt
[upstream]: https://github.com/dechamps/FlexASIO
[upstream issues]: https://github.com/dechamps/FlexASIO/issues
