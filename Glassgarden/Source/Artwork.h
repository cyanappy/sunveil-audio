#pragma once
#include "Parameters.h"
#include <cstdint>
namespace glassgarden::artwork {
constexpr int W=1024,H=1536;
constexpr uint32_t rgb(int r,int g,int b){return uint32_t(r|(g<<8)|(b<<16));}
constexpr uint32_t cream=rgb(245,228,194),muted=rgb(163,192,174),bodyColor=rgb(55,80,67),dark=rgb(21,39,33),metal=rgb(196,190,171),ink=rgb(20,22,20),accent=rgb(169,241,196);
struct Box{int x,y,w,h;bool hit(int a,int b)const{return a>=x&&b>=y&&a<x+w&&b<y+h;}};
constexpr Box presetBox{326,48,373,60},detailsBox{810,57,112,38},clearQuickBox{101,57,112,38};
constexpr Box textureBox{190,690,300,130},harmonyBox{550,690,300,130};
constexpr Box freezeBox{154,1140,714,45},syncBox{154,1202,322,45},divisionBox{502,1202,366,45};
constexpr Box reverseBox{154,1264,208,45},clearBox{389,1264,208,45},bypassBox{624,1264,244,45},closeBox{841,706,46,40};
struct Knob{unsigned id;int x,y,r;};
constexpr Knob knobs[]={{Pitch,268,281,69},{GrainSize,512,281,69},{Scatter,755,281,69},{Density,268,543,69},{Feedback,512,543,69},{Mix,755,543,69}};
constexpr Knob advancedKnobs[]={{Tone,268,836,46},{Drift,512,836,46},{Width,755,836,46},{LowCut,268,1006,46},{Predelay,512,1006,46},{Output,755,1006,46}};
constexpr Knob switches[]={{Bypass,260,1304,78},{Freeze,512,1304,78},{Reverse,766,1304,78}};
inline int knobAt(int x,int y,bool advanced){if(advanced)for(auto k:advancedKnobs)if(std::hypot(double(x-k.x),double(y-k.y))<k.r+13)return int(k.id);for(auto k:knobs)if(std::hypot(double(x-k.x),double(y-k.y))<k.r+10)return int(k.id);return -1;}
inline int switchAt(int x,int y){for(auto k:switches)if(std::hypot(double(x-k.x),double(y-k.y))<k.r)return int(k.id);return -1;}
struct Pedal {
 Values values=defaults();int presetIndex=0,focused=-1;bool hasFocus=false,advanced=false,tapActive=false,clearActive=false;
 template<class C>void knob(C& c,const Knob& k,bool small=false)const{
  const auto source=knobs[k.id<6?k.id:0];c.photoKnob(k.x,k.y,k.r,-135+270*values[k.id],source.x,source.y,69);
  char text[80];format(k.id,values[k.id],text,sizeof(text),true);
  if(k.id==Density&&values[Sync]>=.5)std::snprintf(text,sizeof(text),"%s sync",divisions[int(toPlain(Division,values[Division]))]);
  c.ascii(text,{k.x-106,k.y+(small?81:k.id>=Density?105:116),212,26},22,cream,400,1);
  if(focused==int(k.id)&&hasFocus)c.circle(k.x,k.y,k.r+5,uint32_t(-1),accent,1.5);
 }
 template<class C>void toggle(C& c,int x,int y,unsigned id)const{
  const int state=int(toPlain(id,values[id]));c.circle(x,y+4,35,rgb(16,27,22));c.circle(x,y,34,metal,rgb(242,230,201),2);
  c.circle(x,y,26,ink,rgb(90,83,70),2);c.circle(x,y,20,rgb(36,40,34),cream,1);
  const int dx=(state-1)*11;c.stroke(x,y+9,x+dx,y-15,rgb(99,103,91),15);c.stroke(x-2,y+7,x+dx-2,y-17,rgb(225,220,195),7);
  c.circle(x+dx,y-16,10,metal,cream,1);c.circle(x+dx-2,y-20,4,rgb(247,242,223));
  if(focused==int(id)&&hasFocus)c.circle(x,y,39,uint32_t(-1),accent,1.5);
 }
 template<class C>void jewel(C& c,int x,int y,bool on,bool hold=false)const{
  c.circle(x,y,22,bodyColor);c.circle(x,y,20,metal,ink,2);c.circle(x,y,15,on?(hold?rgb(235,189,102):rgb(101,207,152)):rgb(42,62,51),ink,1);
  if(on){c.circle(x,y,10,hold?rgb(255,230,168):rgb(193,255,217));c.circle(x-2,y-3,5,rgb(250,255,244));}
 }
 template<class C>void draw(C& c)const{
  c.background();c.box(presetBox,rgb(16,25,21),12,rgb(180,171,146),1);
  c.ascii(presetIndex>=0?presets[presetIndex].name:"Custom",{355,48,287,60},27,cream,400,0,false,true);
  c.stroke(657,73,666,82,cream,2);c.stroke(666,82,675,73,cream,2);
  c.box(detailsBox,dark,8,rgb(101,140,112),1);c.text(advanced?L"CLOSE":L"DETAILS",detailsBox,16,cream,400,1,true,true);
  c.box(clearQuickBox,tapActive?rgb(58,102,75):dark,8,tapActive?accent:rgb(101,140,112),1);c.text(L"CLEAR",clearQuickBox,16,cream,400,1,true,true);
  for(auto k:knobs)knob(c,k);toggle(c,325,743,Texture);toggle(c,702,743,Harmony);
  const bool bypass=values[Bypass]>=.5;jewel(c,260,1197,!bypass);jewel(c,512,1197,values[Freeze]>=.5,true);jewel(c,766,1197,values[Reverse]>=.5);
  for(auto k:switches)if(hasFocus&&focused==int(k.id))c.circle(k.x,k.y,k.r+2,uint32_t(-1),accent,2);
  if(bypass){c.box({362,1460,300,35},dark,6);c.text(L"BYPASSED",{362,1460,300,35},19,cream,400,1,true,true);}
  if(!advanced)return;
  c.box({110,676,804,734},dark,20,rgb(140,180,150),2);c.text(L"GARDEN DETAILS",{154,710,650,40},22,cream,400,0,true,true);c.text(L"X",closeBox,22,cream,400,1);
  const wchar_t* labels[]={L"TONE",L"DRIFT",L"WIDTH",L"LOW CUT",L"PRE-DELAY",L"OUTPUT"};
  for(int i=0;i<6;++i){const auto k=advancedKnobs[i];for(int n=0;n<=20;++n){const double a=(135+270*n/20.)*pi/180;c.stroke(k.x+std::cos(a)*56,k.y+std::sin(a)*56,k.x+std::cos(a)*61,k.y+std::sin(a)*61,muted,1);}knob(c,k,true);c.text(labels[i],{k.x-105,k.y+56,210,26},19,cream,400,1,true,true);}
  const Box boxes[]={freezeBox,syncBox,divisionBox,reverseBox,clearBox,bypassBox};for(auto b:boxes){const bool flash=clearActive&&b.x==clearBox.x&&b.y==clearBox.y;c.box(b,flash?rgb(58,102,75):rgb(36,65,48),7,flash?accent:rgb(93,133,104),1);}
  c.text(values[Freeze]>=.5?L"CAPTURE: FROZEN":L"CAPTURE: LIVE",freezeBox,20,cream,400,1);
  c.text(values[Sync]>=.5?L"HOST SYNC: ON":L"HOST SYNC: OFF",syncBox,17,cream,400,1);c.ascii(divisions[int(toPlain(Division,values[Division]))],divisionBox,20,cream,400,1);
  c.text(values[Reverse]>=.5?L"REVERSE: ON":L"REVERSE: OFF",reverseBox,16,cream,400,1);c.text(L"CLEAR BUFFER",clearBox,15,cream,400,1);c.text(bypass?L"BYPASSED":L"ENGAGED",bypassBox,16,cream,400,1);
  c.text(L"Drag / Shift: fine / Double-click: reset",{154,1335,714,30},17,muted,400,1);c.text(L"Space: bypass   F: freeze   R: reverse   C: clear",{154,1368,714,24},16,muted,400,1);
 }
};
} // namespace glassgarden::artwork
