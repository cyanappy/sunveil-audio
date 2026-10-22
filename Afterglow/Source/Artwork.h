#pragma once
#include "Parameters.h"
#include <cstdint>
namespace afterglow::artwork {
constexpr int W=1024,H=1536;
constexpr uint32_t rgb(int r,int g,int b){return uint32_t(r|(g<<8)|(b<<16));}
constexpr uint32_t cream=rgb(243,220,187),muted=rgb(192,161,203),purple=rgb(100,75,117),dark=rgb(39,28,49),metal=rgb(190,183,167),ink=rgb(21,19,21),accent=rgb(215,173,240);
struct Box{int x,y,w,h;bool hit(int a,int b)const{return a>=x&&b>=y&&a<x+w&&b<y+h;}};
constexpr Box presetBox{326,48,373,60},detailsBox{810,57,112,38},tapBox{101,57,112,38};
constexpr Box delayTypeBox{190,690,300,130},reverbTypeBox{550,690,300,130};
constexpr Box routeBox{154,1140,714,45},syncBox{154,1202,322,45},divisionBox{502,1202,366,45};
constexpr Box trailsBox{154,1264,208,45},clearBox{389,1264,208,45},bypassBox{624,1264,244,45},closeBox{841,706,46,40};
struct Knob{unsigned id;int x,y,r;};
constexpr Knob knobs[]={{Time,268,281,69},{Feedback,512,281,69},{DelayMix,755,281,69},{Decay,268,543,69},{Shimmer,512,543,69},{ReverbMix,755,543,69}};
constexpr Knob advancedKnobs[]={{DelayTone,268,836,46},{Drift,512,836,46},{ReverbTone,755,836,46},{Motion,268,1006,46},{Predelay,512,1006,46},{Output,755,1006,46}};
constexpr Knob switches[]={{DelayOn,260,1304,78},{Freeze,512,1304,78},{ReverbOn,766,1304,78}};
inline int knobAt(int x,int y,bool advanced){
 if(advanced){for(auto k:advancedKnobs)if(std::hypot(double(x-k.x),double(y-k.y))<k.r+13)return int(k.id);}
 for(auto k:knobs)if(std::hypot(double(x-k.x),double(y-k.y))<k.r+10)return int(k.id);return -1;
}
inline int switchAt(int x,int y){for(auto k:switches)if(std::hypot(double(x-k.x),double(y-k.y))<k.r)return int(k.id);return -1;}
struct Pedal {
 Values values=defaults();int presetIndex=0,focused=-1;bool hasFocus=false,advanced=false,tapActive=false,clearActive=false;double inputPeak=0,outputPeak=0;
 template<class C>void knob(C& c,const Knob& k,bool small=false) const{
  c.photoKnob(k.x,k.y,k.r,(-135+270*values[k.id]),k.id<3?knobs[k.id].x:knobs[k.id<6?k.id:0].x,k.id<6?knobs[k.id].y:281,69);
  char text[80];format(k.id,values[k.id],text,sizeof(text),true);
  if(k.id==Time&&values[Sync]>=.5)std::snprintf(text,sizeof(text),"%s sync",divisions[int(toPlain(Division,values[Division]))]);
  c.ascii(text,{k.x-106,k.y+(small?81:k.id>=Decay?105:116),212,23},18,cream,400,1);
  if(focused==int(k.id)&&hasFocus)c.circle(k.x,k.y,k.r+5,uint32_t(-1),accent,1.5);
 }
 template<class C>void toggle(C& c,int x,int y,unsigned id) const{
  const int state=int(toPlain(id,values[id]));
  c.circle(x,y+4,35,rgb(34,25,35));c.circle(x,y,34,metal,rgb(242,230,201),2);
  c.circle(x,y,26,ink,rgb(90,77,70),2);c.circle(x,y,20,rgb(40,35,33),cream,1);
  const int dx=(state-1)*11;c.stroke(x,y+9,x+dx,y-15,rgb(100,96,89),15);c.stroke(x-2,y+7,x+dx-2,y-17,rgb(223,213,191),7);
  c.circle(x+dx,y-16,10,metal,cream,1);c.circle(x+dx-2,y-20,4,rgb(242,234,214));
  if(focused==int(id)&&hasFocus)c.circle(x,y,39,uint32_t(-1),accent,1.5);
 }
 template<class C>void jewel(C& c,int x,int y,bool on,bool hold) const{
  c.circle(x,y,22,purple);c.circle(x,y,20,metal,ink,2);c.circle(x,y,15,on?(hold?rgb(255,228,164):rgb(187,115,251)):rgb(67,48,77),ink,1);
  if(on){c.circle(x,y,10,hold?rgb(255,245,207):rgb(222,179,255));c.circle(x-2,y-3,5,rgb(255,248,248));}
 }
 template<class C>void draw(C& c) const{
  c.background();c.box(presetBox,rgb(20,19,21),12,rgb(175,158,139),1);
  c.ascii(presetIndex>=0?presets[presetIndex].name:"Custom",{355,48,287,60},28,cream,400,0,false,true);
  c.stroke(657,73,666,82,cream,2);c.stroke(666,82,675,73,cream,2);
  c.box(detailsBox,dark,8,rgb(121,90,135),1);c.text(advanced?L"CLOSE":L"DETAILS",detailsBox,16,cream,400,1,true,true);
  c.box(tapBox,tapActive?rgb(104,68,128):dark,8,tapActive?accent:rgb(121,90,135),1);c.text(L"TAP",tapBox,16,cream,400,1,true,true);
  for(auto k:knobs)knob(c,k);
  toggle(c,325,743,DelayType);toggle(c,702,743,ReverbType);
  const bool bypass=values[Bypass]>=.5;
  jewel(c,260,1197,!bypass&&values[DelayOn]>=.5,false);jewel(c,512,1197,!bypass&&values[Freeze]>=.5,true);jewel(c,766,1197,!bypass&&values[ReverbOn]>=.5,false);
  for(auto k:switches)if(hasFocus&&focused==int(k.id))c.circle(k.x,k.y,k.r+2,uint32_t(-1),accent,2);
  if(bypass){c.box({362,1460,300,35},dark,6);c.text(L"BYPASSED",{362,1460,300,35},19,cream,400,1,true,true);}
  if(!advanced)return;
  c.box({110,676,804,734},dark,20,rgb(172,134,190),2);
  c.text(L"SPACE & ROUTING",{154,710,650,40},22,cream,400,0,true,true);c.text(L"X",closeBox,22,cream,400,1);
  const wchar_t* labels[]={L"DELAY TONE",L"TAPE DRIFT",L"REVERB TONE",L"MOTION",L"PRE-DELAY",L"OUTPUT"};
  for(int i=0;i<6;i++){const auto k=advancedKnobs[i];
   for(int n=0;n<=20;n++){double a=(135+270*n/20.)*pi/180;c.stroke(k.x+std::cos(a)*56,k.y+std::sin(a)*56,k.x+std::cos(a)*61,k.y+std::sin(a)*61,muted,1);}
   knob(c,k,true);c.text(labels[i],{k.x-105,k.y+59,210,22},17,cream,400,1,true,true);
  }
  const Box boxes[]={routeBox,syncBox,divisionBox,trailsBox,clearBox,bypassBox};for(auto b:boxes){const bool flash=clearActive&&b.x==clearBox.x&&b.y==clearBox.y;c.box(b,flash?rgb(104,68,128):rgb(65,45,78),7,flash?accent:rgb(117,87,132),1);}
  char route[96];std::snprintf(route,sizeof(route),"ROUTING: %s",routes[int(toPlain(Routing,values[Routing]))]);c.ascii(route,routeBox,20,cream,400,1);
  c.text(values[Sync]>=.5?L"HOST SYNC: ON":L"HOST SYNC: OFF",syncBox,17,cream,400,1);
  c.ascii(divisions[int(toPlain(Division,values[Division]))],divisionBox,20,cream,400,1);
  c.text(values[Trails]>=.5?L"TRAILS: ON":L"TRAILS: OFF",trailsBox,16,cream,400,1);
  c.text(L"CLEAR TAILS",clearBox,16,cream,400,1);c.text(bypass?L"BYPASSED":L"ENGAGED",bypassBox,16,cream,400,1);
  c.text(L"Drag knobs  /  Shift: fine  /  Double-click: reset",{154,1335,714,30},16,muted,400,1);
  c.text(L"Space: bypass    F: freeze    P: presets    A: details",{154,1368,714,24},15,muted,400,1);
 }
};
}
