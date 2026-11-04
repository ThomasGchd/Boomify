# Boomify Studio — Alpha 6 (Windows)

Boomify is an experimental Windows DAW built around a readable arrangement timeline, a 64-step drum sequencer and a native piano roll.

## Build

Double-click `build.bat` on Windows 10/11. The build uses Visual Studio 2022 C++ Build Tools, CMake 3.25+ and Git. The script can install missing build tools and Git through WinGet. Internet access is required for the first VST3 SDK download. The Steinberg SDK sources are cached in `.boomify_dependencies` so subsequent builds reuse them.

The build produces **two** executables in `build/Release`:

- `Boomify.exe`: main DAW.
- `BoomifyVST3Host.exe`: separate VST3 instrument rendering helper.

Keep the two executables together when moving the application. The auto-update release ships both.

## Piano roll

Select an instrument track (e.g. Bass or Synth). Click an empty cell to add a note and hear an immediate **native instrument** preview. Click an existing note to audition it without deleting it; right-click the note to erase it. Drag while creating to change its duration. Click on the piano-key strip to audition pitches. Press Space to Play/Pause and use Stop to return to the stopped state.

## External VST3 instruments (early support)

1. Select a **Bass / Synth / Instrument** track.
2. At the bottom of the inspector, choose **CHOISIR / IMPORTER VST3**.
3. Select **Rechercher les VST3 installes** to scan common Windows VST3 locations, or import a specific `.vst3` file/bundle directory.
4. Open the menu again to choose the installed instrument. Press Play to render its MIDI notes through the isolated VST3 host. WAV export uses the same external instrument rendering.
5. Use **REVENIR AU SON NATIF** to restore the built-in instrument.

Plugin paths are stored in an optional trailer of the existing `.boom` project format. Previously saved projects still open. If a plugin is unavailable or fails to render, Boomify falls back to the native instrument and marks the render as failed in the inspector.

**Current alpha limitations:** VST3 **instruments** with 32-bit floating-point audio processing and MIDI note input are supported in offline rendering. This is not yet a low-latency live host. At present, piano-roll note audition always uses the native sound, even on VST3 tracks; VST3 plugin editors, saved plugin parameters/presets and external VST3 audio effects are not yet implemented. VST2, AU and AAX are not supported. No third-party VST3 plugin is bundled.

Loading third-party plugins runs third-party code with your user privileges. Boomify runs them in a separate helper process to limit the impact of a plugin crash, but this is not a security sandbox. Only use plugins from sources you trust.

## Build without VST3

For developers who want just the native DAW, configure CMake with `-DBOOMIFY_ENABLE_VST3=OFF` and build `Boomify`. The standard `build.bat` enables VST3 support.

For third-party acknowledgements, see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
