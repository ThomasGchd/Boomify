// Boomify V16 - Alpha 6 balanced workspace layout
#define BOOMIFY_V6_NO_ENTRY
#define wWinMain wWinMain_v15_engine16
#include "main_v15.cpp"
#undef wWinMain
#undef BOOMIFY_V6_NO_ENTRY

namespace {
struct Workspace16{RECT header{},tracks{},timeline{},fx{},editor{};int tracksW=230;};
Workspace16 ws16{};
constexpr int HEADER16=126;
constexpr int EDITOR_BAR16=38;

void layout16(HWND h){
    RECT c{};GetClientRect(h,&c);
    const int fxW=std::clamp((int)(c.right*0.19),320,370);
    const int minEditor=240;
    const int workBottom=std::clamp((int)(c.bottom*0.58),HEADER16+250,(int)c.bottom-minEditor);
    ws16.header={0,0,c.right,(LONG)HEADER16};
    ws16.tracks={0,(LONG)HEADER16,(LONG)ws16.tracksW,(LONG)workBottom};
    ws16.fx={(LONG)std::max(ws16.tracksW+480,(int)c.right-fxW),(LONG)HEADER16,c.right,c.bottom};
    ws16.timeline={(LONG)ws16.tracksW,(LONG)HEADER16,ws16.fx.left,(LONG)workBottom};
    // Lower editor aligns with the timeline, not with the window edge. This removes the strange
    // empty/legacy strip under the track headers and gives the app a conventional DAW silhouette.
    ws16.editor={(LONG)ws16.tracksW,(LONG)workBottom,ws16.fx.left,c.bottom};
    panels15.tracks=ws16.tracks;panels15.timeline=ws16.timeline;panels15.inspector=ws16.fx;
    int rowsH=std::max(1,(int)(ws16.timeline.bottom-ws16.timeline.top)-PANEL_HEADER15);
    panels15.rowH=std::clamp(rowsH/std::max(1,(int)lanes.size()),38,58);
}

void coverWorkspace16(HDC d){
    // Cover legacy workspace paint completely. Only the top transport is kept from the old renderer.
    fill(d,ws16.tracks,RGB(22,24,29));
    fill(d,ws16.timeline,RGB(19,21,26));
    fill(d,ws16.fx,RGB(22,24,29));
    fill(d,ws16.editor,RGB(18,20,24));
}
void editor16(HDC d){
    RECT bar{ws16.editor.left,ws16.editor.top,ws16.editor.right,ws16.editor.top+EDITOR_BAR16};
    fill(d,bar,RGB(27,30,36));
    std::wstring title=L"EDITEUR";
    if(laneSelected>=0&&laneSelected<(int)lanes.size()){
        if(lanes[laneSelected].type==LaneType::Drums)title+=L"  /  BATTERIE";
        else if(lanes[laneSelected].type==LaneType::Audio)title+=L"  /  AUDIO";
        else title+=L"  /  PIANO ROLL";
    }
    label(d,title.c_str(),bar.left+14,bar.top+13,7,RGB(190,194,203),FW_BOLD);
    line(d,bar.left,bar.bottom-1,bar.right,bar.bottom-1,RGB(53,57,66));
    // The actual note/drum editor still comes from the validated engine for now. Its old pixels are
    // intentionally not allowed to bleed into the other four panels in this shell.
    RECT body{ws16.editor.left+1,bar.bottom,ws16.editor.right-1,ws16.editor.bottom-1};
    fill(d,body,RGB(20,22,27));
    label(d,L"Editeur de la piste selectionnee",body.left+16,body.top+16,7,RGB(108,114,126));
}
void separators16(HDC d){
    COLORREF strong=RGB(67,71,82),soft=RGB(45,49,57);
    line(d,0,HEADER16-1,ws16.fx.right,HEADER16-1,strong,2);
    line(d,ws16.tracks.right-1,ws16.tracks.top,ws16.tracks.right-1,ws16.tracks.bottom,strong,1);
    line(d,ws16.fx.left,ws16.fx.top,ws16.fx.left,ws16.fx.bottom,strong,1);
    line(d,ws16.editor.left,ws16.editor.top,ws16.editor.right,ws16.editor.top,strong,2);
    line(d,0,ws16.tracks.bottom-1,ws16.tracks.right,ws16.tracks.bottom-1,soft,1);
}
void workspace16(HDC d){
    coverWorkspace16(d);
    drawTracks15(d);
    drawTimeline15(d);
    draw15(d);
    editor16(d);
    separators16(d);
}

LRESULT CALLBACK proc_v16(HWND h,UINT m,WPARAM w,LPARAM l){
    if(m==WM_SIZE){proc_v14_base15(h,m,w,l);layout16(h);InvalidateRect(h,nullptr,FALSE);return 0;}
    if(m==WM_PAINT){
        layout16(h);PAINTSTRUCT ps{};HDC s=BeginPaint(h,&ps);RECT c{};GetClientRect(h,&c);
        HDC mem=CreateCompatibleDC(s);HBITMAP bm=CreateCompatibleBitmap(s,std::max(1,(int)c.right),std::max(1,(int)c.bottom));HGDIOBJ old=SelectObject(mem,bm);
        // Keep the validated top transport only; V16 owns every pixel below it.
        draw11(mem,c);drawV13Chrome(mem);drawV14(mem);
        workspace16(mem);
        BitBlt(s,0,0,c.right,c.bottom,mem,0,0,SRCCOPY);SelectObject(mem,old);DeleteObject(bm);DeleteDC(mem);EndPaint(h,&ps);return 0;
    }
    layout16(h);return proc_v15(h,m,w,l);
}
}

int WINAPI wWinMain(HINSTANCE hi,HINSTANCE,LPWSTR,int){
    for(int i=0;i<V6_MAX_TRACKS;i++)colors15[i]=defaultColor15(i);
    v6Fresh();editorOpen=true;sound9[1]=Sound9::SubBass;sound9[2]=Sound9::SawLead;master=100;
    WNDCLASSW wc{};wc.lpfnWndProc=proc_v16;wc.hInstance=hi;wc.lpszClassName=L"BoomifyAlpha6Workspace";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);wc.style=CS_DBLCLKS;RegisterClassW(&wc);
    win=CreateWindowExW(0,wc.lpszClassName,L"Boomify Studio - Alpha 6 Workspace",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1440,900,nullptr,nullptr,hi,nullptr);if(!win)return 1;
    layout11(win);layout16(win);ShowWindow(win,SW_MAXIMIZE);UpdateWindow(win);MSG msg{};while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}return(int)msg.wParam;
}