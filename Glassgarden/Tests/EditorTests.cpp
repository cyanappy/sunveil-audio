#include "EditorCases.h"
#include <cstdio>
#include <cstdlib>
using namespace glassgarden;
using namespace glassgarden::ui;
static unsigned checks=0;
static void check(bool ok,const char* message) {
    ++checks;
    if (!ok) { std::fprintf(stderr,"FAIL: %s\n",message); std::exit(1); }
}
static bool near(double a,double b) { return std::abs(a-b)<1.e-12; }
int main() {
    KnobDrag drag;
    drag.begin(.5,200,600);
    check(near(drag.move(199,false),.5+1./240),"one mouse pixel produces a small, precise increment");
    check(near(drag.move(199,false),.5+1./240),"stationary events do not change the value");
    const double regular=drag.move(198,false);
    check(near(drag.move(197,true)-regular,1./2400),"Shift fine movement is ten times finer without a jump");
    check(near(drag.move(198,true),regular),"fine movement reverses precisely");
    KnobDrag many,one;
    many.begin(.1,300,600); one.begin(.1,300,600);
    double result=0;
    for(int y=299;y>=180;--y) result=many.move(y,false);
    check(near(result,one.move(180,false)),"coalesced mouse events preserve the total travel");
    for (int width : {400,600,900,1200}) {
        drag.begin(.2,600,width);
        check(near(drag.move(600-120.*width/600,false),.7),"drag sensitivity is consistent across editor sizes");
    }
    drag.begin(.99,200,600);
    check(drag.move(-400,false)==1,"drag clamps at the maximum");
    check(near(drag.move(-399,false),1-1./240),"reversing at a limit responds immediately");
    drag.begin(.01,200,600);
    check(drag.move(800,false)==0,"drag clamps at the minimum");
    check(near(drag.move(799,true),1./2400),"fine movement at the minimum has no dead travel");
    artwork::Pedal base;
    check(changes(base,base).count==0,"idle editor needs no repaint");
    auto after=base; after.values[Pitch]=.9; after.presetIndex=-1;
    const auto damage=changes(base,after);
    int area=0;
    for(unsigned i=0;i<damage.count;++i) area+=damage.boxes[i].w*damage.boxes[i].h;
    check(area<artwork::W*artwork::H/20,"a knob edit damages less than five percent of the artwork");
    for (const auto& c : editorCases()) {
        auto d=changes(c.before,c.after);
        check(d.count<d.boxes.size(),"redraw-region list has capacity for every control state");
        for(unsigned i=0;i<d.count;++i)
            check(d.boxes[i].w>0&&d.boxes[i].h>0,"redraw regions have positive area");
    }
    std::printf("PASS: %u editor checks (drag precision, fine control, resize sensitivity, coalescing, limits and redraw regions).\n",checks);
}
