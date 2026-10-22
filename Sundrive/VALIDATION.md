# Validation record — Sundrive 1.3.2

21 October 2026 (Asia/Manila).

## Completed for this release

This release uses the original 1.3.0 source with text size, weight and contrast
changes in Artwork.h, plus version metadata and documentation updates.
The audio engine, control geometry and interaction behavior are unchanged.

- Compiled the native Linux module from current source and ran the included host
  test harness: 398,193 checks passed. It exercises parameters, state, automation,
  buses, latency, precision, numerical robustness, oversampling alignment, and EQ.
- Cross-compiled the Windows x64 VST3 and optional Windows host-test executable
  with MinGW-w64 GCC 13.2.0 and static C++/GCC runtimes.
- Inspected the Windows binary exports and runtime DLL imports.
- Rendered and visually reviewed the shared editor artwork in engaged and bypassed
  states, with different presets. The preview uses the same drawing commands,
  geometry, and parameter values as the Windows editor, through an SVG backend.
- Confirmed the DSP and processor implementation are byte-identical to v1.2.

## Remaining checks

The Windows binaries have not been executed here. Load the plugin in your Windows
DAW/host and check scanning, editor opening, mouse and keyboard interaction,
resizing/high DPI, saved sessions, and guitar playback. CheckPlugin.bat runs the
included Windows harness and captures two editor screenshots on your computer.
The Windows harness includes the new toggle-cycle checks; those GUI checks are
compiled but have not been run.

The official Steinberg validator was not available in this environment for this
release. Tests/Steinberg-validator-1.2.0-historical.txt records the prior v1.2 run,
which passed 47 tests. It is historical evidence, not a new v1.3 validation run.

## Latency

The unchanged engine reports five samples, approximately 0.104 ms at 48 kHz,
and retains 4× oversampling. The impulse peak is at sample six and the measured
DC group delay is five samples; minimum-phase delay varies with frequency.
