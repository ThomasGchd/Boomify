#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <thread>
#include <vector>
#include <algorithm>

#pragma comment(lib, "winmm.lib")

namespace {
constexpr int SR = 44100;
constexpr double PI = 3.14159265358979323846;
std::array<std::atomic_bool,16> steps;
std::atomic_bool playing{false};
std::atomic_int bpm{150};
std::atomic_int currentStep{-1};
std::thread audioThread;
HWND mainWindow{};

RECT stepRects[16];
RECT playRect{}, minusRect{}, plusRect{};

COLORREF bg = RGB(20,21,24), panel = RGB(31,33,38), text = RGB(240,240,244), muted = RGB(155,158,166), accent = RGB(178,255,74), offPad = RGB(53,56,63);

void fill(HDC dc, const RECT& r, COLORREF c) { HBRUSH b=CreateSolidBrush(c); FillRect(dc,&r,b); DeleteObject(b); }
void label(HDC dc, const wchar_t* s, int x,int y,int size, COLORREF c, int weight=FW_NORMAL) {
    HFONT f=CreateFontW(-size,0,0,0,weight,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    auto old=SelectObject(dc,f); SetBkMode(dc,TRANSPARENT); SetTextColor(dc,c); TextOutW(dc,x,y,s,(int)wcslen(s)); SelectObject(dc,old); DeleteObject(f);
}

std::vector<int16_t> makeKick(double seconds=0.22) {
    int n=(int)(SR*seconds); std::vector<int16_t> out(n);
    double phase=0.0;
    for(int i=0;i<n;i++) {
        double t=(double)i/SR;
        double freq=48.0 + 105.0*std::exp(-t*35.0);
        phase += 2.0*PI*freq/SR;
        double env=std::exp(-t*16.0);
        double click=std::exp(-t*90.0)*0.22*std::sin(2.0*PI*900.0*t);
        double v=(std::sin(phase)*env + click)*0.82;
        out[i]=(int16_t)(std::clamp(v,-1.0,1.0)*32767.0);
    }
    return out;
}

void audioLoop() {
    WAVEFORMATEX fmt{}; fmt.wFormatTag=WAVE_FORMAT_PCM; fmt.nChannels=1; fmt.nSamplesPerSec=SR; fmt.wBitsPerSample=16; fmt.nBlockAlign=2; fmt.nAvgBytesPerSec=SR*2;
    HWAVEOUT wo{}; if(waveOutOpen(&wo,WAVE_MAPPER,&fmt,0,0,CALLBACK_NULL)!=MMSYSERR_NOERROR) { playing=false; return; }
    auto kick=makeKick(); int s=0;
    while(playing) {
        currentStep=s; InvalidateRect(mainWindow,nullptr,FALSE);
        if(steps[s]) {
            WAVEHDR h{}; h.lpData=(LPSTR)kick.data(); h.dwBufferLength=(DWORD)(kick.size()*sizeof(int16_t));
            waveOutPrepareHeader(wo,&h,sizeof(h)); waveOutWrite(wo,&h,sizeof(h));
            while(playing && !(h.dwFlags & WHDR_DONE)) Sleep(1);
            waveOutUnprepareHeader(wo,&h,sizeof(h));
        }
        int ms=(int)(60000.0/std::max(60,bpm.load())/4.0);
        int waited=0; while(playing && waited<ms) { Sleep(2); waited+=2; }
        s=(s+1)%16;
    }
    waveOutReset(wo); waveOutClose(wo); currentStep=-1; InvalidateRect(mainWindow,nullptr,FALSE);
}

void togglePlay() {
    if(playing) { playing=false; if(audioThread.joinable()) audioThread.join(); }
    else { playing=true; audioThread=std::thread(audioLoop); }
    InvalidateRect(mainWindow,nullptr,FALSE);
}

void layout(HWND h) {
    RECT c; GetClientRect(h,&c); int w=c.right;
    playRect={32,74,126,116}; minusRect={w-190,74,w-148,116}; plusRect={w-74,74,w-32,116};
    int gap=10, left=32, right=32; int available=w-left-right-gap*15; int pw=std::max(28,available/16);
    for(int i=0;i<16;i++) stepRects[i]={left+i*(pw+gap),222,left+i*(pw+gap)+pw,282};
}

LRESULT CALLBACK proc(HWND h, UINT m, WPARAM wp, LPARAM lp) {
    switch(m) {
    case WM_SIZE: layout(h); InvalidateRect(h,nullptr,TRUE); return 0;
    case WM_KEYDOWN: if(wp==VK_SPACE){togglePlay(); return 0;} break;
    case WM_LBUTTONDOWN: {
        POINT p{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};
        if(PtInRect(&playRect,p)){togglePlay(); return 0;}
        if(PtInRect(&minusRect,p)){bpm=std::max(60,bpm.load()-1); InvalidateRect(h,nullptr,FALSE); return 0;}
        if(PtInRect(&plusRect,p)){bpm=std::min(220,bpm.load()+1); InvalidateRect(h,nullptr,FALSE); return 0;}
        for(int i=0;i<16;i++) if(PtInRect(&stepRects[i],p)){steps[i]=!steps[i].load(); InvalidateRect(h,nullptr,FALSE); return 0;}
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc=BeginPaint(h,&ps); RECT c; GetClientRect(h,&c); fill(dc,c,bg);
        label(dc,L"BOOMIFY",32,24,28,text,FW_BOLD); label(dc,L"first sound",164,31,16,muted);
        fill(dc,playRect,playing?RGB(70,74,82):accent); label(dc,playing?L"STOP":L"PLAY",playing?55:57,84,18,playing?text:bg,FW_BOLD);
        int w=c.right; label(dc,L"BPM",w-292,86,15,muted,FW_BOLD); fill(dc,minusRect,panel); label(dc,L"−",w-177,82,23,text);
        wchar_t b[8]; swprintf_s(b,L"%d",bpm.load()); label(dc,b,w-132,82,22,text,FW_BOLD); fill(dc,plusRect,panel); label(dc,L"+",w-62,82,22,text);
        RECT line{32,145,w-32,146}; fill(dc,line,RGB(48,50,56));
        label(dc,L"DRUM MACHINE",32,174,15,muted,FW_BOLD); label(dc,L"Kick",32,197,18,text,FW_BOLD);
        for(int i=0;i<16;i++) {
            COLORREF col=steps[i]?accent:offPad; if(currentStep==i) col=steps[i]?RGB(220,255,165):RGB(92,96,106); fill(dc,stepRects[i],col);
            wchar_t n[4]; swprintf_s(n,L"%d",i+1); int x=stepRects[i].left+(stepRects[i].right-stepRects[i].left)/2-5; label(dc,n,x,291,13,muted);
        }
        label(dc,L"Clique les pas • Espace = Play/Stop • Aucun sample requis",32,332,15,muted);
        EndPaint(h,&ps); return 0;
    }
    case WM_DESTROY: playing=false; if(audioThread.joinable()) audioThread.join(); PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(h,m,wp,lp);
}
}

int WINAPI wWinMain(HINSTANCE hi,HINSTANCE, PWSTR,int) {
    for(int i=0;i<16;i++) steps[i]=(i%4==0);
    WNDCLASSW wc{}; wc.lpfnWndProc=proc; wc.hInstance=hi; wc.lpszClassName=L"BoomifyMain"; wc.hCursor=LoadCursor(nullptr,IDC_ARROW); wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1); RegisterClassW(&wc);
    mainWindow=CreateWindowExW(0,wc.lpszClassName,L"Boomify 0.0.1",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1180,430,nullptr,nullptr,hi,nullptr);
    ShowWindow(mainWindow,SW_SHOW); layout(mainWindow);
    MSG msg{}; while(GetMessageW(&msg,nullptr,0,0)){TranslateMessage(&msg);DispatchMessageW(&msg);} return 0;
}
