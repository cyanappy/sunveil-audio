#include "DspCases.inc"
static void audioTests(IPluginFactory* factory){
    Instance p(factory);check(p.controller->getParameterCount()==Count,"all 19 host parameters exposed");check(p.audio->getLatencySamples()==0,"host latency is zero samples");check(p.audio->getTailSamples()==kInfiniteTail,"host retains infinite freeze tail");
    for(unsigned id=0;id<Count;id++){
        ParameterInfo info{};check(p.controller->getParameterInfo(id,info)==kResultOk&&info.id==id&&(info.flags&ParameterInfo::kCanAutomate),"stable automatable parameter metadata");
        for(double value:{0.,.2,.5,.99,1.}){String128 text{};double parsed=0,expected=canonical(id,value);check(p.controller->getParamStringByValue(id,expected,text)==kResultOk,"format parameter value");check(p.controller->getParamValueByString(id,text,parsed)==kResultOk&&std::abs(parsed-expected)<.006,"host parameter text round trip");}
    }
    ParameterInfo bypass{};p.controller->getParameterInfo(Bypass,bypass);check((bypass.flags&ParameterInfo::kIsBypass)!=0,"host bypass flag");
    IProcessContextRequirements* req=nullptr;p.component->queryInterface(IProcessContextRequirements_iid,reinterpret_cast<void**>(&req));check(req&&(req->getProcessContextRequirements()&IProcessContextRequirements::kNeedTempo),"processor asks host for tempo");if(req)req->release();
    for(double sr:{44100.,48000.,96000.,192000.})for(int precision:{kSample32,kSample64})for(int mode=0;mode<3;mode++){
        int ic=mode==2?2:1,oc=mode==0?1:2;auto v=neutral();p.set(v);p.configure(sr,precision,ic,oc);
        if(precision==kSample32){std::vector<float> l(257),r(257),a(257),b(257);for(int i=0;i<257;i++){l[i]=float(.2*std::sin(i*.1));r[i]=float(.15*std::cos(i*.07));}runAudio(p,257,l.data(),r.data(),a.data(),b.data(),nullptr,ic,oc);for(int i=0;i<257;i++)check(std::abs(a[i]-l[i])<1.e-7&&(oc==1||std::abs(b[i]-(ic==1?l[i]:r[i]))<1.e-7),"float32 mono/stereo dry path");}
        else{std::vector<double> l(257),r(257),a(257),b(257);for(int i=0;i<257;i++){l[i]=.2*std::sin(i*.1);r[i]=.15*std::cos(i*.07);}runAudio(p,257,l.data(),r.data(),a.data(),b.data(),nullptr,ic,oc);for(int i=0;i<257;i++)check(std::abs(a[i]-l[i])<1.e-12&&(oc==1||std::abs(b[i]-(ic==1?l[i]:r[i]))<1.e-12),"float64 mono/stereo dry path");}
    }
    auto v=defaults();for(unsigned i=0;i<Count;i++)v[i]=canonical(i,(i+1.)/(Count+1));p.set(v);Stream state;check(p.component->getState(&state)==kResultOk,"serialize processor state");state.pos=0;check(p.controller->setComponentState(&state)==kResultOk,"restore controller from processor state");for(unsigned i=0;i<Count;i++)check(std::abs(p.controller->getParamNormalized(i)-v[i])<1.e-12,"state restores every parameter");
    Stream truncated;truncated.bytes=state.bytes;truncated.bytes.resize(17);check(p.component->setState(&truncated)!=kResultOk,"reject truncated state");Stream corrupt;corrupt.bytes=state.bytes;double nan=std::numeric_limits<double>::quiet_NaN();std::memcpy(corrupt.bytes.data()+12,&nan,sizeof(nan));check(p.component->setState(&corrupt)!=kResultOk,"reject nonfinite stored values");
    {
        Instance a(factory),b(factory);auto v=neutral();v[Mix]=.3;v[GrainSize]=fromPlain(GrainSize,100);v[Texture]=.5;v[Drift]=0;a.set(v);b.set(v);a.configure();b.configure();
        std::vector<float> l(1024),r(1024),al(1024),ar(1024),bl(1024),br(1024);for(int i=0;i<1024;i++){l[i]=float(std::sin(i*.17)*.2);r[i]=float(std::cos(i*.13)*.1);}
        Changes changes;changes.add(Mix,37,.1);changes.add(Mix,400,.8);changes.add(Mix,900,.3);changes.add(Feedback,601,.7);runAudio(a,1024,l.data(),r.data(),al.data(),ar.data(),&changes);
        for(int offset=0;offset<1024;offset+=64){Changes block;for(auto event:std::array<Point,3>{{{37,.1},{400,.8},{900,.3}}})if(event.offset>=offset&&event.offset<offset+64)block.add(Mix,event.offset-offset,event.value);if(601>=offset&&601<offset+64)block.add(Feedback,601-offset,.7);runAudio(b,64,l.data()+offset,r.data()+offset,bl.data()+offset,br.data()+offset,&block);}
        for(int i=0;i<1024;i++)check(al[i]==bl[i]&&ar[i]==br[i],"sample-offset automation is independent of block size");
    }
    {
        auto v=neutral();v[Mix]=1;p.set(v);p.configure();std::vector<float> l(1024),r(1024),a(1024),b(1024);for(int i=0;i<1024;++i)l[i]=r[i]=float(.2*std::sin(i*.1));runAudio(p,1024,l.data(),r.data(),a.data(),b.data());
        l.assign(1024,0);r.assign(1024,0);double energy=0;for(int i=0;i<8;i++){runAudio(p,1024,l.data(),r.data(),a.data(),b.data());for(int n=0;n<1024;n++)energy+=a[n]*a[n]+b[n]*b[n];}check(energy>1.e-7,"granular capture renders after input becomes silent");
    }
    std::printf("VST3 host contract passed.\n");
}
#ifdef _WIN32
static void pump(){MSG msg;while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);}}
static void uiTests(IPluginFactory* factory){
    Instance p(factory);p.configure();auto view=p.controller->createView(ViewType::kEditor);check(view!=nullptr,"native Windows editor exists");check(view->isPlatformTypeSupported(kPlatformTypeHWND)==kResultTrue,"editor accepts HWND");
    auto parent=CreateWindowExW(0,L"STATIC",L"Sunveil Glassgarden validation",WS_OVERLAPPEDWINDOW|WS_VISIBLE,30,30,650,970,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);check(parent!=nullptr,"create test host window");check(view->attached(parent,kPlatformTypeHWND)==kResultOk,"attach editor to host");pump();auto child=GetWindow(parent,GW_CHILD);check(child!=nullptr,"native editor child window");
    ViewRect rect;view->getSize(&rect);check(rect.getWidth()==600&&rect.getHeight()==900,"default editor size");
    auto click=[&](int x,int y){ViewRect size;view->getSize(&size);int xx=x*size.getWidth()/artwork::W,yy=y*size.getHeight()/artwork::H;SendMessageW(child,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(xx,yy));SendMessageW(child,WM_LBUTTONUP,0,MAKELPARAM(xx,yy));check(!GetUpdateRect(child,nullptr,FALSE),"click feedback is painted without waiting for a timer");pump();};
    click(325,743);check(p.controller->getParamNormalized(Texture)==1,"texture toggle changes mode");click(325,743);click(325,743);check(p.controller->getParamNormalized(Texture)==.5,"texture toggle cycles all three modes");
    click(512,1304);check(p.controller->getParamNormalized(Freeze)==1,"freeze footswitch holds");click(512,1304);check(p.controller->getParamNormalized(Freeze)==0,"freeze footswitch releases");
    {
        const int xx=512*600/artwork::W,yy=1304*900/artwork::H;
        SendMessageW(child,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(xx,yy));SendMessageW(child,WM_LBUTTONUP,0,MAKELPARAM(xx,yy));
        SendMessageW(child,WM_LBUTTONDBLCLK,MK_LBUTTON,MAKELPARAM(xx,yy));SendMessageW(child,WM_LBUTTONUP,0,MAKELPARAM(xx,yy));
        check(p.controller->getParamNormalized(Freeze)==0,"rapid switch clicks include the DBLCLK press");
    }
    int x=268*600/artwork::W,y=281*900/artwork::H;double old=p.controller->getParamNormalized(GrainSize);SendMessageW(child,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(x,y));SendMessageW(child,WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(x,y-25));SendMessageW(child,WM_LBUTTONUP,0,MAKELPARAM(x,y-25));check(p.controller->getParamNormalized(GrainSize)>old,"knob drag edits a host parameter");check(p.handler.begins==p.handler.ends,"native editor closes every automation gesture");
    check(std::abs(p.controller->getParamNormalized(GrainSize)-std::min(1.,old+25./240))<1.e-12,"native knob drag retains client-pixel precision");
    p.controller->setParamNormalized(GrainSize,1);
    SendMessageW(child,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(x,y));const int edits=p.handler.edits;
    SendMessageW(child,WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(x,y-20));check(p.handler.edits==edits,"clamped knob movements do not flood host automation");
    SendMessageW(child,WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(x,y-19));check(p.controller->getParamNormalized(GrainSize)<1,"reversing a clamped native drag responds immediately");
    SendMessageW(child,WM_CANCELMODE,0,0);check(p.handler.begins==p.handler.ends&&GetCapture()!=child,"cancelled drag closes its gesture and releases capture");
    click(860,76);click(315,1224);check(p.controller->getParamNormalized(Sync)==1,"Details panel changes host tempo sync");
    for(int width:{400,600,900,1200,600}){ViewRect size(0,0,width,width*3/2);check(view->onSize(&size)==kResultOk,"resize cached editor surfaces");UpdateWindow(child);pump();}
    check(view->removed()==kResultOk,"detach native editor");view->release();DestroyWindow(parent);std::printf("Native Windows editor controls passed.\n");
}
#endif
int main(int argc,char** argv){
 if(argc<2){std::fprintf(stderr,"Usage: GlassgardenHostTests <plugin module>\n");return 2;}
#ifdef _WIN32
 HMODULE module=LoadLibraryA(argv[1]);check(module!=nullptr,"load Windows VST3 binary");using Entry=IPluginFactory*(PLUGIN_API*)();auto entry=reinterpret_cast<Entry>(GetProcAddress(module,"GetPluginFactory"));
#else
 void* module=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);if(!module)std::fprintf(stderr,"%s\n",dlerror());check(module!=nullptr,"load native validation module");using Entry=IPluginFactory*(PLUGIN_API*)();auto entry=reinterpret_cast<Entry>(dlsym(module,"GetPluginFactory"));
#endif
 check(entry!=nullptr,"VST3 factory exported");auto factory=entry();check(factory!=nullptr&&factory->countClasses()==2,"processor and controller factory classes");
 for(int i=0;i<2;i++){PClassInfo info{};check(factory->getClassInfo(i,&info)==kResultOk,"class metadata");check(std::strstr(info.name,"Glassgarden")!=nullptr,"separate Glassgarden product identity");}
 audioTests(factory);dspTests();
#ifdef _WIN32
 uiTests(factory);
#endif
 factory->release();std::printf("PASS: %d checks.\n",checks);return 0;
}
