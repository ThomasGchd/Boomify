// Boomify Alpha 4 volume fix: per-track gain is now part of playback/export.
#define wWinMain wWinMain_v7_legacy
#include "main_v7.cpp"
#undef wWinMain

namespace {
struct Voice8{NativeSound sound;int lane=1,note=60,age=0;double ph=0;};
double sample8(Voice8&v){Voice7 q{v.sound,v.note,v.age,v.ph};double out=sample7(q);v.age=q.age;v.ph=q.ph;return out;}
bool alive8(const Voice8&v){Voice7 q{v.sound,v.note,v.age,v.ph};return alive7(q);}
void renderRangeV8(std::vector<int16_t>&out,int start){
    start=std::clamp(start,0,activeBars-1);
    int step=std::max(1,(int)(SR*60.0/bpm/4.0));
    size_t frames=(size_t)step*STEPS*(activeBars-start);
    out.assign(frames*2,0);
    std::vector<Voice> drumsV;
    std::vector<Voice8> notesV;
    bool anySolo=false;for(auto&l:lanes)if(l.solo)anySolo=true;
    for(size_t f=0;f<frames;f++){
        if(f%step==0){
            int g=(int)(f/step),b=start+g/STEPS,s=g%STEPS;
            if(!lanes.empty()&&!lanes[0].mute&&(!anySolo||lanes[0].solo))for(int r=0;r<DR;r++)if(drums[b][r][s])drumsV.push_back({r,0});
            for(int lane=1;lane<(int)lanes.size();lane++){
                if(lanes[lane].type==LaneType::Audio||lanes[lane].mute||(anySolo&&!lanes[lane].solo))continue;
                auto&clip=noteForLane(lane,b);int base=(laneSound[lane]==NativeSound::Bass)?47:71;
                for(int r=0;r<NOTES;r++)if(clip[r][s])notesV.push_back({laneSound[lane],lane,base-r});
            }
        }
        double x=0;
        double drumGain=lanes.empty()?1.0:lanes[0].volume/100.0;
        for(auto&q:drumsV)x+=sample(q)*drumGain;
        for(auto&q:notesV){double gain=(q.lane>=0&&q.lane<(int)lanes.size())?lanes[q.lane].volume/100.0:1.0;x+=sample8(q)*gain;}
        drumsV.erase(std::remove_if(drumsV.begin(),drumsV.end(),[](const Voice&q){return!alive(q);}),drumsV.end());
        notesV.erase(std::remove_if(notesV.begin(),notesV.end(),[](const Voice8&q){return!alive8(q);}),notesV.end());
        int16_t z=(int16_t)(std::clamp(x*.22*(master/100.0),-1.0,1.0)*32767);out[f*2]=out[f*2+1]=z;
    }
}
void playV8(){if(playing){stopAudio();return;}playStartBar=cursorBar;renderRangeV8(playBuffer,playStartBar);WAVEFORMATEX f{};f.wFormatTag=WAVE_FORMAT_PCM;f.nChannels=2;f.nSamplesPerSec=SR;f.wBitsPerSample=16;f.nBlockAlign=4;f.nAvgBytesPerSec=SR*4;if(waveOutOpen(&wave,WAVE_MAPPER,&f,(DWORD_PTR)win,0,CALLBACK_WINDOW)!=MMSYSERR_NOERROR){wave=nullptr;MessageBoxW(win,L"Sortie audio Windows indisponible.",L"Boomify",MB_OK|MB_ICONERROR);return;}waveHdr={};waveHdr.lpData=(LPSTR)playBuffer.data();waveHdr.dwBufferLength=(DWORD)std::min<size_t>(playBuffer.size()*sizeof(int16_t),0xFFFFFFFFu);if(waveOutPrepareHeader(wave,&waveHdr,sizeof(waveHdr))!=MMSYSERR_NOERROR||waveOutWrite(wave,&waveHdr,sizeof(waveHdr))!=MMSYSERR_NOERROR){stopAudio();return;}playing=true;SetTimer(win,1,33,nullptr);InvalidateRect(win,nullptr,FALSE);}
void exportV8(){auto p=dialog(true,L"WAV Audio\0*.wav\0",L"wav");if(p.empty())return;std::vector<int16_t>a;renderRangeV8(a,0);std::ofstream o(p,std::ios::binary);uint32_t ds=(uint32_t)(a.size()*2),rs=36+ds,fs=16,rate=SR,br=SR*4;uint16_t pcm=1,ch=2,align=4,bits=16;o.write("RIFF",4);o.write((char*)&rs,4);o.write("WAVEfmt ",8);o.write((char*)&fs,4);o.write((char*)&pcm,2);o.write((char*)&ch,2);o.write((char*)&rate,4);o.write((char*)&br,4);o.write((char*)&align,2);o.write((char*)&bits,2);o.write("data",4);o.write((char*)&ds,4);o.write((char*)a.data(),ds);}
LRESULT CALLBACK proc_v8(HWND h,UINT m,WPARAM wp,LPARAM lp){if(m==WM_LBUTTONDOWN){POINT p{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};if(inside(playR,p)){playV8();return 0;}if(inside(wavR,p)){exportV8();return 0;}}return proc_v7(h,m,wp,lp);}
}
int WINAPI wWinMain(HINSTANCE hi,HINSTANCE,PWSTR,int){WNDCLASSW wc{};wc.lpfnWndProc=proc_v8;wc.hInstance=hi;wc.lpszClassName=L"BoomifyV8";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);RegisterClassW(&wc);win=CreateWindowExW(0,wc.lpszClassName,L"Boomify Alpha 4 - Native Instruments",WS_OVERLAPPEDWINDOW,0,0,1500,920,nullptr,nullptr,hi,nullptr);if(!win)return 1;v6Fresh();laneSound[1]=NativeSound::Bass;laneSound[2]=NativeSound::Saw;layoutV6(win);ShowWindow(win,SW_MAXIMIZE);UpdateWindow(win);MSG msg{};while(GetMessageW(&msg,nullptr,0,0)){TranslateMessage(&msg);DispatchMessageW(&msg);}return 0;}
