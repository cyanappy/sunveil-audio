# Glassgarden 1.0.0 validation

Build date: 2026-10-22. Target: Windows x64 VST3.

## Passed

- Native C++17 Release build and the VST3 factory/processor contract harness.
  The harness loads the compiled Linux validation module through its public
  factory, creates separate processor and controller instances, and tests all
  19 automatable parameters, string conversions, bypass metadata, and host-tempo
  requirements.
- Dry-path equality with no added samples; 32/64-bit processing, mono,
  mono-to-stereo, and stereo at 44.1, 48, 96, and 192 kHz. Direct DSP smoke checks
  also cover 22.05 and 384 kHz.
- State recall, truncated/nonfinite-state rejection, and sample-offset automation
  consistency across different block sizes.
- Granular pitch spectral checks at -12, 0, +12, and +24 semitones. Reverse output
  differs from forward playback, and scatter produces stereo output from mono.
- Freeze stops capture writes and sustains stored audio for eight seconds;
  clearing removes all held grains and makes the invalidated buffer inaudible.
- All six presets at four sample rates; all texture/harmony combinations at
  maximum feedback, density, size, drift, scatter and output, including live
  Reverse/Freeze transitions. Finite, bounded processed output throughout.
- 200,000 stereo frames with automation and a buffer clear: zero audio-thread
  heap allocations, using the allocation-counting executable.
- Knob drag precision, ten-times-finer Shift adjustment, resize sensitivity,
  clamping reversal, coalesced pointer movement, and redraw bounds.
- 46 representative incremental redraw cases compared pixel-for-pixel with
  full redraws at 600 × 900 and 1200 × 1800. Only one 8-bit color level of
  antialias rounding is allowed. Main and Details previews visually inspected.
- Windows x64 plugin and diagnostic executables cross-compiled using GCC 13.2
  / MinGW-w64. PE inspection confirms GetPluginFactory, InitDll and ExitDll.
  Plugin imports only GDI32, gdiplus, KERNEL32, msvcrt, ole32 and USER32; no
  separate GCC, C++ runtime, or JUCE DLL needs to be installed.
- Distinct product class IDs and versioned state identity. The installer copies
  only the Glassgarden bundle into the per-user VST3 folder.

Native test output and Windows binary inspection are in `Validation/`.
Performance numbers in the test log describe the build machine and test signal;
they do not predict CPU use on another computer or a heavily loaded live rig.

## Validation limits

The Windows binary has NOT been executed in Windows or Gig Performer. A Wine
runtime check was attempted, but this environment disallows the local socket
Wine requires. Native Windows mouse events, font rendering, and DAW scanning
therefore remain unverified here. `CheckPlugin.bat` includes a Windows HWND
editor interaction test as well as the processor and allocation checks.

The previews use the actual shared drawing code and embedded faceplate, rendered
through an SVG canvas. They are layout previews, not screenshots from a DAW.
The faceplate is original generated vector artwork; no third-party photographs
or fonts are embedded in the plugin.

A Windows host smoke test and a real guitar listening session are needed before
public distribution: scan/load, play, map footswitches, save/reopen a rackspace,
resize and drag controls while playing, then unload/reload. This release is a
working first build with automated checks, not a claim of host certification.
