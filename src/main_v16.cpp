// Boomify V16 - Alpha 6 geometry bridge only
// No extra renderer: V15 remains the sole visual owner. This file only forces every
// functional region to share the same responsive boundaries.
#define BOOMIFY_V6_NO_ENTRY
#define wWinMain wWinMain_v15_engine16
#include "main_v15.cpp"
#undef wWinMain
#undef BOOMIFY_V6_NO_ENTRY

namespace {
void alignWorkspace16(HWND h){
    RECT c{}; GetClientRect(h,&c);
    const int trackW=230;
    const int inspectorW=std::clamp((int)(c.right*0.20),300,380);
    const int right=std::max(trackW+320,(int)c.right-inspectorW);

    // layout11 owns the responsive vertical split because the functional piano-roll/drum
    // editor uses editorArea11. We keep that split and make every V15 panel snap to it.
    const LONG top=timeline10.top;
    const LONG bottom=editorArea11.top;

    trackList10={0,top,(LONG)trackW,bottom};
    timeline10={(LONG)trackW,top,(LONG)right,bottom};
    inspector10={(LONG)right,70,c.right,c.bottom};
    browser10={0,bottom,(LONG)right,c.bottom};
    editorArea11={0,bottom,(LONG)right,c.bottom};

    panels15.tracks={0,top,(LONG)trackW,bottom};
    panels15.timeline={(LONG)trackW,top,(LONG)right,bottom};
    panels15.inspector={(LONG)right,top,c.right,bottom};

    const int usable=std::max(1,(int)(bottom-top)-PANEL_HEADER15);
    panels15.rowH=std::clamp(usable/std::max(1,(int)lanes.size()),34,58);
}

LRESULT CALLBACK proc_v16(HWND h,UINT m,WPARAM w,LPARAM l){
    if(m==WM_SIZE){
        // Let the validated engine calculate its responsive editor height first.
        LRESULT r=proc_v15(h,m,w,l);
        alignWorkspace16(h);
        InvalidateRect(h,nullptr,FALSE);
        return r;
    }
    // proc_v15 recalculates panels during paint, so make the legacy/editor boundaries
    // agree with that calculation before every event. No painting is added here.
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