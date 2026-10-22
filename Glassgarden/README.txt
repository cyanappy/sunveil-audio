SOURCE REPOSITORY NOTE: This folder contains Glassgarden 1.0.0 source and build inputs. Build with build_windows.bat; output is in Build/. Prebuilt VST3 bundles and Tools executables are excluded. The original release documentation below describes a prebuilt distribution. Install.bat and CheckPlugin.bat are release-packaging helpers; use the build instructions in the repository root README for this checkout.

SUNVEIL AUDIO — GLASSGARDEN 1.0.0
Windows x64 VST3 · granular pitch and stereo cloud pedal

START HERE
1. Extract the entire ZIP to a normal folder.
2. Close Gig Performer or your DAW, then double-click Install.bat.
3. Rescan VST3 plugins and load Sunveil Audio > Glassgarden.

Install.bat copies the complete Glassgarden.vst3 folder to:
  %LOCALAPPDATA%\Programs\Common\VST3\Glassgarden.vst3
If your host does not scan this location, add
  %LOCALAPPDATA%\Programs\Common\VST3
to its plugin search paths. You can instead copy the complete Glassgarden.vst3
folder manually into C:\Program Files\Common Files\VST3 (admin access needed).
Keep the Contents subfolder intact. The inner .vst3 file alone is not the bundle.

This is a compiled Windows x64 VST3 effect. It needs a compatible audio host and
an audio interface/input for live guitar. No account, activation, subscription,
or internet connection is required. No standalone app, VST2, AU, or macOS build
is included. The host controls audio device, buffer size, and MIDI mapping.

FIRST SOUND
Start with Crystal Canopy. Play a sustained chord or single-note line. Mix at
25–45% leaves your attack in place while pitched fragments drift behind it.
Try +12 st for shimmering octave clouds, +7 st for fifths, or -12 st for low
textures. Play for a few seconds, then press FREEZE to hold the captured phrase.
Press FREEZE again to record new input. REVERSE changes the direction of newly
launched grains. CLEAR releases Freeze and empties the cloud.

MAIN CONTROLS
PITCH       -24 to +24 semitones; continuous, with fine tuning using Shift.
SIZE        15–350 ms per grain. Small = fractured; large = smoother fragments.
SCATTER     0–100%. Adds random lookback (up to 1.6 s), stereo pan, and timing.
DENSITY     2–90 grains/second in Free mode. Host Sync replaces this interval
            with a rhythmic note division; the saved Free value is retained.
FEEDBACK    0–88%. Recaptures filtered grain output for evolving layers.
MIX         Equal-power dry/wet blend. 0% keeps the direct path at unity.

TEXTURE     MIST: soft overlapping windows.
            GLASS: rounded, defined grains.
            SHARDS: sharper windows with varied grain lengths.
HARMONY     SOLO: main pitch only.
            FIFTH: adds a voice 7 semitones above the main Pitch.
            OCTAVE: adds a voice 12 semitones above the main Pitch.

FOOTSWITCHES
ENGAGE      Turns processing on/off. Green LED = engaged. Bypass fades smoothly
            to direct input. The capture engine continues running while bypassed.
FREEZE      Amber LED = capture is held. Incoming audio no longer replaces the
            stored phrase; overlapping grains continue playing from that phrase.
REVERSE     Green LED = new grains play backwards. Existing grains finish their
            current direction so switching does not abruptly cut them off.

DETAILS PANEL
TONE        800–16000 Hz low-pass filter on the cloud.
DRIFT       0–100% pitch variation and slow movement.
WIDTH       0–150% stereo width. 0% centers the wet signal.
LOW CUT     20–800 Hz high-pass filter before capture.
PRE-DELAY   Up to 300 ms additional grain lookback.
OUTPUT      -18 to +6 dB; affects the engaged output. Bypass stays at unity.
HOST SYNC   Grain launches follow host tempo when the host supplies tempo data.
            Default tempo is 120 BPM until a valid tempo is received.
DIVISION    Quarter, eighth, sixteenth, thirty-second, or eighth-note triplet.
CAPTURE     Live / Frozen button, equivalent to the FREEZE footswitch.
CLEAR       Empty the capture buffer, stop grains, and release Freeze.

PRESETS
Crystal Canopy    Bright octave cloud; the starting preset.
Glass Petals      Short, scattered fifths.
Prism Cathedral  Dense, soft octave layers.
Reverse Ferns    Low reversed grains with an added fifth.
Broken Sunlight  Sharp, sparse fragments with wide scatter.
Weightless       Long grains and slowly moving layers.

Presets reset their sound controls and Freeze; your Engage/Bypass state stays
as it was. Custom settings are saved by the host in its project/rackspace.
Freeze/Reverse are saved as parameter states. Captured audio itself is NOT saved:
a reopened project starts with an empty buffer, even if Freeze was saved on.
Release Freeze and play to refill it. CLEAR is a momentary action, not a saved
or automatable parameter. Map Freeze, Reverse, and Bypass through your host.

MOUSE AND KEYBOARD
Drag a knob up/down. Hold Shift for ten-times-finer movement. Use the wheel
for small changes. Double-click a knob to reset it to its starting value.
Right-click the editor or press A to show/hide Details. Click the top preset
name or press P to open presets. Tab/Shift+Tab selects parameters; arrows edit;
Home resets the selected parameter. Space/B = bypass, F = freeze, R = reverse,
C = clear. Shortcuts work while the editor has focus. Default size: 600 x 900;
the host can resize it from 400 x 600 to 1200 x 1800.

LATENCY AND SOUND
The plugin reports zero additional host compensation samples. The direct path
has no lookahead. The wet sound is made from past input, so it naturally arrives
later: grain size, pitch ratio, scatter, and pre-delay all affect that timing.
Audio-interface and host buffers still add their normal live monitoring delay.
This is a creative granular effect. Sparse grains can pulse, high pitch shifts
can sound rough, and Freeze is a sustained texture rather than a phrase looper.
Grain lookback is random; Scatter is not an exact fixed delay. Startup grains
are shortened until enough input has been captured. The buffer holds 6 seconds.
A ceiling in the processed path controls runaway feedback; hot signals may be
softly saturated. Lower input level, Feedback, or Output for a cleaner result.

TROUBLESHOOTING
Not found: scan the per-user path above, or use the system VST3 folder. Restart
and rescan the host. Confirm the host is Windows 64-bit and supports VST3.
No wet audio: release Freeze, play for a few seconds, check Mix and Engage.
MIDI pedal: map the host's MIDI control to Bypass, Freeze, or Reverse. These are
automatable VST3 parameters; there is no direct MIDI event input on the plugin.
For a diagnostic log, close the host and run CheckPlugin.bat. This launches a
small test-host window and writes Glassgarden-Windows-check.txt in this folder.

CONTENTS AND VALIDATION
Glassgarden.vst3     Compiled Windows x64 bundle.
Tools/              Standalone diagnostic executables (not needed for normal use).
Source/, Resources/ Editable C++ and original faceplate assets.
Tests/, Scripts/    Reproducible checks and cross-build tooling.
VALIDATION.md       Exact checks performed and limits of this build's validation.
BUILD.md            Rebuild instructions for developers.
Glassgarden-preview.png and Glassgarden-details.png show the shared UI artwork.

Copyright 2026 Sunveil Audio. Project source is provided under LICENSE.txt.
Third-party licenses and runtime notices are included in ThirdPartyNotices/.
