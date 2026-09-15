# Boomify Alpha 1 - Definition of done

Alpha 1 is not another drum-machine demo. It is done only when a user can make a small complete track from an empty project without external samples or plugins.

## Required workflow

1. Create drums with native Kick / Snare / Clap / Hi-Hat.
2. Create a bass line with a native synth and piano-roll style editor.
3. Create a melody with the native synth.
4. Create multiple patterns.
5. Arrange patterns over multiple bars in a simple song timeline.
6. Play the whole arrangement with one transport clock.
7. Save and reopen the complete project as `.boom`.
8. Export the complete arrangement to WAV.

## UX rule

Boomify must remain understandable without knowing FL Studio, Cakewalk, or another DAW. Prefer one obvious workflow and progressive controls over menus and floating windows.

## Engineering rule

Do not ask for user testing of isolated internal bricks. Push intermediate engineering commits as needed, but request a real test only when the end-to-end workflow above is usable.
