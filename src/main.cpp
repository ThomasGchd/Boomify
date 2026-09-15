#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <commdlg.h>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <thread>
#include <vector>
#include <algorithm>
#include <fstream>
#include <string>

#pragma comment(lib,"winmm.lib")
#pragma comment(lib,"comdlg32.lib")

namespace {
constexpr int SR=44100, STEPS=16, TRACKS=4; constexpr double PI=3.14159265358979323846;
const wchar_t* names[TRACKS]={L"KICK",L"SNARE",L"CLAP",L"HI-HAT"};
std::array<std::array<std::atomic_bool,STEPS>,TRACKS> seq;
std::atomic_bool playing{false}; std::atomic_int bpm{150},currentStep{-1}; std::thread audioThread; HWND mainWindow{};
RECT pads[TRACKS][STEPS],playRect{},minusRect{},plusRect{},saveRect{},loadRect{},exportRect{},clearRect{};
COLORREF bg=RGB(18,19,22),panel=RGB(30,32,37),text=RGB(242,243,246),muted=RGB(145,149,158),accent=RGB(178,255,74),off=RGB(48,51,58),playhead=RGB(92,98,110);

void fill(HDC d,const RECT&r,COLORREF c){HBRUSH b=CreateSolidBrush(c);FillRect(d,&r,b);DeleteObject(b);} 
void label(HDC d,const wchar_t*s,int x,int y,int z,COLORREF c,int w=FW_NORMAL){HFONT f=CreateFontW(-z,0,0,0,w,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");auto o=SelectObject(d,f);SetBkMode(d,TRANSPARENT);SetTextColor(d,c);TextOutW(d,x,y,s,(int)wcslen(s));SelectObject(d,o);DeleteObject(f);} 

struct Voice{int type=0,age=0;double phase=0;};
double noise01(uint32_t x){x^=x<<13;x^=x>>17;x^=x<<5;return ((x&65535)/32767.5)-1.0;}
double voiceSample(Voice&v){double t=(double)v.age/SR,out=0; if(v.type==0){double f=48+110*exp(-t*35);v.phase+=2*PI*f/SR;out=sin(v.phase)*exp(-t*16)+.18*sin(2*PI*900*t)*exp(-t*90);}else if(v.type==1){out=.65*noise01(v.age*747796405u+17)*exp(-t*18);v.phase+=2*PI*180/SR;out+=.35*sin(v.phase)*exp(-t*22);}else if(v.type==2){out=.65*noise01(v.age*2891336453u+91)*exp(-t*30)*(sin(2*PI*35*t)>.0?1:-1);}else{out=.45*noise01(v.age*277803737u+41)*exp(-t*55);}v.age++;return out;}
bool alive(const Voice&v){static const double dur[4]={.28,.20,.14,.09};return v.age<(int)(dur[v.type]*SR);} 

void audioLoop(){WAVEFORMATEX f{};f.wFormatTag=WAVE_FORMAT_PCM;f.nChannels=1;f.nSamplesPerSec=SR;f.wBitsPerSample=16;f.nBlockAlign=2;f.nAvgBytesPerSec=SR*2;HWAVEOUT wo{};if(waveOutOpen(&wo,WAVE_MAPPER,&f,0,0,CALLBACK_NULL)!=MMSYSERR_NOERROR){playing=false;return;}const int N=512,NB=4;std::array<std::array<int16_t,N>,NB> data{};std::array<WAVEHDR,NB> h{};std::vector<Voice> voices;double until=0;int step=0;for(int b=0;b<NB;b++){h[b].lpData=(LPSTR)data[b].data();h[b].dwBufferLength=N*2;waveOutPrepareHeader(wo,&h[b],sizeof(WAVEHDR));h[b].dwFlags|=WHDR_DONE;}int bi=0;while(playing){auto& wh=h[bi];while(playing&&!(wh.dwFlags&WHDR_DONE))Sleep(1);if(!playing)break;double stepLen=SR*60.0/std::max(60,bpm.load())/4.0;for(int i=0;i<N;i++){if(until<=0){currentStep=step;PostMessageW(mainWindow,WM_APP+1,0,0);for(int t=0;t<TRACKS;t++)if(seq[t][step])voices.push_back({t,0,0});step=(step+1)%STEPS;until+=stepLen;}double s=0;for(auto&v:voices)s+=voiceSample(v);voices.erase(std::remove_if(voices.begin(),voices.end(),[](const Voice&v){return !alive(v);}),voices.end());data[bi][i]=(int16_t)(std::clamp(s*.58,-1.0,1.0)*32767);until-=1.0;}wh.dwFlags&=~WHDR_DONE;waveOutWrite(wo,&wh,sizeof(WAVEHDR));bi=(bi+1)%NB;}waveOutReset(wo);for(auto&x:h)waveOutUnprepareHeader(wo,&x,sizeof(WAVEHDR));waveOutClose(wo);currentStep=-1;PostMessageW(mainWindow,WM_APP+1,0,0);} 
void toggle(){if(playing){playing=false;if(audioThread.joinable())audioThread.join();}else{playing=true;audioThread=std::thread(audioLoop);}InvalidateRect(mainWindow,nullptr,FALSE);} 

void writeWav(const wchar_t*path){int tempo=bpm.load(),stepN=(int)(SR*60.0/tempo/4.0),total=stepN*STEPS;std::vector<int16_t>a(total);std::vector<Voice>v;for(int i=0;i<total;i++){if(i%stepN==0){int s=i/stepN;for(int t=0;t<TRACKS;t++)if(seq[t][s])v.push_back({t,0,0});}double x=0;for(auto&q:v)x+=voiceSample(q);v.erase(std::remove_if(v.begin(),v.end(),[](const Voice&q){return !alive(q);}),v.end());a[i]=(int16_t)(std::clamp(x*.58,-1.0,1.0)*32767);}std::ofstream o(path,std::ios::binary);uint32_t ds=(uint32_t)a.size()*2,rs=36+ds;uint16_t pcm=1,ch=1,bits=16,align=2;uint32_t rate=SR,br=SR*2;o.write("RIFF",4);o.write((char*)&rs,4);o.write("WAVEfmt ",8);uint32_t fs=16;o.write((char*)&fs,4);o.write((char*)&pcm,2);o.write((char*)&ch,2);o.write((char*)&rate,4);o.write((char*)&br,4);o.write((char*)&align,2);o.write((char*)&bits,2);o.write("data",4);o.write((char*)&ds,4);o.write((char*)a.data(),ds);} 
std::wstring pick(bool save,const wchar_t*filter,const wchar_t*ext){wchar_t p[MAX_PATH]=L"";OPENFILENAMEW x{};x.lStructSize=sizeof(x);x.hwndOwner=mainWindow;x.lpstrFile=p;x.nMaxFile=MAX_PATH;x.lpstrFilter=filter;x.lpstrDefExt=ext;x.Flags=OFN_PATHMUSTEXIST|(save?OFN_OVERWRITEPROMPT:OFN_FILEMUSTEXIST);BOOL ok=save?GetSaveFileNameW(&x):GetOpenFileNameW(&x);return ok?p:L"";} 
void saveProject(){auto p=pick(true,L"Boomify Project\0*.boom\0",L"boom");if(p.empty())return;std::ofstream o(p);o<<"BOOMIFY1 "<<bpm.load()<<"\n";for(int t=0;t<TRACKS;t++){for(int s=0;s<STEPS;s++)o<<(seq[t][s]?1:0);o<<"\n";}}
void loadProject(){auto p=pick(false,L"Boomify Project\0*.boom\0",L"boom");if(p.empty())return;std::ifstream i(p);std::string magic,row;int b;if(!(i>>magic>>b)||magic!="BOOMIFY1")return;bpm=std::clamp(b,60,220);for(int t=0;t<TRACKS;t++){i>>row;for(int s=0;s<STEPS&&s<(int)row.size();s++)seq[t][s]=(row[s]=='1');}InvalidateRect(mainWindow,nullptr,FALSE);} 
void exportWav(){auto p=pick(true,L"WAV Audio\0*.wav\0",L"wav");if(!p.empty())writeWav(p.c_str());}

void layout(HWND h){RECT c;GetClientRect(h,&c);int w=c.right;playRect={30,67,122,107};minusRect={w-190,67,w-150,107};plusRect={w-72,67,w-32,107};saveRect={30,126,104,160};loadRect={112,126,186,160};exportRect={194,126,300,160};clearRect={308,126,382,160};int left=126,right=32,gap=5,gg=18,avail=w-left-right-gap*12-gg*3,pw=std::max(22,avail/16);for(int t=0;t<TRACKS;t++){int x=left,y=235+t*58;for(int s=0;s<STEPS;s++){pads[t][s]={x,y,x+pw,y+38};x+=pw+(s%4==3?gg:gap);}}}
void button(HDC d,const RECT&r,const wchar_t*s,bool hot=false){fill(d,r,hot?accent:panel);label(d,s,r.left+12,r.top+8,15,hot?bg:text,FW_BOLD);} 

LRESULT CALLBACK proc(HWND h,UINT m,WPARAM wp,LPARAM lp){switch(m){case WM_APP+1:InvalidateRect(h,nullptr,FALSE);return 0;case WM_SIZE:layout(h);return 0;case WM_KEYDOWN:if(wp==VK_SPACE){toggle();return 0;}break;case WM_LBUTTONDOWN:{POINT p{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};if(PtInRect(&playRect,p)){toggle();return 0;}if(PtInRect(&minusRect,p)){bpm=std::max(60,bpm.load()-1);InvalidateRect(h,nullptr,FALSE);return 0;}if(PtInRect(&plusRect,p)){bpm=std::min(220,bpm.load()+1);InvalidateRect(h,nullptr,FALSE);return 0;}if(PtInRect(&saveRect,p)){saveProject();return 0;}if(PtInRect(&loadRect,p)){loadProject();return 0;}if(PtInRect(&exportRect,p)){exportWav();return 0;}if(PtInRect(&clearRect,p)){for(auto&t:seq)for(auto&s:t)s=false;InvalidateRect(h,nullptr,FALSE);return 0;}for(int t=0;t<TRACKS;t++)for(int s=0;s<STEPS;s++)if(PtInRect(&pads[t][s],p)){seq[t][s]=!seq[t][s].load();InvalidateRect(h,nullptr,FALSE);return 0;}break;}case WM_PAINT:{PAINTSTRUCT ps;HDC d=BeginPaint(h,&ps);RECT c;GetClientRect(h,&c);fill(d,c,bg);label(d,L"BOOMIFY",30,20,28,text,FW_BOLD);label(d,L"ALPHA 1  -  make a beat",164,28,15,muted);button(d,playRect,playing?L"STOP":L"PLAY",!playing);int w=c.right;label(d,L"BPM",w-286,78,14,muted,FW_BOLD);button(d,minusRect,L"-");wchar_t z[8];swprintf_s(z,L"%d",bpm.load());label(d,z,w-137,76,21,text,FW_BOLD);button(d,plusRect,L"+");button(d,saveRect,L"SAVE");button(d,loadRect,L"OPEN");button(d,exportRect,L"EXPORT WAV");button(d,clearRect,L"CLEAR");RECT line{30,181,w-30,182};fill(d,line,RGB(48,50,56));label(d,L"DRUM MACHINE",30,198,15,muted,FW_BOLD);for(int b=0;b<4;b++){wchar_t q[12];swprintf_s(q,L"BEAT %d",b+1);label(d,q,pads[0][b*4].left,215,12,muted,FW_BOLD);}for(int t=0;t<TRACKS;t++){label(d,names[t],30,pads[t][0].top+10,14,text,FW_BOLD);for(int s=0;s<STEPS;s++){COLORREF col=seq[t][s]?accent:off;if(currentStep==s)col=seq[t][s]?RGB(220,255,174):playhead;fill(d,pads[t][s],col);}}label(d,L"Click squares to build your beat. Space = Play / Stop. Everything you hear is generated by Boomify.",126,478,14,muted);label(d,L"Project files: .boom     Audio export: .wav",126,503,14,muted);EndPaint(h,&ps);return 0;}case WM_DESTROY:playing=false;if(audioThread.joinable())audioThread.join();PostQuitMessage(0);return 0;}return DefWindowProcW(h,m,wp,lp);} }

int WINAPI wWinMain(HINSTANCE hi,HINSTANCE,PWSTR,int){for(auto&t:seq)for(auto&s:t)s=false;for(int s:{0,4,8,12})seq[0][s]=true;seq[1][4]=seq[1][12]=true;for(int s:{2,6,10,14})seq[3][s]=true;WNDCLASSW wc{};wc.lpfnWndProc=proc;wc.hInstance=hi;wc.lpszClassName=L"BoomifyAlpha";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);RegisterClassW(&wc);mainWindow=CreateWindowExW(0,wc.lpszClassName,L"Boomify Alpha 1",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1220,590,nullptr,nullptr,hi,nullptr);ShowWindow(mainWindow,SW_SHOW);layout(mainWindow);MSG msg{};while(GetMessageW(&msg,nullptr,0,0)){TranslateMessage(&msg);DispatchMessageW(&msg);}return 0;}
