// Boomify V16 - Alpha 6 stabilization: real track colors + clean BoomBox render
#define BOOMIFY_V15_INCLUDE
#define render15 render15_base16
#define play15 play15_base16
#define export15 export15_base16
#define draw15 draw15_base16
#define proc_v15 proc_v15_base16
#define wWinMain wWinMain_v15_embedded16
#include "main_v15.cpp"
#undef wWinMain
#undef proc_v15
#undef draw15
#undef export15
#undef play15
#undef render15
#undef BOOMIFY_V15_INCLUDE

namespace {
void render16(std::vector<int16_t>&a,int start){
    std::array<bool,V6_MAX_TRACKS> oldMute{};
    for(int i=0;i<(int)lanes.size();++i){oldMute[i]=lanes[i].mute;if(kicks15[i].enabled)lanes[i].mute=true;}
    renderRange11(a,start);
    for(int i=0;i<(int)lanes.size();++i)lanes[i].mute=oldMute[i];
    mixSample15(a,start);mixBoom15(a,start);masterBus15(a);
}
void play16(){if(playing){stopAudio();return;}playStartBar=cursorBar;render16(playBuffer,playStartBar);WAVEFORMATEX f{};f.wFormatTag=WAVE_FORMAT_PCM;f.nChannels=2;f.nSamplesPerSec=SR;f.wBitsPerSample=16;f.nBlockAlign=4;f.nAvgBytesPerSec=SR*4;if(waveOutOpen(&wave,WAVE_MAPPER,&f,(DWORD_PTR)win,0,CALLBACK_WINDOW)!=MMSYSERR_NOERROR){wave=nullptr;return;}waveHdr={};waveHdr.lpData=(LPSTR)playBuffer.data();waveHdr.dwBufferLength=(DWORD)std::min<size_t>(playBuffer.size()*sizeof(int16_t),0xFFFFFFFFu);if(waveOutPrepareHeader(wave,&waveHdr,sizeof(waveHdr))!=MMSYSERR_NOERROR||waveOutWrite(wave,&waveHdr,sizeof(waveHdr))!=MMSYSERR_NOERROR){stopAudio();return;}playing=true;SetTimer(win,1,33,nullptr);InvalidateRect(win,nullptr,FALSE);}
void export16(){auto p=dialog(true,L"WAV Audio\0*.wav\0",L"wav");if(p.empty())return;std::vector<int16_t>a;render16(a,0);std::ofstream o(p,std::ios::binary);uint32_t ds=(uint32_t)(a.size()*sizeof(int16_t)),rs=36+ds,fs=16,rate=SR,br=SR*4;uint16_t pcm=1,ch=2,al=4,bits=16;o.write("RIFF",4);o.write((char*)&rs,4);o.write("WAVEfmt ",8);o.write((char*)&fs,4);o.write((char*)&pcm,2);o.write((char*)&ch,2);o.write((char*)&rate,4);o.write((char*)&br,4);o.write((char*)&al,2);o.write((char*)&bits,2);o.write("data",4);o.write((char*)&ds,4);o.write((char*)a.data(),ds);}
void drawTrackColors16(HDC d){for(int l=0;l<(int)lanes.size();++l){COLORREF c=colors15[l]?colors15[l]:defaultColor15(l);for(int v=0;v<visibleBars;++v){int b=scrollBar+v;if(b>=activeBars)break;if(!laneUsed11(l,b))continue;RECT r=v6CellRect(l,v);fill(d,{r.left+2,r.top+2,r.right-2,r.bottom-2},c);}}}
LRESULT CALLBACK proc_v16(HWND h,UINT m,WPARAM w,LPARAM l){POINT p{GET_X_LPARAM(l),GET_Y_LPARAM(l)};if(m==WM_PAINT){PAINTSTRUCT ps;HDC s=BeginPaint(h,&ps);RECT c;GetClientRect(h,&c);HDC mem=CreateCompatibleDC(s);HBITMAP bm=CreateCompatibleBitmap(s,std::max(1,(int)c.right),std::max(1,(int)c.bottom));auto old=SelectObject(mem,bm);draw11(mem,c);drawTrackColors16(mem);overlay12(mem);drawV13Chrome(mem);drawV14(mem);draw15_base16(mem);BitBlt(s,0,0,c.right,c.bottom,mem,0,0,SRCCOPY);SelectObject(mem,old);DeleteObject(bm);DeleteDC(mem);EndPaint(h,&ps);return 0;}if(m==WM_LBUTTONDOWN){if(inside(playR,p)){play16();return 0;}if(inside(wavR,p)){export16();return 0;}}return proc_v15_base16(h,m,w,l);}
}
int WINAPI wWinMain(HINSTANCE hi,HINSTANCE,LPWSTR,int){for(int i=0;i<V6_MAX_TRACKS;i++)colors15[i]=defaultColor15(i);editorOpen=true;sound9[1]=Sound9::SubBass;sound9[2]=Sound9::SawLead;master=100;WNDCLASSW wc{};wc.lpfnWndProc=proc_v16;wc.hInstance=hi;wc.lpszClassName=L"BoomifyAlpha6V16";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);wc.style=CS_DBLCLKS;RegisterClassW(&wc);win=CreateWindowExW(0,wc.lpszClassName,L"Boomify Studio - Alpha 6",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1440,900,nullptr,nullptr,hi,nullptr);if(!win)return 1;ShowWindow(win,SW_MAXIMIZE);UpdateWindow(win);layout11(win);MSG msg{};while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}return(int)msg.wParam;}
