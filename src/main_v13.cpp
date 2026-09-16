// Boomify Alpha 5 - V13 major studio experience
#define BOOMIFY_V13_INCLUDE
#include "main_v12.cpp"
#undef BOOMIFY_V13_INCLUDE

namespace {
RECT bpmMinus13{},bpmPlus13{},editorToggle13{},trackDup13{},fxClear13{},fxUp13{},fxDown13{};
int dragLane13=-1;

void duplicateTrack13(){
 if(laneSelected<0||laneSelected>=(int)lanes.size()||(int)lanes.size()>=V6_MAX_TRACKS)return;
 int src=laneSelected,dst=(int)lanes.size(); Lane n=lanes[src]; n.name+=L" Copy"; n.mute=false;n.solo=false;lanes.push_back(n);
 sound9[dst]=sound9[src]; rack11[dst]=rack11[src];
 for(int b=0;b<MAX_BARS;b++){
  if(n.type==LaneType::Drums)drumForLane11(dst,b)=drumForLane11(src,b);
  else if(n.type==LaneType::Instrument)noteForLane(dst,b)=noteForLane(src,b);
 }
 laneSelected=dst;selectedTrack=std::min(TRACKS-1,dst);clearMulti();layout11(win);InvalidateRect(win,nullptr,FALSE);
}
void setLaneVol13(int lane,int x){if(lane<0||lane>=(int)lanes.size())return;RECT r=laneVolR[lane];lanes[lane].volume=std::clamp((x-(int)r.left)*100/std::max(1,(int)(r.right-r.left)),0,100);InvalidateRect(win,nullptr,FALSE);}
void moveFx13(int dir){if(laneSelected<0||laneSelected>=(int)lanes.size())return;auto&r=rack11[laneSelected];if(r.empty())return;int i=std::clamp(fxScroll11,0,(int)r.size()-1),j=std::clamp(i+dir,0,(int)r.size()-1);if(i!=j){std::swap(r[i],r[j]);fxScroll11=j;InvalidateRect(win,nullptr,FALSE);}}
void drawV13Chrome(HDC d){
 RECT c;GetClientRect(win,&c);
 // stronger top chrome / project identity
 fill(d,{0,0,c.right,70},RGB(12,14,18));
 label(d,L"BOOMIFY",18,14,19,RGB(246,247,249),FW_BOLD);label(d,L"STUDIO",20,43,7,RGB(241,104,72),FW_BOLD);
 RECT project{125,16,300,52};fill(d,project,RGB(24,27,33));label(d,L"Untitled Project",143,29,9,RGB(214,217,223),FW_BOLD);
 // transport centre
 RECT play{(LONG)(c.right/2-48),12,(LONG)(c.right/2+48),56};fill(d,play,playing?RGB(197,72,62):RGB(240,103,72));label(d,playing?L"STOP":L"PLAY",play.left+29,play.top+15,10,RGB(255,255,255),FW_BOLD);playR=play;
 bpmMinus13={(LONG)(c.right/2+70),18,(LONG)(c.right/2+96),48};bpmPlus13={(LONG)(c.right/2+166),18,(LONG)(c.right/2+192),48};fill(d,bpmMinus13,RGB(37,40,48));fill(d,bpmPlus13,RGB(37,40,48));label(d,L"-",bpmMinus13.left+9,bpmMinus13.top+8,9,RGB(220,223,229),FW_BOLD);label(d,L"+",bpmPlus13.left+8,bpmPlus13.top+7,9,RGB(220,223,229),FW_BOLD);std::wstring bp=std::to_wstring(bpm)+L" BPM";label(d,bp.c_str(),bpmMinus13.right+10,29,9,RGB(226,228,233),FW_BOLD);
 // master
 master12={(LONG)std::max(1050,(int)c.right-310),23,(LONG)std::max(1160,(int)c.right-185),34};label(d,L"MASTER",master12.left-59,24,7,RGB(137,142,153),FW_BOLD);fill(d,master12,RGB(31,34,41));RECT mf=master12;mf.right=mf.left+(mf.right-mf.left)*master/100;fill(d,mf,RGB(240,103,72));std::wstring mv=std::to_wstring(master)+L"%";label(d,mv.c_str(),master12.right+8,22,7,RGB(204,207,214),FW_BOLD);
 wavR={c.right-112,18,c.right-16,52};fill(d,wavR,RGB(38,42,50));label(d,L"EXPORT",wavR.left+22,wavR.top+11,8,RGB(228,230,235),FW_BOLD);
 // second toolbar
 fill(d,{0,70,c.right,114},RGB(18,20,25));
 home12={232,78,268,106};stop12={274,78,316,106};dupli12={322,78,392,106};trackDup13={398,78,486,106};editorToggle13={492,78,580,106};
 for(RECT r:{home12,stop12,dupli12,trackDup13,editorToggle13})fill(d,r,RGB(39,43,51));
 label(d,L"|<",243,86,8,RGB(226,228,233),FW_BOLD);label(d,L"STOP",280,86,7,RGB(226,228,233),FW_BOLD);label(d,L"DUPLI",334,86,7,RGB(226,228,233),FW_BOLD);label(d,L"DUP PISTE",410,86,7,RGB(226,228,233),FW_BOLD);label(d,editorOpen?L"EDITOR ON":L"EDITOR OFF",504,86,7,editorOpen?RGB(240,122,90):RGB(170,174,183),FW_BOLD);
 // timeline navigator moved right
 int nx=594,nr=(int)timeline10.right-18;if(nr>nx+150){nav12={(LONG)nx,89,(LONG)nr,99};fill(d,nav12,RGB(30,33,40));int width=std::max(1,nr-nx),tw=std::clamp((int)((long long)width*visibleBars/std::max(1,activeBars)),34,width),ms=std::max(0,activeBars-visibleBars),travel=std::max(0,width-tw),tx=nx+(ms?scrollBar*travel/ms:0);thumb12={(LONG)tx,85,(LONG)(tx+tw),103};fill(d,thumb12,RGB(105,112,126));zminus12={(LONG)(nr-110),76,(LONG)(nr-82),106};zplus12={(LONG)(nr-78),76,(LONG)(nr-50),106};add8_12={(LONG)(nr-46),76,(LONG)nr,106};for(RECT r:{zminus12,zplus12,add8_12})fill(d,r,RGB(48,52,61));label(d,L"-",zminus12.left+10,85,9,RGB(235,236,239),FW_BOLD);label(d,L"+",zplus12.left+9,84,9,RGB(235,236,239),FW_BOLD);label(d,L"+8",add8_12.left+13,84,8,RGB(235,236,239),FW_BOLD);}
 // playhead with head marker
 int bar=playBar12();if(bar>=scrollBar&&bar<scrollBar+visibleBars){int tw=std::max(1,(int)(timeline10.right-timeline10.left));int x=(int)timeline10.left+(int)((long long)(bar-scrollBar)*tw/visibleBars);line(d,x,(int)timeline10.top,x,(int)timeline10.bottom,RGB(255,102,71),2);fill(d,{x-4,(int)timeline10.top,x+5,(int)timeline10.top+8},RGB(255,102,71));}
 // selected track / inspector tools
 if(laneSelected>=0&&laneSelected<(int)lanes.size()){
  std::wstring s=L"TRACK  "+std::to_wstring(laneSelected+1)+L"   "+lanes[laneSelected].name;label(d,s.c_str(),(int)inspector10.left+18,82,9,RGB(235,237,241),FW_BOLD);
  fxUp13={(LONG)((int)inspector10.right-92),78,(LONG)((int)inspector10.right-66),104};fxDown13={(LONG)((int)inspector10.right-62),78,(LONG)((int)inspector10.right-36),104};fxClear13={(LONG)((int)inspector10.right-31),78,(LONG)((int)inspector10.right-8),104};
  for(RECT r:{fxUp13,fxDown13,fxClear13})fill(d,r,RGB(43,47,56));label(d,L"^",fxUp13.left+8,85,8,RGB(220,223,229),FW_BOLD);label(d,L"v",fxDown13.left+8,84,8,RGB(220,223,229),FW_BOLD);label(d,L"x",fxClear13.left+8,84,8,RGB(238,112,92),FW_BOLD);
 }
 // footer hints
 fill(d,{0,c.bottom-30,c.right,c.bottom},RGB(13,15,19));std::wstring info=L"Mesure "+std::to_wstring(selectedBar+1)+L"   |   "+std::to_wstring(multiSelection.size())+L" bloc(s)   |   Ctrl+C/V/D   |   Shift plage   |   Ctrl multi-selection";label(d,info.c_str(),18,c.bottom-20,7,RGB(132,138,150));
}
LRESULT CALLBACK proc_v13(HWND h,UINT m,WPARAM w,LPARAM l){POINT p{GET_X_LPARAM(l),GET_Y_LPARAM(l)};
 if(m==WM_PAINT){PAINTSTRUCT ps;HDC s=BeginPaint(h,&ps);RECT c;GetClientRect(h,&c);HDC mem=CreateCompatibleDC(s);HBITMAP bm=CreateCompatibleBitmap(s,std::max(1,(int)c.right),std::max(1,(int)c.bottom));auto old=SelectObject(mem,bm);draw11(mem,c);overlay12(mem);drawV13Chrome(mem);BitBlt(s,0,0,c.right,c.bottom,mem,0,0,SRCCOPY);SelectObject(mem,old);DeleteObject(bm);DeleteDC(mem);EndPaint(h,&ps);return 0;}
 if(m==WM_LBUTTONDOWN){
  if(inside(bpmMinus13,p)){bpm=std::max(60,bpm-1);InvalidateRect(h,nullptr,FALSE);return 0;}if(inside(bpmPlus13,p)){bpm=std::min(220,bpm+1);InvalidateRect(h,nullptr,FALSE);return 0;}
  if(inside(trackDup13,p)){duplicateTrack13();return 0;}if(inside(editorToggle13,p)){editorOpen=!editorOpen;layout11(h);InvalidateRect(h,nullptr,FALSE);return 0;}
  if(inside(fxUp13,p)){moveFx13(-1);return 0;}if(inside(fxDown13,p)){moveFx13(1);return 0;}if(inside(fxClear13,p)&&laneSelected>=0&&laneSelected<(int)lanes.size()){rack11[laneSelected].clear();fxScroll11=0;InvalidateRect(h,nullptr,FALSE);return 0;}
  for(int i=0;i<(int)lanes.size();i++)if(inside(laneVolR[i],p)){dragLane13=i;setLaneVol13(i,p.x);SetCapture(h);return 0;}
 }
 if(m==WM_MOUSEMOVE&&dragLane13>=0&&(w&MK_LBUTTON)){setLaneVol13(dragLane13,p.x);return 0;}
 if(m==WM_LBUTTONUP&&dragLane13>=0){dragLane13=-1;if(GetCapture()==h)ReleaseCapture();return 0;}
 if(m==WM_MOUSEWHEEL&&inside(inspector10,p)&&laneSelected>=0&&laneSelected<(int)lanes.size()){int n=(int)rack11[laneSelected].size();if(n>0){fxScroll11=std::clamp(fxScroll11+(GET_WHEEL_DELTA_WPARAM(w)<0?1:-1),0,std::max(0,n-1));InvalidateRect(h,nullptr,FALSE);}return 0;}
 return proc_v12(h,m,w,l);
}
}
int WINAPI wWinMain(HINSTANCE hi,HINSTANCE,PWSTR,int){WNDCLASSW wc{};wc.lpfnWndProc=proc_v13;wc.hInstance=hi;wc.lpszClassName=L"BoomifyV13";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);RegisterClassW(&wc);win=CreateWindowExW(0,wc.lpszClassName,L"Boomify Studio - Alpha 5",WS_OVERLAPPEDWINDOW,0,0,1680,1000,nullptr,nullptr,hi,nullptr);if(!win)return 1;v6Fresh();editorOpen=true;sound9[1]=Sound9::SubBass;sound9[2]=Sound9::SawLead;layout11(win);ShowWindow(win,SW_MAXIMIZE);UpdateWindow(win);MSG msg{};while(GetMessageW(&msg,nullptr,0,0)){TranslateMessage(&msg);DispatchMessageW(&msg);}return 0;}
