// Boomify V16 - Alpha 6 isolated workspace layout
// This shell keeps the validated V15 engine/interactions and owns only layout/paint regions.
// V6 has an unguarded legacy entry point in the include chain: suppress it explicitly
// before renaming V15's entry point, otherwise MSVC sees two identical wWinMain symbols.
#define BOOMIFY_V6_NO_ENTRY
#define wWinMain wWinMain_v15_engine16
#include "main_v15.cpp"
#undef wWinMain
#undef BOOMIFY_V6_NO_ENTRY

namespace {
struct Workspace16{RECT header{},tracks{},timeline{},fx{},editor{};int tracksW=230;};
Workspace16 ws16{};

void layout16(HWND h){
    RECT c{};GetClientRect(h,&c);
    // Five independent regions. Nothing derives its X/Y from another renderer anymore.
    const int headerH=128;
    const int fxW=std::clamp((int)(c.right*0.21),360,420);
    const int workBottom=std::clamp((int)(c.bottom*0.64),headerH+240,(int)c.bottom-220);
    ws16.header={0,0,c.right,(LONG)headerH};
    ws16.tracks={0,(LONG)headerH,(LONG)ws16.tracksW,(LONG)workBottom};
    ws16.fx={(LONG)std::max(ws16.tracksW+420,(int)c.right-fxW),(LONG)headerH,c.right,c.bottom};
    ws16.timeline={(LONG)ws16.tracksW,(LONG)headerH,ws16.fx.left,(LONG)workBottom};
    // The editor deliberately owns the complete lower-left workspace. The FX panel never enters it.
    ws16.editor={0,(LONG)workBottom,ws16.fx.left,c.bottom};

    panels15.tracks=ws16.tracks;
    panels15.timeline=ws16.timeline;
    panels15.inspector=ws16.fx;
    int rowsH=std::max(1,(int)(ws16.timeline.bottom-ws16.timeline.top)-PANEL_HEADER15);
    panels15.rowH=std::clamp(rowsH/std::max(1,(int)lanes.size()),34,58);
}

void frame16(HDC d,const RECT&r){
    line(d,r.left,r.top,r.right,r.top,RGB(64,68,78));
    line(d,r.left,r.bottom-1,r.right,r.bottom-1,RGB(43,46,54));
}
void editorFrame16(HDC d){
    // Do NOT repaint the editor body: the validated V14/V15 drum/piano editor stays functional underneath.
    // We only establish a hard visual boundary and cover the tiny seam between regions.
    RECT seam{ws16.editor.left,ws16.editor.top,ws16.editor.right,ws16.editor.top+3};
    fill(d,seam,RGB(64,68,78));
}
void workspace16(HDC d){
    // Header: legacy transport remains visible and functional, but is now a formally isolated region.
    line(d,ws16.header.left,ws16.header.bottom-1,ws16.header.right,ws16.header.bottom-1,RGB(64,68,78),2);

    // Track list, timeline and FX each repaint only their own rectangle.
    drawTracks15(d);
    drawTimeline15(d);
    draw15(d);

    // Hard panel separators. The inspector is full-height below the header, so legacy FX can no longer
    // leak over the lower editor/piano roll (the bug visible in the previous build).
    line(d,ws16.tracks.right-1,ws16.tracks.top,ws16.tracks.right-1,ws16.tracks.bottom,RGB(73,77,88),2);
    line(d,ws16.fx.left,ws16.fx.top,ws16.fx.left,ws16.fx.bottom,RGB(73,77,88),2);
    editorFrame16(d);
    frame16(d,ws16.timeline);
}

LRESULT CALLBACK proc_v16(HWND h,UINT m,WPARAM w,LPARAM l){
    if(m==WM_SIZE){
        // Let the validated engine update its editor hitboxes first, then impose our independent panels.
        proc_v14_base15(h,m,w,l);
        layout16(h);
        InvalidateRect(h,nullptr,FALSE);
        return 0;
    }
    if(m==WM_PAINT){
        layout16(h);
        PAINTSTRUCT ps{};HDC s=BeginPaint(h,&ps);RECT c{};GetClientRect(h,&c);
        HDC mem=CreateCompatibleDC(s);
        HBITMAP bm=CreateCompatibleBitmap(s,std::max(1,(int)c.right),std::max(1,(int)c.bottom));
        HGDIOBJ old=SelectObject(mem,bm);

        // One legacy base pass for the transport and functional lower editor.
        // Everything in the workspace is then covered exactly once by its owning V16 panel.
        draw11(mem,c);
        drawV13Chrome(mem);
        drawV14(mem);
        workspace16(mem);

        BitBlt(s,0,0,c.right,c.bottom,mem,0,0,SRCCOPY);
        SelectObject(mem,old);DeleteObject(bm);DeleteDC(mem);EndPaint(h,&ps);return 0;
    }
    layout16(h);
    return proc_v15(h,m,w,l);
}
}

int WINAPI wWinMain(HINSTANCE hi,HINSTANCE,LPWSTR,int){
    for(int i=0;i<V6_MAX_TRACKS;i++)colors15[i]=defaultColor15(i);
    v6Fresh();editorOpen=true;sound9[1]=Sound9::SubBass;sound9[2]=Sound9::SawLead;master=100;
    WNDCLASSW wc{};wc.lpfnWndProc=proc_v16;wc.hInstance=hi;wc.lpszClassName=L"BoomifyAlpha6Workspace";
    wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);wc.style=CS_DBLCLKS;
    RegisterClassW(&wc);
    win=CreateWindowExW(0,wc.lpszClassName,L"Boomify Studio - Alpha 6 Workspace",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1440,900,nullptr,nullptr,hi,nullptr);
    if(!win)return 1;
    layout11(win);layout16(win);ShowWindow(win,SW_MAXIMIZE);UpdateWindow(win);
    MSG msg{};while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}return(int)msg.wParam;
}