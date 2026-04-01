# rtl-scan

A next-generation RTL-SDR spectrum analyzer with live wideband FFT and waterfall display, built with Dear ImGui.

![rtl-scan screenshot placeholder](docs/screenshot.png)

## Features

- **Wideband sweep** — scans any frequency range (like rtl_power) by hopping the RTL-SDR center frequency and stitching FFT slices together
- **Live spectrum** — filled dBFS spectrum plot with frequency and dB grid, max-hold downsampling to display width
- **Scrolling waterfall** — 512-row history with a turbo-inspired colormap (4096 px wide texture)
- **ImGui docking UI** — Controls, Spectrum, and Waterfall panels dock and resize freely; layout persists across sessions
- **Real-time range control** — change start/end frequency and apply without restarting the app; built-in presets for common bands
- **DC spike suppression** — center ±2 bins interpolated on every hop
- **Tuner rolloff cropping** — configurable edge crop (default 10%) per hop to discard anti-alias rolloff

## Default layout

```
┌─────────────────────────────────────────┐
│  Controls  (range · presets · apply)    │
├─────────────────────────────────────────┤
│                                         │
│              Spectrum                   │
│                                         │
├─────────────────────────────────────────┤
│                                         │
│              Waterfall                  │
│                                         │
└─────────────────────────────────────────┘
```

## Dependencies

| Library | Purpose |
|---|---|
| [librtlsdr](https://github.com/osmocom/rtl-sdr) | RTL-SDR hardware access |
| [fftw3f](https://www.fftw.org/) | Single-precision FFT |
| [Dear ImGui](https://github.com/ocornut/imgui) (docking branch, vendored) | GUI |
| GLFW3 + OpenGL 3.3 | Window and rendering backend |

## Build

### Linux

```bash
sudo apt install librtlsdr-dev libfftw3-dev libglfw3-dev libgl-dev cmake build-essential
git clone https://github.com/SarahRoseLives/rtl-scan.git
cd rtl-scan
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Windows (MSYS2 / MinGW64)

```bash
pacman -S mingw-w64-x86_64-{cmake,gcc,rtl-sdr,fftw,glfw}
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
mingw32-make -j$(nproc)
```

## Usage

```bash
# Default range: 80–108 MHz (FM broadcast band)
./rtl-scan

# Specify start and end frequency (Hz, MHz with M suffix, GHz with G suffix)
./rtl-scan 88M 108M
./rtl-scan 400M 512M        # PMR/LPD/ISM
./rtl-scan 1090M 1091M      # ADS-B
./rtl-scan 24M 1766M        # Full RTL-SDR range (~870 hops)
```

You can also change the range live inside the app using the **Controls** panel.

## Architecture

```
[Sweep thread]  SweepController: hop → read_sync → FFT → stitch → SweepFrame
                        │
                RingBuffer<SweepFrame>
                        │
[Main thread]   App: drain ring → SpectrumView + WaterfallView → ImGui/OpenGL
```

- **SdrDevice** — RAII librtlsdr wrapper with sync read and runtime freq change
- **FftProcessor** — Hann window → fftw3f forward FFT → dBFS magnitude (FFT_SIZE bins)
- **SweepController** — dedicated thread, hops across the configured range, stitches per-hop bins into a full `SweepFrame`
- **RingBuffer** — lock-based circular buffer (mutex + condvar) connecting the two threads
- **SpectrumView** — ImGui DrawList filled spectrum, pixel-bucket max-hold downsampling
- **WaterfallView** — scrolling OpenGL RGBA texture (4096 × 512), per-frame colormap row inserted at top

## License

MIT — see [LICENSE](LICENSE)
