#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
namespace afterglow {
constexpr const char* kName="Afterglow";
constexpr const char* kVersion="1.0.1";
constexpr const char* kVendor="Sunveil Audio";
constexpr double pi=3.14159265358979323846;
enum Param: unsigned {Time,Feedback,DelayMix,Decay,Shimmer,ReverbMix,DelayType,ReverbType,
 DelayOn,ReverbOn,Freeze,DelayTone,Drift,ReverbTone,Motion,Predelay,Output,Routing,Trails,Sync,Division,Bypass,Count};
using Values=std::array<double,Count>;
inline double clamp01(double v){return std::isfinite(v)?std::clamp(v,0.0,1.0):0.0;}
inline double dbToGain(double db){return std::pow(10.,db/20.);}
inline double gainToDb(double g){return 20*std::log10(std::max(1.e-12,g));}
struct Definition {const char* name;const char* unit;int steps;double initial;};
inline const Definition definitions[Count]={
 {"Delay time","ms",0,std::log(520./40)/std::log(45.)},{"Delay repeats","%",0,.46/.93},{"Delay mix","%",0,.30},
 {"Reverb decay","s",0,(18.-.8)/39.2},{"Shimmer","%",0,.22},{"Reverb mix","%",0,.52},
 {"Delay type","",2,0},{"Reverb type","",2,.5},{"Delay enabled","",1,1},{"Reverb enabled","",1,1},{"Freeze","",1,0},
 {"Delay tone","%",0,.55},{"Tape drift","%",0,.35},{"Reverb tone","%",0,.48},{"Reverb motion","%",0,.40},
 {"Reverb pre-delay","ms",0,40./180},{"Output","dB",0,.80},{"Routing","",2,0},{"Trails","",1,1},
 {"Tempo sync","",1,0},{"Note division","",4,0},{"Bypass","",1,0}};
inline constexpr const char* delayTypes[]={"Tape","Digital","Ping Pong"};
inline constexpr const char* reverbTypes[]={"Hall","Bloom","Shimmer"};
inline constexpr const char* routes[]={"Delay > Reverb","Reverb > Delay","Parallel"};
inline constexpr const char* divisions[]={"1/4","Dotted 1/8","1/8","1/2","Triplet 1/8"};
inline constexpr double beatLengths[]={1.,.75,.5,2.,1./3.};
inline double canonical(unsigned id,double v){v=clamp01(v);return id<Count&&definitions[id].steps?std::round(v*definitions[id].steps)/definitions[id].steps:v;}
inline double toPlain(unsigned id,double n){
 n=canonical(id,n);if(id>=Count)return 0;
 if(id==Time)return 40*std::pow(45.,n);
 if(id==Feedback)return 93*n;
 if(id==Decay)return .8+39.2*n;
 if(id==Predelay)return 180*n;
 if(id==Output)return -24+30*n;
 if(definitions[id].steps)return std::round(n*definitions[id].steps);
 return n*100;
}
inline double fromPlain(unsigned id,double v){
 if(id>=Count||!std::isfinite(v))return 0;
 if(id==Time)return clamp01(std::log(std::max(40.,v)/40)/std::log(45.));
 if(id==Feedback)return clamp01(v/93);
 if(id==Decay)return clamp01((v-.8)/39.2);
 if(id==Predelay)return clamp01(v/180);
 if(id==Output)return clamp01((v+24)/30);
 return canonical(id,definitions[id].steps?v/definitions[id].steps:v/100);
}
inline void format(unsigned id,double n,char* out,size_t size,bool units=false){
 n=canonical(id,n);const int i=id<Count?int(std::round(n*definitions[id].steps)):0;
 if(id==DelayType)std::snprintf(out,size,"%s",delayTypes[i]);
 else if(id==ReverbType)std::snprintf(out,size,"%s",reverbTypes[i]);
 else if(id==Routing)std::snprintf(out,size,"%s",routes[i]);
 else if(id==Division)std::snprintf(out,size,"%s",divisions[i]);
 else if(id==Sync)std::snprintf(out,size,"%s",i?"Host tempo":"Free time");
 else if(id==Bypass)std::snprintf(out,size,"%s",i?"Bypassed":"Engaged");
 else if(id==Freeze)std::snprintf(out,size,"%s",i?"Hold":"Release");
 else if(id==Trails)std::snprintf(out,size,"%s",i?"Spill":"Cut");
 else if(id==DelayOn||id==ReverbOn)std::snprintf(out,size,"%s",i?"On":"Off");
 else if(id==Time||id==Predelay)std::snprintf(out,size,units?"%.0f ms":"%.0f",toPlain(id,n));
 else if(id==Decay)std::snprintf(out,size,units?"%.1f s":"%.1f",toPlain(id,n));
 else if(id==Output)std::snprintf(out,size,units?"%+.1f dB":"%+.1f",toPlain(id,n));
 else std::snprintf(out,size,units?"%.0f %%":"%.0f",toPlain(id,n));
}
struct Preset{const char* name;Values values;};
inline Values makePreset(double time,double feedback,double dm,double decay,double shimmer,double rm,
 double dt,double drift,double rt,double motion,double pre,int delayType,int reverbType,int route){
 Values v{};for(unsigned i=0;i<Count;i++)v[i]=definitions[i].initial;
 v[Time]=fromPlain(Time,time);v[Feedback]=fromPlain(Feedback,feedback);v[DelayMix]=dm;
 v[Decay]=fromPlain(Decay,decay);v[Shimmer]=shimmer;v[ReverbMix]=rm;
 v[DelayTone]=dt;v[Drift]=drift;v[ReverbTone]=rt;v[Motion]=motion;v[Predelay]=fromPlain(Predelay,pre);
 v[DelayType]=delayType/2.;v[ReverbType]=reverbType/2.;v[Routing]=route/2.;return v;
}
inline const Preset presets[]={
 {"Violet Haze",makePreset(520,46,.30,18,.22,.52,.55,.35,.48,.40,40,0,1,0)},
 {"Soft Focus",makePreset(330,28,.22,6.5,0,.34,.65,.20,.61,.22,24,0,0,0)},
 {"Dream Cathedral",makePreset(680,52,.28,32,.16,.66,.45,.30,.43,.42,65,2,0,0)},
 {"Slow Bloom",makePreset(430,40,.27,26,.12,.70,.48,.48,.38,.60,80,0,1,1)},
 {"Orbiting Light",makePreset(750,63,.38,28,.62,.60,.62,.40,.57,.56,55,2,2,2)},
 {"Silver Room",makePreset(190,18,.16,2.8,0,.24,.75,.10,.70,.16,12,1,0,0)}};
inline constexpr int presetCount=sizeof(presets)/sizeof(presets[0]);
inline Values defaults(){return presets[0].values;}
}
