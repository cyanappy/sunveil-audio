// Uses exactly the artwork commands used by the native Windows editor. This
// verifies layout independently of a Windows window server; it is not a DAW screenshot.
#include "../Source/Artwork.h"
#include <fstream>
#include <sstream>
#include <string>
#include <locale>
#include <codecvt>
#include <cstdlib>
using namespace sundrive;
using namespace sundrive::artwork;
struct SVG {
    std::ostream &out;
    static std::string color(uint32_t c) {
        char t[16];
        std::snprintf(t, sizeof(t), "#%02x%02x%02x", c & 255, (c >> 8) & 255, (c >> 16) & 255);
        return t;
    }
    static std::string escaped(std::string s) {
        std::string result;
        for (char c : s) {
            if (c == '&')
                result += "&amp;";
            else if (c == '<')
                result += "&lt;";
            else if (c == '>')
                result += "&gt;";
            else
                result += c;
        }
        return result;
    }
    void box(Box b, uint32_t fill, int radius = 0, uint32_t border = uint32_t(-1), int width = 1) {
        out << "<rect x='" << b.x << "' y='" << b.y << "' width='" << b.w << "' height='" << b.h
            << "' rx='" << radius << "' fill='" << color(fill) << "'";
        if (border != uint32_t(-1))
            out << " stroke='" << color(border) << "' stroke-width='" << width << "'";
        out << "/>\n";
    }
    void circle(double x, double y, double r, uint32_t fill, uint32_t border = uint32_t(-1),
                double width = 1) {
        out << "<circle cx='" << x << "' cy='" << y << "' r='" << r << "' fill='" << color(fill)
            << "'";
        if (border != uint32_t(-1))
            out << " stroke='" << color(border) << "' stroke-width='" << width << "'";
        out << "/>\n";
    }
    void stroke(double x1, double y1, double x2, double y2, uint32_t c, double width = 1) {
        out << "<line x1='" << x1 << "' y1='" << y1 << "' x2='" << x2 << "' y2='" << y2
            << "' stroke='" << color(c) << "' stroke-width='" << width
            << "' stroke-linecap='round'/>\n";
    }
    void ascii(const char *text, Box b, int size, uint32_t c, int weight = 400, unsigned align = 0,
               bool spaced = false, bool serif = false) {
        const double x = align == 1 ? b.x + b.w * .5 : align == 2 ? b.x + b.w : b.x;
        out << "<text x='" << x << "' y='" << b.y + b.h * .5
            << "' dominant-baseline='central' text-anchor='"
            << (align == 1   ? "middle"
                : align == 2 ? "end"
                             : "start")
            << "' font-family='"
            << (serif ? "Georgia, DejaVu Serif, serif" : "Segoe UI, DejaVu Sans, sans-serif")
            << "' font-size='" << size << "' font-weight='" << weight << "' letter-spacing='"
            << (spaced ? 1 : 0) << "' fill='" << color(c) << "'>" << escaped(text) << "</text>\n";
    }
    void text(const wchar_t *text, Box b, int size, uint32_t c, int weight = 400,
              unsigned align = 0, bool spaced = false, bool serif = false) {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
        ascii(conv.to_bytes(text).c_str(), b, size, c, weight, align, spaced, serif);
    }
};
int main(int argc, char **argv) {
    std::ofstream file(argc > 1 ? argv[1] : "Sundrive-preview.svg");
    file << "<svg xmlns='http://www.w3.org/2000/svg' width='" << W << "' height='" << H
         << "' viewBox='0 0 " << W << " " << H << "'>\n";
    SVG canvas{file};
    Pedal pedal;
    if (argc > 2) {
        const int index = std::atoi(argv[2]);
        if (index >= 0 && index < presetCount) {
            pedal.presetIndex = index;
            pedal.values = presets[index].values;
        }
    }
    if (argc > 3)
        pedal.values[Bypass] = std::atoi(argv[3]) ? 1 : 0;
    pedal.draw(canvas);
    file << "</svg>\n";
    return file ? 0 : 1;
}
