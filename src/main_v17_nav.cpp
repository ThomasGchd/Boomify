// Boomify V17 navigation layer - isolated from the validated persistence source.
// This file is the build entry point; main_v17.cpp remains the stable engine/UI base.
#define wWinMain wWinMain_v17_base
#include "main_v17.cpp"
#undef wWinMain

namespace {
RECT zoomOutNav{}, zoomInNav{}, fitNav{}, loopNav{}, setLoopNav{};
bool loopOnNav=false;
int loopStartNav=0, loopEndNav=3;

void scrollTimelineNav(int dir){
    if(!dir) return;
    const int step=std::max(1,visibleBars/4);
    if(dir>0){
        const int wanted=scrollBar+step;
        const int needed=std::min(MAX_BARS,wanted+visibleBars);
        if(needed>activeBars) activeBars=needed;
        scrollBar=std::min(wanted,std::max(0,MAX_BARS-visibleBars));
    }else scrollBar=std::max(0,scrollBar-step);
    cursorBar=std::clamp(cursorBar,scrollBar,std::min(MAX_BARS-1,scrollBar+visibleBars-1));
    layout11(win); sync16(win); InvalidateRect(win,nullptr,FALSE);
}

void zoomNav(int dir){
    const int old=visibleBars;
    if(dir>0) visibleBars=std::max(4,visibleBars/2);
    else visibleBars=std::min(MAX_BARS,std::max(visibleBars+1,visibleBars*2));
    if(dir<0 && visibleBars>activeBars) visibleBars=std::min(MAX_BARS,std::max(activeBars,4));
    if(old==visibleBars) return;
    scrollBar=std::clamp(scrollBar,0,std::max(0,MAX_BARS-visibleBars));
    if(cursorBar<scrollBar) scrollBar=cursorBar;
    if(cursorBar>=scrollBar+visibleBars) scrollBar=std::max(0,cursorBar-visibleBars+1);
    layout11(win); sync16(win); InvalidateRect(win,nullptr,FALSE);
}

void fitNav(){
    visibleBars=std::clamp(activeBars,4,MAX_BARS);
    scrollBar=0;
    layout11(win); sync16(win); InvalidateRect(win,nullptr,FALSE);
}

void setLoopNavFromSelection(){
    int lo=MAX_BARS,hi=-1;
    for(auto&q:multiSelection){lo=std::min(lo,q.bar);hi=std::max(hi,q.bar);}
    if(hi<0){lo=std::clamp(selectedBar,0,std::max(0,activeBars-1));hi=std::min(activeBars-1,lo+3);}
    loopStartNav=std::clamp(lo,0,std::max(0,activeBars-1));
    loopEndNav=std::clamp(std::max(loopStartNav,hi),loopStartNav,std::max(loopStartNav,activeBars-1));
    loopOnNav=true; InvalidateRect(win,nullptr,FALSE);
}

void drawNavTools(HDC d){
    RECT p=panels15.timeline; LONG y=p.top+13;
    setLoopNav={p.right-350,y,p.right-275,y+27}; loopNav={p.right-269,y,p.right-205,y+27};
    fitNav={p.right-199,y,p.right-145,y+27}; zoomOutNav={p.right-139,y,p.right-97,y+27}; zoomInNav={p.right-91,y,p.right-49,y+27};
    pill16(d,setLoopNav,L"SET LOOP",CYAN16); pill16(d,loopNav,L"LOOP",PINK16,loopOnNav);
    pill16(d,fitNav,L"FIT",PURPLE16); pill16(d,zoomOutNav,L"-",PURPLE16); pill16(d,zoomInNav,L"+",PURPLE16);
    if(loopOnNav){
        int left=(int)p.left+15,right=(int)p.right-15,w=std::max(1,right-left);
        int a=loopStartNav-scrollBar,b=loopEndNav-scrollBar+1;
        if(b>0&&a<visibleBars){a=std::clamp(a,0,visibleBars);b=std::clamp(b,0,visibleBars);LONG x0=left+(LONG)((long long)a*w/std::max(1,visibleBars)),x1=left+(LONG)((long long)b*w/std::max(1,visibleBars));fill(d,{x0,p.top+38,x1,p.top+43},PINK16);}
    }
}

// Removes the tiny discontinuity around inherited drum-kick tails without touching BoomBox kicks.
void smoothKickTailsNav(std::vector<int16_t>&a,int startBar){
    if(a.empty()) return;
    const int bpmNow=std::max(60,bpm.load());
    const double secBar=240.0/(double)bpmNow;
    const long long barFrames=(long long)(secBar*SR);
    const long long stepFrames=std::max<long long>(1,barFrames/STEPS);
    const long long tail=(long long)(0.30*SR), half=96;
    for(int lane=0;lane<(int)lanes.size();++lane){
        if(lanes[lane].type!=LaneType::Drums||kicks15[lane].enabled||lanes[lane].mute) continue;
        for(int bar=std::max(0,startBar);bar<activeBars;++bar){
            auto&dr=drumForLane11(lane,bar);
            for(int step=0;step<STEPS;++step){
                if(!dr[0][step]) continue;
                const long long center=((long long)bar-startBar)*barFrames+(long long)step*stepFrames+tail;
                const long long a0=center-half,a1=center+half;
                if(a0<0||(size_t)(a1*2+1)>=a.size()) continue;
                for(int ch=0;ch<2;++ch){
                    const double left=a[(size_t)a0*2+ch],right=a[(size_t)a1*2+ch];
                    for(long long q=0;q<=2*half;++q){
                        const double u=(double)q/(2.0*half),sm=u*u*(3.0-2.0*u);
                        a[(size_t)(a0+q)*2+ch]=(int16_t)std::lrint(left+(right-left)*sm);
                    }
                }
            }
        }
    }
}

void renderNav(std::vector<int16_t>&a,int start){render16(a,start);smoothKickTailsNav(a,start);}

void playNav(){
    if(playing){stopAudio();return;}
    int start=loopOnNav?loopStartNav:cursorBar;
    start=std::clamp(start,0,std::max(0,activeBars-1)); playStartBar=start; renderNav(playBuffer,start);
    if(loopOnNav){const int bpmNow=std::max(60,bpm.load());double secBar=240.0/(double)bpmNow;size_t frames=(size_t)(secBar*SR)*std::max(1,loopEndNav-loopStartNav+1),samples=frames*2;if(playBuffer.size()>samples)playBuffer.resize(samples);}
    WAVEFORMATEX f{};f.wFormatTag=WAVE_FORMAT_PCM;f.nChannels=2;f.nSamplesPerSec=SR;f.wBitsPerSample=16;f.nBlockAlign=4;f.nAvgBytesPerSec=SR*4;
    if(waveOutOpen(&wave,WAVE_MAPPER,&f,(DWORD_PTR)win,0,CALLBACK_WINDOW)!=MMSYSERR_NOERROR){wave=nullptr;return;}
    waveHdr={};waveHdr.lpData=(LPSTR)playBuffer.data();waveHdr.dwBufferLength=(DWORD)std::min<size_t>(playBuffer.size()*sizeof(int16_t),0xFFFFFFFFu);
    if(loopOnNav){waveHdr.dwFlags=WHDR_BEGINLOOP|WHDR_ENDLOOP;waveHdr.dwLoops=0xFFFFFFFFu;}
    if(waveOutPrepareHeader(wave,&waveHdr,sizeof(waveHdr))!=MMSYSERR_NOERROR||waveOutWrite(wave,&waveHdr,sizeof(waveHdr))!=MMSYSERR_NOERROR){stopAudio();return;}
    playing=true;SetTimer(win,1,33,nullptr);InvalidateRect(win,nullptr,FALSE);
}

LRESULT CALLBACK proc_v17_nav(HWND h,UINT m,WPARAM w,LPARAM l){
    POINT p{GET_X_LPARAM(l),GET_Y_LPARAM(l)};
    if(m==WM_PAINT){LRESULT r=proc_v17(h,m,w,l);HDC d=GetDC(h);if(d){drawNavTools(d);ReleaseDC(h,d);}return r;}
    if(m==WM_MOUSEWHEEL||m==WM_MOUSEHWHEEL){POINT q{GET_X_LPARAM(l),GET_Y_LPARAM(l)};ScreenToClient(h,&q);if(inside(timeline10,q)){short delta=GET_WHEEL_DELTA_WPARAM(w);bool ctrl=(GET_KEYSTATE_WPARAM(w)&MK_CONTROL)!=0;if(ctrl)zoomNav(delta>0?1:-1);else scrollTimelineNav(delta<0?1:-1);return 0;}}
    if(m==WM_LBUTTONDOWN){
        if(inside(playR,p)){playNav();return 0;} if(inside(setLoopNav,p)){setLoopNavFromSelection();return 0;}
        if(inside(loopNav,p)){if(loopOnNav)loopOnNav=false;else setLoopNavFromSelection();InvalidateRect(h,nullptr,FALSE);return 0;}
        if(inside(fitNav,p)){fitNav();return 0;} if(inside(zoomOutNav,p)){zoomNav(-1);return 0;} if(inside(zoomInNav,p)){zoomNav(1);return 0;}
    }
    if(m==WM_KEYDOWN){bool ctrl=(GetKeyState(VK_CONTROL)&0x8000)!=0;if(w==VK_SPACE){playNav();return 0;}if(ctrl&&w=='L'){if(loopOnNav)loopOnNav=false;else setLoopNavFromSelection();InvalidateRect(h,nullptr,FALSE);return 0;}if(ctrl&&(w==VK_OEM_PLUS||w==VK_ADD)){zoomNav(1);return 0;}if(ctrl&&(w==VK_OEM_MINUS||w==VK_SUBTRACT)){zoomNav(-1);return 0;}}
    return proc_v17(h,m,w,l);
}
}

int WINAPI wWinMain(HINSTANCE hi,HINSTANCE,LPWSTR,int){
    for(int i=0;i<V6_MAX_TRACKS;i++)colors15[i]=defaultColor15(i);v6Fresh();editorOpen=true;sound9[1]=Sound9::SubBass;sound9[2]=Sound9::SawLead;master=100;
    WNDCLASSW wc{};wc.lpfnWndProc=proc_v17_nav;wc.hInstance=hi;wc.lpszClassName=L"BoomifyAlpha6Nav";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);wc.style=CS_DBLCLKS;RegisterClassW(&wc);
    win=CreateWindowExW(0,wc.lpszClassName,L"Boomify Studio - Alpha 6 Neon",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1440,900,nullptr,nullptr,hi,nullptr);if(!win)return 1;sync16(win);ShowWindow(win,SW_MAXIMIZE);UpdateWindow(win);
    MSG msg{};while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}return(int)msg.wParam;
}
