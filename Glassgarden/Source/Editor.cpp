#include "Plugin.h"
#include "Artwork.h"
#include "EditorModel.h"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>
#include "pluginterfaces/base/keycodes.h"
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"
#include <algorithm>
#include <vector>
#include <string>
#include <cmath>
#include <chrono>
extern HINSTANCE glassgardenModule;
#include "WinCanvas.h"

namespace glassgarden {
namespace {
using namespace artwork;
constexpr wchar_t className[] = L"SunveilAudio.Glassgarden.Editor.1.0.0";
std::atomic<int> liveWindows{0};
} // namespace

class Editor final : public IPlugView, public IPlugViewContentScaleSupport {
  public:
    explicit Editor(Controller *c) : controller(c) { controller->addRef(); }
    ~Editor() {
        removed();
        if (frame)
            frame->release();
        controller->release();
    }
    tresult PLUGIN_API queryInterface(const TUID id, void **out) override {
        if (!out)
            return kInvalidArgument;
        *out = nullptr;
        if (sameID(id, FUnknown_iid) || sameID(id, IPlugView_iid))
            *out = static_cast<IPlugView *>(this);
        else if (sameID(id, IPlugViewContentScaleSupport_iid))
            *out = static_cast<IPlugViewContentScaleSupport *>(this);
        if (!*out)
            return kNoInterface;
        addRef();
        return kResultOk;
    }
    uint32 PLUGIN_API addRef() override { return ++refs; }
    uint32 PLUGIN_API release() override {
        auto n = --refs;
        if (!n)
            delete this;
        return n;
    }
    tresult PLUGIN_API isPlatformTypeSupported(FIDString p) override {
        return p && !std::strcmp(p, kPlatformTypeHWND) ? kResultTrue : kResultFalse;
    }
    tresult PLUGIN_API attached(void *parent, FIDString type) override {
        if (!parent || window || isPlatformTypeSupported(type) != kResultTrue)
            return kResultFalse;
        WNDCLASSW wc{};
        wc.lpfnWndProc = windowProc;
        wc.hInstance = glassgardenModule;
        wc.lpszClassName = className;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.style = CS_DBLCLKS;
        if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            return kResultFalse;
        window = CreateWindowExW(0, className, L"Glassgarden",
                                 WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS, 0, 0,
                                 rect.getWidth(), rect.getHeight(), static_cast<HWND>(parent),
                                 nullptr, glassgardenModule, this);
        if (!window)
            return kResultFalse;
        ++liveWindows;
        SetTimer(window, 1, 16, nullptr);
        return kResultOk;
    }
    tresult PLUGIN_API removed() override {
        if (window) {
            finishDrag();
            auto w = window;
            window = nullptr;
            KillTimer(w, 1);
            DestroyWindow(w);
            backdrop.reset();
            buffer.reset();
            havePainted = false;
            if (--liveWindows == 0)
                UnregisterClassW(className, glassgardenModule);
        }
        return kResultOk;
    }
    tresult PLUGIN_API onWheel(float d) override {
        if (focused >= 0) {
            adjust(focused, d * .02);
            return kResultTrue;
        }
        return kResultFalse;
    }
    tresult PLUGIN_API onKeyDown(char16 key, int16 code, int16 modifiers) override {
        int nativeKey = key;
        switch (code) {
        case KEY_TAB:
            nativeKey = VK_TAB;
            break;
        case KEY_SPACE:
            nativeKey = VK_SPACE;
            break;
        case KEY_HOME:
            nativeKey = VK_HOME;
            break;
        case KEY_LEFT:
            nativeKey = VK_LEFT;
            break;
        case KEY_RIGHT:
            nativeKey = VK_RIGHT;
            break;
        case KEY_UP:
            nativeKey = VK_UP;
            break;
        case KEY_DOWN:
            nativeKey = VK_DOWN;
            break;
        default:
            if (!nativeKey && code >= VKEY_FIRST_ASCII)
                nativeKey = VirtualKeyCodeToChar(static_cast<uint8>(code));
            break;
        }
        if (nativeKey == 'p')
            nativeKey = 'P';
        return handleKey(nativeKey, modifiers) ? kResultTrue : kResultFalse;
    }
    tresult PLUGIN_API onKeyUp(char16, int16, int16) override { return kResultFalse; }
    tresult PLUGIN_API getSize(ViewRect *r) override {
        if (!r)
            return kInvalidArgument;
        *r = rect;
        return kResultOk;
    }
    tresult PLUGIN_API onSize(ViewRect *r) override {
        if (!r || r->getWidth() <= 0 || r->getHeight() <= 0)
            return kInvalidArgument;
        rect = *r;
        if (window) {
            havePainted = false;
            SetWindowPos(window, nullptr, 0, 0, rect.getWidth(), rect.getHeight(),
                         SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            InvalidateRect(window, nullptr, FALSE);
        }
        return kResultOk;
    }
    tresult PLUGIN_API onFocus(TBool v) override {
        if (v && window)
            SetFocus(window);
        return kResultOk;
    }
    tresult PLUGIN_API setFrame(IPlugFrame *p) override {
        if (p)
            p->addRef();
        if (frame)
            frame->release();
        frame = p;
        return kResultOk;
    }
    tresult PLUGIN_API canResize() override { return kResultTrue; }
    tresult PLUGIN_API checkSizeConstraint(ViewRect *r) override {
        if (!r)
            return kInvalidArgument;
        const int width = std::clamp(r->getWidth(), 400, 1200);
        r->right = r->left + width;
        r->bottom = r->top + int(std::lround(width * double(H) / W));
        return kResultTrue;
    }
    tresult PLUGIN_API setContentScaleFactor(ScaleFactor factor) override {
        if (!std::isfinite(factor) || factor <= 0)
            return kInvalidArgument;
        factor = std::clamp(factor, .75f, 3.f);
        ViewRect wanted(0, 0, int(std::lround(600 * factor)), int(std::lround(900 * factor)));
        rect = wanted;
        if (frame)
            frame->resizeView(this, &wanted);
        if (window)
            onSize(&wanted);
        return kResultTrue;
    }

  private:
    static LRESULT CALLBACK windowProc(HWND w, UINT message, WPARAM wp, LPARAM lp) {
        Editor *e = reinterpret_cast<Editor *>(GetWindowLongPtrW(w, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            e = static_cast<Editor *>(reinterpret_cast<CREATESTRUCTW *>(lp)->lpCreateParams);
            SetWindowLongPtrW(w, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(e));
        }
        return e ? e->message(w, message, wp, lp) : DefWindowProcW(w, message, wp, lp);
    }
    int knobAt(int x,int y) const { return artwork::knobAt(x,y,advanced); }
    void cycle(unsigned id) {
        int steps=definitions[id].steps;
        if(steps<=0) return;
        controller->setFromUI(id,((int(std::round(controller->getParamNormalized(id)*steps))+1)%(steps+1))/double(steps));
    }
    void clearCloud() {
        controller->clearTails();
        clearUntil=GetTickCount64()+160; tapUntil=clearUntil;
        redraw();
    }
    artwork::Pedal snapshot() const {
        artwork::Pedal art;
        for (unsigned i=0; i<Count; ++i) art.values[i]=controller->getParamNormalized(i);
        art.presetIndex=controller->currentPreset();
        art.focused=focused;
        art.hasFocus=window && GetFocus()==window;
        art.advanced=advanced;
        const auto now=GetTickCount64();
        art.tapActive=now<tapUntil;
        art.clearActive=now<clearUntil;
        return art;
    }
    void invalidateChanges(const artwork::Pedal& art) {
        if (!window) return;
        if (!havePainted) { InvalidateRect(window,nullptr,FALSE); return; }
        const auto damage=ui::changes(painted,art);
        const double sx=double(rect.getWidth())/W, sy=double(rect.getHeight())/H;
        for (unsigned i=0; i<damage.count; ++i) {
            const auto b=damage.boxes[i];
            RECT dirty{std::max(0,int(std::floor(b.x*sx))-2),std::max(0,int(std::floor(b.y*sy))-2),
                       std::min(rect.getWidth(),int(std::ceil((b.x+b.w)*sx))+2),
                       std::min(rect.getHeight(),int(std::ceil((b.y+b.h)*sy))+2)};
            if (dirty.right>dirty.left && dirty.bottom>dirty.top) InvalidateRect(window,&dirty,FALSE);
        }
    }
    void redraw(bool immediate=true) {
        if (!window) return;
        const auto now=std::chrono::steady_clock::now();
        // Values reach the host on every mouse event. Coalesce visual work to
        // about 120 fps; clicks/releases bypass the limit. UpdateWindow avoids
        // leaving WM_PAINT behind a continuous queue of mouse messages.
        if (immediate || now-lastPresent>=std::chrono::milliseconds(8)) {
            invalidateChanges(snapshot());
            UpdateWindow(window);
        }
    }
    void finishDrag(bool releaseCapture=true) {
        if (drag<0) return;
        const int id=drag;
        drag=-1;
        controller->end(id);
        if (releaseCapture && GetCapture()==window) ReleaseCapture();
    }
    void adjust(int id, double amount) {
        if (id < 0 || id >= Count || amount == 0)
            return;
        
        const double step = definitions[id].steps ? 1.0 / definitions[id].steps : amount;
        controller->setFromUI(id, controller->getParamNormalized(id) +
                                      (amount < 0 ? -std::abs(step) : std::abs(step)));
        redraw();
    }
    bool handleKey(int key,int modifiers=-1) {
        const bool shift=modifiers<0?(GetKeyState(VK_SHIFT)&0x8000)!=0:(modifiers&kShiftKey)!=0;
        if(key>='a'&&key<='z')key-=32;
        if(key==VK_TAB){focused=focused<0?(shift?Count-1:0):(focused+(shift?-1:1)+Count)%Count;advanced=focused>=Tone;redraw();return true;}
        if(key==VK_SPACE||key=='B'){cycle(Bypass);redraw();return true;}
        if(key=='F'){cycle(Freeze);redraw();return true;}
        if(key=='R'){cycle(Reverse);redraw();return true;}
        if(key=='A'){advanced=!advanced;redraw();return true;}
        if(key=='C'){clearCloud();return true;}
        if(key=='P'){showPresets();return true;}
        if(focused>=0&&(key==VK_UP||key==VK_RIGHT||key==VK_DOWN||key==VK_LEFT)){
            adjust(focused,((key==VK_DOWN||key==VK_LEFT)?-1:1)*(shift?.001:.01));return true;
        }
        if(focused>=0&&key==VK_HOME){controller->setFromUI(focused,definitions[focused].initial);redraw();return true;}
        return false;
    }
    void showPresets() {
        if (!window)
            return;
        HMENU menu = CreatePopupMenu();
        const int current = controller->currentPreset();
        for (int i = 0; i < presetCount; ++i) {
            wchar_t name[128]{};
            const auto t = presets[i].name;
            for (int j = 0; t[j]; ++j)
                name[j] = t[j];
            AppendMenuW(menu, MF_STRING | (current == i ? MF_CHECKED : 0), i + 1, name);
        }
        POINT pt{int(presetBox.x * rect.getWidth() / W),
                 int((presetBox.y + presetBox.h) * rect.getHeight() / H)};
        ClientToScreen(window, &pt);
        const int chosen = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTALIGN, pt.x,
                                          pt.y, 0, window, nullptr);
        DestroyMenu(menu);
        if (chosen > 0)
            controller->loadPreset(chosen - 1);
        redraw();
    }
    LRESULT message(HWND w, UINT m, WPARAM wp, LPARAM lp) {
        const int x = int(GET_X_LPARAM(lp) * double(W) / std::max(1, rect.getWidth()));
        const int y = int(GET_Y_LPARAM(lp) * double(H) / std::max(1, rect.getHeight()));
        switch (m) {
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT: {
            const auto art=snapshot();
            // Include host changes even if this paint was triggered by uncovering
            // the window before the automation timer got a chance to run.
            invalidateChanges(art);
            PAINTSTRUCT ps;
            HDC target = BeginPaint(w, &ps);
            paint(target,ps.rcPaint,art);
            EndPaint(w, &ps);
            return 0;
        }
        case WM_TIMER:
            // There are no level meters on the pedal; don't make synchronous
            // connection-point requests to the processor just to refresh knobs.
            if (wp==1) redraw(false);
            return 0;
        case WM_SIZE:
            if (LOWORD(lp)>0 && HIWORD(lp)>0) {
                rect.right=rect.left+LOWORD(lp); rect.bottom=rect.top+HIWORD(lp);
                havePainted=false; InvalidateRect(w,nullptr,FALSE);
            }
            return 0;
        case WM_RBUTTONUP:advanced=!advanced;redraw();return 0;
        case WM_GETDLGCODE:
            return DLGC_WANTARROWS | DLGC_WANTTAB | DLGC_WANTCHARS;
        case WM_KEYDOWN:
            if (handleKey(int(wp)))
                return 0;
            break;
        case WM_LBUTTONDBLCLK: {
            const int id=knobAt(x,y);
            if (id>=0) {
                finishDrag();
                
                controller->setFromUI(id,definitions[id].initial);
                focused=id;
                redraw();
                return 0;
            }
            // Windows sends DBLCLK instead of the second DOWN. Treat it as a
            // normal press for switches/buttons, so rapid clicks aren't lost.
            [[fallthrough]];
        }
        case WM_LBUTTONDOWN: {
            SetFocus(w);const int id=knobAt(x,y);
            if(id>=0){finishDrag();focused=drag=id;knobDrag.begin(controller->getParamNormalized(id),GET_Y_LPARAM(lp),rect.getWidth());controller->begin(id);SetCapture(w);redraw();return 0;}
            if(detailsBox.hit(x,y)||(advanced&&closeBox.hit(x,y))){advanced=!advanced;redraw();return 0;}
            if(clearQuickBox.hit(x,y)){clearCloud();return 0;}
            if(presetBox.hit(x,y)){showPresets();return 0;}
            if(advanced){
                if(freezeBox.hit(x,y)){focused=Freeze;cycle(Freeze);}
                else if(syncBox.hit(x,y)){focused=Sync;cycle(Sync);}
                else if(divisionBox.hit(x,y)){focused=Division;cycle(Division);}
                else if(reverseBox.hit(x,y)){focused=Reverse;cycle(Reverse);}
                else if(bypassBox.hit(x,y)){focused=Bypass;cycle(Bypass);}
                else if(clearBox.hit(x,y)){controller->clearTails();clearUntil=GetTickCount64()+120;}
            }else if(textureBox.hit(x,y)){focused=Texture;cycle(Texture);}
            else if(harmonyBox.hit(x,y)){focused=Harmony;cycle(Harmony);}
            else {const int id=switchAt(x,y);if(id>=0){focused=id;cycle(id);}}
            redraw();return 0;
        }
        case WM_LBUTTONUP:
            if (drag >= 0) {
                finishDrag();
                redraw();
            }
            return 0;
        case WM_CAPTURECHANGED:
            finishDrag(false);
            redraw();
            return 0;
        case WM_CANCELMODE:
            finishDrag();
            redraw();
            return 0;
        case WM_MOUSEMOVE: {
            if (drag >= 0) {
                controller->edit(drag,knobDrag.move(GET_Y_LPARAM(lp),(GetKeyState(VK_SHIFT)&0x8000)!=0));
                redraw(false);
            }
            const int id = knobAt(x, y);
            SetCursor(LoadCursor(nullptr,id>=0||presetBox.hit(x,y)||detailsBox.hit(x,y)||clearQuickBox.hit(x,y)||(!advanced&&(textureBox.hit(x,y)||harmonyBox.hit(x,y)||switchAt(x,y)>=0))?IDC_HAND:IDC_ARROW));
            return 0;
        }
        case WM_MOUSEWHEEL: {
            POINT point{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
            ScreenToClient(w, &point);
            const int id = knobAt(int(point.x * double(W) / rect.getWidth()),
                                  int(point.y * double(H) / rect.getHeight()));
            if (id >= 0) {
                focused = id;
                adjust(id, GET_WHEEL_DELTA_WPARAM(wp) / 120.0 *
                               ((GetKeyState(VK_SHIFT) & 0x8000) ? .002 : .02));
                return 0;
            }
            break;
        }
        case WM_KILLFOCUS:
            finishDrag();
            redraw();
            return 0;
        case WM_SETFOCUS:
            redraw();
            return 0;
        }
        return DefWindowProcW(w, m, wp, lp);
    }
    void paint(HDC target, RECT dirty, const artwork::Pedal& art) {
        const int width=rect.getWidth(),height=rect.getHeight();
        if (width<=0 || height<=0 || dirty.right<=dirty.left || dirty.bottom<=dirty.top) return;
        const bool rebuild=backdrop.width!=width || backdrop.height!=height;
        if (!backdrop.resize(target,width,height) || !buffer.resize(target,width,height)) {
            FillRect(target,&dirty,static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
            backdrop.reset(); buffer.reset();
            havePainted=false;
            return;
        }
        if (rebuild) {
            RECT full{0,0,width,height};
            Canvas c(backdrop.dc,width,height,source,full,true);
            c.background();
        }
        const int dx=dirty.left,dy=dirty.top,dw=dirty.right-dx,dh=dirty.bottom-dy;
        BitBlt(buffer.dc,dx,dy,dw,dh,backdrop.dc,dx,dy,SRCCOPY);
        {
            Canvas c(buffer.dc,width,height,source,dirty);
            art.draw(c);
        }
        BitBlt(target,dx,dy,dw,dh,buffer.dc,dx,dy,SRCCOPY);
        painted=art;
        havePainted=true;
        lastPresent=std::chrono::steady_clock::now();
    }
    std::atomic<uint32> refs{1};
    Controller *controller;
    HWND window = nullptr;
    IPlugFrame *frame = nullptr;
    PaintSource source;
    PaintSurface backdrop,buffer;
    bool advanced=false;
    bool havePainted=false;
    ULONGLONG tapUntil=0,clearUntil=0;
    artwork::Pedal painted;
    ui::KnobDrag knobDrag;
    std::chrono::steady_clock::time_point lastPresent{};
    ViewRect rect{0, 0, 600, 900};
    int focused = -1, drag = -1;
};
IPlugView *makeEditor(Controller *c) {
    return new Editor(c);
}
} // namespace glassgarden
#else
namespace glassgarden {
IPlugView *makeEditor(Controller *) {
    return nullptr;
}
} // namespace glassgarden
#endif
