// Renders the same artwork and live parameter geometry used by the HWND editor.
// This is a layout validation image, not a screenshot from a Windows DAW.
#include "../Source/Artwork.h"
#include "EditorCases.h"
#include <filesystem>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <locale>
#include <codecvt>
#include <cstdlib>
using namespace glassgarden;using namespace glassgarden::artwork;
static std::string base64(const std::vector<unsigned char>& bytes){
 const char* t="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";std::string out;out.reserve((bytes.size()+2)/3*4);
 for(size_t i=0;i<bytes.size();i+=3){unsigned v=unsigned(bytes[i])<<16;if(i+1<bytes.size())v|=unsigned(bytes[i+1])<<8;if(i+2<bytes.size())v|=bytes[i+2];out+=t[(v>>18)&63];out+=t[(v>>12)&63];out+=i+1<bytes.size()?t[(v>>6)&63]:'=';out+=i+2<bytes.size()?t[v&63]:'=';}return out;
}
struct SVG{
 std::ostream& out;int clips=0;
 static std::string color(uint32_t c){if(c==uint32_t(-1))return "none";char s[16];std::snprintf(s,sizeof(s),"#%02x%02x%02x",c&255,(c>>8)&255,(c>>16)&255);return s;}
 static std::string escaped(const std::string& s){std::string r;for(char c:s){if(c=='&')r+="&amp;";else if(c=='<')r+="&lt;";else if(c=='>')r+="&gt;";else r+=c;}return r;}
 void background(){out<<"<use xlink:href='#plate'/>\n";}
 void photoKnob(double x,double y,double r,double a,double sx,double sy,double sr){
  auto crop=[&](){out<<"<svg x='"<<x-r<<"' y='"<<y-r<<"' width='"<<r*2<<"' height='"<<r*2<<"' viewBox='"<<sx-sr<<" "<<sy-sr<<" "<<sr*2<<" "<<sr*2<<"'><use xlink:href='#plate'/></svg>";};
  auto outer=++clips;out<<"<defs><clipPath id='c"<<outer<<"'><circle cx='"<<x<<"' cy='"<<y<<"' r='"<<r<<"'/></clipPath></defs><g clip-path='url(#c"<<outer<<")'>";crop();
  auto inner=++clips;double fy=y-7*r/sr,fr=(sr-8)*r/sr;
  out<<"<defs><clipPath id='c"<<inner<<"'><circle cx='"<<x<<"' cy='"<<fy<<"' r='"<<fr<<"'/></clipPath></defs><g clip-path='url(#c"<<inner<<")'><g transform='rotate("<<a<<" "<<x<<" "<<fy<<")'>";crop();out<<"</g></g></g>\n";
 }
 void box(Box b,uint32_t fill,int radius=0,uint32_t border=uint32_t(-1),int width=1){out<<"<rect x='"<<b.x<<"' y='"<<b.y<<"' width='"<<b.w<<"' height='"<<b.h<<"' rx='"<<radius<<"' fill='"<<color(fill)<<"' stroke='"<<color(border)<<"' stroke-width='"<<width<<"'/>\n";}
 void circle(double x,double y,double r,uint32_t fill,uint32_t border=uint32_t(-1),double width=1){out<<"<circle cx='"<<x<<"' cy='"<<y<<"' r='"<<r<<"' fill='"<<color(fill)<<"' stroke='"<<color(border)<<"' stroke-width='"<<width<<"'/>\n";}
 void stroke(double x,double y,double xx,double yy,uint32_t c,double width=1){out<<"<line x1='"<<x<<"' y1='"<<y<<"' x2='"<<xx<<"' y2='"<<yy<<"' stroke='"<<color(c)<<"' stroke-width='"<<width<<"' stroke-linecap='round'/>\n";}
 void ascii(const char* text,Box b,int size,uint32_t c,int weight=400,unsigned align=0,bool=false,bool serif=false){out<<"<text x='"<<(align==1?b.x+b.w*.5:align==2?b.x+b.w:b.x)<<"' y='"<<b.y+b.h*.5<<"' dominant-baseline='central' text-anchor='"<<(align==1?"middle":align==2?"end":"start")<<"' font-family='"<<(serif?"DejaVu Serif":"DejaVu Sans")<<"' font-size='"<<size<<"' font-weight='"<<weight<<"' fill='"<<color(c)<<"'>"<<escaped(text)<<"</text>\n";}
 void text(const wchar_t* t,Box b,int size,uint32_t c,int weight=400,unsigned align=0,bool spaced=false,bool serif=false){std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;ascii(convert.to_bytes(t).c_str(),b,size,c,weight,align,spaced,serif);}
};
static void header(std::ostream& out,const std::string& image,int width=W,int height=H) {
 out<<"<svg xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink' width='"<<width<<"' height='"<<height<<"' viewBox='0 0 "<<W<<" "<<H<<"'><defs><image id='plate' width='"<<W<<"' height='"<<H<<"' xlink:href='"<<image<<"'/></defs>\n";
}
static int regression(const char* destination) {
 const std::filesystem::path directory(destination);std::filesystem::create_directories(directory);
 std::ofstream manifest(directory/"cases.json");manifest<<"[";bool first=true;
 for(int width:{600,1200})for(const auto& test:glassgarden::ui::editorCases()) {
  const int height=width*H/W;const auto name=test.name+"-"+std::to_string(width);
  if(!first)manifest<<",";first=false;manifest<<"\""<<name<<"\"";
  for(bool partial:{false,true}) {
   std::ofstream out(directory/(name+(partial?"-partial.svg":"-full.svg")));
   out<<std::setprecision(17);
   header(out,"__FACEPLATE__",width,height);SVG canvas{out};
   if(partial) {
    test.before.draw(canvas);
    out<<"<defs><clipPath id='damage'>";
    const auto damage=glassgarden::ui::changes(test.before,test.after);
    for(unsigned i=0;i<damage.count;++i) {
     const auto b=damage.boxes[i];
     const int left=std::max(0,int(std::floor(b.x*double(width)/W))-2),top=std::max(0,int(std::floor(b.y*double(height)/H))-2);
     const int right=std::min(width,int(std::ceil((b.x+b.w)*double(width)/W))+2),bottom=std::min(height,int(std::ceil((b.y+b.h)*double(height)/H))+2);
     out<<"<rect x='"<<left*double(W)/width<<"' y='"<<top*double(H)/height<<"' width='"<<(right-left)*double(W)/width<<"' height='"<<(bottom-top)*double(H)/height<<"'/>";
    }
    out<<"</clipPath></defs><g clip-path='url(#damage)'>";
   }
   test.after.draw(canvas);
   if(partial)out<<"</g>";
   out<<"</svg>\n";if(!out)return 1;
  }
 }
 manifest<<"]\n";return manifest?0:1;
}
int main(int argc,char** argv){
 if(argc>3&&std::string(argv[3])=="--regression")return regression(argv[2]);
 if(argc<3)return 2;std::ifstream png(argv[1],std::ios::binary);std::vector<unsigned char> data((std::istreambuf_iterator<char>(png)),std::istreambuf_iterator<char>());if(data.empty())return 3;
 std::ofstream file(argv[2]);header(file,"data:image/png;base64,"+base64(data));
 SVG canvas{file};Pedal art;if(argc>3){int i=std::atoi(argv[3]);if(i>=0&&i<presetCount){art.values=presets[i].values;art.presetIndex=i;}}
 if(argc>4)art.advanced=std::atoi(argv[4])!=0;if(argc>5)art.values[Freeze]=std::atoi(argv[5])?1:0;if(argc>6)art.values[Bypass]=std::atoi(argv[6])?1:0;
 art.draw(canvas);file<<"</svg>\n";return file?0:1;
}
