static Values neutral(){auto v=defaults();v[Mix]=0;v[Output]=fromPlain(Output,0);return v;}
static void dspTests(){
    for(double sr:{22050.,44100.,48000.,96000.,192000.,384000.}){
        Engine e;auto v=neutral();e.prepare(sr,v);
        for(int n=0;n<4096;n++){double x=.3*std::sin(n*.13),l,r;e.process(x,-x*.7,l,r);check(std::abs(l-x)<1.e-12&&std::abs(r+x*.7)<1.e-12,"zero-mix dry signal has zero added samples");}
        v[Bypass]=1;v[Mix]=1;e.prepare(sr,v);
        for(int n=0;n<1024;n++){double x=1.4*std::sin(n*.13),l,r;e.process(x,x*.3,l,r);check(l==x&&r==x*.3,"bypass is sample-identical, including hot inputs");}
    }
    for (int semitones : {-12,0,12,24}) {
        auto v=neutral();v[Mix]=1;v[Pitch]=fromPlain(Pitch,semitones);v[GrainSize]=fromPlain(GrainSize,100);v[Density]=fromPlain(Density,40);v[Scatter]=v[Drift]=v[Feedback]=v[Predelay]=0;v[Tone]=1;v[LowCut]=0;
        Engine e;e.prepare(48000,v);std::complex<double> wanted{},wrong{};double energy=0;
        const double frequency=240*std::pow(2.,semitones/12.);
        for(int i=0;i<144000;++i){double l,r,x=.2*std::sin(2*pi*240*i/48000);e.process(x,x,l,r);if(i>=48000){wanted+=l*std::polar(1.,-2*pi*frequency*i/48000);wrong+=l*std::polar(1.,-2*pi*1733*i/48000);energy+=l*l;}}
        check(energy>1,"pitched grains produce wet audio");check(std::abs(wanted)>std::abs(wrong)*20,"grain playback follows the selected pitch ratio");
        std::printf("Pitch %+.0f st: %.0f Hz target, coherent magnitude %.2f\n",double(semitones),frequency,std::abs(wanted));
    }
    {
        Engine e;auto v=defaults();v[Mix]=1;v[Feedback]=v[Drift]=0;e.prepare(48000,v);
        for(int i=0;i<144000;i++){double l,r,x=.1*(std::sin(i*.028)+std::sin(i*.046));e.process(x,x,l,r);}
        const auto captured=e.capturedSamples();e.set(Freeze,1);double early=0,late=0;
        for(int i=0;i<384000;i++){double l,r;e.process(0,0,l,r);if(i>=48000&&i<144000)early+=l*l+r*r;if(i>=288000)late+=l*l+r*r;}
        check(e.capturedSamples()==captured,"freeze stops new input from overwriting captured audio");
        check(early>1.e-6&&late/early>.7&&late/early<1.3,"frozen grains sustain a captured phrase over eight seconds");
        e.clear();double peak=0;for(int i=0;i<10000;i++){double l,r;e.process(0,0,l,r);peak=std::max({peak,std::abs(l),std::abs(r)});}check(peak==0&&e.capturedSamples()==0,"clear invalidates all captured audio and active grains");
        std::printf("Freeze late/early energy: %.4f\n",late/early);
    }
    {
        Engine a,b;auto v=defaults();v[Mix]=1;v[Drift]=v[Feedback]=0;a.prepare(48000,v);v[Reverse]=1;b.prepare(48000,v);double difference=0,stereo=0;
        for(int i=0;i<96000;i++){double l,r,ll,rr,x=.15*std::sin(i*.0000008*i);a.process(x,x,l,r);b.process(x,x,ll,rr);if(i>48000){difference+=(l-ll)*(l-ll);stereo+=(l-r)*(l-r);}}
        check(difference>1,"reverse changes grain playback direction");check(stereo>.001,"scatter creates a stereo cloud from mono input");
    }
    {
        Engine e;auto v=defaults();v[Mix]=1;v[Sync]=1;v[Division]=1;v[Harmony]=0;v[GrainSize]=0;v[Scatter]=0;v[Drift]=0;e.prepare(48000,v);
        // A long division and short grains leave silent gaps. Faster host tempo
        // doubles grain launches, rather than changing the stored free Density.
        auto countBursts=[&](double tempo){e.clear();e.setTempo(tempo);int bursts=0;bool on=false;for(int i=0;i<96000;i++){double l,r;e.process(.1*std::sin(i*.05),.1*std::sin(i*.05),l,r);bool now=std::abs(l)>1.e-4;if(now&&!on)++bursts;on=now;}return bursts;};
        int slow=countBursts(60),fast=countBursts(120);check(fast>slow*1.6,"grain density follows host tempo and note division");
    }
    for(double sr:{44100.,48000.,96000.,192000.})for(const auto& preset:presets){
        Engine e;e.prepare(sr,preset.values);double energy=0;
        for(int i=0;i<int(sr*1.2);i++){double l,r,x=i<int(sr*.8)?.2*(std::sin(i*.041)+std::sin(i*.067)):0;e.process(x,x*.8,l,r);check(std::isfinite(l)&&std::isfinite(r)&&std::abs(l)<1.9&&std::abs(r)<1.9,"factory presets remain finite and bounded");energy+=l*l+r*r;}
        check(energy>1,"factory preset renders audio at every tested sample rate");
    }
    for(int texture=0;texture<3;texture++)for(int harmony=0;harmony<3;harmony++){
        Engine e;auto v=defaults();v[Texture]=texture/2.;v[Harmony]=harmony/2.;v[Feedback]=v[Density]=v[GrainSize]=v[Drift]=v[Scatter]=v[Output]=1;v[Mix]=1;e.prepare(48000,v);
        for(int i=0;i<96000;i++){if(i==40000)e.set(Reverse,1);if(i==50000)e.set(Freeze,1);if(i==70000)e.set(Freeze,0);double l,r,x=.6*(std::sin(i*.026)+std::sin(i*.071));e.process(x,x*.8,l,r);check(std::isfinite(l)&&std::isfinite(r)&&std::abs(l)<=1.25&&std::abs(r)<=1.25,"maximum controls and mode transitions remain bounded");}
        double l,r;e.process(std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),l,r);check(std::isfinite(l)&&std::isfinite(r),"nonfinite input cannot poison the engine");
    }
    {
        Engine e;e.prepare(48000,defaults());double a=0,b=0;auto start=std::chrono::steady_clock::now();
        for(int i=0;i<480000;i++)e.process(std::sin(i*.03)*.1,0,a,b);
        double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();std::printf("DSP: 10 seconds rendered in %.4f s (%.2f%% of one core on this test machine)\n",seconds,seconds*10);
    }
}
