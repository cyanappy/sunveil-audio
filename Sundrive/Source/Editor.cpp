#include "Plugin.h"
#include "Artwork.h"
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
extern HINSTANCE sundriveModule;

namespace sundrive {
namespace {
using namespace artwork;
constexpr wchar_t className[] = L"SunveilAudio.Sundrive.Editor.3";
std::atomic<int> liveWindows{0};
class Canvas {
  public:
    HDC dc;
    int s = 2;
    int p(double x) const { return int(std::lround(x * s)); }
    void box(Box b, COLORREF color, int radius = 0, COLORREF border = CLR_INVALID, int width = 1) {
        auto brush = CreateSolidBrush(color);
        auto oldBrush = SelectObject(dc, brush);
        auto pen = border == CLR_INVALID ? CreatePen(PS_NULL, 0, color)
                                         : CreatePen(PS_SOLID, p(width), border);
        auto oldPen = SelectObject(dc, pen);
        if (radius)
            RoundRect(dc, p(b.x), p(b.y), p(b.x + b.w), p(b.y + b.h), p(radius * 2), p(radius * 2));
        else
            Rectangle(dc, p(b.x), p(b.y), p(b.x + b.w), p(b.y + b.h));
        SelectObject(dc, oldPen);
        SelectObject(dc, oldBrush);
        DeleteObject(pen);
        DeleteObject(brush);
    }
    void circle(double x, double y, double r, COLORREF color, COLORREF border = CLR_INVALID,
                double width = 1) {
        auto brush = CreateSolidBrush(color);
        auto ob = SelectObject(dc, brush);
        auto pen = border == CLR_INVALID ? CreatePen(PS_NULL, 0, color)
                                         : CreatePen(PS_SOLID, p(width), border);
        auto op = SelectObject(dc, pen);
        Ellipse(dc, p(x - r), p(y - r), p(x + r), p(y + r));
        SelectObject(dc, op);
        SelectObject(dc, ob);
        DeleteObject(pen);
        DeleteObject(brush);
    }
    void stroke(double x1, double y1, double x2, double y2, COLORREF color, double width = 1) {
        auto pen = CreatePen(PS_SOLID, std::max(1, p(width)), color);
        auto old = SelectObject(dc, pen);
        MoveToEx(dc, p(x1), p(y1), nullptr);
        LineTo(dc, p(x2), p(y2));
        SelectObject(dc, old);
        DeleteObject(pen);
    }
    void text(const wchar_t *text, Box b, int size, COLORREF color, int weight = FW_NORMAL,
              UINT align = DT_LEFT, bool spaced = false, bool serif = false) {
        auto font = CreateFontW(-p(size), 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                                DEFAULT_PITCH, serif ? L"Georgia" : L"Segoe UI");
        auto old = SelectObject(dc, font);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, color);
        const int prior = SetTextCharacterExtra(dc, spaced ? p(1) : 0);
        RECT r{p(b.x), p(b.y), p(b.x + b.w), p(b.y + b.h)};
        DrawTextW(dc, text, -1, &r, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | align);
        SetTextCharacterExtra(dc, prior);
        SelectObject(dc, old);
        DeleteObject(font);
    }
    void ascii(const char *t, Box b, int size, COLORREF color, int weight = FW_NORMAL,
               UINT align = DT_LEFT) {
        wchar_t wide[256]{};
        for (int i = 0; i < 255 && t[i]; ++i)
            wide[i] = static_cast<unsigned char>(t[i]);
        text(wide, b, size, color, weight, align);
    }
};
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
        wc.hInstance = sundriveModule;
        wc.lpszClassName = className;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.style = CS_DBLCLKS;
        if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            return kResultFalse;
        window = CreateWindowExW(0, className, L"Sundrive",
                                 WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS, 0, 0,
                                 rect.getWidth(), rect.getHeight(), static_cast<HWND>(parent),
                                 nullptr, sundriveModule, this);
        if (!window)
            return kResultFalse;
        ++liveWindows;
        SetTimer(window, 1, 50, nullptr);
        return kResultOk;
    }
    tresult PLUGIN_API removed() override {
        if (window) {
            if (drag >= 0) {
                controller->end(drag);
                drag = -1;
            }
            auto w = window;
            window = nullptr;
            KillTimer(w, 1);
            DestroyWindow(w);
            if (--liveWindows == 0)
                UnregisterClassW(className, sundriveModule);
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
        const int width = std::clamp(r->getWidth(), W * 3 / 4, W * 3);
        r->right = r->left + width;
        r->bottom = r->top + int(std::lround(width * double(H) / W));
        return kResultTrue;
    }
    tresult PLUGIN_API setContentScaleFactor(ScaleFactor factor) override {
        if (!std::isfinite(factor) || factor <= 0)
            return kInvalidArgument;
        factor = std::clamp(factor, .75f, 3.f);
        ViewRect wanted(0, 0, int(std::lround(W * factor)), int(std::lround(H * factor)));
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
    int knobAt(int x, int y) const {
        for (auto k : knobs)
            if (std::hypot(double(x - k.x), double(y - k.y)) < k.r + 20)
                return k.id;
        return -1;
    }
    void redraw() {
        if (window)
            InvalidateRect(window, nullptr, FALSE);
    }
    void adjust(int id, double amount) {
        if (id < 0 || id >= Count || amount == 0)
            return;
        const double step = definitions[id].steps ? 1.0 / definitions[id].steps : amount;
        controller->setFromUI(id, controller->getParamNormalized(id) +
                                      (amount < 0 ? -std::abs(step) : std::abs(step)));
        redraw();
    }
    bool handleKey(int key, int modifiers = -1) {
        const bool shift =
            modifiers < 0 ? (GetKeyState(VK_SHIFT) & 0x8000) != 0 : (modifiers & kShiftKey) != 0;
        if (key == VK_TAB) {
            const int dir = shift ? -1 : 1;
            focused = focused < 0 ? (shift ? Count - 1 : 0) : (focused + dir + Count) % Count;
            redraw();
            return true;
        }
        if (key == VK_SPACE) {
            controller->setFromUI(Bypass, controller->getParamNormalized(Bypass) < .5 ? 1 : 0);
            redraw();
            return true;
        }
        if (key == 'P') {
            showPresets();
            return true;
        }
        if (focused >= 0 && (key == VK_UP || key == VK_RIGHT || key == VK_DOWN || key == VK_LEFT)) {
            const double v = shift ? .001 : .01;
            adjust(focused, (key == VK_DOWN || key == VK_LEFT) ? -v : v);
            return true;
        }
        if (focused >= 0 && key == VK_HOME) {
            controller->setFromUI(focused, definitions[focused].initial);
            redraw();
            return true;
        }
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
            PAINTSTRUCT ps;
            HDC target = BeginPaint(w, &ps);
            paint(target);
            EndPaint(w, &ps);
            return 0;
        }
        case WM_TIMER:
            controller->pollMeters();
            redraw();
            return 0;
        case WM_GETDLGCODE:
            return DLGC_WANTARROWS | DLGC_WANTTAB | DLGC_WANTCHARS;
        case WM_KEYDOWN:
            if (handleKey(int(wp)))
                return 0;
            break;
        case WM_LBUTTONDOWN: {
            SetFocus(w);
            const int id = knobAt(x, y);
            if (id >= 0) {
                focused = drag = id;
                lastY = y;
                controller->begin(id);
                SetCapture(w);
                redraw();
                return 0;
            }
            if (voiceBox.hit(x, y)) {
                focused = Voice;
                controller->setFromUI(Voice, controller->getParamNormalized(Voice) < .5 ? 1 : 0);
            } else if (clipBox.hit(x, y)) {
                focused = Clipping;
                controller->setFromUI(Clipping,
                                      ((int(std::round(controller->getParamNormalized(Clipping) * 2)) + 1) % 3) / 2.0);
            } else if (stompHit(x, y)) {
                focused = Bypass;
                controller->setFromUI(Bypass, controller->getParamNormalized(Bypass) < .5 ? 1 : 0);
            } else if (presetBox.hit(x, y))
                showPresets();
            redraw();
            return 0;
        }
        case WM_LBUTTONDBLCLK: {
            if (drag >= 0) {
                const int old = drag;
                drag = -1;
                controller->end(old);
                ReleaseCapture();
            }
            const int id = knobAt(x, y);
            if (id >= 0)
                controller->setFromUI(id, definitions[id].initial);
            redraw();
            return 0;
        }
        case WM_LBUTTONUP:
            if (drag >= 0) {
                const int id = drag;
                drag = -1;
                controller->end(id);
                ReleaseCapture();
                redraw();
            }
            return 0;
        case WM_CAPTURECHANGED:
            if (drag >= 0) {
                controller->end(drag);
                drag = -1;
            }
            return 0;
        case WM_MOUSEMOVE: {
            if (drag >= 0) {
                const double divisor = (GetKeyState(VK_SHIFT) & 0x8000) ? 1800.0 : 180.0;
                controller->edit(drag,
                                 controller->getParamNormalized(drag) + (lastY - y) / divisor);
                lastY = y;
                redraw();
            }
            const int id = knobAt(x, y);
            SetCursor(LoadCursor(nullptr, id >= 0 || voiceBox.hit(x, y) || clipBox.hit(x, y) ||
                                                  presetBox.hit(x, y) || stompHit(x, y)
                                              ? IDC_HAND
                                              : IDC_ARROW));
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
        case WM_SETFOCUS:
        case WM_KILLFOCUS:
            redraw();
            return 0;
        }
        return DefWindowProcW(w, m, wp, lp);
    }
    void paint(HDC target) {
        const int renderScale = std::clamp(int(std::ceil(double(rect.getWidth()) / W)), 2, 3);
        const int bw = W * renderScale, bh = H * renderScale;
        HDC buffer = CreateCompatibleDC(target);
        HBITMAP bitmap = CreateCompatibleBitmap(target, bw, bh);
        auto old = SelectObject(buffer, bitmap);
        Canvas c{buffer, renderScale};
        artwork::Pedal art;
        for (unsigned i = 0; i < Count; ++i)
            art.values[i] = controller->getParamNormalized(i);
        art.presetIndex = controller->currentPreset();
        art.focused = focused;
        art.hasFocus = GetFocus() == window;
        art.inputPeak = controller->inputPeak();
        art.outputPeak = controller->outputPeak();
        art.draw(c);
        SetStretchBltMode(target, HALFTONE);
        SetBrushOrgEx(target, 0, 0, nullptr);
        StretchBlt(target, 0, 0, rect.getWidth(), rect.getHeight(), buffer, 0, 0, bw, bh, SRCCOPY);
        SelectObject(buffer, old);
        DeleteObject(bitmap);
        DeleteDC(buffer);
    }
    std::atomic<uint32> refs{1};
    Controller *controller;
    HWND window = nullptr;
    IPlugFrame *frame = nullptr;
    ViewRect rect{0, 0, W, H};
    int focused = -1, drag = -1, lastY = 0;
};
IPlugView *makeEditor(Controller *c) {
    return new Editor(c);
}
} // namespace sundrive
#else
namespace sundrive {
IPlugView *makeEditor(Controller *) {
    return nullptr;
}
} // namespace sundrive
#endif
