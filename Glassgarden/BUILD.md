# Building Glassgarden

> SOURCE REPOSITORY NOTE: This folder contains Glassgarden 1.0.0 source and build inputs. Build with build_windows.bat; output is in Build/. Prebuilt VST3 bundles and Tools executables are excluded. The original release documentation below describes a prebuilt distribution. Install.bat and CheckPlugin.bat are release-packaging helpers; use the build instructions in the repository root README for this checkout.

The distributed `Glassgarden.vst3` is already compiled. Building is optional.
The source uses C++17 and bundled VST3 interface headers; no JUCE dependency.

## Windows x64

Use Visual Studio 2022 with Desktop development with C++, Windows SDK and CMake.
Open an x64 developer command prompt and run `build_windows.bat`. It configures
CMake, builds Release, and runs the host, DSP, allocation and interaction tests.
The bundle is `Build/Glassgarden.vst3`.

Alternatively:

```powershell
cmake -S . -B Build -G "Visual Studio 17 2022" -A x64
cmake --build Build --config Release --parallel
ctest --test-dir Build -C Release --output-on-failure
```

An existing MinGW-w64 POSIX x64 toolchain on Linux can cross-build the release:

```sh
python3 Scripts/cross_build.py --mingw-root /absolute/toolchain/prefix --out /absolute/build/windows
```

The cross-build statically links the C++ and MinGW runtimes. Only Windows system
DLLs are imported. Distribute the notices alongside the plugin.

## Headless checks on Linux

```sh
cmake -S . -B BuildNative -DCMAKE_BUILD_TYPE=Release
cmake --build BuildNative --parallel
ctest --test-dir BuildNative --output-on-failure
```

The Linux module exists for processor contract tests. It has no native editor
and is not an end-user Linux release.

`Tests/HostTests.cpp` loads the module through its exported VST3 factory. On
Windows it also attaches the actual HWND editor, exercises mouse and keyboard
interaction, checks automation gestures, and resizes the cached drawing surfaces.
`CheckPlugin.bat` runs the distributed Windows executables against the bundle.

## Artwork and redraw checks

`python3 Scripts/make_faceplate.py` regenerates the editable faceplate SVG.
Rasterize it to `Resources/Faceplate.png` at 1024 × 1536 with an SVG renderer
(e.g. Sharp). This PNG is embedded by `Resources/Glassgarden.rc`. Main readouts,
knob rotation, toggles, LEDs, focus rings, the preset bar, and Details are rendered
live by `Source/Artwork.h` through both native GDI+ and the preview SVG canvas.

```sh
BuildNative/GlassgardenPreview Resources/Faceplate.png preview.svg
BuildNative/GlassgardenPreview Resources/Faceplate.png details.svg 0 1
BuildNative/GlassgardenPreview Resources/Faceplate.png redraw --regression
node Tests/check_editor_redraw.js redraw Resources/Faceplate.png --controls
```

The last command needs the Sharp package. Rendered incremental redraws are
compared to full redraws at 600 × 900 and 1200 × 1800. The one-level color
allowance handles antialias compositing rounding only.

## Product identity

Processor: `09B4F80A0BF04FBA9A218B873AD15A47`  
Controller: `88913B6C10ED4371A9AD9A711D129AFF`  
State magic: `GGD1`, version 1, 19 normalized double parameters.

IDs are distinct from Afterglow and Sundrive. Keep them stable for compatible
Glassgarden updates; use a new product ID for a separate plugin. The state stores
controls, never live capture audio. All audio processing and clear operations
avoid heap allocation; preparation allocates the six-second stereo buffer.
