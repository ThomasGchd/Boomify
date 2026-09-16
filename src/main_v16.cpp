// Boomify V16 - Alpha 6 modular workspace shell
#define wWinMain wWinMain_v15_engine16
#include "main_v15.cpp"
#undef wWinMain

namespace {
struct Workspace16{RECT header{},tracks{},timeline{},fx{},editor{};int tracksW=230;};
Workspace16 ws16{};

void layout16(HWND h){
    RECT c{};GetClientRect(h,&c);
    const int headerH=72;
    const int fxW=std::clamp((int)(c.right*0.21),300,380);
    const int editorH=std::clamp((int)(c.bottom*0.31),220,320);
    const int workBottom=std::max(headerH+180,(int)c.bottom-editorH);
    ws16.header={0,0,c.right,(LONG)headerH};
    ws16.tracks={0,(LONG)headerH,(LONG)ws16.tracksW,(LONG)workBottom};
    ws16.fx={(LONG)(c.right-fxW),(LONG)headerH,c.right,c.bottom};
    ws16.timeline={(LONG)ws16.tracksW,(LONG)headerH,ws16.fx.left,(LONG)workBottom};
    ws16.editor={(LONG)ws16.tracksW,(LONG)workBottom,ws16.fx.left,c.bottom};
    panels15.tracks=ws16.tracks;panels15.timeline=ws16.timeline;panels15.inspector=ws16.fx;
    int rowsH=std::max(1,(int)(ws16.timeline.bottom-ws16.timeline.top)-PANEL_HEADER15);
    panels15.rowH=std::clamp(rowsH/std::max(1,(int)lanes.size()),34,58);
}
void panelFrame16(HDC d,const RECT&r,COLORREF bg){fill(d,r,bg);line(d,r.left,r.top,r.right,r.top,RGB(55,59,68));line(d,r.left,r.bottom-1,r.right,r.bottom-1,RGB(42,46,54));}
void header16(HDC d){panelFrame16(d,ws16.header,RGB(20,22,27));label(d,L"BOOMIFY",18,17,13,RGB(242,243,246),FW_BOLD);label(d,L"ALPHA 6",18,43,6,RGB(130,136,148),FW_BOLD);}
void editor16(HDC d){panelFrame16(d,ws16.editor,RGB(18,20,24));RECT bar{ws16.editor.left,ws16.editor.top,ws16.editor.right,ws16.editor.top+34};fill(d,bar,RGB(27,30,36));std::wstring t=L"EDITEUR";if(laneSelected>=0&&laneSelected<(int)lanes.size())t+=lanes[laneSelected].type==LaneType::Drums?L"  /  BATTERIE":lanes[laneSelected].type==LaneType::Audio?L"  /  AUDIO":L"  /  PIANO ROLL";label(d,t.c_str(),bar.left+12,bar.top+11,7,RGB(183,188,198),FW_BOLD);}
void workspace16(HDC d,RECT c){fill(d,c,RGB(16,18,22));header16(d);drawTracks15(d);drawTimeline15(d);editor16(d);draw15(d);line(d,ws16.tracks.right-1,ws16.tracks.top,ws16.tracks.right-1,ws16.tracks.bottom,RGB(73,77,88),2);line(d,ws16.fx.left,ws16.fx.top,ws16.fx.left,ws16.fx.bottom,RGB(73,77,88),2);line(d,ws16.editor.left,ws16.editor.top,ws16.editor.right,ws16.editor.top,RGB(73,77,88),2);}
LRESULT CALLBACK proc_v16(HWND h,UINT m,WPARAM w,LPARAM l){
    if(m==WM_SIZE){proc_v14_base15(h,m,w,l);layout16(h);InvalidateRect(h,nullptr,FALSE);return 0;}
    if(m==WM_PAINT){layout16(h);PAINTSTRUCT ps{};HDC s=BeginPaint(h,&ps);RECT c{};GetClientRect(h,&c);HDC mem=CreateCompatibleDC(s);HBITMAP bm=CreateCompatibleBitmap(s,std::max(1,(int)c.right),std::max(1,(int)c.bottom));HGDIOBJ old=SelectObject(mem,bm);draw11(mem,c);drawV13Chrome(mem);drawV14(mem);workspace16(mem,c);BitBlt(s,0,0,c.right,c.bottom,mem,0,0,SRCCOPY);SelectObject(mem,old);DeleteObject(bm);DeleteDC(mem);EndPaint(h,&ps);return 0;}
    layout16(h);return proc_v15(h,m,w,l);
}
}
int WINAPI wWinMain(HINSTANCE hi,HINSTANCE,LPWSTR,int){for(int i=0;i<V6_MAX_TRACKS;i++)colors15[i]=defaultColor15(i);v6Fresh();editorOpen=true;sound9[1]=Sound9::SubBass;sound9[2]=Sound9::SawLead;master=100;WNDCLASSW wc{};wc.lpfnWndProc=proc_v16;wc.hInstance=hi;wc.lpszClassName=L"BoomifyAlpha6Workspace";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);wc.style=CS_DBLCLKS;RegisterClassW(&wc);win=CreateWindowExW(0,wc.lpszClassName,L"Boomify Studio - Alpha 6 Workspace",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1440,900,nullptr,nullptr,hi,nullptr);if(!win)return 1;layout11(win);layout16(win);ShowWindow(win,SW_MAXIMIZE);UpdateWindow(win);MSG msg{};while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}return(int)msg.wParam;}