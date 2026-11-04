// Boomify VST3 isolated render helper.
// Uses the official Steinberg VST 3.8 SDK (MIT license), supplied at build time.
// Plugin code runs out of process so a third-party crash cannot corrupt Boomify's memory.
#include <windows.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/hosting/plugprovider.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst/hosting/processdata.h"
#include "public.sdk/source/vst/hosting/eventlist.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"

namespace fs=std::filesystem;
using namespace Steinberg;
using namespace Steinberg::Vst;
struct Note { int pitch=60; int64_t at=0,off=0; float velocity=0.8f; };
struct MidiEdge { int64_t frame; int pitch; float velocity; bool on; };
static std::string toUtf8(const std::wstring& s){
 int len=WideCharToMultiByte(CP_UTF8,0,s.c_str(),-1,nullptr,0,nullptr,nullptr);
 if(len<=1)return {};
 std::string output((size_t)len,'\0');
 WideCharToMultiByte(CP_UTF8,0,s.c_str(),-1,output.data(),len,nullptr,nullptr);
 output.pop_back();return output;
}
static void writeWav(const fs::path& filename,const std::vector<int16_t>& pcm,int sampleRate){
 std::ofstream o(filename,std::ios::binary);if(!o)throw std::runtime_error("Cannot write WAV");
 uint32_t bytes=(uint32_t)(pcm.size()*sizeof(int16_t)),riff=36+bytes,fmt=16,rate=(uint32_t)sampleRate,byteRate=rate*4;
 uint16_t format=1,channels=2,align=4,bits=16;
 o.write("RIFF",4);o.write((char*)&riff,4);o.write("WAVEfmt ",8);
 o.write((char*)&fmt,4);o.write((char*)&format,2);o.write((char*)&channels,2);
 o.write((char*)&rate,4);o.write((char*)&byteRate,4);o.write((char*)&align,2);
 o.write((char*)&bits,2);o.write("data",4);o.write((char*)&bytes,4);
 o.write((char*)pcm.data(),bytes);
 if(!o)throw std::runtime_error("Cannot finish WAV");
}
static int execute(const fs::path& modulePath,const fs::path& notesPath,const fs::path& outPath){
 std::ifstream input(notesPath);if(!input)throw std::runtime_error("Cannot open note events");
 int sampleRate=44100,bpm=150;int64_t totalFrames=0;
 if(!(input>>sampleRate>>bpm>>totalFrames)||sampleRate!=44100||bpm<40||bpm>400||totalFrames<=0||totalFrames>44100LL*60*30)
  throw std::runtime_error("Invalid render parameters");
 std::vector<MidiEdge> edges;Note n{};
 while(input>>n.pitch>>n.at>>n.off>>n.velocity){
  if(n.pitch<0||n.pitch>127||n.at<0||n.off<=n.at||n.at>=totalFrames)continue;
  edges.push_back({n.at,n.pitch,std::clamp(n.velocity,0.01f,1.0f),true});
  if(n.off<totalFrames)edges.push_back({n.off,n.pitch,0.0f,false});
 }
 std::sort(edges.begin(),edges.end(),[](const MidiEdge&a,const MidiEdge&b){
  if(a.frame!=b.frame)return a.frame<b.frame;
  return a.on<b.on; // send note-off before note-on at an identical frame
 });
 std::string error;
 auto module=VST3::Hosting::Module::create(toUtf8(modulePath.wstring()),error);
 if(!module)throw std::runtime_error("VST3 load failed: "+error);
 auto factory=module->getFactory();
 auto classes=factory.classInfos();
 auto it=std::find_if(classes.begin(),classes.end(),[](const VST3::Hosting::ClassInfo& info){
  return info.category()==kVstAudioEffectClass;
 });
 if(it==classes.end())throw std::runtime_error("No VST3 audio processor in bundle");
 auto host=owned(new HostApplication());
 PluginContextFactory::instance().setPluginContext(host.get());
 auto provider=std::make_unique<PlugProvider>(factory,*it,true);
 if(!provider->initialize())throw std::runtime_error("VST3 failed to initialize");
 auto component=provider->getComponentPtr();
 FUnknownPtr<IAudioProcessor> processor(component);
 if(!processor)throw std::runtime_error("VST3 has no audio processor");
 if(processor->canProcessSampleSize(kSample32)!=kResultTrue)
  throw std::runtime_error("VST3 does not support 32-bit float samples");
 for(int d=0;d<2;++d)for(int media=0;media<2;++media){
  const auto md=media==0?kAudio:kEvent;
  int count=component->getBusCount(md,d==0?kInput:kOutput);
  for(int i=0;i<count;++i)component->activateBus(md,d==0?kInput:kOutput,i,true);
 }
 constexpr int BLOCK=512;
 ProcessSetup setup{};setup.processMode=kRealtime;setup.symbolicSampleSize=kSample32;
 setup.maxSamplesPerBlock=BLOCK;setup.sampleRate=sampleRate;
 if(processor->setupProcessing(setup)!=kResultOk)throw std::runtime_error("Plugin cannot setup audio processing");
 HostProcessData pd;
 if(!pd.prepare(*component,BLOCK,kSample32))throw std::runtime_error("Cannot prepare plugin buffers");
 EventList events(1024);pd.inputEvents=&events;
 ProcessContext ctx{};ctx.sampleRate=sampleRate;ctx.tempo=bpm;ctx.timeSigNumerator=4;
 ctx.timeSigDenominator=4;ctx.state=ProcessContext::kTempoValid | ProcessContext::kTimeSigValid;
 pd.processContext=&ctx;
 if(component->setActive(true)!=kResultOk)throw std::runtime_error("Plugin activation failed");
 if(processor->setProcessing(true)!=kResultOk){component->setActive(false);throw std::runtime_error("Plugin processing failed");}
 const int64_t count=totalFrames;
 std::vector<int16_t> samples((size_t)count*2,0);
 size_t pos=0,eventIndex=0;
 bool bad=false;
 while(pos<(size_t)count){
  int num=(int)std::min<int64_t>(BLOCK,count-(int64_t)pos);
  events.clear();pd.numSamples=num;
  ctx.projectTimeSamples=(int64_t)pos;ctx.continousTimeSamples=(int64_t)pos;
  for(;eventIndex<edges.size()&&edges[eventIndex].frame<(int64_t)(pos+num);++eventIndex){
   auto&e=edges[eventIndex];Event event{};
   event.busIndex=0;event.sampleOffset=(int32)((size_t)e.frame-pos);
   event.type=e.on?Event::kNoteOnEvent:Event::kNoteOffEvent;
   if(e.on){event.noteOn.channel=0;event.noteOn.pitch=(int16)e.pitch;event.noteOn.velocity=e.velocity;event.noteOn.noteId=-1;}
   else{event.noteOff.channel=0;event.noteOff.pitch=(int16)e.pitch;event.noteOff.velocity=0;event.noteOff.noteId=-1;}
   events.addEvent(event);
  }
  if(processor->process(pd)!=kResultOk){bad=true;break;}
  for(int i=0;i<num;++i){
   for(int channel=0;channel<2;++channel){
    float value=0.0f;
    if(pd.numOutputs>0&&pd.outputs&&pd.outputs[0].numChannels>0&&pd.outputs[0].channelBuffers32){
     int src=std::min(channel,(int)pd.outputs[0].numChannels-1);
     auto ptr=pd.outputs[0].channelBuffers32[src];
     if(ptr)value=ptr[i];
    }
    if(!std::isfinite(value))value=0;
    samples[(pos+(size_t)i)*2+channel]=(int16_t)std::lround(std::clamp(value,-0.99f,0.99f)*32767.0f);
   }
  }
  pos+=(size_t)num;
 }
 processor->setProcessing(false);
 component->setActive(false);
 provider.reset();
 PluginContextFactory::instance().setPluginContext(nullptr);
 if(bad)throw std::runtime_error("Plugin returned an audio processing error");
 writeWav(outPath,samples,sampleRate);
 return 0;
}
int wmain(int argc,wchar_t**argv){
 if(argc!=4){std::cerr<<"Usage: BoomifyVST3Host <plugin.vst3> <events.txt> <out.wav>\n";return 2;}
 try{return execute(fs::path(argv[1]),fs::path(argv[2]),fs::path(argv[3]));}
 catch(const std::exception&e){
  std::cerr<<"Boomify VST3: "<<e.what()<<"\n";
  std::ofstream err(fs::path(argv[3]).wstring()+L".error.txt");err<<e.what();
  return 1;
 }
}
