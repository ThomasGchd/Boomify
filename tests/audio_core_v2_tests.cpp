#include "../src/audio_core_v2.hpp"
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <vector>

void check(bool condition) { if (!condition) std::abort(); }

using boomify::audio::FloatGraph;
using boomify::audio::Note;

int main() {
    const Note first{128, 512, 220.0f, 0.5f, 0};
    const Note second{256, 400, 440.0f, 0.4f, 1};
    FloatGraph whole, chunked;
    whole.prepare({first, second});
    chunked.prepare({second, first});
    constexpr size_t frames = 2048;
    std::vector<float> continuous(frames * 2, 123.0f);
    std::vector<float> fragmented(frames * 2, 123.0f);
    whole.process(continuous.data(), frames);
    for (size_t pos = 0; pos < frames; pos += 64)
        chunked.process(fragmented.data() + pos * 2,
                        std::min<size_t>(64, frames - pos));
    for (size_t i = 0; i < continuous.size(); ++i) {
        check(std::isfinite(continuous[i]));
        check(continuous[i] == fragmented[i]); // block-size independence
        check(continuous[i] >= -1.0f && continuous[i] <= 1.0f);
    }
    for (size_t i = 0; i < first.frame * 2; ++i)
        check(continuous[i] == 0.0f); // exact scheduled start
    check(continuous[130 * 2] != 0.0f);
    for (size_t i = 660 * 2; i < continuous.size(); ++i)
        check(continuous[i] == 0.0f); // no stuck voices
    check(whole.position() == frames);

    // Mute through gain does not require a new render plan.
    FloatGraph muted;
    muted.prepare({first});
    muted.setTrackGain(0, 0.0f);
    std::vector<float> zero(frames * 2, 1.0f);
    muted.process(zero.data(), frames);
    for (float v : zero) check(v == 0.0f);

    // Identical output across loop boundaries at a fixed sample position.
    FloatGraph looping;
    looping.prepare({Note{0, 128, 1000.0f, 0.3f, 0}});
    looping.setLoop(0, 256);
    std::vector<float> loops(512 * 2);
    looping.process(loops.data(), 512);
    for (size_t i = 0; i < 256 * 2; ++i)
        check(loops[i] == loops[i + 256 * 2]);
    check(looping.position() == 256);

    // Invalid notes are discarded without invalid output.
    FloatGraph safe;
    safe.prepare({Note{0, 64, -1.0f, 1.0f, 0}, Note{0, 0, 220.0f, 1.0f, 0},
                  Note{0, 64, NAN, 1.0f, 0}, Note{0, 64, 220.0f, 1.0f, 99}});
    std::vector<float> invalid(512, 1.0f);
    safe.process(invalid.data(), 256);
    for (float v : invalid) check(v == 0.0f);

    // Notes are audible while the song transport is stopped, without seek/play.
    FloatGraph preview;
    preview.prepare({});
    check(preview.audition(0, 440.0f, 0.1f, 0.45f));
    std::vector<float> auditioned(8192 * 2);
    preview.process(auditioned.data(), 8192);
    bool hasAudibleSound = false;
    for (float sample : auditioned)
        if (std::abs(sample) > 0.001f) hasAudibleSound = true;
    check(hasAudibleSound);
    for (size_t i = 6000 * 2; i < auditioned.size(); ++i)
        check(auditioned[i] == 0.0f);

    // Gain is live: editing it takes effect in the next block without resetting
    // the transport or the currently playing note.
    FloatGraph realtime;
    realtime.prepare({Note{0, 6000, 440.0f, 0.7f, 0}});
    std::vector<float> audibleBlock(512 * 2), silenced(512 * 2), restored(512 * 2);
    realtime.process(audibleBlock.data(), 512);
    check(std::any_of(audibleBlock.begin(), audibleBlock.end(), [](float x) { return std::abs(x) > 0.01f; }));
    realtime.setTrackGain(0, 0.0f);
    realtime.process(silenced.data(), 512);
    check(std::all_of(silenced.begin(), silenced.end(), [](float x) { return x == 0.0f; }));
    realtime.setTrackGain(0, 1.0f);
    realtime.process(restored.data(), 512);
    check(std::any_of(restored.begin(), restored.end(), [](float x) { return std::abs(x) > 0.01f; }));
    check(realtime.position() == 1536);
    check(!preview.audition(-1, 440.0f));
    check(!preview.audition(0, NAN));

    std::cout << "Boomify Audio V2 basic invariants OK\n";
    return 0;
}
