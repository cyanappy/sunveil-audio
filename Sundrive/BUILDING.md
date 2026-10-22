# Building Sundrive

> SOURCE REPOSITORY NOTE: This folder contains Sundrive 1.3.2 source and build inputs. Build with build_windows.bat; output is in Build/. Prebuilt VST3 bundles and Tools executables are excluded. The original release documentation below describes a prebuilt distribution. Install.bat and CheckPlugin.bat are release-packaging helpers; use the build instructions in the repository root README for this checkout.

The ready-built Windows plugin is included. Building is only necessary if you
want to modify the code.

## Windows with Visual Studio 2022

Install Visual Studio 2022 or its Build Tools with **Desktop development with C++**,
the Windows SDK, and CMake 3.20 or newer. Then run `build_windows.bat`, or use:

```bat
cmake -S . -B Build -G "Visual Studio 17 2022" -A x64
cmake --build Build --config Release
ctest --test-dir Build -C Release --output-on-failure
```

The plugin folder will be `Build\Sundrive.vst3`. The Windows tests load that
binary through the VST3 factory, exercise the audio and controller interfaces,
and open a small native host window to test the editor controls and DPI scaling.
Two BMP screenshots are written into the test working directory when GUI tests run.

## MinGW-w64

A 64-bit MinGW-w64 toolchain can also build this project:

```bat
cmake -S . -B BuildMinGW -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build BuildMinGW
ctest --test-dir BuildMinGW --output-on-failure
```

The distributed binary was cross-compiled with GCC 13.2.0 / MinGW-w64, using
static C++ and GCC runtimes. The project requires no downloads during a build:
the official VST3 3.8.1 interface headers are already in `third_party`.

## Linux audio/interface checks

```sh
cmake -S . -B BuildLinux -DCMAKE_BUILD_TYPE=Release
cmake --build BuildLinux -j 4
ctest --test-dir BuildLinux --output-on-failure
```

This creates a headless Linux test plugin. The custom editor is implemented for
Windows. If you have the official VST3 validator built, run:

```sh
/path/to/validator BuildLinux/Sundrive.vst3
```

`SundrivePreview preview.svg` renders the shared editor artwork to SVG.
`SundrivePreview preview.svg 4 1` renders preset 5 in the bypassed state.
The same layout constants drive the editor hit regions and Windows GUI tests. PNG
conversion is optional and can use CairoSVG or another SVG renderer.

## Source map

| File | Responsibility |
| --- | --- |
| `Source/Parameters.h` | Stable parameter IDs, ranges, names, and six factory presets. |
| `Source/Dsp.h` | Oversampling, filters, waveshapers, smoothing, and bypass alignment. |
| `Source/OversamplingFilter.h` | Checked-in minimum-phase FIR coefficients and fractional alignment coefficient. |
| `Source/VstSupport.h` | VST3 interface includes, class IDs, and string helpers. |
| `Source/Plugin.h`, `Source/Plugin.cpp` | Factory, audio processor, controller, state, automation, and meters. |
| `Source/Artwork.h` | Drawing commands shared by the Windows editor and design preview. |
| `Source/Editor.cpp` | Windows child window, drawing backend, mouse/keyboard controls, and DPI support. |
| `Tests/HostTests.cpp` | Module loading, DSP, automation, saved-state, and Windows editor checks. |
| `Tests/RenderPreview.cpp` | SVG backend for the shared artwork. |
| `Scripts/design_oversampling.py` | Optional reproducible filter design and response comparison, using NumPy and SciPy. |

The audio callback uses preallocated filter histories and parameter queues. It
does not allocate memory, acquire a mutex, access the filesystem, or send UI
messages. Meter messages are polled from the editor's UI timer; the audio thread
only updates atomic meter envelopes.

The 129-tap FIR stages run at 4× sample rate around the nonlinearity. A full-length
minimum-phase conversion retains the v1.0 prototype's magnitude response. Its
round-trip low-frequency delay is about 4.824 base-rate samples. A first-order
allpass supplies the remaining fractional delay, making the delay exactly 5
samples at DC. The processor reports 5 samples and delays bypass by 5 samples.
Minimum-phase group delay varies with frequency; the identity-path impulse peak
is at sample 6. This is a real filter change, not simply a smaller host latency
number. Keep class IDs and parameter IDs unchanged when making compatible updates
so saved sessions continue to load.

The generated coefficients are included and building needs no Python packages.
To regenerate them for development, install NumPy and SciPy 1.14 or newer, then run:

```sh
python Scripts/design_oversampling.py
```

The script writes `Source/OversamplingFilter.h` and a response comparison in
`Tests/Oversampling-design.json`. It uses the full-length `half=False` conversion:
https://docs.scipy.org/doc/scipy/reference/generated/scipy.signal.minimum_phase.html

Official VST3 documentation: https://steinbergmedia.github.io/vst3_dev_portal/
