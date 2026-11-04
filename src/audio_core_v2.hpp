#pragma once
// Boomify Audio V2 prototype: deterministic 32-bit-float block renderer.
// This module has no Windows dependencies and performs no heap allocation in process().
// Playback device integration will follow only after unit tests and audio comparisons.
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace boomify::audio {
constexpr int kMaxTracks = 16;
constexpr int kMaxVoices = 64;
constexpr float kPi2 = 6.2831853071795864769f;

struct Note {
    std::uint64_t frame = 0;
    std::uint64_t length = 0;
    float frequency = 220.0f;
    float velocity = 0.5f;
    int track = 0;
};

class FloatGraph {
public:
    explicit FloatGraph(unsigned sampleRate = 44100)
        : sampleRate_(sampleRate ? sampleRate : 44100) {
        for (auto& g : gains_) g.store(1.0f, std::memory_order_relaxed);
    }

    // Only call on the control thread while the audio device is stopped.
    void prepare(std::vector<Note> notes) {
        notes.erase(std::remove_if(notes.begin(), notes.end(),
            [](const Note& n) {
                return n.track < 0 || n.track >= kMaxTracks ||
                       n.length == 0 || !(n.frequency > 0.0f) ||
                       !std::isfinite(n.frequency) || !std::isfinite(n.velocity);
            }), notes.end());
        std::stable_sort(notes.begin(), notes.end(),
            [](const Note& a, const Note& b) { return a.frame < b.frame; });
        notes_ = std::move(notes);
        seek(0);
    }

    // Audition one note without starting or seeking the song transport.
    // Single-producer (UI) to single-consumer (audio callback) lock-free queue.
    // Returns false rather than blocking when the queue is full.
    bool audition(int track, float frequency, float seconds = 0.24f,
                  float velocity = 0.45f) noexcept {
        if (track < 0 || track >= kMaxTracks || !(frequency > 0.0f) ||
            !std::isfinite(frequency) || !(seconds > 0.0f) ||
            !std::isfinite(seconds) || !std::isfinite(velocity)) return false;
        const auto write = previewWrite_.load(std::memory_order_relaxed);
        const auto read = previewRead_.load(std::memory_order_acquire);
        if (write - read >= kPreviewQueueCapacity) return false;
        Note note{};
        note.track = track;
        note.frequency = std::min(frequency, sampleRate_ * 0.45f);
        note.velocity = std::clamp(velocity, 0.0f, 1.0f);
        note.length = std::max<std::uint64_t>(1, (std::uint64_t)(
            std::min(seconds, 5.0f) * sampleRate_));
        previews_[write % kPreviewQueueCapacity] = note;
        previewWrite_.store(write + 1, std::memory_order_release);
        return true;
    }

    void setTrackGain(int track, float gain) noexcept {
        if (track >= 0 && track < kMaxTracks && std::isfinite(gain))
            gains_[(size_t)track].store(std::clamp(gain, 0.0f, 2.0f),
                                        std::memory_order_relaxed);
    }

    // Set while stopped. Half-open loop [start, end); disable with (0, 0).
    void setLoop(std::uint64_t start, std::uint64_t end) noexcept {
        loopBegin_ = start;
        loopEnd_ = end > start ? end : 0;
        if (loopEnd_ && frame_ >= loopEnd_) seek(loopBegin_);
    }

    void seek(std::uint64_t frame) noexcept {
        frame_ = frame;
        nextNote_ = static_cast<size_t>(std::lower_bound(
            notes_.begin(), notes_.end(), frame,
            [](const Note& n, std::uint64_t f) { return n.frame < f; }) -
            notes_.begin());
        for (Voice& v : voices_) v.active = false;
    }

    std::uint64_t position() const noexcept { return frame_; }

    // Interleaved stereo float32; no new/delete, locks, system calls or I/O here.
    void process(float* output, size_t frames) noexcept {
        if (!output || frames == 0) return;
        const auto write = previewWrite_.load(std::memory_order_acquire);
        auto read = previewRead_.load(std::memory_order_relaxed);
        while (read != write) {
            startVoice(previews_[read % kPreviewQueueCapacity]);
            ++read;
        }
        previewRead_.store(read, std::memory_order_release);
        for (size_t i = 0; i < frames; ++i) {
            if (loopEnd_ && frame_ >= loopEnd_) seek(loopBegin_);
            while (nextNote_ < notes_.size() && notes_[nextNote_].frame <= frame_) {
                if (notes_[nextNote_].frame == frame_) startVoice(notes_[nextNote_]);
                ++nextNote_;
            }
            float mix = 0.0f;
            for (Voice& voice : voices_) {
                if (!voice.active) continue;
                const auto elapsed = frame_ - voice.start;
                if (elapsed >= voice.length) { voice.active = false; continue; }
                constexpr float attackSeconds = 0.002f;
                constexpr float releaseSeconds = 0.008f;
                const float attackFrames = std::max(1.0f, sampleRate_ * attackSeconds);
                const float releaseFrames = std::max(1.0f, sampleRate_ * releaseSeconds);
                float env = std::min(1.0f, elapsed / attackFrames);
                env *= std::min(1.0f, (voice.length - elapsed) / releaseFrames);
                mix += std::sin(voice.phase) * env * voice.velocity *
                       gains_[(size_t)voice.track].load(std::memory_order_relaxed);
                voice.phase += voice.increment;
                if (voice.phase >= kPi2) voice.phase -= kPi2;
            }
            // A per-track and master DSP graph will replace this safety clamp.
            const float value = std::clamp(mix, -1.0f, 1.0f);
            output[i * 2] = value;
            output[i * 2 + 1] = value;
            ++frame_;
        }
    }

private:
    struct Voice {
        bool active = false;
        std::uint64_t start = 0, length = 0;
        float phase = 0.0f, increment = 0.0f, velocity = 0.0f;
        int track = 0;
    };

    void startVoice(const Note& note) noexcept {
        for (Voice& v : voices_) {
            if (!v.active) {
                v.active = true;
                v.start = frame_;
                v.length = note.length;
                v.phase = 0.0f;
                v.increment = kPi2 * note.frequency / (float)sampleRate_;
                v.velocity = std::clamp(note.velocity, 0.0f, 1.0f);
                v.track = note.track;
                return;
            }
        }
        // A voice cap is deliberate: overload must not allocate in the callback.
    }

    static constexpr std::uint32_t kPreviewQueueCapacity = 32;
    std::array<Note, kPreviewQueueCapacity> previews_{};
    std::atomic<std::uint32_t> previewWrite_{0}, previewRead_{0};
    unsigned sampleRate_;
    std::uint64_t frame_ = 0, loopBegin_ = 0, loopEnd_ = 0;
    size_t nextNote_ = 0;
    std::vector<Note> notes_;
    std::array<Voice, kMaxVoices> voices_{};
    std::array<std::atomic<float>, kMaxTracks> gains_{};
};
} // namespace boomify::audio
