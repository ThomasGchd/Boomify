// Boomify V14 - musical editor + interface polish
#define wWinMain wWinMain_v13_embedded
#include "main_v13.cpp"
#undef wWinMain

namespace {
bool paintNotes14=false, paintValue14=false; int lastRow14=-1,lastStep14=-1;
RECT clearNotes14{},shiftUp14{},shiftDown14{};
const wchar_t* noteClass14(int n){static const wchar_t* a[]={L"C",L"C#",L"D",L"D#",L"E",L"F",L"F#",L"G",L"G#",L"A",L"A#",L"B"};return a[(n%12+12)%12];}
std::wstring noteName14(int midi){return std::wstring(noteClass14(midi))+std::to_wstring(midi/12-1);}
bool noteCell14(POINT p,int&row,int&step){if(!editorOpen||laneSelected<0||laneSelected>=(int)lanes.size()||lanes[laneSelected].type!=LaneType::Instrument)return false;for(int r=0;r<NOTES;r++)for(int s=0;s<STEPS;s++)if(inside(np[r][s],p)){row=r;step=s;return true;}return false;}
void setNote14(int r,int s,bool value){if(laneSelected<0||laneSelected>=(int)lanes.size())return;noteForLane(laneSelected,selectedBar)[r][s]=value;InvalidateRect(win,nullptr,FALSE);}
void shiftNotes14(int dir){if(laneSelected<0||laneSelected>=(int)lanes.size()||lanes[laneSelected].type!=LaneType::Instrument)return;auto&n=noteForLane(laneSelected,selectedBar);NoteClip out{};for(auto&r:out)r.fill(false);for(int r=0;r<NOTES;r++)for(int s=0;s<STEPS;s++)if(n[r][s]){int nr=r-dir;if(nr>=0&&nr<NOTES)out[nr][s]=true;}n=out;InvalidateRect(win,nullptr,FALSE);}
void clearNotes14Fn(){if(laneSelected<0||laneSelected>=(int)lanes.size()||lanes[laneSelected].type!=LaneType::Instrument)return;clearNote(noteForLane(laneSelected,selectedBar));InvalidateRect(win,nullptr,FALSE);}
void drawV14(HDC d){RECT c;GetClientRect(win,&c);
 // Clean inspector header: V13 used to stack TRACK over the old INSPECTEUR title.
 if(inspector10.right>inspector10.left){RECT head{inspector10.left,70,inspector10.right,140};fill(d,head,RGB(18,20,25));if(laneSelected>=0&&laneSelected<(int)lanes.size()){label(d,L"INSPECTEUR DE PISTE",(int)inspector10.left+18,84,7,RGB(116,122,134),FW_BOLD);label(d,lanes[laneSelected].name.c_str(),(int)inspector10.left+18,106,13,RGB(239,241,244),FW_BOLD);}}
 if(!editorOpen||laneSelected<0||laneSelected>=(int)lanes.size()||lanes[laneSelected].type!=LaneType::Instrument)return;
 int top=(int)editorArea11.top; int base=familyOf(sound9[laneSelected])==Family9::Bass?47:71;
 // Real musical note names replace NOTE 1..12.
 for(int r=0;r<NOTES;r++){RECT q=np[r][0];if(q.bottom<=q.top)continue;RECT lab{0,q.top,85,q.bottom};fill(d,lab,((base-r)%12==0)?RGB(35,38,45):RGB(23,25,30));std::wstring nm=noteName14(base-r);label(d,nm.c_str(),22,(int)q.top+std::max(2,(int)(q.bottom-q.top)/2-5),8,((base-r)%12==0)?RGB(240,120,88):RGB(157,162,173),FW_BOLD);}
 // Beat accents and a clearer piano-roll rhythm grid.
 for(int s=0;s<STEPS;s+=4){if(np[0][s].right<=np[0][s].left)continue;line(d,np[0][s].left,np[0][s].top,np[0][s].left,np[NOTES-1][s].bottom,RGB(82,87,98),2);std::wstring beat=std::to_wstring(s/4+1);label(d,beat.c_str(),np[0][s].left+5,np[0][s].top-16,7,RGB(145,150,161),FW_BOLD);}
 clearNotes14={(LONG)(editorArea11.right-108),(LONG)(top+7),(LONG)(editorArea11.right-18),(LONG)(top+32)};shiftDown14={(LONG)(editorArea11.right-196),(LONG)(top+7),(LONG)(editorArea11.right-160),(LONG)(top+32)};shiftUp14={(LONG)(editorArea11.right-154),(LONG)(top+7),(LONG)(editorArea11.right-114),(LONG)(top+32)};
 fill(d,shiftDown14,RGB(40,44,52));fill(d,shiftUp14,RGB(40,44,52));fill(d,clearNotes14,RGB(40,44,52));label(d,L"-1",shiftDown14.left+10,shiftDown14.top+7,7,RGB(220,223,229),FW_BOLD);label(d,L"+1",shiftUp14.left+10,shiftUp14.top+7,7,RGB(220,223,229),FW_BOLD);label(d,L"EFFACER",clearNotes14.left+18,clearNotes14.top+7,7,RGB(232,137,118),FW_BOLD);
 label(d,L"Clic + glisser pour dessiner / effacer des notes",300,top+13,7,RGB(130,136,148));
}
LRESULT CALLBACK proc_v14(HWND h,UINT m,WPARAM w,LPARAM l){POINT p{GET_X_LPARAM(l),GET_Y_LPARAM(l)};
 if(m==WM_PAINT){PAINTSTRUCT ps;HDC s=BeginPaint(h,&ps);RECT c;GetClientRect(h,&c);HDC mem=CreateCompatibleDC(s);HBITMAP bm=CreateCompatibleBitmap(s,std::max(1,(int)c.right),std::max(1,(int)c.bottom));auto old=SelectObject(mem,bm);draw11(mem,c);overlay12(mem);drawV13Chrome(mem);drawV14(mem);BitBlt(s,0,0,c.right,c.bottom,mem,0,0,SRCCOPY);SelectObject(mem,old);DeleteObject(bm);DeleteDC(mem);EndPaint(h,&ps);return 0;}
 if(m==WM_LBUTTONDOWN){if(inside(clearNotes14,p)){clearNotes14Fn();return 0;}if(inside(shiftDown14,p)){shiftNotes14(-1);return 0;}if(inside(shiftUp14,p)){shiftNotes14(1);return 0;}int r=-1,s=-1;if(noteCell14(p,r,s)){paintNotes14=true;paintValue14=!noteForLane(laneSelected,selectedBar)[r][s];lastRow14=r;lastStep14=s;setNote14(r,s,paintValue14);SetCapture(h);return 0;}}
 if(m==WM_MOUSEMOVE&&paintNotes14&&(w&MK_LBUTTON)){int r=-1,s=-1;if(noteCell14(p,r,s)&&(r!=lastRow14||s!=lastStep14)){lastRow14=r;lastStep14=s;setNote14(r,s,paintValue14);}return 0;}
 if(m==WM_LBUTTONUP&&paintNotes14){paintNotes14=false;lastRow14=lastStep14=-1;if(GetCapture()==h)ReleaseCapture();return 0;}
 return proc_v13(h,m,w,l);
}
}
int WINAPI wWinMain(HINSTANCE hi,HINSTANCE,PWSTR,int){WNDCLASSW wc{};wc.lpfnWndProc=proc_v14;wc.hInstance=hi;wc.lpszClassName=L"BoomifyV14";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);RegisterClassW(&wc);win=CreateWindowExW(0,wc.lpszClassName,L"Boomify Studio - Alpha 5.1",WS_OVERLAPPEDWINDOW,0,0,1680,1000,nullptr,nullptr,hi,nullptr);if(!win)return 1;v6Fresh();editorOpen=true;sound9[1]=Sound9::SubBass;sound9[2]=Sound9::SawLead;layout11(win);ShowWindow(win,SW_MAXIMIZE);UpdateWindow(win);MSG msg{};while(GetMessageW(&msg,nullptr,0,0)){TranslateMessage(&msg);DispatchMessageW(&msg);}return 0;}
