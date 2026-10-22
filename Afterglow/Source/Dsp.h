#pragma once
#include "Parameters.h"
#include <vector>
#include <limits>
namespace afterglow {
class Ring {
 std::vector<double> data;size_t pos=0;
 public:
 void prepare(size_t n){data.assign(std::max(size_t(8),n),0);pos=0;}
 void clear(){std::fill(data.begin(),data.end(),0);pos=0;}
 void put(double x){data[pos]=std::abs(x)<1.e-24?0:x;if(++pos==data.size())pos=0;}
 double read(double delay)const{
  delay=std::clamp(delay,1.,double(data.size()-2));double p=double(pos)-delay;if(p<0)p+=data.size();
  const size_t a=size_t(p),b=a+1==data.size()?0:a+1;return data[a]+(data[b]-data[a])*(p-a);
 }
};
class Diffuser {
 std::vector<double> data;size_t pos=0;
 public:
 void prepare(size_t n){data.assign(std::max(size_t(2),n),0);pos=0;}
 void clear(){std::fill(data.begin(),data.end(),0);pos=0;}
 double run(double x){const double y=data[pos]-.62*x;data[pos]=x+.62*y;if(++pos==data.size())pos=0;return y;}
};
class Octave {
 Ring line;double phase=0,window=1;
 public:
 void prepare(double sr){window=std::round(sr*.065);line.prepare(size_t(window)+64);phase=0;}
 void clear(){line.clear();phase=0;}
 double run(double x){line.put(x);phase+=1/window;if(phase>=1)phase-=1;
  double q=phase+.5;if(q>=1)q-=1;const double a=.5-.5*std::cos(phase*2*pi);
  return line.read(12+(1-phase)*window)*a+line.read(12+(1-q)*window)*(1-a);
 }
};
class Engine {
 double sr=48000,tempo=120,smooth=.001,timeSmooth=.001,bypassSmooth=.001,dcCut=.001,envAttack=.001,envRelease=.001,bloomRate=.001;
 Values target{},plain{},current{};
 std::array<Ring,2> delay,pre;
 std::array<Diffuser,4> ap;
 std::array<Ring,8> fd;
 std::array<double,8> base{},low{},dc{},v{},h{},phaseCos{},phaseSin{},gain{},gainTarget{};
 std::array<double,2> delayLow{},delayDC{};
 Octave octave;
 double lfo=0,envelope=0,bloom=1,wetLast=0,delayCut=0,verbCut=0,delayCutTarget=0,verbCutTarget=0;
 double gainOut=1,gainOutTarget=1,effectiveTime=520,bypass=0;
 double dl=0,dr=0,dwl=0,dwr=0,rl=0,rr=0,rwl=0,rwr=0;
 void updateTime(){
  effectiveTime=target[Sync]>=.5?60000/tempo*beatLengths[int(toPlain(Division,target[Division]))]:toPlain(Time,target[Time]);
  effectiveTime=std::clamp(effectiveTime,40.,4000.);
 }
 void delayTick(double l,double r,double sn,double cs){
  const int mode=int(toPlain(DelayType,target[DelayType]));const double drift=mode==1?0:current[Drift]*.01;
  const double t=current[Time]*.001*sr;
  double a=delay[0].read(std::max(2.,t+sn*drift*sr*.0008));
  double b=delay[1].read(std::max(2.,t*(mode==0?1.003:1)+(sn*.7+cs*.7)*drift*sr*.0008));
  delayLow[0]+=delayCut*(a-delayLow[0]);delayLow[1]+=delayCut*(b-delayLow[1]);
  delayDC[0]+=dcCut*(delayLow[0]-delayDC[0]);delayDC[1]+=dcCut*(delayLow[1]-delayDC[1]);
  for(int i=0;i<2;i++){if(std::abs(delayLow[i])<1.e-24)delayLow[i]=0;if(std::abs(delayDC[i])<1.e-24)delayDC[i]=0;}
  a=delayLow[0]-delayDC[0];b=delayLow[1]-delayDC[1];
  if(mode==0){a=std::tanh(a*1.15)/1.15;b=std::tanh(b*1.15)/1.15;}
  const double on=current[DelayOn],loop=current[Feedback]*.01*(target[Trails]>=.5?1:on);
  double il=l*on,ir=r*on;if(mode==2){il=(l+r)*.5*on;ir=0;}
  delay[0].put(il+(mode==2?b:a)*loop);delay[1].put(ir+(mode==2?a:b)*loop);
  const double mix=current[DelayMix]*.01,wet=std::sin(mix*pi*.5)*(target[Trails]>=.5?1:on),dry=1+(std::cos(mix*pi*.5)-1)*on;
  dwl=a*wet;dwr=b*wet;dl=l*dry+dwl;dr=r*dry+dwr;
 }
 void reverbTick(double l,double r,double sn,double cs){
  const double fr=current[Freeze],send=current[ReverbOn]*(1-fr);
  pre[0].put(l*send);pre[1].put(r*send);const double pd=std::max(1.,current[Predelay]*sr*.001);
  const double a=ap[1].run(ap[0].run(pre[0].read(pd))),b=ap[3].run(ap[2].run(pre[1].read(pd)));
  const double env=std::max(std::abs(l),std::abs(r));envelope+=(env>envelope?envAttack:envRelease)*(env-envelope);if(std::abs(envelope)<1.e-24)envelope=0;
  const int mode=int(toPlain(ReverbType,target[ReverbType]));
  const double duck=mode==1?1/(1+envelope*13):1;bloom+=bloomRate*(duck-bloom);
  for(int i=0;i<8;i++){
   const double mod=(sn*phaseCos[i]+cs*phaseSin[i])*current[Motion]*.01*sr*.0007*(1-fr);
   const double x=fd[i].read(std::max(2.,base[i]+mod));
   low[i]+=verbCut*(x-low[i]);dc[i]+=dcCut*(low[i]-dc[i]);if(std::abs(low[i])<1.e-24)low[i]=0;if(std::abs(dc[i])<1.e-24)dc[i]=0;
   v[i]=(low[i]-dc[i])*(1-fr)+x*fr;h[i]=v[i];gain[i]+=smooth*(gainTarget[i]-gain[i]);
  }
  for(int span=1;span<8;span*=2)for(int start=0;start<8;start+=span*2)for(int j=0;j<span;j++){
   const double x=h[start+j],y=h[start+j+span];h[start+j]=x+y;h[start+j+span]=x-y;
  }
  const double sh=current[Shimmer]*.01*(mode==2?1:.8)*(1-fr);
  const double shifted=std::tanh(octave.run(wetLast)*1.2)/1.2,gate=target[Trails]>=.5?1:current[ReverbOn];
  for(int i=0;i<8;i++){
   const double feedback=(gain[i]*(1-fr)+fr*.999999)*gate;
   double x=((i&1)?b:a)*((i&2)?-.24:.24)+feedback*(h[i]*.3535533905932738*(1-sh*.035)+shifted*sh*((i&1)?-.012:.012));
   if(std::abs(x)>2)x=2*std::tanh(x*.5);fd[i].put(x);
  }
  const double wl=(v[0]+v[2]-v[4]+v[6])*.5,wr=(v[1]-v[3]+v[5]+v[7])*.5;
  wetLast=(wl+wr)*.5;const double mix=current[ReverbMix]*.01;
  const double wet=std::sin(mix*pi*.5)*(target[Trails]>=.5?1:current[ReverbOn])*bloom,dry=1+(std::cos(mix*pi*.5)-1)*current[ReverbOn];
  rwl=wl*wet;rwr=wr*wet;rl=l*dry+rwl;rr=r*dry+rwr;
 }
 static double safeOutput(double x){return std::abs(x)>.9?std::copysign(.9+.1*std::tanh((std::abs(x)-.9)*10),x):x;}
 public:
 static constexpr unsigned latency=0;
 void prepare(double sampleRate,const Values& values){
  sr=std::clamp(sampleRate,22050.,384000.);
  for(auto& x:delay)x.prepare(size_t(std::ceil(sr*4.2))+16);
  for(auto& x:pre)x.prepare(size_t(std::ceil(sr*.21))+16);
  const double diffuserTimes[]={.0047,.0137,.0067,.0179};for(int i=0;i<4;i++)ap[i].prepare(size_t(std::round(sr*diffuserTimes[i])));
  const double times[]={.0311,.0379,.0437,.0499,.0581,.0677,.0793,.0899};
  for(int i=0;i<8;i++){base[i]=size_t(std::round(sr*times[i]))|1;fd[i].prepare(size_t(base[i]+sr*.004+16));phaseCos[i]=std::cos(i*1.732);phaseSin[i]=std::sin(i*1.732);}
  octave.prepare(sr);smooth=1-std::exp(-1/(.025*sr));timeSmooth=1-std::exp(-1/(.12*sr));bypassSmooth=1-std::exp(-1/(.005*sr));
  dcCut=1-std::exp(-2*pi*35/sr);envAttack=1-std::exp(-1/(.005*sr));envRelease=1-std::exp(-1/(.35*sr));bloomRate=1-std::exp(-1/(.35*sr));
  target.fill(-1);setAll(values);for(unsigned i=0;i<Count;i++)current[i]=plain[i];current[Time]=effectiveTime;
  gain=gainTarget;gainOut=gainOutTarget;delayCut=delayCutTarget;verbCut=verbCutTarget;bypass=target[Bypass];clear();
 }
 void clear(){for(auto& x:delay)x.clear();for(auto& x:pre)x.clear();for(auto& x:fd)x.clear();for(auto& x:ap)x.clear();octave.clear();
  low.fill(0);dc.fill(0);v.fill(0);h.fill(0);delayLow.fill(0);delayDC.fill(0);lfo=0;envelope=0;bloom=1;wetLast=0;
 }
 void setAll(const Values& values){for(unsigned i=0;i<Count;i++)set(i,values[i]);}
 void set(unsigned id,double normalized){
  if(id>=Count)return;normalized=canonical(id,normalized);if(normalized==target[id])return;
  target[id]=normalized;plain[id]=toPlain(id,normalized);
  if(id==Time||id==Sync||id==Division)updateTime();
  if(id==Decay)for(int i=0;i<8;i++)gainTarget[i]=std::pow(.001,base[i]/sr/plain[Decay]);
  if(id==DelayTone)delayCutTarget=1-std::exp(-2*pi*(700+14000*normalized*normalized)/sr);
  if(id==ReverbTone)verbCutTarget=1-std::exp(-2*pi*(650+14000*normalized*normalized)/sr);
  if(id==Output)gainOutTarget=dbToGain(plain[id]);
 }
 void setTempo(double bpm){if(std::isfinite(bpm)&&bpm>=20&&bpm<=400&&bpm!=tempo){tempo=bpm;updateTime();}}
 void process(double l,double r,double& outL,double& outR){
  if(!std::isfinite(l))l=0;if(!std::isfinite(r))r=0;
  constexpr unsigned continuous[]={Feedback,DelayMix,Shimmer,ReverbMix,Drift,Motion,Predelay,DelayOn,ReverbOn,Freeze};
  for(auto id:continuous){current[id]+=smooth*(plain[id]-current[id]);if(std::abs(current[id]-plain[id])<1.e-12)current[id]=plain[id];}
  current[Time]+=timeSmooth*(effectiveTime-current[Time]);delayCut+=smooth*(delayCutTarget-delayCut);verbCut+=smooth*(verbCutTarget-verbCut);gainOut+=smooth*(gainOutTarget-gainOut);
  bypass+=bypassSmooth*(target[Bypass]-bypass);if(std::abs(bypass-target[Bypass])<1.e-12)bypass=target[Bypass];
  lfo+=2*pi*.19/sr;if(lfo>2*pi)lfo-=2*pi;const double sn=std::sin(lfo),cs=std::cos(lfo);double a,b;
  const int route=int(toPlain(Routing,target[Routing]));
  if(route==0){delayTick(l,r,sn,cs);reverbTick(dl,dr,sn,cs);a=rl;b=rr;}
  else if(route==1){reverbTick(l,r,sn,cs);delayTick(rl,rr,sn,cs);a=dl;b=dr;}
  else {delayTick(l,r,sn,cs);reverbTick(l,r,sn,cs);const double dry=std::min(1+(std::cos(current[DelayMix]*.01*pi*.5)-1)*current[DelayOn],1+(std::cos(current[ReverbMix]*.01*pi*.5)-1)*current[ReverbOn]);a=l*dry+dwl+rwl;b=r*dry+dwr+rwr;}
  outL=safeOutput(a*gainOut)*(1-bypass)+l*bypass;outR=safeOutput(b*gainOut)*(1-bypass)+r*bypass;
 }
 double effectiveDelayMilliseconds()const{return effectiveTime;}
};
}
