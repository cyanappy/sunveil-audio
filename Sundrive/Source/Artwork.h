#pragma once
#include "Dsp.h"
#include <cstdint>

namespace sundrive::artwork {
constexpr int W = 600, H = 900;
constexpr uint32_t rgb(int r,int g,int b) { return uint32_t(r|(g<<8)|(b<<16)); }
constexpr uint32_t ink=rgb(34,33,30), panel=rgb(239,232,216), paper=rgb(248,243,231),
 line=rgb(210,199,179), cream=rgb(244,234,211), muted=rgb(132,119,100),
 copper=rgb(160,91,55), copperLight=rgb(217,166,120), sideLine=rgb(65,61,52);
struct Box { int x,y,w,h; bool hit(int a,int b) const {return a>=x&&b>=y&&a<x+w&&b<y+h;} };
constexpr Box presetBox{160,14,280,34}, voiceBox{65,453,220,70}, clipBox{315,453,220,70};
constexpr int stompX=300, stompY=795, stompRadius=44;
inline bool stompHit(int x,int y) {return std::hypot(double(x-stompX),double(y-stompY))<stompRadius;}
struct Knob {unsigned id; int x,y,r;};
constexpr Knob knobs[]={{Drive,140,155,43},{Tone,300,155,43},{Level,460,155,43},
 {Bass,140,320,43},{Mid,300,320,43},{Treble,460,320,43}};
struct Pedal {
 Values values=defaults(); int presetIndex=0,focused=-1; bool hasFocus=false;
 double inputPeak=0,outputPeak=0;
 double value(unsigned id) const {return values[id];}
 template<class C> void screw(C& c,int x,int y) {
  c.circle(x,y+2,10,rgb(176,162,139)); c.circle(x,y,9,rgb(209,205,193),ink,1);
  c.circle(x,y,6,rgb(118,115,107)); c.stroke(x-4,y+4,x+4,y-4,ink,2);
 }
 template<class C> void drawKnob(C& c,const Knob& k) {
  const double v=value(k.id), a=(135+270*v)*pi/180;
  for(int i=0;i<=24;++i) {double t=(135+270*i/24.)*pi/180;
   c.stroke(k.x+std::cos(t)*53,k.y+std::sin(t)*53,k.x+std::cos(t)*(i%4?57:61),k.y+std::sin(t)*(i%4?57:61),copper,i%4?1:2);}
  c.circle(k.x,k.y+5,k.r+2,rgb(190,179,158));
  c.circle(k.x,k.y,k.r,ink,rgb(103,100,91),1);
  for(int i=0;i<40;++i) {double t=i*2*pi/40;
   c.stroke(k.x+std::cos(t)*38,k.y+std::sin(t)*38,k.x+std::cos(t)*42,k.y+std::sin(t)*42,rgb(77,74,67),2);}
  c.circle(k.x,k.y-2,35,rgb(47,46,42),rgb(88,85,76),1);
  for(int i=31;i>2;i-=4) c.circle(k.x,k.y-2,i,rgb(47,46,42),rgb(53,52,47),.5);
  c.stroke(k.x+std::cos(a)*18,k.y-2+std::sin(a)*18,k.x+std::cos(a)*33,k.y-2+std::sin(a)*33,paper,3);
  if(focused==int(k.id)&&hasFocus) c.circle(k.x,k.y,3,copperLight);
  char t[64]; format(k.id,v,t,sizeof(t),true);
  c.ascii(t,{k.x-65,k.y+87,130,20},14,ink,500,1);
 }
 template<class C> void toggle(C& c,Box b,unsigned id,const wchar_t* title) {
  const int x=b.x+b.w/2,y=b.y+38;
  c.text(title,{b.x,b.y-20,b.w,20},14,ink,600,1,true,true);
  c.circle(x,y+2,17,rgb(184,172,149)); c.circle(x,y,16,rgb(211,209,199),ink,1);
  c.circle(x,y,11,ink,rgb(248,244,226),2);
  const int state=int(std::round(value(id)*(id==Voice?1:2)));
  const int dx=id==Voice?(state?8:-8):(state-1)*8;
  c.stroke(x,y+3,x+dx,y-10,rgb(130,128,118),8);
  c.stroke(x-1,y,x+dx-1,y-11,rgb(235,233,218),4);
  c.circle(x+dx,y-11,5,rgb(214,213,200),rgb(105,103,94),1);
  if(focused==int(id)&&hasFocus) c.stroke(x-19,y+22,x+19,y+22,copper,1);
 }
 template<class C> void footswitch(C& c) {
  const bool off=value(Bypass)>=.5;
  for(int i=0;i<28;++i) {double a=(20+i*140/27.)*pi/180;
   c.stroke(stompX+std::cos(a)*55,stompY+std::sin(a)*55,stompX+std::cos(a)*105,stompY+std::sin(a)*70,copper,.7);}
  c.circle(stompX,stompY+4,42,rgb(184,171,149));
  c.circle(stompX,stompY,41,rgb(129,128,119),ink,1);
  c.circle(stompX,stompY-2,36,rgb(231,230,215),rgb(249,246,232),2);
  c.circle(stompX,stompY-2,29,rgb(142,141,131),ink,1);
  c.circle(stompX,stompY-3,26,rgb(188,187,173),rgb(236,234,218),1);
  for(int i=-18;i<=18;i+=3) c.stroke(stompX-std::sqrt(26.*26-i*i),stompY-3+i,stompX+std::sqrt(26.*26-i*i),stompY-3+i,rgb(172,171,158),.5);
  c.circle(300,705,10,rgb(220,214,195),copper,1);
  c.circle(300,705,7,off?rgb(100,78,47):rgb(255,184,57),ink,1);
  if(!off) c.circle(298,703,3,rgb(255,235,157));
  c.text(off?L"BYPASSED":L"ENGAGED",{180,725,240,22},14,ink,600,1,true,true);
  if(focused==Bypass&&hasFocus) c.circle(stompX,stompY-3,5,copper);
 }
 template<class C> void draw(C& c) {
  c.box({0,0,W,H},rgb(31,31,29));
  c.box({45,66,518,814},rgb(20,20,18),30);
  c.box({38,58,524,814},rgb(181,169,145),28,rgb(110,102,88));
  c.box({40,58,520,808},paper,27,rgb(228,218,198));
  c.box({47,65,506,792},panel,22,rgb(250,245,231));
  c.box({58,76,484,770},panel,15,copper);
  // Quiet powder-coat speckle, stable at every parameter value.
  for(int i=0;i<1100;++i) {int x=63+(i*137)%474,y=81+(i*251)%758;
   c.stroke(x,y,x+.6,y+.2,i%2?rgb(231,223,205):rgb(244,237,221),.5);}
  screw(c,73,91); screw(c,527,91); screw(c,73,831); screw(c,527,831);
  c.box(presetBox,rgb(38,37,33),7,rgb(114,102,82));
  c.ascii(presetIndex>=0?presets[presetIndex].name:"Custom",{174,14,225,34},15,cream,500);
  c.stroke(416,27,422,33,cream,1);c.stroke(422,33,428,27,cream,1);
  const wchar_t* labels[]={L"DRIVE",L"TONE",L"LEVEL",L"BASS",L"MID",L"TREBLE"};
  for(int i=0;i<6;++i) {drawKnob(c,knobs[i]);c.text(labels[i],{knobs[i].x-65,knobs[i].y+62,130,24},17,ink,600,1,true,true);}
  toggle(c,voiceBox,Voice,L"VOICE");toggle(c,clipBox,Clipping,L"CLIPPING");
  c.text(L"TRANSPARENT",{64,497,92,18},11,value(Voice)<.5?rgb(116,59,32):rgb(91,82,68),600,1);
  c.text(L"MID PUSH",{195,497,89,18},11,value(Voice)>=.5?rgb(116,59,32):rgb(91,82,68),600,1);
  const wchar_t* cl[]={L"SOFT",L"LED",L"ASYM"};
  for(int i=0;i<3;++i)c.text(cl[i],{320+i*70,520,70,17},12,int(std::round(value(Clipping)*2))==i?rgb(116,59,32):rgb(91,82,68),600,1);
  // Copper sunrise, clipped by the horizon using the faceplate color.
  for(int i=0;i<=30;++i) {double a=(180+i*6)*pi/180;
   c.stroke(300+std::cos(a)*52,596+std::sin(a)*52,300+std::cos(a)*(i%2?58:62),596+std::sin(a)*(i%2?58:62),copper,.8);}
  c.circle(300,596,48,copper);c.box({245,596,110,52},panel);
  for(int i=0;i<5;++i)c.stroke(76,596+i*5,524,596+i*5,copper,i? .6:1.5);
  c.text(L"SUNVEIL AUDIO",{85,627,430,27},21,copper,500,1,true,true);
  c.text(L"S U N D R I V E",{85,660,430,25},23,copper,500,1,false,true);
  footswitch(c);
  c.text(L"Drag: turn   ·   Shift: fine   ·   Double-click: reset",{45,879,510,16},11,rgb(218,209,189),400,1);
 }
};
} // namespace sundrive::artwork
