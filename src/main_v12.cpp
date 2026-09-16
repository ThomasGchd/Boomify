// Boomify Alpha 5 - modern DAW workspace interaction layer
#define BOOMIFY_V12_INCLUDE
#include "main_v11.cpp"
#undef BOOMIFY_V12_INCLUDE

namespace {
struct Clip12{int lane=0,offset=0;LaneType type=LaneType::Instrument;DrumClip drum{};NoteClip note{};};
std::vector<Clip12> clipboard12;
bool range12=false; POINT rangeStart12{},rangeNow12{}; int anchorLane12=-1,anchorBar12=-1;
RECT master12{},nav12{},navThumb12{},zoomMinus12{},zoomPlus12{},barsPlus12{},rewind12{},stop12{},dup12{};
int dragMode12=0,dragFx12=-1;

std::vector<BlockRef> refs12(){
    if(!multiSelection.empty())return multiSelection;
    if(laneSelected>=0&&laneSelected<(int)lanes.size()&&selectedBar>=0&&selectedBar<activeBars)return{{laneSelected,selectedBar}};
    return{};
}
void copy12(){
    auto src=refs12(); clipboard12.clear(); if(src.empty())return;
    int minB=MAX_BARS; for(auto&q:src)minB=std::min(minB,q.bar);
    for(auto&q:src){
        if(q.track<0||q.track>=(int)lanes.size()||q.bar<0||q.bar>=MAX_BARS)continue;
        Clip12 s{};s.lane=q.track;s.offset=q.bar-minB;s.type=lanes[q.track].type;
        if(s.type==LaneType::Drums)s.drum=drumForLane11(q.track,q.bar);
        else if(s.type==LaneType::Instrument)s.note=noteForLane(q.track,q.bar);
        clipboard12.push_back(s);
    }
}
void paste12(int base=-1){
    if(clipboard12.empty())return;if(base<0)base=selectedBar;base=std::clamp(base,0,MAX_BARS-1);clearMulti();
    int first=MAX_BARS;
    for(auto&s:clipboard12){int dst=base+s.offset;if(dst<0||dst>=MAX_BARS||s.lane<0||s.lane>=(int)lanes.size())continue;if(dst>=activeBars)activeBars=dst+1;
        if(s.type==LaneType::Drums&&lanes[s.lane].type==LaneType::Drums)drumForLane11(s.lane,dst)=s.drum;
        else if(s.type==LaneType::Instrument&&lanes[s.lane].type==LaneType::Instrument)noteForLane(s.lane,dst)=s.note;
        addMulti(s.lane,dst);first=std::min(first,dst);
    }
    if(first<MAX_BARS){selectedBar=first;ensureVisible(first);}layout11(win);InvalidateRect(win,nullptr,FALSE);
}
void duplicate12(){
    auto src=refs12();if(src.empty())return;int minB=MAX_BARS,maxB=-1;for(auto&q:src){minB=std::min(minB,q.bar);maxB=std::max(maxB,q.bar);}int span=maxB-minB+1;if(span<=0)return;
    std::vector<Clip12> snap;for(auto&q:src){if(q.track<0||q.track>=(int)lanes.size()||q.bar<0||q.bar>=MAX_BARS)continue;Clip12 s{};s.lane=q.track;s.offset=q.bar-minB;s.type=lanes[q.track].type;if(s.type==LaneType::Drums)s.drum=drumForLane11(q.track,q.bar);else if(s.type==LaneType::Instrument)s.note=noteForLane(q.track,q.bar);snap.push_back(s);}
    int base=minB+span;clearMulti();int first=MAX_BARS;
    for(auto&s:snap){int dst=base+s.offset;if(dst<0||dst>=MAX_BARS)continue;if(dst>=activeBars)activeBars=dst+1;if(s.type==LaneType::Drums)drumForLane11(s.lane,dst)=s.drum;else if(s.type==LaneType::Instrument)noteForLane(s.lane,dst)=s.note;addMulti(s.lane,dst);first=std::min(first,dst);}
    if(first<MAX_BARS){selectedBar=first;ensureVisible(first);}layout11(win);InvalidateRect(win,nullptr,FALSE);
}
void clear12(){for(auto&q:refs12()){if(q.track<0||q.track>=(int)lanes.size()||q.bar<0||q.bar>=MAX_BARS)continue;if(lanes[q.track].type==LaneType::Drums)clearDrum(drumForLane11(q.track,q.bar));else if(lanes[q.track].type==LaneType::Instrument)clearNote(noteForLane(q.track,q.bar));}clearMulti();InvalidateRect(win,nullptr,FALSE);}
void select12(int lane,int bar,bool add,bool extend){lane=std::clamp(lane,0,(int)lanes.size()-1);bar=std::clamp(bar,0,activeBars-1);if(extend&&anchorLane12>=0){clearMulti();for(int l=std::min(anchorLane12,lane);l<=std::max(anchorLane12,lane);l++)for(int b=std::min(anchorBar12,bar);b<=std::max(anchorBar12,bar);b++)addMulti(l,b);}else{if(!add)clearMulti();bool found=false;for(auto&q:multiSelection)if(q.track==lane&&q.bar==bar)found=true;if(add&&found)multiSelection.erase(std::remove_if(multiSelection.begin(),multiSelection.end(),[&](const BlockRef&q){return q.track==lane&&q.bar==bar;}),multiSelection.end());else addMulti(lane,bar);anchorLane12=lane;anchorBar12=bar;}laneSelected=lane;selectedTrack=std::min(TRACKS-1,lane);selectedBar=bar;}
void selectRange12(POINT a,POINT b){clearMulti();RECT rr{std::min(a.x,b.x),std::min(a.y,b.y),std::max(a.x,b.x)+1,std::max(a.y,b.y)+1};for(int l=0;l<(int)lanes.size();l++)for(int v=0;v<visibleBars;v++){int bar=scrollBar+v;if(bar>=activeBars)continue;RECT q=cell11(l,v),z{};if(IntersectRect(&z,&rr,&q))addMulti(l,bar);}if(!multiSelection.empty()){laneSelected=multiSelection.front().track;selectedTrack=std::min(TRACKS-1,laneSelected);selectedBar=multiSelection.front().bar;anchorLane12=laneSelected;anchorBar12=selectedBar;}}
void setPlayhead12(POINT p){int w=std::max(1,(int)(timeline10.right-timeline10.left));int v=std::clamp((int)((long long)(p.x-(int)timeline10.left)*visibleBars/w),0,visibleBars-1);cursorBar=std::clamp(scrollBar+v,0,activeBars-1);InvalidateRect(win,nullptr,FALSE);}
int currentPlayBar12(){if(!playing||!wave)return cursorBar;MMTIME mt{};mt.wType=TIME_SAMPLES;if(waveOutGetPosition(wave,&mt,sizeof(mt))!=MMSYSERR_NOERROR)return cursorBar;double sec=(double)mt.u.sample/SR,barSec=(60.0/std::max(1,bpm))*4.0;return std::clamp(playStartBar+(int)(sec/barSec),0,activeBars-1);}
void setMaster12(int x){int w=std::max(1,(int)(master12.right-master12.left));master=std::clamp((x-(int)master12.left)*100/w,0,100);InvalidateRect(win,nullptr,FALSE);}
void setNav12(int x){int maxScroll=std::max(0,activeBars-visibleBars);if(maxScroll<=0){scrollBar=0;return;}int thumb=std::max(1,(int)(navThumb12.right-navThumb12.left)),travel=std::max(1,(int)(nav12.right-nav12.left)-thumb);int pos=std::clamp(x-(int)nav12.left-thumb/2,0,travel);scrollBar=std::clamp((int)((long long)pos*maxScroll/travel),0,maxScroll);InvalidateRect(win,nullptr,FALSE);}
RECT fxAmountRect12(size_t j){RECT q=fxRects11[j];return{q.left+13,q.top+30,q.right-13,q.top+46};}
void setFx12(int idx,int x){if(laneSelected<0||laneSelected>=(int)lanes.size()||idx<0||idx>=(int)fxRects11.size())return;int real=fxScroll11+idx;if(real<0||real>=(int)rack11[laneSelected].size())return;RECT r=fxAmountRect12((size_t)idx);int w=std::max(1,(int)(r.right-r.left));rack11[laneSelected][real].amount=std::clamp((x-(int)r.left)*100/w,0,100);InvalidateRect(win,nullptr,FALSE);}
void context12(HWND h,POINT p,int lane,int bar){select12(lane,bar,false,false);HMENU m=CreatePopupMenu();AppendMenuW(m,MF_STRING,71001,L"Copier\tCtrl+C");AppendMenuW(m,MF_STRING,71002,L"Coller\tCtrl+V");AppendMenuW(m,MF_STRING,71003,L"Dupliquer\tCtrl+D");AppendMenuW(m,MF_SEPARATOR,0,nullptr);AppendMenuW(m,MF_STRING,71004,L"Vider le bloc\tSuppr");POINT s=p;ClientToScreen(h,&s);int cmd=TrackPopupMenu(m,TPM_RETURNCMD|TPM_RIGHTBUTTON,s.x,s.y,0,h,nullptr);DestroyMenu(m);if(cmd==71001)copy12();else if(cmd==71002)paste12(bar);else if(cmd==71003)duplicate12();else if(cmd==71004)clear12();}
void overlay12(HDC d){
    RECT c;GetClientRect(win,&c);int bar=currentPlayBar12();
    // transport/navigation strip
    fill(d,{0,70,c.right,114},RGB(18,20,25));rewind12={310,78,346,106};stop12={352,78,388,106};dup12={394,78,468,106};fill(d,rewind12,RGB(40,43,51));fill(d,stop12,RGB(40,43,51));fill(d,dup12,RGB(40,43,51));label(d,L"|<",321,86,8,RGB(218,220,225),FW_BOLD);label(d,L"STOP",357,86,7,RGB(218,220,225),FW_BOLD);label(d,L"DUPLI",408,86,7,RGB(218,220,225),FW_BOLD);
    if(bar>=scrollBar&&bar<scrollBar+visibleBars){int v=bar-scrollBar,w=std::max(1,(int)(timeline10.right-timeline10.left));int x=(int)timeline10.left+(int)((long long)v*w/visibleBars);line(d,x,(int)timeline10.top,x,(int)timeline10.bottom,RGB(255,105,74),3);RECT tag{(LONG)x,(LONG)timeline10.top,(LONG)std::min((int)timeline10.right,x+42),(LONG)((int)timeline10.top+21)};fill(d,tag,RGB(235,103,74));std::wstring n=L"M"+std::to_wstring(bar+1);label(d,n.c_str(),x+7,(int)timeline10.top+5,7,RGB(255,255,255),FW_BOLD);}
    master12={(LONG)std::max(920,(int)c.right-330),24,(LONG)std::max(1040,(int)c.right-190),35};label(d,L"MASTER",master12.left-60,24,7,RGB(155,159,169),FW_BOLD);fill(d,master12,RGB(35,38,45));RECT mf=master12;mf.right=mf.left+(mf.right-mf.left)*master/100;fill(d,mf,RGB(235,103,74));std::wstring mv=std::to_wstring(master)+L"%";label(d,mv.c_str(),master12.right+7,22,7,RGB(205,207,213),FW_BOLD);
    int nx=490,nr=(int)timeline10.right-20;nav12={(LONG)nx,88,(LONG)nr,99};fill(d,nav12,RGB(32,35,42));int total=std::max(1,activeBars),trackW=std::max(1,nr-nx),thumbW=std::clamp((int)((long long)trackW*visibleBars/total),36,trackW);int maxScroll=std::max(0,activeBars-visibleBars),travel=std::max(0,trackW-thumbW),tx=nx+(maxScroll?scrollBar*travel/maxScroll:0);navThumb12={(LONG)tx,85,(LONG)(tx+thumbW),102};fill(d,navThumb12,RGB(105,111,124));
    zoomMinus12={(LONG)(nr-112),76,(LONG)(nr-84),106};zoomPlus12={(LONG)(nr-80),76,(LONG)(nr-52),106};barsPlus12={(LONG)(nr-48),76,(LONG)nr,106};fill(d,zoomMinus12,RGB(47,50,59));fill(d,zoomPlus12,RGB(47,50,59));fill(d,barsPlus12,RGB(47,50,59));label(d,L"-",zoomMinus12.left+10,85,9,RGB(230,232,236),FW_BOLD);label(d,L"+",zoomPlus12.left+9,84,9,RGB(230,232,236),FW_BOLD);label(d,L"+8",barsPlus12.left+14,84,8,RGB(230,232,236),FW_BOLD);
    if(laneSelected>=0&&laneSelected<(int)lanes.size()){std::wstring info=lanes[laneSelected].name+L"   |   Mesure "+std::to_wstring(selectedBar+1)+L"   |   "+std::to_wstring(multiSelection.size())+L" bloc(s)";label(d,info.c_str(),18,(int)c.bottom-22,7,RGB(128,133,144),FW_NORMAL);}
    if(range12){RECT r{std::min(rangeStart12.x,rangeNow12.x),std::min(rangeStart12.y,rangeNow12.y),std::max(rangeStart12.x,rangeNow12.x),std::max(rangeStart12.y,rangeNow12.y)};line(d,r.left,r.top,r.right,r.top,RGB(245,245,245),2);line(d,r.left,r.bottom,r.right,r.bottom,RGB(245,245,245),2);line(d,r.left,r.top,r.left,r.bottom,RGB(245,245,245),2);line(d,r.right,r.top,r.right,r.bottom,RGB(245,245,245),2);}
}
LRESULT CALLBACK proc_v12(HWND h,UINT m,WPARAM wp,LPARAM lp){
    POINT p{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};
    if(m==WM_PAINT){PAINTSTRUCT ps;HDC s=BeginPaint(h,&ps);RECT c;GetClientRect(h,&c);HDC mem=CreateCompatibleDC(s);HBITMAP bm=CreateCompatibleBitmap(s,std::max(1,(int)c.right),std::max(1,(int)c.bottom));auto old=SelectObject(mem,bm);draw11(mem,c);overlay12(mem);BitBlt(s,0,0,c.right,c.bottom,mem,0,0,SRCCOPY);SelectObject(mem,old);DeleteObject(bm);DeleteDC(mem);EndPaint(h,&ps);return 0;}
    if(m==WM_TIMER&&wp==1){InvalidateRect(h,nullptr,FALSE);return proc_v11(h,m,wp,lp);}
    if(m==WM_RBUTTONDOWN&&inside(timeline10,p)){int lane=-1,bar=-1;if(hitTimeline11(p,lane,bar)){context12(h,p,lane,bar);InvalidateRect(h,nullptr,FALSE);return 0;}}
    if(m==WM_LBUTTONDOWN){
        if(inside(rewind12,p)){cursorBar=0;scrollBar=0;if(playing)stopAudio();InvalidateRect(h,nullptr,FALSE);return 0;}if(inside(stop12,p)){if(playing)stopAudio();InvalidateRect(h,nullptr,FALSE);return 0;}if(inside(dup12,p)){duplicate12();return 0;}
        if(inside(master12,p)){dragMode12=1;setMaster12(p.x);SetCapture(h);return 0;}if(inside(zoomMinus12,p)){zoom(-1);InvalidateRect(h,nullptr,FALSE);return 0;}if(inside(zoomPlus12,p)){zoom(1);InvalidateRect(h,nullptr,FALSE);return 0;}if(inside(barsPlus12,p)){addBars();layout11(h);InvalidateRect(h,nullptr,FALSE);return 0;}if(inside(nav12,p)||inside(navThumb12,p)){dragMode12=2;setNav12(p.x);SetCapture(h);return 0;}
        if(laneSelected>=0&&laneSelected<(int)lanes.size())for(size_t j=0;j<fxRects11.size();j++){RECT a=fxAmountRect12(j);if(inside(a,p)){dragMode12=3;dragFx12=(int)j;setFx12(dragFx12,p.x);SetCapture(h);return 0;}}
        if(inside(timeline10,p)){int lane=-1,bar=-1;if(hitTimeline11(p,lane,bar)){setPlayhead12(p);bool ctrl=(GetKeyState(VK_CONTROL)&0x8000)!=0,shift=(GetKeyState(VK_SHIFT)&0x8000)!=0;select12(lane,bar,ctrl,shift);if(!ctrl&&!shift){range12=true;rangeStart12=rangeNow12=p;SetCapture(h);}InvalidateRect(h,nullptr,FALSE);return 0;}}
    }
    if(m==WM_MOUSEMOVE){if(dragMode12==1&&(wp&MK_LBUTTON)){setMaster12(p.x);return 0;}if(dragMode12==2&&(wp&MK_LBUTTON)){setNav12(p.x);return 0;}if(dragMode12==3&&(wp&MK_LBUTTON)){setFx12(dragFx12,p.x);return 0;}if(range12){rangeNow12=p;InvalidateRect(h,nullptr,FALSE);return 0;}}
    if(m==WM_LBUTTONUP){if(dragMode12){dragMode12=0;dragFx12=-1;if(GetCapture()==h)ReleaseCapture();return 0;}if(range12){rangeNow12=p;if(std::abs(rangeNow12.x-rangeStart12.x)>5||std::abs(rangeNow12.y-rangeStart12.y)>5)selectRange12(rangeStart12,rangeNow12);range12=false;if(GetCapture()==h)ReleaseCapture();InvalidateRect(h,nullptr,FALSE);return 0;}}
    if(m==WM_KEYDOWN){bool ctrl=(GetKeyState(VK_CONTROL)&0x8000)!=0;if(ctrl&&wp=='C'){copy12();return 0;}if(ctrl&&wp=='V'){paste12();return 0;}if(ctrl&&wp=='D'){duplicate12();return 0;}if(wp==VK_DELETE){clear12();return 0;}if(wp==VK_HOME){cursorBar=0;scrollBar=0;InvalidateRect(h,nullptr,FALSE);return 0;}if(wp==VK_ESCAPE){clearMulti();range12=false;dragMode12=0;if(GetCapture()==h)ReleaseCapture();InvalidateRect(h,nullptr,FALSE);return 0;}}
    return proc_v11(h,m,wp,lp);
}
}

int WINAPI wWinMain(HINSTANCE hi,HINSTANCE,PWSTR,int){WNDCLASSW wc{};wc.lpfnWndProc=proc_v12;wc.hInstance=hi;wc.lpszClassName=L"BoomifyV12";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);RegisterClassW(&wc);win=CreateWindowExW(0,wc.lpszClassName,L"Boomify Alpha 5 - Studio",WS_OVERLAPPEDWINDOW,0,0,1600,960,nullptr,nullptr,hi,nullptr);if(!win)return 1;v6Fresh();editorOpen=true;sound9[1]=Sound9::SubBass;sound9[2]=Sound9::SawLead;layout11(win);ShowWindow(win,SW_MAXIMIZE);UpdateWindow(win);MSG msg{};while(GetMessageW(&msg,nullptr,0,0)){TranslateMessage(&msg);DispatchMessageW(&msg);}return 0;}
