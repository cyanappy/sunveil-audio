# Changes

## 1.3.2 — Original v1.3 design, clearer text

- Restored the exact v1.3 enclosure, knobs, artwork, colors and control layout.
- Slightly enlarged labels, mode text, values, preset and bypass status.
- Darkened numeric values and mode text for readability.
- No hardware shading, artwork, hit region or audio engine changes from v1.3.

## 1.3.0

- Implemented the saved stompbox design as a live 600 × 900 portrait editor.
- Ivory enclosure, copper sunrise, six fluted knobs, miniature toggle switches,
  amber status LED, chrome footswitch, and corner screws.
- Voice toggles between two modes; Clipping cycles through all three characters.
- Preserved resizing, DPI support, fine adjustment, reset, keyboard controls, presets,
  automation, class IDs, saved-state format, DSP, and five-sample reported latency.
- Updated the optional Windows UI tests for the cycling switches.

## 1.2.0

- Redesigned the editor for Sunveil Audio with ivory, charcoal, copper, and a small
  sunrise mark.
- Added a company side panel with vertical meters and the bypass footswitch;
  enlarged Drive and separated the EQ controls.
- Updated host manufacturer metadata to Sunveil Audio and retained plugin class IDs.
- Kept the v1.1 audio engine, presets, parameter IDs, and 5-sample reported latency.

## 1.1.0

- Reduced reported latency from 32 to 5 samples while retaining 4× oversampling.
- Converted both oversampling FIRs to minimum phase and added fractional
  alignment; preserved the prototype's magnitude response and anti-alias rejection.
- Updated the dry bypass delay to 5 samples and retained the smooth crossfade.
- Added measured group-delay and frequency-response checks and a reproducible
  coefficient design script.
- Kept the same plugin class IDs, parameter IDs, presets, and saved-state format.

The filter phase response changes in this version. At 48 kHz the host-compensated
latency is approximately 0.104 ms; the identity-path impulse peak is at 0.125 ms.
