#pragma once
#include "Artwork.h"
namespace glassgarden::ui {
class KnobDrag {
public:
 void begin(double value,double clientY,int width){current=clamp01(value);lastY=clientY;sensitivity=600.0/std::max(1,width)/240.0;}
 double move(double clientY,bool fine){current=clamp01(current+(lastY-clientY)*sensitivity*(fine?.1:1.0));lastY=clientY;return current;}
private:double current=0,lastY=0,sensitivity=1.0/240.0;
};
inline artwork::Box knobBounds(const artwork::Knob& k,bool small=false){const int offset=small?81:k.id>=Density?105:116;return{k.x-110,k.y-k.r-9,220,k.r+9+offset+29};}
inline artwork::Box padded(artwork::Box b,int pad=3){return{b.x-pad,b.y-pad,b.w+pad*2,b.h+pad*2};}
struct Damage {
 std::array<artwork::Box,Count*4+12> boxes{};unsigned count=0;
 void add(artwork::Box b){if(count<boxes.size())boxes[count++]=b;}
 void parameter(unsigned id,bool advanced){using namespace artwork;
  for(auto k:knobs)if(k.id==id){add(knobBounds(k));return;}
  if(advanced){for(auto k:advancedKnobs)if(k.id==id){add(knobBounds(k,true));return;}
   switch(id){case Freeze:add(padded(freezeBox));break;case Reverse:add(padded(reverseBox));break;case Sync:add(padded(syncBox));break;case Division:add(padded(divisionBox));break;case Bypass:add(padded(bypassBox));break;default:break;}
  }else{if(id==Texture)add({282,700,86,88});if(id==Harmony)add({659,700,86,88});if(id==Bypass)add({235,1172,50,50});if(id==Freeze)add({487,1172,50,50});if(id==Reverse)add({741,1172,50,50});}
  if(id==Bypass)add({358,1456,308,43});
  if(id==Sync||id==Division)add(knobBounds(knobs[3]));
 }
 void focus(int id,bool advanced){using namespace artwork;if(id<0)return;
  for(auto k:knobs)if(int(k.id)==id){add(knobBounds(k));return;}
  if(advanced){for(auto k:advancedKnobs)if(int(k.id)==id)add(knobBounds(k,true));}
  else{if(id==Texture||id==Harmony)parameter(unsigned(id),false);for(auto k:switches)if(int(k.id)==id)add({k.x-k.r-5,k.y-k.r-5,(k.r+5)*2,(k.r+5)*2});}
 }
};
inline Damage changes(const artwork::Pedal& before,const artwork::Pedal& after){Damage d;
 if(before.advanced!=after.advanced){d.add({0,0,artwork::W,artwork::H});return d;}
 for(unsigned i=0;i<Count;++i)if(before.values[i]!=after.values[i])d.parameter(i,after.advanced);
 if(before.presetIndex!=after.presetIndex)d.add(padded(artwork::presetBox));
 if(before.hasFocus!=after.hasFocus||before.focused!=after.focused){if(before.hasFocus)d.focus(before.focused,before.advanced);if(after.hasFocus)d.focus(after.focused,after.advanced);}
 if(before.tapActive!=after.tapActive)d.add(padded(artwork::clearQuickBox));
 if(before.clearActive!=after.clearActive&&after.advanced)d.add(padded(artwork::clearBox));return d;
}
} // namespace glassgarden::ui
