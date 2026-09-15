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
constexpr int SR=44100; constexpr double PI=3.14159265358979323846;
std::array<std::atomic_bool,16> steps; std::atomic_bool playing{false}; std::atomic_int bpm{150}, currentStep{-1};
std::thread audioThread; HWND mainWindow{}; RECT stepRects[16],playRect{},minusRect{},plusRect{};
COLORREF bg=RGB(20,21,24),panel=RGB(31,33,38),text=RGB(240,240,244),muted=RGB(155,158,166),accent=RGB(178,255,74),offPad=RGB(48,51,58),beatBg=RGB(25,27,31);

void fill(HDC dc,const RECT&r,COLORREF c){HBRUSH b=CreateSolidBrush(c);FillRect(dc,&r,b);DeleteObject(b);}
void label(HDC dc,const wchar_t*s,int x,int y,int size,COLORREF c,int weight=FW_NORMAL){HFONT f=CreateFontW(-size,0,0,0,weight,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");auto old=SelectObject(dc,f);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,c);TextOutW(dc,x,y,s,(int)wcslen(s));SelectObject(dc,old);DeleteObject(f);}

std::vector<int16_t> makeKick(){int n=(int)(SR*.22);std::vector<int16_t>o(n);double ph=0;for(int i=0;i<n;i++){double t=(double)i/SR,f=48+105*std::exp(-t*35);ph+=2*PI*f/SR;double e=std::exp(-t*16),cl=std::exp(-t*90)*.22*std::sin(2*PI*900*t);o[i]=(int16_t)(std::clamp((std::sin(ph)*e+cl)*.82,-1.0,1.0)*32767);}return o;}

std::vector<int16_t> makeBar(int tempo,const std::vector<int16_t>&kick){int stepSamples=(int)std::llround((60.0/tempo/4.0)*SR);std::vector<int32_t>mix(stepSamples*16+kick.size(),0);for(int s=0;s<16;s++)if(steps[s]){int at=s*stepSamples;for(size_t i=0;i<kick.size();i++)mix[at+i]+=kick[i];}std::vector<int16_t>out(stepSamples*16);for(size_t i=0;i<out.size();i++)out[i]=(int16_t)std::clamp(mix[i],-32768,32767);return out;}

void audioLoop(){WAVEFORMATEX fmt{};fmt.wFormatTag=WAVE_FORMAT_PCM;fmt.nChannels=1;fmt.nSamplesPerSec=SR;fmt.wBitsPerSample=16;fmt.nBlockAlign=2;fmt.nAvgBytesPerSec=SR*2;HWAVEOUT wo{};if(waveOutOpen(&wo,WAVE_MAPPER,&fmt,0,0,CALLBACK_NULL)!=MMSYSERR_NOERROR){playing=false;return;}auto kick=makeKick();while(playing){int tempo=bpm.load();auto bar=makeBar(tempo,kick);WAVEHDR h{};h.lpData=(LPSTR)bar.data();h.dwBufferLength=(DWORD)(bar.size()*sizeof(int16_t));waveOutPrepareHeader(wo,&h,sizeof(h));waveOutWrite(wo,&h,sizeof(h));double stepMs=60000.0/tempo/4.0;for(int s=0;s<16&&playing;s++){currentStep=s;InvalidateRect(mainWindow,nullptr,FALSE);DWORD target=(DWORD)std::llround(stepMs*(s+1));DWORD start=timeGetTime();while(playing&&timeGetTime()-start<target-(DWORD)std::llround(stepMs*s))Sleep(1);}while(playing&&!(h.dwFlags&WHDR_DONE))Sleep(1);if(!(h.dwFlags&WHDR_DONE))waveOutReset(wo);waveOutUnprepareHeader(wo,&h,sizeof(h));}waveOutReset(wo);waveOutClose(wo);currentStep=-1;InvalidateRect(mainWindow,nullptr,FALSE);}
void togglePlay(){if(playing){playing=false;if(audioThread.joinable())audioThread.join();}else{playing=true;audioThread=std::thread(audioLoop);}InvalidateRect(mainWindow,nullptr,FALSE);}
void layout(HWND h){RECT c;GetClientRect(h,&c);int w=c.right;playRect={32,74,126,116};minusRect={w-190,74,w-148,116};plusRect={w-74,74,w-32,116};int left=56,right=32,groupGap=18,padGap=5;int available=w-left-right-groupGap*3-padGap*12;int pw=std::max(24,available/16);int x=left;for(int i=0;i<16;i++){stepRects[i]={x,246,x+pw,296};x+=pw;if(i<15)x+=(i%4==3?groupGap:padGap);}}

LRESULT CALLBACK proc(HWND h,UINT m,WPARAM wp,LPARAM lp){switch(m){case WM_SIZE:layout(h);InvalidateRect(h,nullptr,TRUE);return 0;case WM_KEYDOWN:if(wp==VK_SPACE){togglePlay();return 0;}break;case WM_LBUTTONDOWN:{POINT p{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};if(PtInRect(&playRect,p)){togglePlay();return 0;}if(PtInRect(&minusRect,p)){bpm=std::max(60,bpm.load()-1);InvalidateRect(h,nullptr,FALSE);return 0;}if(PtInRect(&plusRect,p)){bpm=std::min(220,bpm.load()+1);InvalidateRect(h,nullptr,FALSE);return 0;}for(int i=0;i<16;i++)if(PtInRect(&stepRects[i],p)){steps[i]=!steps[i].load();InvalidateRect(h,nullptr,FALSE);return 0;}break;}case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);RECT c;GetClientRect(h,&c);fill(dc,c,bg);label(dc,L"BOOMIFY",32,24,28,text,FW_BOLD);label(dc,L"first sound",164,31,16,muted);fill(dc,playRect,playing?RGB(70,74,82):accent);label(dc,playing?L"STOP":L"PLAY",playing?55:57,84,18,playing?text:bg,FW_BOLD);int w=c.right;label(dc,L"BPM",w-292,86,15,muted,FW_BOLD);fill(dc,minusRect,panel);label(dc,L"-",w-174,82,23,text);wchar_t b[8];swprintf_s(b,L"%d",bpm.load());label(dc,b,w-132,82,22,text,FW_BOLD);fill(dc,plusRect,panel);label(dc,L"+",w-62,82,22,text);RECT line{32,145,w-32,146};fill(dc,line,RGB(48,50,56));label(dc,L"DRUM MACHINE",32,174,15,muted,FW_BOLD);label(dc,L"1 mesure = 4 temps",32,197,17,text,FW_BOLD);for(int beat=0;beat<4;beat++){wchar_t bt[16];swprintf_s(bt,L"TEMPS %d",beat+1);label(dc,bt,stepRects[beat*4].left,220,13,muted,FW_BOLD);}label(dc,L"KICK",10,260,13,muted,FW_BOLD);for(int i=0;i<16;i++){COLORREF col=steps[i]?accent:offPad;if(currentStep==i)col=steps[i]?RGB(225,255,178):RGB(95,100,110);fill(dc,stepRects[i],col);if(i%4==0){RECT mark{stepRects[i].left,300,stepRects[i].right,303};fill(dc,mark,accent);}}label(dc,L"Vert = le kick joue ici     Gris = silence     Espace = Play / Stop",56,326,15,muted);label(dc,L"Astuce : 1 kick au debut de chaque temps = BOUM BOUM BOUM BOUM",56,351,14,muted);EndPaint(h,&ps);return 0;}case WM_DESTROY:playing=false;if(audioThread.joinable())audioThread.join();PostQuitMessage(0);return 0;}return DefWindowProcW(h,m,wp,lp);}}

int WINAPI wWinMain(HINSTANCE hi,HINSTANCE,PWSTR,int){for(int i=0;i<16;i++)steps[i]=(i%4==0);WNDCLASSW wc{};wc.lpfnWndProc=proc;wc.hInstance=hi;wc.lpszClassName=L"BoomifyMain";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);RegisterClassW(&wc);mainWindow=CreateWindowExW(0,wc.lpszClassName,L"Boomify 0.0.2",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1180,440,nullptr,nullptr,hi,nullptr);ShowWindow(mainWindow,SW_SHOW);layout(mainWindow);MSG msg{};while(GetMessageW(&msg,nullptr,0,0)){TranslateMessage(&msg);DispatchMessageW(&msg);}return 0;}
