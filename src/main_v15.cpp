// Boomify V15 - Alpha 6 independent panel architecture
#define BOOMIFY_V15_INCLUDE
#define proc_v14 proc_v14_base15
#include "main_v14.cpp"
#undef proc_v14
#undef BOOMIFY_V15_INCLUDE
#include <cmath>
namespace {
struct Sample15{std::wstring path;int bar=-1;std::vector<int16_t> pcm;};
struct Kick15{bool enabled=false;int pitch=42,decay=38,punch=42,drive=22;};
struct Panels15{RECT tracks{},timeline{},inspector{};int rowH=48;};
std::array<Sample15,V6_MAX_TRACKS> samples15{};std::array<Kick15,V6_MAX_TRACKS> kicks15{};std::array<COLORREF,V6_MAX_TRACKS> colors15{};
RECT import15{},boom15{},master15{},addTrack15{},removeTrack15{},addFx15{};std::array<RECT,V6_MAX_TRACKS> colorHit15{},nameHit15{};std::array<RECT,6> fxMinus15{},fxPlus15{};RECT kpMinus15{},kpPlus15{},kdMinus15{},kdPlus15{},kuMinus15{},kuPlus15{},krMinus15{},krPlus15{};bool masterOn15=true;
int audioSelected15=-1;bool audioDrag15=false;int audioDragX15=0,audioDragBar15=0;Panels15 panels15{};
constexpr int PANEL_HEADER15=42;
COLORREF defaultColor15(int i){static COLORREF c[]={RGB(220,96,69),RGB(69,132,210),RGB(162,93,205),RGB(62,169,126),RGB(220,154,55),RGB(205,76,118),RGB(70,170,181),RGB(130,139,151)};return c[i%8];}
void layoutPanels15(HWND h){RECT c{};GetClientRect(h,&c);int top=(int)timeline10.top,bottom=(int)timeline10.bottom;if(bottom<=top){top=116;bottom=std::max(top+160,(int)c.bottom-286);}int trackW=230,inspectW=std::clamp((int)(c.right*0.20),300,380);panels15.tracks={0,(LONG)top,(LONG)trackW,(LONG)bottom};panels15.inspector={(LONG)std::max(trackW+320,(int)c.right-inspectW),(LONG)top,c.right,(LONG)bottom};panels15.timeline={(LONG)trackW,(LONG)top,panels15.inspector.left,(LONG)bottom};int avail=std::max(1,bottom-top-PANEL_HEADER15);panels15.rowH=std::clamp(avail/std::max(1,(int)lanes.size()),34,58);}
RECT row15(int lane){int y=(int)panels15.timeline.top+PANEL_HEADER15+lane*panels15.rowH;return{panels15.timeline.left,(LONG)y,panels15.timeline.right,(LONG)std::min((int)panels15.timeline.bottom,y+panels15.rowH)};}
RECT trackRow15(int lane){RECT r=row15(lane);return{panels15.tracks.left,r.top,panels15.tracks.right,r.bottom};}
RECT cell15(int lane,int visible){RECT r=row15(lane);int w=std::max(1,(int)(panels15.timeline.right-panels15.timeline.left));int x0=(int)panels15.timeline.left+(int)((long long)visible*w/std::max(1,visibleBars));int x1=(int)panels15.timeline.left+(int)((long long)(visible+1)*w/std::max(1,visibleBars));return{(LONG)x0,r.top,(LONG)x1,r.bottom};}
// Remaining implementation intentionally unchanged from 89ffd1b baseline.
#include "main_v15_impl.inc"
