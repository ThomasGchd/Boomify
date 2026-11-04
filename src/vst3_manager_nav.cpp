// Native Win32 VST3 browser. No third-party plugin is loaded in Boomify.exe.
// Each selected instrument is rendered by BoomifyVST3Host.exe in a child process.
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <shlobj.h>

namespace {
namespace boomfs=std::filesystem;
std::vector<std::wstring> availableVST3Nav;
RECT vst3PickNavR{},vst3ClearNavR{},vst3StatusNavR{};
bool vst3CatalogLoadedNav=false;
// Cache short VST3 previews to keep repeated piano-roll clicks responsive.
std::vector<std::pair<std::wstring,std::vector<int16_t>>> vst3PreviewCacheNav;
std::array<std::wstring,V6_MAX_TRACKS> vst3RenderErrorsNav{};
std::array<int,V6_MAX_TRACKS> vst3RenderStatusNav{}; // 0=not tried, 1=rendered, -1=failed
static std::wstring vst3ShortNameNav(const std::wstring& path){
 if(path.empty())return L"NATIVE INSTRUMENT";
 auto name=boomfs::path(path).stem().wstring();
 return name.empty()?L"VST3":name;
}
static boomfs::path vst3CatalogFileNav(){
 wchar_t b[MAX_PATH]{};DWORD n=GetEnvironmentVariableW(L"APPDATA",b,MAX_PATH);
 boomfs::path root=(n>0&&n<MAX_PATH)?boomfs::path(b):boomfs::temp_directory_path();
 return root/L"Boomify"/L"vst3_catalog.txt";
}
static void rememberVST3Nav(const std::wstring& path){
 if(path.empty())return;
 auto normalized=boomfs::path(path).lexically_normal().wstring();
 if(std::find(availableVST3Nav.begin(),availableVST3Nav.end(),normalized)==availableVST3Nav.end())
   availableVST3Nav.push_back(normalized);
}
static void saveVST3CatalogNav(){
 auto p=vst3CatalogFileNav();std::error_code ec;boomfs::create_directories(p.parent_path(),ec);
 std::ofstream out(p,std::ios::binary|std::ios::trunc);
 if(!out)return;
 for(const auto&v:availableVST3Nav)out<<narrowName(v)<<"\n";
}
static void initVST3CatalogNav(){
 if(vst3CatalogLoadedNav)return;
 vst3CatalogLoadedNav=true;
 std::ifstream f(vst3CatalogFileNav(),std::ios::binary);
 std::string line;
 while(std::getline(f,line)){
  auto val=widenName(line);std::error_code ec;
  if(!val.empty()&&boomfs::exists(boomfs::path(val),ec))rememberVST3Nav(val);
 }
}
static void scanVST3FolderNav(const boomfs::path& root){
 std::error_code ec;if(!boomfs::is_directory(root,ec))return;
 boomfs::recursive_directory_iterator it(root,boomfs::directory_options::skip_permission_denied,ec),end;
 size_t visited=0;
 while(it!=end&&visited++<12000&&availableVST3Nav.size()<300){
  auto path=it->path();std::error_code one;
  if(path.extension()==L".vst3"){
   if(boomfs::is_directory(path,one))it.disable_recursion_pending();
   rememberVST3Nav(path.wstring());
  }
  it.increment(ec);
  if(ec)ec.clear();
 }
}
static void scanSystemVST3Nav(){
 initVST3CatalogNav();
 wchar_t b[MAX_PATH]{};DWORD n=GetEnvironmentVariableW(L"ProgramFiles",b,MAX_PATH);
 if(n>0&&n<MAX_PATH)scanVST3FolderNav(boomfs::path(b)/L"Common Files"/L"VST3");
 // Per-user VST3 installations are supported as well as system-wide ones.
 n=GetEnvironmentVariableW(L"LOCALAPPDATA",b,MAX_PATH);
 if(n>0&&n<MAX_PATH)scanVST3FolderNav(boomfs::path(b)/L"Programs"/L"Common"/L"VST3");
 saveVST3CatalogNav();
}
static std::wstring chooseVST3BundleNav(HWND h){
 wchar_t p[MAX_PATH]{};OPENFILENAMEW ofn{};ofn.lStructSize=sizeof(ofn);
 ofn.hwndOwner=h;ofn.lpstrFile=p;ofn.nMaxFile=MAX_PATH;
 ofn.lpstrFilter=L"Plugins VST3 (*.vst3)\0*.vst3\0Tous les fichiers\0*.*\0";
 ofn.Flags=OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;
 return GetOpenFileNameW(&ofn)?std::wstring(p):L"";
}
static std::wstring chooseVST3FolderNav(HWND h){
 BROWSEINFOW bi{};bi.hwndOwner=h;bi.lpszTitle=L"Choisir le dossier du plugin .vst3";
 bi.ulFlags=BIF_RETURNONLYFSDIRS|BIF_NEWDIALOGSTYLE;
 PIDLIST_ABSOLUTE id=SHBrowseForFolderW(&bi);
 if(!id)return {};
 wchar_t b[MAX_PATH]{};bool ok=SHGetPathFromIDListW(id,b)!=FALSE;
 CoTaskMemFree(id);
 if(!ok)return {};
 boomfs::path path(b);
 return path.extension()==L".vst3"?path.wstring():L"";
}
static void chooseVST3OnTrackNav(HWND h,POINT p){
 if(laneSelected<0||laneSelected>=(int)lanes.size()||
    lanes[laneSelected].type!=LaneType::Instrument)return;
 initVST3CatalogNav();
 HMENU m=CreatePopupMenu();
 constexpr int IDM_SCAN=77001,IDM_IMPORT_FILE=77002,IDM_IMPORT_FOLDER=77003,IDM_NATIVE=77004,IDM_FIRST=77100;
 AppendMenuW(m,MF_STRING,IDM_SCAN,L"Rechercher les VST3 installes");
 AppendMenuW(m,MF_STRING,IDM_IMPORT_FILE,L"Importer un fichier .vst3...");
 AppendMenuW(m,MF_STRING,IDM_IMPORT_FOLDER,L"Importer un dossier .vst3...");
 AppendMenuW(m,MF_STRING,IDM_NATIVE,L"Revenir a l'instrument natif");
 AppendMenuW(m,MF_SEPARATOR,0,nullptr);
 for(size_t i=0;i<availableVST3Nav.size()&&i<120;i++){
  auto name=vst3ShortNameNav(availableVST3Nav[i]);
  UINT flags=MF_STRING|(vst3ForLaneNav[laneSelected]==availableVST3Nav[i]?MF_CHECKED:0);
  AppendMenuW(m,flags,IDM_FIRST+(UINT)i,name.c_str());
 }
 POINT q=p;ClientToScreen(h,&q);
 int cmd=TrackPopupMenu(m,TPM_RETURNCMD|TPM_RIGHTBUTTON,q.x,q.y,0,h,nullptr);
 DestroyMenu(m);
 if(cmd==IDM_SCAN){scanSystemVST3Nav();MessageBoxW(h,L"Recherche terminee. Rouvre le menu VST3 pour choisir un instrument.",L"Boomify VST3",MB_OK|MB_ICONINFORMATION);}
 else if(cmd==IDM_IMPORT_FILE||cmd==IDM_IMPORT_FOLDER){
  auto path=cmd==IDM_IMPORT_FILE?chooseVST3BundleNav(h):chooseVST3FolderNav(h);
  if(!path.empty()){rememberVST3Nav(path);saveVST3CatalogNav();vst3ForLaneNav[laneSelected]=path;vst3RenderStatusNav[laneSelected]=0;vst3RenderErrorsNav[laneSelected].clear();}
  else if(cmd==IDM_IMPORT_FOLDER)MessageBoxW(h,L"Selectionne le dossier qui se termine par .vst3.",L"Boomify VST3",MB_OK|MB_ICONINFORMATION);
 }else if(cmd==IDM_NATIVE){vst3ForLaneNav[laneSelected].clear();vst3RenderStatusNav[laneSelected]=0;vst3RenderErrorsNav[laneSelected].clear();}
 else if(cmd>=IDM_FIRST&&(size_t)(cmd-IDM_FIRST)<availableVST3Nav.size())
  {vst3ForLaneNav[laneSelected]=availableVST3Nav[cmd-IDM_FIRST];vst3RenderStatusNav[laneSelected]=0;vst3RenderErrorsNav[laneSelected].clear();}
 InvalidateRect(h,nullptr,FALSE);
}
static void drawVST3PanelNav(HDC d){
 vst3PickNavR={};vst3ClearNavR={};vst3StatusNavR={};
 if(laneSelected<0||laneSelected>=(int)lanes.size()||lanes[laneSelected].type!=LaneType::Instrument)return;
 auto p=panels15.inspector;
 int left=(int)p.left+19,right=(int)p.right-20,top=(int)p.bottom-180;
 if(right-left<190||top<(int)p.top+310)return;
 RECT panel{left-7,top-12,right+7,top+156};
 rr16(d,panel,14,RGB(29,33,43),RGB(65,73,90),1);
 text16(d,L"INSTRUMENT EXTERNE  /  VST3",{(LONG)left,(LONG)top,(LONG)right,(LONG)(top+28)},9,CYAN16,FW_BOLD);
 auto current=vst3ForLaneNav[laneSelected];
 std::wstring name=vst3ShortNameNav(current);
 text16(d,name,{(LONG)left,(LONG)(top+28),(LONG)right,(LONG)(top+60)},10,RGB(236,237,244),FW_BOLD);
 vst3PickNavR={left,(LONG)(top+70),right,(LONG)(top+106)};
 pill16(d,vst3PickNavR,current.empty()?L"CHOISIR / IMPORTER VST3":L"CHANGER DE VST3",PURPLE16);
 if(!current.empty()){
  const wchar_t* status=vst3RenderStatusNav[laneSelected]<0?L"VST3 EN ECHEC - CLIQUER POUR DETAILS":vst3RenderStatusNav[laneSelected]>0?L"VST3 ACTIF":L"PRET - CLIQUER SUR UNE NOTE";
   vst3StatusNavR={left,(LONG)(top+54),right,(LONG)(top+71)};
  text16(d,status,{(LONG)left,(LONG)(top+55),(LONG)right,(LONG)(top+70)},7,vst3RenderStatusNav[laneSelected]<0?PINK16:RGB(166,171,191),FW_BOLD);
  vst3ClearNavR={left,(LONG)(top+112),right,(LONG)(top+143)};
  pill16(d,vst3ClearNavR,L"REVENIR AU SON NATIF",CYAN16);
 }else{
  text16(d,L"Plugins charges dans un processus separe",
   {(LONG)left,(LONG)(top+114),(LONG)right,(LONG)(top+143)},7,RGB(166,171,191));
 }
}
static bool handleVST3PanelNav(HWND h,POINT p){
 if(inside(vst3StatusNavR,p)){if(laneSelected>=0&&laneSelected<(int)lanes.size()&&vst3RenderStatusNav[laneSelected]<0){const auto&msg=vst3RenderErrorsNav[laneSelected];MessageBoxW(h,msg.empty()?L"Le VST3 ne produit pas de son. Verifiez son installation et reessayez.":msg.c_str(),L"Boomify - Diagnostic VST3",MB_OK|MB_ICONWARNING);}return true;}
 if(inside(vst3PickNavR,p)){chooseVST3OnTrackNav(h,p);return true;}
 if(inside(vst3ClearNavR,p)){if(laneSelected>=0&&laneSelected<(int)lanes.size()){vst3ForLaneNav[laneSelected].clear();vst3RenderStatusNav[laneSelected]=0;vst3RenderErrorsNav[laneSelected].clear();}InvalidateRect(h,nullptr,FALSE);return true;}
 return false;
}
static boomfs::path vst3HostExeNav(){
 wchar_t b[MAX_PATH]{};DWORD len=GetModuleFileNameW(nullptr,b,MAX_PATH);
 if(!len||len>=MAX_PATH)return {};
 return boomfs::path(b).parent_path()/L"BoomifyVST3Host.exe";
}
static bool runVST3ChildNav(const std::wstring& plugin,const boomfs::path& notes,const boomfs::path& output,DWORD timeoutMs=30000){
 auto host=vst3HostExeNav();std::error_code ec;
 if(!boomfs::is_regular_file(host,ec))return false;
 std::wstring args=L"\""+host.wstring()+L"\" \""+plugin+L"\" \""+
  notes.wstring()+L"\" \""+output.wstring()+L"\"";
 std::vector<wchar_t> cmd(args.begin(),args.end());cmd.push_back(L'\0');
 STARTUPINFOW si{};si.cb=sizeof(si);
 PROCESS_INFORMATION pi{};
 if(!CreateProcessW(host.c_str(),cmd.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,
                      nullptr,host.parent_path().c_str(),&si,&pi))return false;
 DWORD wait=WaitForSingleObject(pi.hProcess,timeoutMs);
 if(wait==WAIT_TIMEOUT){TerminateProcess(pi.hProcess,1);WaitForSingleObject(pi.hProcess,1000);}
 DWORD code=1;GetExitCodeProcess(pi.hProcess,&code);
 CloseHandle(pi.hThread);CloseHandle(pi.hProcess);
 return wait==WAIT_OBJECT_0&&code==0;
}
static void setVST3RenderFailureNav(int lane,const boomfs::path& output){
 if(lane<0||lane>=V6_MAX_TRACKS)return;
 vst3RenderStatusNav[lane]=-1;
 auto path=boomfs::path(output.wstring()+L".error.txt");
 std::ifstream err(path,std::ios::binary);
 std::string msg;
 if(err)std::getline(err,msg);
 std::error_code ec;boomfs::remove(path,ec);
 if(msg.size()>240)msg.resize(240);
 if(msg.empty()){
  if(!boomfs::is_regular_file(vst3HostExeNav(),ec))
   msg="BoomifyVST3Host.exe est introuvable. Placez-le a cote de Boomify.exe.";
  else msg="Le VST3 est inaccessible, incompatible ou a depasse le delai de rendu.";
 }
 vst3RenderErrorsNav[lane]=std::wstring(msg.begin(),msg.end())+
     L"\n\nBoomify utilise le son natif pour cette piste.";
 InvalidateRect(win,nullptr,FALSE);
}
static bool hasAudibleVST3SignalNav(const std::vector<int16_t>&samples){
 for(int16_t value:samples)if(value>16||value< -16)return true;
 return false;
}
// Render a single MIDI note for audition. This is a first-click offline preview:
// subsequent clicks on the same plugin and pitch use the in-memory PCM cache.
static bool renderVST3PreviewNav(int lane,int row,std::vector<int16_t>&samples){
 if(lane<0||lane>=(int)lanes.size()||row<0||row>=NOTES||
    vst3ForLaneNav[lane].empty()||vst3RenderStatusNav[lane]<0)return false;
 int midiBase=familyOf(sound9[lane])==Family9::Bass?47:71;
 int pitch=midiBase-row;
 if(pitch<0||pitch>127)return false;
 const std::wstring key=vst3ForLaneNav[lane]+L"|"+std::to_wstring(pitch);
 std::vector<int16_t> raw;
 for(const auto&c:vst3PreviewCacheNav)if(c.first==key){raw=c.second;break;}
 if(raw.empty()){
  std::error_code ec;
  boomfs::path dir=boomfs::temp_directory_path(ec)/L"BoomifyVST3";
  if(ec)return false;
  boomfs::create_directories(dir,ec);
  if(ec)return false;
  std::wstring id=std::to_wstring(GetCurrentProcessId())+L"_preview_"+
                   std::to_wstring(lane)+L"_"+std::to_wstring(pitch);
  auto notes=dir/(L"notes_"+id+L".txt");
  auto audio=dir/(L"render_"+id+L".wav");
  {
   std::ofstream out(notes,std::ios::trunc);
   if(!out)return false;
   out<<SR<<" "<<bpm<<" "<<(SR*8/10)<<"\n";
   out<<pitch<<" 0 "<<(SR*3/10)<<" 0.9\n";
   if(!out)return false;
  }
  boomfs::remove(audio,ec);
  bool success=runVST3ChildNav(vst3ForLaneNav[lane],notes,audio,6500) &&
               loadWav15(audio.wstring(),raw);
  boomfs::remove(notes,ec);
  boomfs::remove(audio,ec);
  if(!success||raw.empty()||!hasAudibleVST3SignalNav(raw)){
   setVST3RenderFailureNav(lane,audio);
   InvalidateRect(win,nullptr,FALSE);
   return false;
  }
  vst3PreviewCacheNav.push_back({key,raw});
  if(vst3PreviewCacheNav.size()>24)vst3PreviewCacheNav.erase(vst3PreviewCacheNav.begin());
 }
 const double gain=std::clamp(lanes[lane].volume,0,100)/100.0*
                   std::clamp(master,0,100)/100.0;
 samples.resize(raw.size());
 for(size_t i=0;i<raw.size();++i)
  samples[i]=(int16_t)std::clamp((int)std::lround(raw[i]*gain),-32768,32767);
 vst3RenderStatusNav[lane]=1;vst3RenderErrorsNav[lane].clear();
 return true;
}

static bool renderVST3LaneNav(int lane,int startBar,std::vector<int16_t>&mix){
 if(lane<0||lane>=(int)lanes.size()||vst3ForLaneNav[lane].empty())return false;
 if(lanes[lane].mute)return true;
 bool anySolo=false;for(const auto&ln:lanes)anySolo|=ln.solo;
 if(anySolo&&!lanes[lane].solo)return true;
 const double secBar=240.0/std::max(60,bpm);
 const int64_t barFrames=(int64_t)std::lround(secBar*SR);
 const int64_t tickFrames=std::max<int64_t>(1,barFrames/FINE_TICKS16);
 int64_t frames=(int64_t)mix.size()/2;
 boomfs::path base;
 try{base=boomfs::temp_directory_path()/L"BoomifyVST3";std::error_code ec;boomfs::create_directories(base,ec);}
 catch(...){return false;}
 auto id=std::to_wstring(GetCurrentProcessId())+L"_"+std::to_wstring(lane);
 auto input=base/(L"notes_"+id+L".txt"),output=base/(L"render_"+id+L".wav");
 std::ofstream events(input,std::ios::trunc);
 if(!events)return false;
 events<<SR<<" "<<bpm<<" "<<frames<<"\n";
 int midiBase=familyOf(sound9[lane])==Family9::Bass?47:71;
 bool hasNotes=false;
 for(int b=std::max(0,startBar);b<activeBars;++b){
  int64_t offset=(b-startBar)*barFrames;
  if(!fine16[lane][b].empty()){
   for(const auto&n:fine16[lane][b]){
    int64_t at=offset+n.start*tickFrames,dur=std::max<int64_t>(1,n.dur*tickFrames);
    if(at>=0&&at<frames){hasNotes=true;events<<midiBase-n.row<<" "<<at<<" "<<at+dur<<" 0.85\n";}
   }
  }else{
   auto&notes=noteForLane(lane,b);
   for(int r=0;r<NOTES;++r)for(int step=0;step<STEPS;++step)if(notes[r][step]){
    int64_t at=offset+step*barFrames/STEPS;
    int64_t dur=std::max<int64_t>(1,barFrames/STEPS);
    if(at>=0&&at<frames){hasNotes=true;events<<midiBase-r<<" "<<at<<" "<<at+dur<<" 0.85\n";}
   }
  }
 }
 events.close();
 if(!hasNotes)return true; // Nothing for this VST to play: avoid launching a slow plugin.
 std::error_code removeError;boomfs::remove(output,removeError);
 if(!runVST3ChildNav(vst3ForLaneNav[lane],input,output)){setVST3RenderFailureNav(lane,output);return false;}
 std::vector<int16_t> rendered;
 if(!loadWav15(output.wstring(),rendered)||!hasAudibleVST3SignalNav(rendered)){setVST3RenderFailureNav(lane,output);return false;}
 const double gain=std::clamp(lanes[lane].volume,0,100)/100.0*std::clamp(master,0,100)/100.0;
 for(size_t k=0;k<mix.size()&&k<rendered.size();++k){
  int value=(int)mix[k]+(int)std::lround(rendered[k]*gain);
  mix[k]=(int16_t)std::clamp(value,-32768,32767);
 }
 vst3RenderStatusNav[lane]=1;
 return true;
}
} // namespace
