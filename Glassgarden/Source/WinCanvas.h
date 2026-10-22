#pragma once
#include <objidl.h>
#include <gdiplus.h>
#include <memory>
#include <string>
#include <vector>

namespace glassgarden {
// These surfaces live for the window's lifetime and resize only with the window.
// Paints never allocate a full-resolution bitmap or rescale the entire pedal.
class PaintSurface {
  public:
    HDC dc = nullptr;
    int width = 0, height = 0;
    ~PaintSurface() { reset(); }
    PaintSurface() = default;
    PaintSurface(const PaintSurface&) = delete;
    PaintSurface& operator=(const PaintSurface&) = delete;
    void reset() {
        if (dc && previous) SelectObject(dc, previous);
        if (bitmap) DeleteObject(bitmap);
        if (dc) DeleteDC(dc);
        dc = nullptr; bitmap = nullptr; previous = nullptr;
        width = height = 0;
    }
    bool resize(HDC target, int w, int h) {
        if (dc && w == width && h == height) return true;
        reset();
        if (w <= 0 || h <= 0) return false;
        dc = CreateCompatibleDC(target);
        if (!dc) return false;
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = w; info.bmiHeader.biHeight = -h;
        info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        void* bits = nullptr;
        bitmap = CreateDIBSection(target, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
        if (!bitmap) { reset(); return false; }
        previous = SelectObject(dc, bitmap);
        width = w; height = h;
        return true;
    }
  private:
    HBITMAP bitmap = nullptr;
    HGDIOBJ previous = nullptr;
};

class PaintSource {
  public:
    std::unique_ptr<Gdiplus::Bitmap> image;
    PaintSource() {
        Gdiplus::GdiplusStartupInput input;
        if (Gdiplus::GdiplusStartup(&token, &input, nullptr) != Gdiplus::Ok) return;
        auto resource = FindResourceW(glassgardenModule, MAKEINTRESOURCEW(101), RT_RCDATA);
        if (!resource) return;
        const DWORD size = SizeofResource(glassgardenModule, resource);
        auto loaded = LoadResource(glassgardenModule, resource);
        auto resourceBytes = loaded ? LockResource(loaded) : nullptr;
        if (!resourceBytes || !size) return;
        auto memory = GlobalAlloc(GMEM_MOVEABLE, size);
        if (!memory) return;
        auto bytes = GlobalLock(memory);
        if (!bytes) { GlobalFree(memory); return; }
        std::memcpy(bytes, resourceBytes, size); GlobalUnlock(memory);
        IStream* stream = nullptr;
        if (CreateStreamOnHGlobal(memory, TRUE, &stream) != S_OK) { GlobalFree(memory); return; }
        // Force PNG decoding once, then use small, already-decoded knob sprites.
        {
            std::unique_ptr<Gdiplus::Image> encoded(Gdiplus::Image::FromStream(stream, FALSE));
            if (encoded && encoded->GetLastStatus() == Gdiplus::Ok) {
                auto decoded = std::make_unique<Gdiplus::Bitmap>(artwork::W, artwork::H, PixelFormat32bppPARGB);
                Gdiplus::Status result;
                {
                    Gdiplus::Graphics g(decoded.get());
                    result = g.DrawImage(encoded.get(), 0, 0, artwork::W, artwork::H);
                }
                if (decoded->GetLastStatus() == Gdiplus::Ok && result == Gdiplus::Ok)
                    image = std::move(decoded);
            }
        }
        stream->Release();
        if (image) {
            for (unsigned i = 0; i < knobImages.size(); ++i) {
                const auto k = artwork::knobs[i];
                knobImages[i].reset(image->Clone(k.x-k.r, k.y-k.r, k.r*2, k.r*2, PixelFormat32bppPARGB));
            }
        }
        fonts.reserve(16);
    }
    ~PaintSource() {
        fonts.clear();
        for (auto& knob : knobImages) knob.reset();
        image.reset();
        if (token) Gdiplus::GdiplusShutdown(token);
    }
    PaintSource(const PaintSource&) = delete;
    PaintSource& operator=(const PaintSource&) = delete;
    Gdiplus::Bitmap* knob(double x, double y) const {
        for (unsigned i = 0; i < knobImages.size(); ++i)
            if (x == artwork::knobs[i].x && y == artwork::knobs[i].y) return knobImages[i].get();
        return nullptr;
    }
    Gdiplus::Font* font(int size, bool bold, bool serif) {
        for (auto& f : fonts)
            if (f.size == size && f.bold == bold && f.serif == serif) return f.font.get();
        Gdiplus::FontFamily family(serif ? L"Georgia" : L"Segoe UI");
        fonts.push_back({size, bold, serif, std::make_unique<Gdiplus::Font>(&family, float(size),
            bold ? Gdiplus::FontStyleBold : Gdiplus::FontStyleRegular, Gdiplus::UnitPixel)});
        return fonts.back().font.get();
    }
  private:
    struct FontEntry { int size; bool bold, serif; std::unique_ptr<Gdiplus::Font> font; };
    ULONG_PTR token = 0;
    std::array<std::unique_ptr<Gdiplus::Bitmap>, 6> knobImages;
    std::vector<FontEntry> fonts;
};

class Canvas {
    Gdiplus::Graphics graphics;
    PaintSource& source;
    Gdiplus::RectF clip;
    bool drawBackground;
    static Gdiplus::Color color(uint32_t c) {
        return Gdiplus::Color(255, GetRValue(c), GetGValue(c), GetBValue(c));
    }
    bool visible(double x, double y, double w, double h) const {
        return x+w+3 >= clip.X && y+h+3 >= clip.Y && x-3 <= clip.GetRight() && y-3 <= clip.GetBottom();
    }
  public:
    Canvas(HDC dc, int width, int height, PaintSource& assets, const RECT& dirty, bool background = false)
      : graphics(dc), source(assets),
        clip(float(dirty.left)*artwork::W/width, float(dirty.top)*artwork::H/height,
             float(dirty.right-dirty.left)*artwork::W/width, float(dirty.bottom-dirty.top)*artwork::H/height),
        drawBackground(background) {
        graphics.SetPageUnit(Gdiplus::UnitPixel);
        graphics.ScaleTransform(float(width)/artwork::W, float(height)/artwork::H);
        graphics.SetClip(clip);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
        graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
    }
    void background() {
        if (!drawBackground) return;
        if (source.image) graphics.DrawImage(source.image.get(), 0, 0, artwork::W, artwork::H);
        else box({0,0,artwork::W,artwork::H}, artwork::bodyColor);
    }
    void photoKnob(double x, double y, double radius, double angle, double sx, double sy, double sr) {
        if (!visible(x-radius, y-radius, radius*2, radius*2) || !source.image) return;
        auto state = graphics.Save();
        Gdiplus::GraphicsPath outer;
        outer.AddEllipse(float(x-radius), float(y-radius), float(radius*2), float(radius*2));
        graphics.SetClip(&outer, Gdiplus::CombineModeIntersect);
        const Gdiplus::RectF destination(float(x-radius), float(y-radius), float(radius*2), float(radius*2));
        auto sprite = source.knob(sx, sy);
        auto draw = [&] {
            if (sprite) graphics.DrawImage(sprite, destination, 0.f, 0.f, float(sr*2), float(sr*2), Gdiplus::UnitPixel);
            else graphics.DrawImage(source.image.get(), destination, float(sx-sr), float(sy-sr), float(sr*2), float(sr*2), Gdiplus::UnitPixel);
        };
        draw();
        const float faceY = float(y-7*radius/sr), faceR = float((sr-8)*radius/sr);
        Gdiplus::GraphicsPath face;
        face.AddEllipse(float(x)-faceR, faceY-faceR, faceR*2, faceR*2);
        graphics.SetClip(&face, Gdiplus::CombineModeIntersect);
        graphics.TranslateTransform(float(x), faceY);
        graphics.RotateTransform(float(angle));
        graphics.TranslateTransform(float(-x), -faceY);
        draw();
        graphics.Restore(state);
    }
    void box(artwork::Box b, uint32_t fill, int radius=0, uint32_t border=uint32_t(-1), int width=1) {
        if (!visible(b.x-width, b.y-width, b.w+width*2, b.h+width*2)) return;
        Gdiplus::GraphicsPath path;
        const float x=float(b.x), y=float(b.y), w=float(b.w), h=float(b.h), d=float(std::min({radius*2,b.w,b.h}));
        if (d>0) {
            path.AddArc(x,y,d,d,180.f,90.f); path.AddArc(x+w-d,y,d,d,270.f,90.f);
            path.AddArc(x+w-d,y+h-d,d,d,0.f,90.f); path.AddArc(x,y+h-d,d,d,90.f,90.f); path.CloseFigure();
        } else path.AddRectangle(Gdiplus::RectF(x,y,w,h));
        if (fill!=uint32_t(-1)) { Gdiplus::SolidBrush brush(color(fill)); graphics.FillPath(&brush,&path); }
        if (border!=uint32_t(-1)) { Gdiplus::Pen pen(color(border),float(width)); graphics.DrawPath(&pen,&path); }
    }
    void circle(double x, double y, double r, uint32_t fill, uint32_t border=uint32_t(-1), double width=1) {
        if (!visible(x-r-width,y-r-width,(r+width)*2,(r+width)*2)) return;
        const Gdiplus::RectF rect(float(x-r),float(y-r),float(r*2),float(r*2));
        if (fill!=uint32_t(-1)) { Gdiplus::SolidBrush brush(color(fill)); graphics.FillEllipse(&brush,rect); }
        if (border!=uint32_t(-1)) { Gdiplus::Pen pen(color(border),float(width)); graphics.DrawEllipse(&pen,rect); }
    }
    void stroke(double x, double y, double xx, double yy, uint32_t c, double width=1) {
        if (!visible(std::min(x,xx)-width,std::min(y,yy)-width,std::abs(xx-x)+width*2,std::abs(yy-y)+width*2)) return;
        Gdiplus::Pen pen(color(c),float(width));
        pen.SetStartCap(Gdiplus::LineCapRound); pen.SetEndCap(Gdiplus::LineCapRound);
        graphics.DrawLine(&pen,float(x),float(y),float(xx),float(yy));
    }
    void text(const wchar_t* text, artwork::Box b, int size, uint32_t c, int weight=400, unsigned align=0, bool=false, bool serif=false) {
        if (!visible(b.x,b.y,b.w,b.h)) return;
        Gdiplus::SolidBrush brush(color(c));
        Gdiplus::StringFormat format;
        format.SetAlignment(align==1 ? Gdiplus::StringAlignmentCenter : align==2 ? Gdiplus::StringAlignmentFar : Gdiplus::StringAlignmentNear);
        format.SetLineAlignment(Gdiplus::StringAlignmentCenter);
        format.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
        format.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);
        graphics.DrawString(text,-1,source.font(size,weight>=600,serif),Gdiplus::RectF(float(b.x),float(b.y),float(b.w),float(b.h)),&format,&brush);
    }
    void ascii(const char* text, artwork::Box b, int size, uint32_t c, int weight=400, unsigned align=0, bool spaced=false, bool serif=false) {
        if (!visible(b.x,b.y,b.w,b.h)) return;
        // UI labels are short and ASCII: avoid a heap allocation in the paint path.
        wchar_t wide[256]{};
        for (unsigned i=0; i<255 && text[i]; ++i) wide[i]=static_cast<unsigned char>(text[i]);
        this->text(wide,b,size,c,weight,align,spaced,serif);
    }
};
} // namespace glassgarden
