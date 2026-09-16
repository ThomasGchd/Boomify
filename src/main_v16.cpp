// Boomify V16 - Alpha 6 modern neon workspace
#define BOOMIFY_V6_NO_ENTRY
#define wWinMain wWinMain_v15_engine16
#include "main_v15.cpp"
#undef wWinMain
#undef BOOMIFY_V6_NO_ENTRY

namespace {
constexpr COLORREF BG16=RGB(9,10,17), SURF16=RGB(18,20,31), CARD16=RGB(25,27,42), GRID16=RGB(43,45,65);
constexpr COLORREF PURPLE16=RGB(137,92,246), PINK16=RGB(238,86,170), CYAN16=RGB(56,205,224), ORANGE16=RGB(255,112,72);

void rr16(HDC d,RECT r,int rad,COLORREF c,COLORREF border=RGB(0,0,0),int bw=0){
    HBRUSH b=CreateSolidBrush(c);HPEN p=CreatePen(PS_SOLID,std::max(1,bw),bw?border:c);auto ob=SelectObject(d,b);auto op=SelectObject(d,p);
    RoundRect(d,r.left,r.top,r.right,r.bottom,rad,rad);SelectObject(d,ob);SelectObject(d,op);DeleteObject(b);DeleteObject(p);
}
void glow16(HDC d,RECT r,COLORREF c,int spread=5){for(int i=spread;i>0;--i){COLORREF q=RGB(GetRValue(c)/5,GetGValue(c)/5,GetBValue(c)/5);RECT z{r.left-i,r.top-i,r.right+i,r.bottom+i};rr16(d,z,12+i,q);}}
void text16(HDC d,const std::wstring&s,RECT r,int pt,COLORREF c,int weight=FW_NORMAL,UINT align=DT_LEFT|DT_VCENTER|DT_SINGLELINE){
    HFONT f=CreateFontW(-MulDiv(pt,GetDeviceCaps(d,LOGPIXELSY),72),0,0,0,weight,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_DONTCARE,L"Segoe UI");auto of=SelectObject(d,f);SetBkMode(d,TRANSPARENT);SetTextColor(d,c);DrawTextW(d,s.c_str(),-1,&r,align|DT_END_ELLIPSIS);SelectObject(d,of);DeleteObject(f);
}
void pill16(HDC d,RECT r,const std::wstring&s,COLORREF c,bool active=false){if(active)glow16(d,r,c,3);rr16(d,r,16,active?c:RGB(34,36,53),active?c:RGB(63,66,88),1);text16(d,s,{r.left+10,r.top,r.right-10,r.bottom},8,active?RGB(255,255,255):RGB(214,217,230),FW_BOLD,DT_CENTER|DT_VCENTER|DT_SINGLELINE);}
void sync16(HWND h){
    layout11(h);layoutPanels15(h);trackList10=panels15.tracks;timeline10=panels15.timeline;
    editorArea11.left=0;editorArea11.top=panels15.timeline.bottom;editorArea11.right=panels15.timeline.right;browser10=editorArea11;
    inspector10=panels15.inspector;inspector10.bottom=GetSystemMetrics(SM_CYSCREEN)*2;
    for(int i=0;i<V6_MAX_TRACKS;i++){laneHeadR[i]={0,0,0,0};laneMuteR[i]={0,0,0,0};soloR[i]={0,0,0,0};laneVolR[i]={0,0,0,0};}
    for(int i=0;i<(int)lanes.size();++i)laneHeadR[i]=trackRow15(i);
}
void header16(HDC d,RECT c){
    RECT h{0,0,c.right,panels15.tracks.top};fill(d,h,BG16);
    for(int x=0;x<c.right;x+=3){double t=(double)x/std::max(1L,c.right);COLORREF q=t<.55?RGB(110+(int)(90*t),65+(int)(40*t),235):RGB(238,78+(int)(30*t),151-(int)(45*t));fill(d,{x,0,x+3,3},q);}
    text16(d,L"BOOMIFY",{24,13,205,48},21,RGB(247,247,252),FW_HEAVY);
    text16(d,L"STUDIO  /  ALPHA 6",{27,46,220,66},7,PURPLE16,FW_BOLD);
    RECT proj{220,14,420,52};rr16(d,proj,18,SURF16,RGB(55,57,78),1);text16(d,L"UNTITLED PROJECT",{238,14,402,52},8,RGB(190,194,210),FW_BOLD);
    save14={438,16,506,50};open14={514,16,588,50};pill16(d,save14,L"SAVE",PURPLE16);pill16(d,open14,L"OPEN",CYAN16);
    playR={c.right/2-55,14,c.right/2+5,58};glow16(d,playR,playing?PINK16:PURPLE16,4);rr16(d,playR,22,playing?PINK16:PURPLE16);text16(d,playing?L"II":L">",playR,15,RGB(255,255,255),FW_HEAVY,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    RECT bpmR{c.right/2+18,14,c.right/2+108,58};rr16(d,bpmR,18,SURF16,RGB(62,64,87),1);text16(d,std::to_wstring(bpm)+L" BPM",bpmR,9,CYAN16,FW_BOLD,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    wavR={c.right-126,16,c.right-22,52};pill16(d,wavR,L"EXPORT",ORANGE16,true);
}
void tracks16(HDC d){
    RECT p=panels15.tracks;fill(d,p,BG16);RECT inner{p.left+10,p.top+8,p.right-10,p.bottom-8};rr16(d,inner,18,SURF16,RGB(44,46,65),1);
    addTrack15={inner.left+12,inner.top+10,inner.left+106,inner.top+42};removeTrack15={inner.left+112,inner.top+10,inner.right-12,inner.top+42};pill16(d,addTrack15,L"+ TRACK",PURPLE16,true);pill16(d,removeTrack15,L"REMOVE",PINK16);
    for(int i=0;i<V6_MAX_TRACKS;i++){colorHit15[i]={0,0,0,0};nameHit15[i]={0,0,0,0};}
    for(int l=0;l<(int)lanes.size();++l){RECT r=trackRow15(l);r.left+=12;r.right-=12;r.top+=4;r.bottom-=4;if(r.bottom<=r.top)continue;COLORREF cc=colors15[l]?colors15[l]:defaultColor15(l);if(l==laneSelected)glow16(d,r,cc,3);rr16(d,r,14,l==laneSelected?RGB(34,35,53):CARD16,l==laneSelected?cc:RGB(49,51,71),1);fill(d,{r.left,r.top+8,r.left+4,r.bottom-8},cc);nameHit15[l]={r.left+15,r.top+3,r.right-36,r.top+27};colorHit15[l]={r.right-27,r.top+10,r.right-13,r.top+24};text16(d,lanes[l].name,nameHit15[l],9,RGB(240,241,247),FW_BOLD);rr16(d,colorHit15[l],8,cc);std::wstring meta=(lanes[l].type==LaneType::Drums?L"DRUMS":lanes[l].type==LaneType::Audio?L"AUDIO":L"INSTRUMENT");meta+=L"   "+std::to_wstring(lanes[l].volume)+L"%";text16(d,meta,{r.left+15,r.top+24,r.right-12,r.bottom-3},6,RGB(139,144,166),FW_BOLD);}
}
void timeline16(HDC d){
    RECT p=panels15.timeline;fill(d,p,BG16);RECT inner{p.left+7,p.top+8,p.right-7,p.bottom-8};rr16(d,inner,18,SURF16,RGB(44,46,65),1);int left=(int)inner.left+8,right=(int)inner.right-8;
    int tw=std::max(1,right-left);text16(d,L"ARRANGEMENT",{inner.left+14,inner.top,inner.left+150,inner.top+36},7,RGB(147,151,174),FW_BOLD);
    for(int v=0;v<visibleBars;v++){int b=scrollBar+v;if(b>=MAX_BARS)break;int x=left+(int)((long long)v*tw/std::max(1,visibleBars));line(d,x,inner.top+32,x,inner.bottom-8,(v%4==0)?RGB(61,63,84):GRID16,v%4==0?2:1);text16(d,std::to_wstring(b+1),{x+5,inner.top+7,x+40,inner.top+29},7,RGB(145,150,170),FW_BOLD);}
    for(int l=0;l<(int)lanes.size();++l){RECT rr=row15(l);rr.left=(LONG)left;rr.right=(LONG)right;rr.top+=4;rr.bottom-=4;if(rr.bottom<=rr.top)continue;rr16(d,rr,10,(l&1)?RGB(20,22,34):RGB(23,25,38));for(int v=0;v<visibleBars;v++){int b=scrollBar+v;if(b>=MAX_BARS)break;int x0=left+(int)((long long)v*tw/std::max(1,visibleBars)),x1=left+(int)((long long)(v+1)*tw/std::max(1,visibleBars));if(b<activeBars&&lanes[l].type!=LaneType::Audio&&laneUsed11(l,b)){COLORREF cc=colors15[l]?colors15[l]:defaultColor15(l);RECT q{(LONG)(x0+4),rr.top+3,(LONG)(x1-4),rr.bottom-3};if(selectedCell15(l,b))glow16(d,q,cc,3);rr16(d,q,10,cc,selectedCell15(l,b)?RGB(255,255,255):cc,selectedCell15(l,b)?2:0);}}
        if(lanes[l].type==LaneType::Audio){RECT ar=audioRect15(l);if(ar.right>ar.left){ar.left=std::max<LONG>(ar.left,(LONG)left);ar.right=std::min<LONG>(ar.right,(LONG)right);COLORREF cc=colors15[l]?colors15[l]:defaultColor15(l);rr16(d,ar,10,cc);drawWave15(d,{ar.left+4,ar.top+3,ar.right-4,ar.bottom-3},samples15[l],cc);}}
    }
    int bar=playing?playBar12():cursorBar;if(bar>=scrollBar&&bar<scrollBar+visibleBars){int x=left+(int)((long long)(bar-scrollBar)*tw/std::max(1,visibleBars));line(d,x,inner.top+30,x,inner.bottom-7,ORANGE16,2);RECT cap{(LONG)(x-5),inner.top+27,(LONG)(x+6),inner.top+36};rr16(d,cap,5,ORANGE16);}
}
void inspector16(HDC d){
    RECT p=panels15.inspector;fill(d,p,BG16);RECT in{p.left+8,p.top+8,p.right-10,p.bottom-8};rr16(d,in,18,SURF16,RGB(48,50,70),1);int x=(int)in.left+14,w=std::max(120,(int)(in.right-in.left)-28),y=(int)in.top+12;
    text16(d,L"INSPECTOR",{(LONG)x,(LONG)y,(LONG)(x+w),(LONG)(y+22)},8,PURPLE16,FW_BOLD);y+=26;if(laneSelected>=0&&laneSelected<(int)lanes.size()){COLORREF cc=colors15[laneSelected]?colors15[laneSelected]:defaultColor15(laneSelected);text16(d,lanes[laneSelected].name,{(LONG)x,(LONG)y,(LONG)(x+w),(LONG)(y+32)},14,RGB(245,245,250),FW_HEAVY);fill(d,{(LONG)x,(LONG)(y+34),(LONG)(x+50),(LONG)(y+37)},cc);y+=48;}
    import15={(LONG)x,(LONG)y,(LONG)(x+w),(LONG)(y+34)};pill16(d,import15,L"IMPORT WAV",CYAN16);y+=42;boom15={(LONG)x,(LONG)y,(LONG)(x+w),(LONG)(y+34)};pill16(d,boom15,L"+ BOOMBOX",ORANGE16);y+=44;master15={(LONG)x,(LONG)y,(LONG)(x+w),(LONG)(y+38)};pill16(d,master15,L"MASTER  "+std::to_wstring(master)+L"%",PURPLE16,masterOn15);y+=50;
    addFx15={(LONG)x,(LONG)y,(LONG)(x+w),(LONG)(y+36)};pill16(d,addFx15,L"+ ADD EFFECT",PINK16,true);y+=48;text16(d,L"FX CHAIN",{(LONG)x,(LONG)y,(LONG)(x+w),(LONG)(y+22)},7,RGB(145,150,170),FW_BOLD);y+=25;fxMinus15.fill(RECT{});fxPlus15.fill(RECT{});
    if(laneSelected>=0&&laneSelected<(int)lanes.size()){auto&r=rack11[laneSelected];for(int i=0;i<(int)r.size()&&i<6&&y+40<(int)in.bottom;i++){RECT fr{(LONG)x,(LONG)y,(LONG)(x+w),(LONG)(y+38)};COLORREF cc=fxColor11(r[i].type);rr16(d,fr,12,CARD16,RGB(54,56,78),1);fill(d,{fr.left,fr.top+8,fr.left+4,fr.bottom-8},cc);text16(d,fxName11(r[i].type),{fr.left+12,fr.top,fr.right-92,fr.bottom},7,RGB(229,231,240),FW_BOLD);fxMinus15[i]={fr.right-84,fr.top+7,fr.right-60,fr.bottom-7};fxPlus15[i]={fr.right-26,fr.top+7,fr.right-2,fr.bottom-7};pill16(d,fxMinus15[i],L"-",PURPLE16);pill16(d,fxPlus15[i],L"+",PURPLE16);text16(d,std::to_wstring(r[i].amount),{fr.right-58,fr.top,fr.right-28,fr.bottom},7,CYAN16,FW_BOLD,DT_CENTER|DT_VCENTER|DT_SINGLELINE);y+=44;}}
}
void editorFrame16(HDC d,RECT c){
    if(editorArea11.bottom<=editorArea11.top)return;RECT ed=editorArea11;ed.right=panels15.timeline.right;fill(d,ed,BG16);RECT card{ed.left+10,ed.top+7,ed.right-7,ed.bottom-10};if(card.bottom>card.top)rr16(d,card,18,SURF16,RGB(46,48,68),1);
    if(laneSelected>=0&&laneSelected<(int)lanes.size()){std::wstring t=lanes[laneSelected].type==LaneType::Drums?L"DRUM SEQUENCER":L"PIANO ROLL";text16(d,t,{card.left+16,card.top+4,card.left+220,card.top+34},8,ORANGE16,FW_BOLD);}
}
void paint16(HWND h,HDC out){
    sync16(h);RECT c{};GetClientRect(h,&c);HDC d=CreateCompatibleDC(out);HBITMAP bm=CreateCompatibleBitmap(out,std::max<LONG>(1,c.right),std::max<LONG>(1,c.bottom));auto ob=SelectObject(d,bm);fill(d,c,BG16);
    draw11(d,c);drawV13Chrome(d);drawV14(d);
    header16(d,c);tracks16(d);timeline16(d);inspector16(d);editorFrame16(d,c);
    int sv=SaveDC(d);IntersectClipRect(d,editorArea11.left+12,editorArea11.top+34,editorArea11.right-9,editorArea11.bottom-12);draw11(d,c);drawV14(d);RestoreDC(d,sv);
    BitBlt(out,0,0,c.right,c.bottom,d,0,0,SRCCOPY);SelectObject(d,ob);DeleteObject(bm);DeleteDC(d);
}
LRESULT CALLBACK proc_v16(HWND h,UINT m,WPARAM w,LPARAM l){
    if(m==WM_SIZE){LRESULT r=proc_v14_base15(h,m,w,l);sync16(h);InvalidateRect(h,nullptr,FALSE);return r;}
    if(m==WM_ERASEBKGND)return 1;
    if(m==WM_PAINT){PAINTSTRUCT ps{};HDC d=BeginPaint(h,&ps);paint16(h,d);EndPaint(h,&ps);return 0;}
    return proc_v15(h,m,w,l);
}
}
int WINAPI wWinMain(HINSTANCE hi,HINSTANCE,LPWSTR,int){
    for(int i=0;i<V6_MAX_TRACKS;i++)colors15[i]=defaultColor15(i);v6Fresh();editorOpen=true;sound9[1]=Sound9::SubBass;sound9[2]=Sound9::SawLead;master=100;
    WNDCLASSW wc{};wc.lpfnWndProc=proc_v16;wc.hInstance=hi;wc.lpszClassName=L"BoomifyAlpha6Neon";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);wc.style=CS_DBLCLKS;RegisterClassW(&wc);
    win=CreateWindowExW(0,wc.lpszClassName,L"Boomify Studio - Alpha 6 Neon",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1440,900,nullptr,nullptr,hi,nullptr);if(!win)return 1;sync16(win);ShowWindow(win,SW_MAXIMIZE);UpdateWindow(win);MSG msg{};while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}return(int)msg.wParam;
}