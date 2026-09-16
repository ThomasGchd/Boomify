// Boomify V16 - Alpha 6 clean modular workspace
// Layout validation shell: one owner per visible region, no stacked workspace painters.
#define BOOMIFY_V6_NO_ENTRY
#define wWinMain wWinMain_v15_engine16
#include "main_v15.cpp"
#undef wWinMain
#undef BOOMIFY_V6_NO_ENTRY

namespace {
struct Workspace16{RECT header{},tracks{},timeline{},fx{},editor{};int tracksW=230;};
Workspace16 ws16{};
constexpr int HEADER16=128;

void layout16(HWND h){
    RECT c{};GetClientRect(h,&c);
    const int fxW=std::clamp((int)(c.right*0.20),330,390);
    const int split=editorOpen?std::max(455,(int)(c.bottom*.64)):c.bottom-45;
    ws16.header={0,0,c.right,(LONG)HEADER16};
    ws16.tracks={0,(LONG)HEADER16,(LONG)ws16.tracksW,(LONG)split};
    ws16.fx={(LONG)std::max(ws16.tracksW+480,(int)c.right-fxW),(LONG)HEADER16,c.right,c.bottom};
    ws16.timeline={(LONG)ws16.tracksW,(LONG)HEADER16,ws16.fx.left,(LONG)split};
    ws16.editor={0,(LONG)split,ws16.fx.left,c.bottom};
    panels15.tracks=ws16.tracks;panels15.timeline=ws16.timeline;panels15.inspector=ws16.fx;
    int rowsH=std::max(1,(int)(ws16.timeline.bottom-ws16.timeline.top)-PANEL_HEADER15);
    panels15.rowH=std::clamp(rowsH/std::max(1,(int)lanes.size()),34,58);
}

void clearOwnedPanels16(HDC d){
    // Erase every legacy pixel in the three regions now owned by V16/V15.
    // The lower editor is deliberately excluded: its validated painter remains the sole owner there.
    fill(d,ws16.tracks,RGB(24,26,31));
    fill(d,ws16.timeline,RGB(19,21,26));
    fill(d,ws16.fx,RGB(22,24,29));
}
void workspace16(HDC d){
    clearOwnedPanels16(d);
    drawTracks15(d);
    drawTimeline15(d);
    draw15(d);
    const COLORREF sep=RGB(67,71,82);
    line(d,0,HEADER16-1,ws16.fx.right,HEADER16-1,sep,2);
    line(d,ws16.tracks.right-1,ws16.tracks.top,ws16.tracks.right-1,ws16.tracks.bottom,sep,1);
    line(d,ws16.fx.left,ws16.fx.top,ws16.fx.left,ws16.fx.bottom,sep,1);
    line(d,ws16.editor.left,ws16.editor.top,ws16.editor.right,ws16.editor.top,sep,2);
}

void paint16(HWND h,HDC target){
    RECT c{};GetClientRect(h,&c);
    HDC mem=CreateCompatibleDC(target);
    HBITMAP bm=CreateCompatibleBitmap(target,std::max(1,(int)c.right),std::max(1,(int)c.bottom));
    HGDIOBJ old=SelectObject(mem,bm);
    fill(mem,c,RGB(16,18,22));
    // Legacy pass is retained ONLY because it still owns the functional transport and lower editor.
    // Its track/timeline/FX pixels are immediately erased before the modular painters run.
    draw11(mem,c);drawV13Chrome(mem);drawV14(mem);
    workspace16(mem);
    BitBlt(target,0,0,c.right,c.bottom,mem,0,0,SRCCOPY);
    SelectObject(mem,old);DeleteObject(bm);DeleteDC(mem);
}

LRESULT CALLBACK proc_v16(HWND h,UINT m,WPARAM w,LPARAM l){
    if(m==WM_SIZE){
        proc_v14_base15(h,m,w,l); // keeps lower editor hitboxes synchronized
        layout16(h);InvalidateRect(h,nullptr,FALSE);return 0;
    }
    if(m==WM_ERASEBKGND)return 1; // double-buffered paint owns the full frame; prevents white/legacy flashes
    if(m==WM_PAINT){
        layout16(h);PAINTSTRUCT ps{};HDC s=BeginPaint(h,&ps);paint16(h,s);EndPaint(h,&ps);return 0;
    }
    layout16(h);return proc_v15(h,m,w,l);
}
}

int WINAPI wWinMain(HINSTANCE hi,HINSTANCE,LPWSTR,int){
    for(int i=0;i<V6_MAX_TRACKS;i++)colors15[i]=defaultColor15(i);
    v6Fresh();editorOpen=true;sound9[1]=Sound9::SubBass;sound9[2]=Sound9::SawLead;master=100;
    WNDCLASSW wc{};wc.lpfnWndProc=proc_v16;wc.hInstance=hi;wc.lpszClassName=L"BoomifyAlpha6Workspace";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hbrBackground=nullptr;wc.style=CS_DBLCLKS;RegisterClassW(&wc);
    win=CreateWindowExW(0,wc.lpszClassName,L"Boomify Studio - Alpha 6 Workspace",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1440,900,nullptr,nullptr,hi,nullptr);if(!win)return 1;
    layout11(win);layout16(win);ShowWindow(win,SW_MAXIMIZE);UpdateWindow(win);
    MSG msg{};while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}return(int)msg.wParam;
}