// Boomify V16 - Alpha 6 canonical responsive geometry
// V15 stays the visual/functional owner. V16 owns geometry only.
#define BOOMIFY_V6_NO_ENTRY
#define wWinMain wWinMain_v15_engine16
#include "main_v15.cpp"
#undef wWinMain
#undef BOOMIFY_V6_NO_ENTRY

namespace {
struct Geometry16{LONG left=0,trackRight=230,timelineRight=0,top=116,rowsTop=158,split=0,bottom=0;};
Geometry16 geo16{};
bool aligning16=false;

void alignWorkspace16(HWND h){
    if(aligning16)return;
    aligning16=true;
    RECT c{};GetClientRect(h,&c);
    const int trackW=std::clamp((int)(c.right*0.15),210,250);
    const int inspectorW=std::clamp((int)(c.right*0.20),300,380);
    const int right=std::max(trackW+420,(int)c.right-inspectorW);
    const int editorH=editorOpen?std::clamp((int)(c.bottom*.34),260,390):0;
    const int split=editorOpen?std::max(360,(int)c.bottom-editorH):(int)c.bottom;
    const int top=116;

    geo16={0,(LONG)trackW,(LONG)right,(LONG)top,(LONG)(top+PANEL_HEADER15),(LONG)split,c.bottom};

    // ONE geometry for every subsystem. No value is copied from an older layout.
    trackList10={0,geo16.top,geo16.trackRight,geo16.split};
    timeline10={geo16.trackRight,geo16.top,geo16.timelineRight,geo16.split};
    inspector10={geo16.timelineRight,70,c.right,c.bottom};
    browser10={0,geo16.split,geo16.timelineRight,c.bottom};
    editorArea11={0,geo16.split,geo16.timelineRight,c.bottom};

    panels15.tracks=trackList10;
    panels15.timeline=timeline10;
    panels15.inspector={geo16.timelineRight,geo16.top,c.right,geo16.split};

    const int laneCount=std::max(1,(int)lanes.size());
    const int rowSpace=std::max(1,(int)(geo16.split-geo16.rowsTop));
    panels15.rowH=std::clamp(rowSpace/laneCount,34,58);

    // Legacy hit rectangles are snapped to the exact same row rectangles used by V15 paint.
    for(int i=0;i<V6_MAX_TRACKS;i++){
        laneHeadR[i]={0,0,0,0};laneMuteR[i]={0,0,0,0};soloR[i]={0,0,0,0};laneVolR[i]={0,0,0,0};
    }
    for(int i=0;i<(int)lanes.size();i++){
        RECT rr=row15(i);
        laneHeadR[i]={8,rr.top,(LONG)std::max(9,trackW-18),rr.bottom};
        laneMuteR[i]={(LONG)std::max(8,trackW-80),rr.top+7,(LONG)std::max(32,trackW-54),rr.top+29};
        soloR[i]={(LONG)std::max(36,trackW-50),rr.top+7,(LONG)std::max(60,trackW-24),rr.top+29};
        laneVolR[i]={18,(LONG)std::max((int)rr.top+32,(int)rr.bottom-13),(LONG)std::max(80,trackW-98),(LONG)std::max((int)rr.top+38,(int)rr.bottom-7)};
    }
    aligning16=false;
}

LRESULT CALLBACK proc_v16(HWND h,UINT m,WPARAM w,LPARAM l){
    if(m==WM_SIZE){
        // Let legacy controls update, then overwrite every workspace coordinate atomically.
        LRESULT r=proc_v15(h,m,w,l);
        alignWorkspace16(h);
        InvalidateRect(h,nullptr,FALSE);
        return r;
    }
    if(m==WM_PAINT){
        // proc_v15 calls layoutPanels15() during paint; feed it canonical legacy bounds first.
        alignWorkspace16(h);
        return proc_v15(h,m,w,l);
    }
    alignWorkspace16(h);
    return proc_v15(h,m,w,l);
}
}

int WINAPI wWinMain(HINSTANCE hi,HINSTANCE,LPWSTR,int){
    for(int i=0;i<V6_MAX_TRACKS;i++)colors15[i]=defaultColor15(i);
    v6Fresh();editorOpen=true;sound9[1]=Sound9::SubBass;sound9[2]=Sound9::SawLead;master=100;
    WNDCLASSW wc{};wc.lpfnWndProc=proc_v16;wc.hInstance=hi;wc.lpszClassName=L"BoomifyAlpha6Aligned";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);wc.style=CS_DBLCLKS;RegisterClassW(&wc);
    win=CreateWindowExW(0,wc.lpszClassName,L"Boomify Studio - Alpha 6",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1440,900,nullptr,nullptr,hi,nullptr);if(!win)return 1;
    layout11(win);alignWorkspace16(win);ShowWindow(win,SW_MAXIMIZE);UpdateWindow(win);
    MSG msg{};while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}return(int)msg.wParam;
}