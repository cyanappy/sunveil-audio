# Afterglow 1.0.1

- Reuse drawing buffers at the editor's actual pixel size.
- Cache the scaled faceplate, decoded knob sprites and UI fonts.
- Repaint only changed controls instead of scaling the whole pedal every frame.
- Send knob changes directly to host automation while coalescing visual frames.
- Keep fractional, size-aware drag movement and ten-times-finer Shift control.
- Repaint button clicks and the final drag value immediately.
- Recognize rapid switch clicks delivered as Windows double-click messages.
- Add visible Tap/Clear Tails feedback and avoid unused meter requests.
- Skip unchanged automation values and close gestures on capture loss/cancellation.

The sound engine, zero-sample reported processing latency, plugin IDs, parameter
IDs, presets and saved-state format are unchanged from 1.0.0.
