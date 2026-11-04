#include "../src/audio_core_v2.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

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
        assert(std::isfinite(continuous[i]));
        assert(continuous[i] == fragmented[i]); // block-size independence
        assert(continuous[i] >= -1.0f && continuous[i] <= 1.0f);
    }
    for (size_t i = 0; i < first.frame * 2; ++i)
        assert(continuous[i] == 0.0f); // exact scheduled start
    assert(continuous[130 * 2] != 0.0f);
    for (size_t i = 660 * 2; i < continuous.size(); ++i)
        assert(continuous[i] == 0.0f); // no stuck voices
    assert(whole.position() == frames);

    // Mute through gain does not require a new render plan.
    FloatGraph muted;
    muted.prepare({first});
    muted.setTrackGain(0, 0.0f);
    std::vector<float> zero(frames * 2, 1.0f);
    muted.process(zero.data(), frames);
    for (float v : zero) assert(v == 0.0f);

    // Identical output across loop boundaries at a fixed sample position.
    FloatGraph looping;
    looping.prepare({Note{0, 128, 1000.0f, 0.3f, 0}});
    looping.setLoop(0, 256);
    std::vector<float> loops(512 * 2);
    looping.process(loops.data(), 512);
    for (size_t i = 0; i < 256 * 2; ++i)
        assert(loops[i] == loops[i + 256 * 2]);
    assert(looping.position() == 256);

    // Invalid notes are discarded without invalid output.
    FloatGraph safe;
    safe.prepare({Note{0, 64, -1.0f, 1.0f, 0}, Note{0, 0, 220.0f, 1.0f, 0},
                  Note{0, 64, NAN, 1.0f, 0}, Note{0, 64, 220.0f, 1.0f, 99}});
    std::vector<float> invalid(512, 1.0f);
    safe.process(invalid.data(), 256);
    for (float v : invalid) assert(v == 0.0f);

    std::cout << "Boomify Audio V2 basic invariants OK\n";
    return 0;
}
