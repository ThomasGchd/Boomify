// Boomify V4: interaction layer built on the validated V3 engine.
// Keeps the stable audio/timeline implementation while upgrading DAW-style block editing.
#define proc boomify_v3_proc
#define wWinMain boomify_v3_wWinMain
#include "main_v3.cpp"
#undef wWinMain
#undef proc

namespace {
enum { CTX_COPY=41001, CTX_PASTE, CTX_DUP, CTX_CLEAR, CTX_CURSOR, CTX_DELETE_BAR };
bool draggingClip=false;
bool draggingScrollbar=false;
int dragTrack=-1,dragBar=-1;
int scrollbarGrabOffset=0;
POINT dragOrigin{};

// V4 projects start empty. The V3 engine still owns the reset logic, so we clear
// its old demo drum beat immediately after reset instead of carrying a template
// into every new project.
void freshEmpty(){
    fresh();
    clearDrum(0);
    selectedTrack=0;selectedBar=0;cursorBar=0;scrollBar=0;
    if(win){layout(win);InvalidateRect(win,nullptr,FALSE);}
}

bool hitClipAt(POINT p,int& tr,int& bar){
    for(int r=0;r<TRACKS;r++) for(int v=0;v<visibleBars;v++){
        int b=scrollBar+v;
        if(b<activeBars && inside(clipR[r][v],p)){tr=r;bar=b;return true;}
    }
    return false;
}

void selectCell(int tr,int bar){
    if(tr<0||tr>=TRACKS||bar<0||bar>=activeBars)return;
    selectedTrack=tr;selectedBar=bar;ensureVisible(bar);layout(win);InvalidateRect(win,nullptr,FALSE);
}

bool compatibleCells(int a,int b){return (a==0&&b==0)||(a>0&&b>0);}

void moveCell(int srcTrack,int srcBar,int dstTrack,int dstBar){
    if(srcBar==dstBar&&srcTrack==dstTrack)return;
    if(!compatibleCells(srcTrack,dstTrack))return;
    if(srcTrack==0){drums[dstBar]=drums[srcBar];clearDrum(srcBar);}
    else {
        NoteClip temp=srcTrack==1?bass[srcBar]:melody[srcBar];
        if(dstTrack==1)bass[dstBar]=temp;else melody[dstBar]=temp;
        clearNote(srcTrack==1?bass[srcBar]:melody[srcBar]);
    }
    selectedTrack=dstTrack;selectedBar=dstBar;ensureVisible(dstBar);layout(win);InvalidateRect(win,nullptr,FALSE);
}

void setScrollFromMouse(HWND h,int mouseX,bool preserveGrab){
    if(activeBars<=visibleBars){scrollBar=0;layout(h);InvalidateRect(h,nullptr,FALSE);return;}
    int trackLeft=scrollTrackR.left;
    int trackWidth=scrollTrackR.right-scrollTrackR.left;
    int thumbWidth=scrollThumbR.right-scrollThumbR.left;
    int travel=std::max(1,trackWidth-thumbWidth);
    int x=mouseX-(preserveGrab?scrollbarGrabOffset:thumbWidth/2);
    x=std::clamp(x,trackLeft,trackLeft+travel);
    int range=activeBars-visibleBars;
    scrollBar=(int)(((long long)(x-trackLeft)*range+travel/2)/travel);
    clampScroll();layout(h);InvalidateRect(h,nullptr,FALSE);
}

void showClipMenu(HWND h,POINT client,int tr,int bar){
    selectCell(tr,bar);
    HMENU menu=CreatePopupMenu();
    AppendMenuW(menu,MF_STRING,CTX_COPY,L"Copier\tCtrl+C");
    AppendMenuW(menu,MF_STRING|(copied?0:MF_GRAYED),CTX_PASTE,L"Coller\tCtrl+V");
    AppendMenuW(menu,MF_STRING,CTX_DUP,L"Dupliquer\tCtrl+D");
    AppendMenuW(menu,MF_SEPARATOR,0,nullptr);
    AppendMenuW(menu,MF_STRING,CTX_CURSOR,L"Placer le curseur de lecture ici");
    AppendMenuW(menu,MF_SEPARATOR,0,nullptr);
    AppendMenuW(menu,MF_STRING,CTX_CLEAR,L"Vider ce bloc\tSuppr");
    AppendMenuW(menu,MF_STRING,CTX_DELETE_BAR,L"Supprimer la mesure entiere\tMaj+Suppr");
    POINT screen=client;ClientToScreen(h,&screen);
    int cmd=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_RIGHTBUTTON,screen.x,screen.y,0,h,nullptr);
    DestroyMenu(menu);
    switch(cmd){
        case CTX_COPY: copyClip(); break;
        case CTX_PASTE: pasteClip(); break;
        case CTX_DUP: duplicateClip(); layout(h); break;
        case CTX_CLEAR: clearCell(selectedTrack,selectedBar);InvalidateRect(h,nullptr,FALSE);break;
        case CTX_CURSOR: cursorBar=selectedBar;InvalidateRect(h,nullptr,FALSE);break;
        case CTX_DELETE_BAR: deleteBar();layout(h);break;
    }
}

LRESULT CALLBACK proc_v4(HWND h,UINT m,WPARAM wp,LPARAM lp){
    if(m==WM_KEYDOWN){
        if(wp==VK_DELETE){
            if(GetKeyState(VK_SHIFT)&0x8000){deleteBar();layout(h);}
            else {clearCell(selectedTrack,selectedBar);InvalidateRect(h,nullptr,FALSE);}
            return 0;
        }
    }
    if(m==WM_RBUTTONDOWN){
        POINT p{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};int tr,b;
        if(hitClipAt(p,tr,b)){showClipMenu(h,p,tr,b);return 0;}
    }
    if(m==WM_LBUTTONDOWN){
        POINT p{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};
        if(inside(newR,p)){freshEmpty();return 0;}
        if(inside(scrollThumbR,p) && activeBars>visibleBars){
            draggingScrollbar=true;
            scrollbarGrabOffset=p.x-scrollThumbR.left;
            SetCapture(h);
            return 0;
        }
        if(inside(scrollTrackR,p) && activeBars>visibleBars){
            scrollbarGrabOffset=(scrollThumbR.right-scrollThumbR.left)/2;
            setScrollFromMouse(h,p.x,false);
            draggingScrollbar=true;
            SetCapture(h);
            return 0;
        }
        int tr,b;
        if(hitClipAt(p,tr,b)){
            draggingClip=true;dragTrack=tr;dragBar=b;dragOrigin=p;
            SetCapture(h);
        }
    }
    if(m==WM_MOUSEMOVE && draggingScrollbar && (wp&MK_LBUTTON)){
        POINT p{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};
        setScrollFromMouse(h,p.x,true);
        return 0;
    }
    if(m==WM_LBUTTONUP && draggingScrollbar){
        POINT p{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};
        setScrollFromMouse(h,p.x,true);
        ReleaseCapture();draggingScrollbar=false;
        return 0;
    }
    if(m==WM_MOUSEMOVE && draggingClip && (wp&MK_LBUTTON)){
        POINT p{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};
        if(abs(p.x-dragOrigin.x)>5||abs(p.y-dragOrigin.y)>5)SetCursor(LoadCursor(nullptr,IDC_SIZEALL));
    }
    if(m==WM_LBUTTONUP && draggingClip){
        POINT p{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};int tr,b;
        ReleaseCapture();draggingClip=false;
        bool moved=(abs(p.x-dragOrigin.x)>5||abs(p.y-dragOrigin.y)>5);
        if(moved && hitClipAt(p,tr,b) && compatibleCells(dragTrack,tr)){
            moveCell(dragTrack,dragBar,tr,b);return 0;
        }
    }
    return boomify_v3_proc(h,m,wp,lp);
}
}

int WINAPI wWinMain(HINSTANCE hi,HINSTANCE,PWSTR,int){
    WNDCLASSW wc{};wc.lpfnWndProc=proc_v4;wc.hInstance=hi;wc.lpszClassName=L"BoomifyV4";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);
    RegisterClassW(&wc);
    win=CreateWindowExW(0,wc.lpszClassName,L"Boomify Alpha 2 - Timeline V4",WS_OVERLAPPEDWINDOW,0,0,1500,920,nullptr,nullptr,hi,nullptr);
    if(!win)return 1;
    freshEmpty();layout(win);ShowWindow(win,SW_MAXIMIZE);UpdateWindow(win);
    MSG msg{};while(GetMessageW(&msg,nullptr,0,0)){TranslateMessage(&msg);DispatchMessageW(&msg);}return 0;
}
