// Boomify V6 - dynamic track workspace
// Major architecture step: the timeline is no longer limited to Drums/Bass/Melody.
#define BOOMIFY_V4_NO_ENTRY
#include "main_v4.cpp"
#undef BOOMIFY_V4_NO_ENTRY

namespace {
constexpr int V6_MAX_TRACKS=16;
enum class LaneType{Drums,Instrument};
struct Lane{std::wstring name;LaneType type=LaneType::Instrument;bool mute=false,solo=false;int volume=80;};
std::vector<Lane> lanes={{L"Drums",LaneType::Drums,false,false,85},{L"Bass",LaneType::Instrument,false,false,80},{L"Synth",LaneType::Instrument,false,false,80}};
int laneSelected=0;
RECT addTrackR{},removeTrackR{},renameTrackR{},soloR[V6_MAX_TRACKS]{},laneMuteR[V6_MAX_TRACKS]{},laneVolR[V6_MAX_TRACKS]{},laneHeadR[V6_MAX_TRACKS]{};

// Extra lanes use the same proven clip representation as the original note lanes.
std::array<std::array<NoteClip,MAX_BARS>,V6_MAX_TRACKS-3> extraNotes{};
NoteClip& noteForLane(int lane,int bar){if(lane==1)return bass[bar];if(lane==2)return melody[bar];return extraNotes[lane-3][bar];}
bool laneUsed(int lane,int bar){if(lane==0)return usedDrum(bar);return usedNote(noteForLane(lane,bar));}
void clearLaneClip(int lane,int bar){if(lane==0)clearDrum(bar);else clearNote(noteForLane(lane,bar));}

void addTrack(){if((int)lanes.size()>=V6_MAX_TRACKS)return;int n=(int)lanes.size()+1;lanes.push_back({L"Instrument "+std::to_wstring(n),LaneType::Instrument,false,false,80});laneSelected=(int)lanes.size()-1;selectedTrack=std::min(laneSelected,2);InvalidateRect(win,nullptr,FALSE);}
void removeTrack(){if(laneSelected<0||laneSelected>=(int)lanes.size()||lanes.size()<=1)return;if(laneSelected<3){MessageBoxW(win,L"Les 3 pistes moteur historiques seront rendues supprimables dans la prochaine migration de projet.\nLes nouvelles pistes, elles, sont deja totalement dynamiques.",L"Boomify",MB_OK|MB_ICONINFORMATION);return;}int idx=laneSelected-3;for(int k=idx;k<V6_MAX_TRACKS-4;k++)extraNotes[k]=extraNotes[k+1];for(auto&c:extraNotes[V6_MAX_TRACKS-4])clearNote(c);lanes.erase(lanes.begin()+laneSelected);laneSelected=std::min(laneSelected,(int)lanes.size()-1);InvalidateRect(win,nullptr,FALSE);}
void renameTrack(){if(laneSelected<0||laneSelected>=(int)lanes.size())return;static int renameCounter=1;lanes[laneSelected].name=L"Track "+std::to_wstring(renameCounter++);InvalidateRect(win,nullptr,FALSE);}
void moveLane(int from,int to){if(from<3||to<3||from==to||from>=(int)lanes.size()||to>=(int)lanes.size())return;std::swap(lanes[from],lanes[to]);std::swap(extraNotes[from-3],extraNotes[to-3]);laneSelected=to;InvalidateRect(win,nullptr,FALSE);}

void layoutV6(HWND h){layout(h);RECT c;GetClientRect(h,&c);addTrackR={16,88,82,116};removeTrackR={88,88,154,116};renameTrackR={160,88,236,116};int top=126,bottom=editorOpen?std::max(455,(int)(c.bottom*.64)):c.bottom-45;int n=std::max(1,(int)lanes.size());int rh=std::max(34,(bottom-top)/n);for(int i=0;i<n;i++){int y=top+i*rh;laneHeadR[i]={4,y,174,std::min(bottom,y+rh-2)};laneMuteR[i]={112,y+8,136,y+30};soloR[i]={140,y+8,164,y+30};laneVolR[i]={12,y+rh-16,100,y+rh-8};}}
void drawV6(HDC d,RECT c){
    paint(d,c); // retain validated transport/editor/audio visuals underneath
    fill(d,{0,82,c.right,124},RGB(24,25,27));button(d,addTrackR,L"+ TRACK");button(d,removeTrackR,L"- TRACK");button(d,renameTrackR,L"RENAME");
    int top=126,bottom=editorOpen?std::max(455,(int)(c.bottom*.64)):c.bottom-45;fill(d,{0,top,179,bottom},RGB(31,32,35));
    int n=(int)lanes.size(),rh=std::max(34,(bottom-top)/std::max(1,n));
    for(int i=0;i<n;i++){
        RECT hr=laneHeadR[i];fill(d,hr,i==laneSelected?RGB(49,50,54):RGB(39,40,43));
        if(i==laneSelected)fill(d,{4,hr.top,8,hr.bottom},accent);
        label(d,lanes[i].name.c_str(),16,hr.top+8,12,txt,FW_BOLD);
        button(d,laneMuteR[i],L"M",lanes[i].mute);button(d,soloR[i],L"S",lanes[i].solo);
        fill(d,laneVolR[i],RGB(18,19,21));int vw=(laneVolR[i].right-laneVolR[i].left)*lanes[i].volume/100;fill(d,{laneVolR[i].left,laneVolR[i].top,laneVolR[i].left+vw,laneVolR[i].bottom},accent);
        if(i>=3){
            int left=180,right=c.right-20,aw=std::max(1,right-left),cw=std::max(1,aw/visibleBars);int y=hr.top;
            for(int v=0;v<visibleBars;v++){int b=scrollBar+v;if(b>=activeBars)break;RECT r{left+v*cw,y,left+(v+1)*cw-1,hr.bottom};fill(d,r,laneUsed(i,b)?RGB(191,118,72):RGB(28,29,31));line(d,r.left,r.top,r.left,r.bottom,grid);if(b==selectedBar&&i==laneSelected){line(d,r.left+1,r.top+1,r.right-1,r.top+1,RGB(245,245,245),2);line(d,r.left+1,r.bottom-2,r.right-1,r.bottom-2,RGB(245,245,245),2);}}
        }
    }
    label(d,L"TRACK WORKSPACE",250,94,11,RGB(175,177,181),FW_BOLD);
}

bool hitExtraClip(POINT p,int&lane,int&bar){RECT c;GetClientRect(win,&c);int left=180,right=c.right-20,aw=std::max(1,right-left),cw=std::max(1,aw/visibleBars);for(int i=3;i<(int)lanes.size();i++){if(p.y>=laneHeadR[i].top&&p.y<laneHeadR[i].bottom&&p.x>=left&&p.x<right){int v=(p.x-left)/cw;if(v>=0&&v<visibleBars){lane=i;bar=scrollBar+v;return bar<activeBars;}}}return false;}
void toggleExtraNote(int lane,int bar,POINT p){RECT c;GetClientRect(win,&c);int left=180,right=c.right-20,cw=std::max(1,(right-left)/visibleBars);int local=(p.x-(left+(bar-scrollBar)*cw));int step=std::clamp(local*STEPS/std::max(1,cw),0,STEPS-1);auto&clip=noteForLane(lane,bar);clip[6][step]=!clip[6][step];InvalidateRect(win,nullptr,FALSE);}

void v6Fresh(){freshEmpty();for(auto&a:extraNotes)for(auto&c:a)clearNote(c);lanes={{L"Drums",LaneType::Drums,false,false,85},{L"Bass",LaneType::Instrument,false,false,80},{L"Synth",LaneType::Instrument,false,false,80}};laneSelected=0;}

LRESULT CALLBACK proc_v6(HWND h,UINT m,WPARAM wp,LPARAM lp){
    if(m==WM_SIZE){layoutV6(h);return proc_v4(h,m,wp,lp);}
    if(m==WM_PAINT){PAINTSTRUCT ps;HDC s=BeginPaint(h,&ps);RECT c;GetClientRect(h,&c);HDC mem=CreateCompatibleDC(s);HBITMAP bm=CreateCompatibleBitmap(s,std::max(1L,c.right),std::max(1L,c.bottom));auto old=SelectObject(mem,bm);drawV6(mem,c);BitBlt(s,0,0,c.right,c.bottom,mem,0,0,SRCCOPY);SelectObject(mem,old);DeleteObject(bm);DeleteDC(mem);EndPaint(h,&ps);return 0;}
    if(m==WM_LBUTTONDOWN){POINT p{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};if(inside(addTrackR,p)){addTrack();layoutV6(h);return 0;}if(inside(removeTrackR,p)){removeTrack();layoutV6(h);return 0;}if(inside(renameTrackR,p)){renameTrack();return 0;}for(int i=0;i<(int)lanes.size();i++){if(inside(laneMuteR[i],p)){lanes[i].mute=!lanes[i].mute;if(i<3)muted[i]=lanes[i].mute;InvalidateRect(h,nullptr,FALSE);return 0;}if(inside(soloR[i],p)){lanes[i].solo=!lanes[i].solo;InvalidateRect(h,nullptr,FALSE);return 0;}if(inside(laneVolR[i],p)){lanes[i].volume=std::clamp((p.x-laneVolR[i].left)*100/std::max(1L,laneVolR[i].right-laneVolR[i].left),0,100);InvalidateRect(h,nullptr,FALSE);return 0;}if(inside(laneHeadR[i],p)){laneSelected=i;if(i<3)selectedTrack=i;InvalidateRect(h,nullptr,FALSE);return 0;}}
        int lane,bar;if(hitExtraClip(p,lane,bar)){laneSelected=lane;selectedBar=bar;toggleExtraNote(lane,bar,p);return 0;}
    }
    if(m==WM_KEYDOWN&&wp==VK_UP&&laneSelected>3){moveLane(laneSelected,laneSelected-1);return 0;}if(m==WM_KEYDOWN&&wp==VK_DOWN&&laneSelected>=3&&laneSelected+1<(int)lanes.size()){moveLane(laneSelected,laneSelected+1);return 0;}
    return proc_v4(h,m,wp,lp);
}
}

int WINAPI wWinMain(HINSTANCE hi,HINSTANCE,PWSTR,int){WNDCLASSW wc{};wc.lpfnWndProc=proc_v6;wc.hInstance=hi;wc.lpszClassName=L"BoomifyV6";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);RegisterClassW(&wc);win=CreateWindowExW(0,wc.lpszClassName,L"Boomify Alpha 3 - Track Workspace",WS_OVERLAPPEDWINDOW,0,0,1500,920,nullptr,nullptr,hi,nullptr);if(!win)return 1;v6Fresh();layoutV6(win);ShowWindow(win,SW_MAXIMIZE);UpdateWindow(win);MSG msg{};while(GetMessageW(&msg,nullptr,0,0)){TranslateMessage(&msg);DispatchMessageW(&msg);}return 0;}
