// Small, self-contained preview voice for the legacy WinMM player.
// This is a temporary audition path while Audio V2 is being integrated.
// The preview uses its own waveOut stream and never changes the song transport.
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace {
constexpr UINT_PTR kNotePreviewTimerNav = 4242;
HWAVEOUT notePreviewWaveNav = nullptr;
WAVEHDR notePreviewHeaderNav{};
bool notePreviewPreparedNav = false;
std::vector<int16_t> notePreviewSamplesNav;

void stopNotePreviewNav() {
    if (win) KillTimer(win, kNotePreviewTimerNav);
    if (notePreviewWaveNav) {
        waveOutReset(notePreviewWaveNav);
        if (notePreviewPreparedNav) {
            waveOutUnprepareHeader(notePreviewWaveNav, &notePreviewHeaderNav,
                                   sizeof(notePreviewHeaderNav));
        }
        waveOutClose(notePreviewWaveNav);
        notePreviewWaveNav = nullptr;
    }
    notePreviewPreparedNav = false;
    notePreviewHeaderNav = {};
    notePreviewSamplesNav.clear();
}

void playPianoPreviewNav(int lane, int row) {
    if (lane < 0 || lane >= (int)lanes.size() || row < 0 || row >= NOTES ||
        lanes[lane].type != LaneType::Instrument) return;
    stopNotePreviewNav();

    const Family9 family = familyOf(sound9[lane]);
    const int base = family == Family9::Bass ? 47 : 71;
    const double frequency = hz(base - row);
    if (!(frequency > 0.0) || !std::isfinite(frequency)) return;

    const size_t frames = (size_t)(SR * 0.36);
    notePreviewSamplesNav.assign(frames * 2, 0);
    double phase = 0.0;
    const double level = 0.44 * (lanes[lane].volume / 100.0) *
                         (std::clamp(master, 0, 100) / 100.0);
    for (size_t f = 0; f < frames; ++f) {
        const double t = (double)f / SR;
        const double remaining = (double)(frames - f) / SR;
        double env = std::min(1.0, t / 0.006) *
                     std::min(1.0, remaining / 0.042);
        if (family == Family9::Pluck) env *= std::exp(-t * 5.0);
        else if (family == Family9::Keys) env *= 0.72 + 0.28 * std::exp(-t * 2.0);
        phase += frequency / SR;
        if (phase >= 1.0) phase -= std::floor(phase);
        const double sample = fineWave16(sound9[lane], phase, t) * env * level;
        const int16_t value = (int16_t)std::lrint(
            std::clamp(sample, -0.85, 0.85) * 32767.0);
        notePreviewSamplesNav[f * 2] = value;
        notePreviewSamplesNav[f * 2 + 1] = value;
    }

    // If this track uses an external VST3, audition that instrument rather
    // than the native placeholder. Failed/cold plugins fall back to the
    // generated native preview above without losing the click response.
    if(!vst3ForLaneNav[lane].empty()){
        std::vector<int16_t> vst3Audio;
        if(renderVST3PreviewNav(lane,row,vst3Audio)&&!vst3Audio.empty())
            notePreviewSamplesNav.swap(vst3Audio);
    }

    WAVEFORMATEX format{};
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = 2;
    format.nSamplesPerSec = SR;
    format.wBitsPerSample = 16;
    format.nBlockAlign = 4;
    format.nAvgBytesPerSec = SR * 4;
    if (waveOutOpen(&notePreviewWaveNav, WAVE_MAPPER, &format, 0, 0,
                    CALLBACK_NULL) != MMSYSERR_NOERROR) {
        notePreviewWaveNav = nullptr;
        notePreviewSamplesNav.clear();
        MessageBoxW(win,L"Impossible d'ouvrir la sortie audio pour la preecoute des notes.",L"Boomify - Piano roll",MB_OK|MB_ICONERROR);
        return;
    }
    notePreviewHeaderNav = {};
    notePreviewHeaderNav.lpData = (LPSTR)notePreviewSamplesNav.data();
    notePreviewHeaderNav.dwBufferLength =
        (DWORD)(notePreviewSamplesNav.size() * sizeof(int16_t));
    if (waveOutPrepareHeader(notePreviewWaveNav, &notePreviewHeaderNav,
                             sizeof(notePreviewHeaderNav)) != MMSYSERR_NOERROR) {
        stopNotePreviewNav();
        return;
    }
    notePreviewPreparedNav = true;
    if (waveOutWrite(notePreviewWaveNav, &notePreviewHeaderNav,
                     sizeof(notePreviewHeaderNav)) != MMSYSERR_NOERROR) {
        stopNotePreviewNav();
        return;
    }
    if (win) SetTimer(win, kNotePreviewTimerNav, 40, nullptr);
}

void pollNotePreviewNav() {
    if (notePreviewWaveNav &&
        (notePreviewHeaderNav.dwFlags & WHDR_DONE) != 0) {
        stopNotePreviewNav();
    }
}
} // namespace
