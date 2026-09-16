// Boomify V16 - Alpha 6 canonical row bridge
// V15 already draws tracks + timeline from row15(). This bridge no longer invents a second
// responsive layout. It only snaps the legacy editor boundary to the exact V15 panel boundary.
#define BOOMIFY_V6_NO_ENTRY
#define wWinMain wWinMain_v15_engine16
#include "main_v15.cpp"
#undef wWinMain
#undef BOOMIFY_V6_NO_ENTRY

namespace {
void sync16(HWND h){
    // First let the validated layout calculate itself exactly once.
    layout11(h);
    layoutPanels15(h);

    // V15 is the source of truth. Never recalculate track width, inspector width, row height,
    // timeline top or timeline bottom here.
    trackList10=panels15.tracks;
    timeline10=panels15.timeline;

    // The lower editor starts on the SAME physical pixel as the timeline ends.
    editorArea11.left=0;
    editorArea11.top=panels15.timeline.bottom;
    editorArea11.right=panels15.timeline.right;
    browser10=editorArea11;

    // Keep the old inspector hit area behind the actual V15 inspector instead of beside it.
    inspector10.left=panels15.inspector.left;
    inspector10.top=panels15.inspector.top;
    inspector10.right=panels15.inspector.right;
    inspector10.bottom=GetSystemMetrics(SM_CYSCREEN)*2;

    // Every legacy lane hitbox is derived from the exact rectangle V15 paints.
    for(int i=0;i<V6_MAX_TRACKS;i++){
        laneHeadR[i]={0,0,0,0};laneMuteR[i]={0,0,0,0};soloR[i]={0,0,0,0};laneVolR[i]={0,0,0,0};
    }
    for(int i=0;i<(int)lanes.size();++i){
        RECT rr=trackRow15(i);
        laneHeadR[i]=rr;
    }
}

LRESULT CALLBACK proc_v16(HWND h,UINT m,WPARAM w,LPARAM l){
    if(m==WM_SIZE){
        LRESULT r=proc_v14_base15(h,m,w,l);
        sync16(h);
        InvalidateRect(h,nullptr,FALSE);
        return r;
    }
    if(m==WM_PAINT){
        sync16(h);
        return proc_v15(h,m,w,l);
    }
    return proc_v15(h,m,w,l);
}
}

int WINAPI wWinMain(HINSTANCE hi,HINSTANCE,LPWSTR,int){
    for(int i=0;i<V6_MAX_TRACKS;i++)colors15[i]=defaultColor15(i);
    v6Fresh();editorOpen=true;sound9[1]=Sound9::SubBass;sound9[2]=Sound9::SawLead;master=100;
    WNDCLASSW wc{};wc.lpfnWndProc=proc_v16;wc.hInstance=hi;wc.lpszClassName=L"BoomifyAlpha6Aligned";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);wc.style=CS_DBLCLKS;RegisterClassW(&wc);
    win=CreateWindowExW(0,wc.lpszClassName,L"Boomify Studio - Alpha 6",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1440,900,nullptr,nullptr,hi,nullptr);if(!win)return 1;
    sync16(win);ShowWindow(win,SW_MAXIMIZE);UpdateWindow(win);
    MSG msg{};while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}return(int)msg.wParam;
}