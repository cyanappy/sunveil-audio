# Afterglow 1.0.1 validation

20 October 2026, UTC.

Completed:

- Built the native Linux validation module and loaded it through its exported
  VST3 factory into the included host harness.
- Verified parameter metadata, textual values, mono/stereo buses, float32/64
  audio, state recall, invalid-state rejection, zero dry-path delay, infinite
  Freeze tail reporting, and block-independent sample-offset automation.
- Tested DSP at 44.1, 48, 96 and 192 kHz. Verified ping-pong channel alternation,
  octave shifting, Bloom ducking and release, long-decay behavior, Freeze sustain,
  and finite bounded output across all 27 routing/delay/reverb combinations at
  extreme settings.
- Ran a separate allocation detector while processing 200,000 stereo frames,
  parameter changes and a clear-tails command: no heap allocations.
- Cross-compiled the actual Windows x64 plugin and Windows host-test tools using
  MinGW-w64, with static GCC/C++ runtimes. Checked PE architecture, VST3 exports,
  and DLL imports. Only Windows system libraries are imported.
- Rendered default and Details editor states with the same shared artwork code
  used by the Windows editor, and checked control geometry and labels visually.

- Executed editor-model checks for fractional drag movement, fine adjustment,
  event coalescing, size-dependent sensitivity, clamp reversal and redraw regions.
- Compared 52 selected incremental redraws against complete renders of the shared artwork at
  600 x 900 and 1200 x 1800. This checks the dirty regions used by the native UI;
  differences are limited to one 8-bit color level at antialiased edges.
  It does not execute Windows GDI+ or measure actual Windows frame times.
- Compiled additional Windows host checks for immediate click repainting, rapid
  double-click switch presses, precise drags, duplicate automation suppression,
  cancelled gestures and buffer resizing. Those Windows checks remain unexecuted.

See `Tests/native-validation.txt` and `Tests/editor-redraw-validation.txt` for
executed test results.

Remaining:

- The Windows binaries have not been executed in this environment. Windows host
  scanning/playback, native window interaction/responsiveness, high DPI, and real saved-session
  behavior need validation on Windows. `CheckPlugin.bat` runs the included
  Windows host and GUI-control checks; it records its results separately.
- The official Steinberg validator was not available for this build. No
  validator certification or Windows DAW test result is claimed.

The reported zero-sample latency applies to plugin processing. Hardware and host
I/O buffers add their own latency. Reverb pre-delay, diffusion, shimmer windows,
and delay repeats are intentional wet-signal timing.
