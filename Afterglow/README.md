# Sunveil Audio — Afterglow 1.0.1

> SOURCE REPOSITORY NOTE: This folder contains Afterglow 1.0.1 source and build inputs. Build with build_windows.bat; output is in Build/. Prebuilt VST3 bundles and Tools executables are excluded. The original release documentation below describes a prebuilt distribution. Install.bat and CheckPlugin.bat are release-packaging helpers; use the build instructions in the repository root README for this checkout.

Windows x64 VST3 development build. The compiled plugin is included in
`Afterglow.vst3`; you can install it without installing a compiler.

Afterglow combines stereo delay and atmospheric reverb in a purple stompbox
interface based on Sunveil Audio's new Sundrive design. Its plugin IDs are
independent of Sundrive, so both products can be installed together.

## Controls update in 1.0.1

- Cached faceplate and knob graphics, reusable drawing buffers, and repainting
  only the changed areas reduce UI work during dragging.
- Knob movement retains fractional precision, with consistent sensitivity at
  different editor sizes and ten-times-finer movement while holding Shift.
- Local control changes reach the host on each mouse event. Visual updates are
  coalesced around 120 Hz during continuous dragging; the final release and
  button clicks repaint immediately. Host-driven changes are checked every 16 ms.
- Rapid switch clicks are recognized, including the second click of a double-click.
  Tap and Clear Tails flash briefly so every press has visible feedback.
- Repeated values at knob limits no longer send redundant host automation.

Plugin IDs, parameter IDs, saved-state format and the sound engine are unchanged
from 1.0.0. To update, close your host completely and run the new `Install.bat`
from this extracted package, replacing the installed Afterglow bundle.

## Install and use

1. Extract this entire ZIP to a normal folder on your computer.
2. Close your audio host and run `Install.bat`.
3. Open Gig Performer 5 or another Windows VST3 host and rescan plugins.
4. Load **Afterglow** by **Sunveil Audio**, normally after your amp/cabinet block.

The installer copies the complete `Afterglow.vst3` folder to:

`%LOCALAPPDATA%\Programs\Common\VST3\Afterglow.vst3`

If your host does not scan that user folder automatically, add
`%LOCALAPPDATA%\Programs\Common\VST3` to its plugin search locations and rescan.
You can instead copy the complete `Afterglow.vst3` folder to
`C:\Program Files\Common Files\VST3` manually; that location normally needs
administrator permission. Install one copy in your chosen location.

The `.vst3` item is a **folder/bundle** containing a Windows binary and metadata.
Copy the whole folder, including its Contents directory.

## Controls

| Control | Range / behavior |
| --- | --- |
| Time | 40–1,800 ms in free time; host sync can reach 4 seconds |
| Repeats | 0–93% delay feedback |
| Delay mix | Dry through fully wet, with an equal-power blend |
| Decay | 0.8–40-second decay setting; damping and shimmer affect the audible tail |
| Shimmer | Blends octave-up pitch shifting into the reverb network |
| Reverb mix | Dry through fully wet, with an equal-power blend |
| Delay type | Tape / Digital / Ping Pong |
| Reverb type | Hall / Bloom / Shimmer |
| Delay footswitch | Enables/disables the delay input; Trails controls spillover |
| Freeze footswitch | Captures and sustains the current reverb tail; press again to release |
| Reverb footswitch | Enables/disables reverb input; Trails controls spillover |

**Bloom** ducks the reverb while you play and lets it rise into the gaps.
**Freeze** needs an existing tail: play a chord first, then engage Freeze.
Freeze stops feeding new notes into the held reverb until you release it.
Audio tail buffers are not serialized into sessions. Release a recalled Freeze
state before feeding fresh notes if no tail has yet been captured.

Click **DETAILS** to adjust delay tone, tape drift, reverb tone, motion, pre-delay
(0–180 ms), output (−24 to +6 dB), routing, tempo sync and Trails. Routing offers
Delay → Reverb, Reverb → Delay, and Parallel. The Clear Tails button releases
Freeze and clears both engines.

Host sync uses the DAW's supplied tempo, with quarter, dotted-eighth, eighth,
half, and eighth-triplet divisions. If the host supplies no tempo, the engine
uses 120 BPM. Tap twice or more to set free delay time; turning Time also exits
host sync. The Tap button and `T` key measure successive taps.

Presets: Violet Haze, Soft Focus, Dream Cathedral, Slow Bloom, Orbiting Light,
and Silver Room. Select a preset from the dropdown above the pedal.

### Interaction

- Drag a knob vertically; hold Shift for fine changes.
- Use the mouse wheel over a knob, or double-click to restore its factory default.
- Tab selects a parameter; arrow keys change it; Home restores its default.
- Space or B: master bypass. F: Freeze. D/R: individual effect switches.
- P: presets. A or right-click: Details. T: tap delay time.
- The editor can be resized while preserving its aspect ratio.

Master bypass returns the unchanged dry input. Individual section switches can
spill their tails when Trails is on. Output has a soft peak ceiling on the
processed path. All 22 parameters support host automation and session recall.

## Latency and formats

- Reported processing latency: **0 samples**; the dry signal is processed in the
  current sample. Your audio interface and host buffer still determine I/O latency.
- 32-bit and 64-bit floating-point host audio.
- Mono → mono, mono → stereo, and stereo → stereo.
- Sample rates from 22.05 to 384 kHz; functional tests cover 44.1/48/96/192 kHz.
- The host tail is reported as infinite so held Freeze tails are retained.
- Audio buffers are allocated when preparing the engine, outside the sample loop.

## Validation and build status

The Windows x64 VST3 was cross-compiled successfully. Native headless VST3 host
tests and DSP tests passed, including sample alignment, mode stability, stereo
echoes, octave shifting, Bloom ducking, Freeze sustain, state recall, and
sample-offset automation. Editor-model tests cover fine dragging, event coalescing,
limits and resize sensitivity. Incremental redraws are compared with full
renders at two sizes using the shared artwork code. A separate realtime test verifies zero heap allocations
during processing, automation and clearing tails. The editor layout was rendered
from the same artwork code used by the Windows editor and visually checked.

**Windows DAW execution has not been tested in this build environment.** Run
`CheckPlugin.bat` on Windows to execute the included native host and editor tests,
then check scanning, playback, controls, saved sessions and high-DPI behavior in
your host. See `VALIDATION.md` and `Tests/native-validation.txt` for details.

## Rebuild from source

The full C++ source, artwork and VST3 interface headers are included. To rebuild
on Windows, install Visual Studio 2022 / Build Tools with Desktop development
with C++, the Windows SDK, and CMake tools; then run `build_windows.bat` from a
developer command prompt. Build output is `Build\Afterglow.vst3`.

This project uses the VST3 interfaces directly and a native Windows editor.
No JUCE dependency is required. Licensing information is in `LICENSE.txt`,
`ThirdPartyNotices`, and `third_party/pluginterfaces/LICENSE.txt`.
