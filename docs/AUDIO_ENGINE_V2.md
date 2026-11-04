# Boomify — Audio Engine V2 migration

## Decision

Keep Boomify as a small, offline-first, Windows-native DAW. Do **not** introduce
a server or browser front end for audio processing.

Use [miniaudio](https://github.com/mackron/miniaudio) (public domain / MIT-0)
as the future audio-device backend. On Windows, prefer WASAPI with a float32
callback. This is not a complete DAW engine: Boomify must still provide an
event scheduler, voice management, instrument synthesis, sample playback,
effects, buses, automation, mastering, project model and reliable export.

Do not simply port the existing `waveOut` buffer to miniaudio. The existing
system builds a whole song in an interleaved `int16_t` buffer, routes its
synthesized kick separately, applies multiple downstream processors and often
re-renders when an edit is made. Those design constraints cause wasted memory,
awkward editing and avoidable discontinuities.

## Architecture target

```text
.boom parser (BOOMIFY17/18 accepted)
    |
    v
Immutable arrangement snapshot + live parameter queue
    |
    v
Sample-accurate transport / clip scheduler
    |
    +-- Instrument voices (float32, ADSR)
    +-- Drums and Kick Designer (same event schedule)
    +-- Decoded sample clips (streaming, resampled when necessary)
    |
    v
Track gain/pan -> per-track FX chain -> group/master buses
    |
    +--> WASAPI device callback via miniaudio (real-time)
    +--> WAV renderer using the SAME signal path (offline)
```

Important: Keep UI and disk I/O off the audio callback thread. Decode
files and build immutable event snapshots outside the callback; no
allocation, locks, logging, dialogs or file operations in a real-time callback.
Keep per-track FX state continuous between adjacent audio blocks. The project
file format must not expose internal engine implementation details.

## Phase gates

- [x] Independent float32 block-renderer prototype added
- [x] Unit tests for note start frame, chunk-size consistency, looping, mute,
      invalid input and release of expired voices
- [ ] Windows CI proves the isolated module compiles and tests pass
- [ ] Integrate a pinned miniaudio version, preserving old playback as fallback
- [ ] Reconnect all drum steps, fine notes, samples, kick envelopes and FX
      through one stable timeline, without mutating the project during render
- [ ] Playback and WAV export share one processing graph
- [ ] Sample-accurate play/stop/seek/loop without regenerating the whole song
- [ ] Instrument/filter/FX states stay continuous between callbacks
- [ ] Measure memory use and callback timing (44.1/48 kHz; 128/256/512 frames)
- [ ] Add golden WAV tests: deterministic kick start, bass/kick alignment,
      per-bar independent 64-step drums, hat audible, solo/mute, FX order
- [ ] Validate loading old `.boom` projects without converting them in place
- [ ] Switch default playback only after the candidate is measurably better

## Licensing and scope

- miniaudio is permissively licensed; retain required notices if bundled.
- Tracktion Engine and JUCE provide significantly more infrastructure, but
  require major adaptation and a separate GPL/AGPL/commercial licence review.
- Time-stretching, MP3, VST3/CLAP hosting and ASIO support are distinct
  future integration decisions, not automatic benefits of replacing `waveOut`.
- Audio V2's current sine-wave prototype is **not** the finished synth engine:
  it validates real-time invariants before porting instruments and FX.

## Compatibility

The `.boom` file remains the user-facing project format. This migration does
not change the writer or the existing BOOMIFY17 / BOOMIFY18 reader.
Forward compatibility with old projects is mandatory; reopening a saved
project must reproduce its notes, rhythms and parameter values.
