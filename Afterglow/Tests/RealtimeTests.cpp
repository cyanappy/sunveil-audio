#include "../Source/Dsp.h"
#include <cstdlib>
#include <atomic>
#include <cstdio>
static std::atomic<bool> tracking{false};static std::atomic<size_t> allocations{0};
void* operator new(size_t n){if(tracking.load(std::memory_order_relaxed))allocations.fetch_add(1,std::memory_order_relaxed);if(void* p=std::malloc(n?n:1))return p;std::abort();}
void* operator new[](size_t n){return ::operator new(n);}
void operator delete(void* p) noexcept{std::free(p);}void operator delete[](void* p) noexcept{std::free(p);}
void operator delete(void* p,size_t) noexcept{std::free(p);}void operator delete[](void* p,size_t) noexcept{std::free(p);}
int main(){afterglow::Engine engine;auto values=afterglow::defaults();engine.prepare(48000,values);double l=0,r=0;tracking=true;
 for(int i=0;i<200000;i++){
  if(i%1024==0){engine.set(afterglow::Decay,(i%8192)/8192.);engine.set(afterglow::Motion,(i%16384)/16384.);engine.set(afterglow::Time,.2+(i%32768)/65536.);}
  if(i==100000)engine.clear();engine.process(std::sin(i*.017)*.2,std::sin(i*.021)*.1,l,r);
 }
 tracking=false;if(allocations!=0||!std::isfinite(l)||!std::isfinite(r)){std::printf("FAIL: audio-thread allocations=%zu\n",allocations.load());return 1;}
 std::printf("PASS: 200000 stereo frames, automation and clear-tails: zero heap allocations.\n");return 0;
}
