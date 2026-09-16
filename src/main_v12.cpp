// Boomify Alpha 5 - DAW interaction/navigation restoration layer
// Keep V11 DSP/editor/FX, replace only the workspace interaction + playhead overlay.
#define BOOMIFY_V12_INCLUDE
#include "main_v11.cpp"
#undef BOOMIFY_V12_INCLUDE

namespace {
bool range12=false;
POINT rangeStart12{},rangeNow12{};
int anchorLane12=-1,anchorBar12=-1;

void syncSelection12(int lane,int bar,bool add,bool extend){
    lane=std::clamp(lane,0,(int)lanes.size()-1); bar=std::clamp(bar,0,activeBars-1);
    if(extend && anchorLane12>=0){
        clearMulti();
        int l0=std::min(anchorLane12,lane),l1=std::max(anchorLane12,lane);
        int b0=std::min(anchorBar12,bar),b1=std::max(anchorBar12,bar);
        for(int l=l0;l<=l1;l++) for(int b=b0;b<=b1;b++) addMulti(l,b);
    } else {
        if(!add) clearMulti();
        bool found=false; for(auto&q:multiSelection) if(q.track==lane&&q.bar==bar){found=true;break;}
        if(add&&found){ multiSelection.erase(std::remove_if(multiSelection.begin(),multiSelection.end(),[&](const BlockRef&q){return q.track==lane&&q.bar==bar;}),multiSelection.end()); }
        else addMulti(lane,bar);
        anchorLane12=lane; anchorBar12=bar;
    }
    laneSelected=lane; selectedTrack=std::min(TRACKS-1,lane); selectedBar=bar;
}

void selectRange12(POINT a,POINT b){
    clearMulti();
    RECT rr{std::min(a.x,b.x),std::min(a.y,b.y),std::max(a.x,b.x)+1,std::max(a.y,b.y)+1};
    for(int l=0;l<(int)lanes.size();l++) for(int v=0;v<visibleBars;v++){
        int bar=scrollBar+v; if(bar>=activeBars) continue;
        RECT q=cell11(l,v),inter{}; if(IntersectRect(&inter,&rr,&q)) addMulti(l,bar);
    }
    if(!multiSelection.empty()){
        laneSelected=multiSelection.front().track; selectedTrack=std::min(TRACKS-1,laneSelected); selectedBar=multiSelection.front().bar;
        anchorLane12=laneSelected; anchorBar12=selectedBar;
    }
}

void setPlayhead12(POINT p){
    if(p.x<(int)timeline10.left||p.x>=(int)timeline10.right)return;
    int w=std::max(1,(int)(timeline10.right-timeline10.left));
    int v=std::clamp((int)((long long)(p.x-(int)timeline10.left)*visibleBars/w),0,visibleBars-1);
    cursorBar=std::clamp(scrollBar+v,0,activeBars-1);
    InvalidateRect(win,nullptr,FALSE);
}

int currentPlayBar12(){
    if(!playing||!wave) return cursorBar;
    MMTIME mt{}; mt.wType=TIME_SAMPLES;
    if(waveOutGetPosition(wave,&mt,sizeof(mt))!=MMSYSERR_NOERROR) return cursorBar;
    double sec=(double)mt.u.sample/SR;
    double barSec=(60.0/std::max(1,bpm))*4.0;
    return std::clamp(playStartBar+(int)(sec/barSec),0,activeBars-1);
}

void overlay12(HDC d){
    int bar=currentPlayBar12();
    if(bar>=scrollBar&&bar<scrollBar+visibleBars){
        int v=bar-scrollBar,w=std::max(1,(int)(timeline10.right-timeline10.left));
        int x=(int)timeline10.left+(int)((long long)v*w/visibleBars);
        line(d,x,(int)timeline10.top,x,(int)timeline10.bottom,RGB(255,104,75),3);
        RECT tag{(LONG)x,(LONG)timeline10.top,(LONG)std::min((int)timeline10.right,x+38),(LONG)((int)timeline10.top+20)};
        fill(d,tag,RGB(235,103,74));
        std::wstring n=std::to_wstring(bar+1); label(d,n.c_str(),x+8,(int)timeline10.top+5,7,RGB(255,255,255),FW_BOLD);
    }
    if(range12){
        RECT r{std::min(rangeStart12.x,rangeNow12.x),std::min(rangeStart12.y,rangeNow12.y),std::max(rangeStart12.x,rangeNow12.x),std::max(rangeStart12.y,rangeNow12.y)};
        line(d,r.left,r.top,r.right,r.top,RGB(245,245,245),2); line(d,r.left,r.bottom,r.right,r.bottom,RGB(245,245,245),2);
        line(d,r.left,r.top,r.left,r.bottom,RGB(245,245,245),2); line(d,r.right,r.top,r.right,r.bottom,RGB(245,245,245),2);
    }
}

LRESULT CALLBACK proc_v12(HWND h,UINT m,WPARAM wp,LPARAM lp){
    POINT p{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};
    if(m==WM_PAINT){
        PAINTSTRUCT ps; HDC s=BeginPaint(h,&ps); RECT c; GetClientRect(h,&c);
        HDC mem=CreateCompatibleDC(s); HBITMAP bm=CreateCompatibleBitmap(s,std::max(1,(int)c.right),std::max(1,(int)c.bottom)); auto old=SelectObject(mem,bm);
        draw11(mem,c); overlay12(mem); BitBlt(s,0,0,c.right,c.bottom,mem,0,0,SRCCOPY);
        SelectObject(mem,old); DeleteObject(bm); DeleteDC(mem); EndPaint(h,&ps); return 0;
    }
    if(m==WM_TIMER&&wp==1){ InvalidateRect(h,nullptr,FALSE); return proc_v11(h,m,wp,lp); }
    if(m==WM_LBUTTONDOWN && inside(timeline10,p)){
        int lane=-1,bar=-1; bool onCell=hitTimeline11(p,lane,bar);
        setPlayhead12(p);
        if(onCell){
            bool ctrl=(GetKeyState(VK_CONTROL)&0x8000)!=0,shift=(GetKeyState(VK_SHIFT)&0x8000)!=0;
            syncSelection12(lane,bar,ctrl,shift);
            if(!ctrl&&!shift){ range12=true; rangeStart12=rangeNow12=p; SetCapture(h); }
        }
        InvalidateRect(h,nullptr,FALSE); return 0;
    }
    if(m==WM_MOUSEMOVE&&range12){ rangeNow12=p; InvalidateRect(h,nullptr,FALSE); return 0; }
    if(m==WM_LBUTTONUP&&range12){
        rangeNow12=p; int dx=std::abs(rangeNow12.x-rangeStart12.x),dy=std::abs(rangeNow12.y-rangeStart12.y);
        if(dx>5||dy>5) selectRange12(rangeStart12,rangeNow12);
        range12=false; if(GetCapture()==h)ReleaseCapture(); InvalidateRect(h,nullptr,FALSE); return 0;
    }
    if(m==WM_KEYDOWN){
        bool ctrl=(GetKeyState(VK_CONTROL)&0x8000)!=0;
        if(ctrl&&wp=='D'){ duplicateV6(); return 0; }
        if(wp==VK_DELETE){ clearMultiBlocks(); return 0; }
        if(wp==VK_ESCAPE){ clearMulti(); range12=false; if(GetCapture()==h)ReleaseCapture(); InvalidateRect(h,nullptr,FALSE); return 0; }
    }
    return proc_v11(h,m,wp,lp);
}
}

int WINAPI wWinMain(HINSTANCE hi,HINSTANCE,PWSTR,int){
    WNDCLASSW wc{}; wc.lpfnWndProc=proc_v12; wc.hInstance=hi; wc.lpszClassName=L"BoomifyV12"; wc.hCursor=LoadCursor(nullptr,IDC_ARROW); RegisterClassW(&wc);
    win=CreateWindowExW(0,wc.lpszClassName,L"Boomify Alpha 5 - DAW Workspace",WS_OVERLAPPEDWINDOW,0,0,1600,960,nullptr,nullptr,hi,nullptr); if(!win)return 1;
    v6Fresh(); editorOpen=true; sound9[1]=Sound9::SubBass; sound9[2]=Sound9::SawLead; rack11[0]={{FxType11::SoftClip,true,35}}; layout11(win);
    ShowWindow(win,SW_MAXIMIZE); UpdateWindow(win); MSG msg{}; while(GetMessageW(&msg,nullptr,0,0)){TranslateMessage(&msg);DispatchMessageW(&msg);} return 0;
}
