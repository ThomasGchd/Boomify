// Boomify V17 navigation layer - compiled as helpers over the stable V17 source.
// V17 owns the application entry point; this layer only adds safe navigation/audio helpers.
#define BOOMIFY_V17_NAV_HELPERS
#include "main_v17.cpp"

namespace {
RECT zoomOutNavR{},zoomInNavR{},fitNavR{},loopNavR{},setLoopNavR{};
bool loopOnNav=false; int loopStartNav=0,loopEndNav=3;
void scrollTimelineNav(int dir){if(!dir)return;int step=std::max(1,visibleBars/4);if(dir>0){int wanted=scrollBar+step,needed=std::min(MAX_BARS,wanted+visibleBars);if(needed>activeBars)activeBars=needed;scrollBar=std::min(wanted,std::max(0,MAX_BARS-visibleBars));}else scrollBar=std::max(0,scrollBar-step);cursorBar=std::clamp(cursorBar,scrollBar,std::min(MAX_BARS-1,scrollBar+visibleBars-1));layout11(win);sync16(win);InvalidateRect(win,nullptr,FALSE);}
void zoomNav(int dir){int old=visibleBars;if(dir>0)visibleBars=std::max(4,visibleBars/2);else visibleBars=std::min(MAX_BARS,std::max(visibleBars+1,visibleBars*2));if(dir<0&&visibleBars>activeBars)visibleBars=std::min(MAX_BARS,std::max(activeBars,4));if(old==visibleBars)return;scrollBar=std::clamp(scrollBar,0,std::max(0,MAX_BARS-visibleBars));layout11(win);sync16(win);InvalidateRect(win,nullptr,FALSE);}
void fitTimelineNav(){visibleBars=std::clamp(activeBars,4,MAX_BARS);scrollBar=0;layout11(win);sync16(win);InvalidateRect(win,nullptr,FALSE);}
void setLoopNavFromSelection(){int lo=MAX_BARS,hi=-1;for(auto&q:multiSelection){lo=std::min(lo,q.bar);hi=std::max(hi,q.bar);}if(hi<0){lo=std::clamp(selectedBar,0,std::max(0,activeBars-1));hi=std::min(activeBars-1,lo+3);}loopStartNav=std::clamp(lo,0,std::max(0,activeBars-1));loopEndNav=std::clamp(std::max(loopStartNav,hi),loopStartNav,std::max(loopStartNav,activeBars-1));loopOnNav=true;}
void smoothKickTailsNav(std::vector<int16_t>&a,int startBar){if(a.empty())return;const int bpmNow=std::max(60,(int)bpm);const double secBar=240.0/(double)bpmNow;const long long barFrames=(long long)(secBar*SR),stepFrames=std::max<long long>(1,barFrames/STEPS),tail=(long long)(0.30*SR),half=96;for(int lane=0;lane<(int)lanes.size();++lane){if(lanes[lane].type!=LaneType::Drums||kicks15[lane].enabled||lanes[lane].mute)continue;for(int bar=std::max(0,startBar);bar<activeBars;++bar){auto&dr=drumForLane11(lane,bar);for(int step=0;step<STEPS;++step){if(!dr[0][step])continue;long long center=((long long)bar-startBar)*barFrames+(long long)step*stepFrames+tail,a0=center-half,a1=center+half;if(a0<0||(size_t)(a1*2+1)>=a.size())continue;for(int ch=0;ch<2;++ch){double left=a[(size_t)a0*2+ch],right=a[(size_t)a1*2+ch];for(long long q=0;q<=2*half;++q){double u=(double)q/(2.0*half),sm=u*u*(3.0-2.0*u);a[(size_t)(a0+q)*2+ch]=(int16_t)std::lrint(left+(right-left)*sm);}}}}}}
}
