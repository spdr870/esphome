#include <vector>
#include <unordered_map>
#include <functional>
#include <string>
#include <cmath>
#include <algorithm>

#include "esphome.h"
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

#include "PanelItem.h"

#ifndef clockItem_h
#define clockItem_h

namespace touch_panel {

class ClockItem : public BaseItem {
public:
  ClockItem(const char* id="clock", int page=0) : BaseItem(id, page) {}

  void OnTimeUpdate(int hours, int minutes, int seconds) override {
    if (hours_ == hours && minutes_ == minutes && seconds_ == seconds) return;
    hours_ = hours; minutes_ = minutes; seconds_ = seconds;    
    Invalidate();
  }

  bool RenderIfDirty(TFT_eSPI& tft, TFT_eSprite& spr) override {
    spr.setTextDatum(TL_DATUM);
    spr.setTextColor(TFT_LIGHTGREY, TFT_BLACK);

    static int pad = 0;
    if (pad == 0) pad = spr.textWidth("00:00:00", 4);
    spr.setTextPadding(pad);

    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", hours_, minutes_, seconds_);
    spr.drawString(buf, 6, 6, 4);
    return true;
  }

private:
  int hours_{0}, minutes_{0}, seconds_{0};
};

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class AnalogClockItem : public BaseItem {
public:
  AnalogClockItem(const char* id="analog_clock", int page=0)
  : BaseItem(id, page) {}

  void OnTimeUpdate(int hours, int minutes, int seconds) override {
    if (hours_ == hours && minutes_ == minutes && seconds_ == seconds) return;
    hours_ = hours; minutes_ = minutes; seconds_ = seconds;
    Invalidate();
  }

  bool RenderIfDirty(TFT_eSPI& tft, TFT_eSprite& spr) override {
    const int w = B().w, h = B().h;
    const int cx = w / 2, cy = h / 2;
    const int r  = (std::min(w, h) / 2) - 2;

    // palette: quiet + monochrome-ish
    const uint16_t bg       = TFT_BLACK;
    const uint16_t ticks    = TFT_WHITE;      // soft grey
    const uint16_t hourCol  = TFT_WHITE;  // slim, elegant
    const uint16_t minCol   = TFT_SILVER;
    const uint16_t secCol   = TFT_RED;      // very light grey (keeps it subtle)

    spr.setTextDatum(TC_DATUM);
    spr.setTextColor(TFT_LIGHTGREY, TFT_BLACK);

    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d", hours_, minutes_);
    spr.drawString(buf, B().w / 2, B().h - B().h / 3, 4);

    // faint 12 dots (major hours only), 12/3/6/9 slightly larger
    for (int i = 0; i < 12; ++i) {
      float a = (i / 12.0f) * 2.0f * M_PI;
      int x, y; polar_(cx, cy, a, r - 6, x, y);
      int dot = (i % 3 == 0) ? 2 : 1;
      spr.fillCircle(x, y, dot, ticks);
    }

    // angles (smooth hour/minute)
    const float sa = (seconds_ / 60.0f) * 2.0f * M_PI;
    const float ma = ((minutes_ + seconds_ / 60.0f) / 60.0f) * 2.0f * M_PI;
    const float ha = (((hours_ % 12) + minutes_ / 60.0f + seconds_ / 3600.0f) / 12.0f) * 2.0f * M_PI;

    // hands — slim & tapered, no shadows, no tail
    drawHandTaper_(spr, cx, cy, ha, r * 0.55f, 2, hourCol); // hour (slightly thicker)
    drawHandTaper_(spr, cx, cy, ma, r * 0.78f, 1, minCol);  // minute
    drawSecondLine_(spr, cx, cy, sa, r * 0.82f, secCol);    // second (single thin line)

    // tiny center dot
    spr.fillCircle(cx, cy, 2, hourCol);
    return true;
  }

private:
  int hours_{0}, minutes_{0}, seconds_{0};

  static void polar_(int cx, int cy, float a, float rad, int& x, int& y) {
    x = (int)std::lround(cx + std::sin(a) * rad);
    y = (int)std::lround(cy - std::cos(a) * rad);
  }

  // Replace your existing drawHandTaper_ with this:
  static void drawHandTaper_(TFT_eSprite& spr, int cx, int cy,
                            float a, float len, int baseW, uint16_t col) {
    // tip (points to angle a, with your polar_ convention)
    int xt, yt, xtail, ytail;
    polar_(cx, cy, a, len, xt, yt);
    polar_(cx, cy, a, -(len*0.2f), xtail, ytail);

    // perpendicular to the hand direction:
    // direction d = (sin a, -cos a)  -->  perp p = (-dy, dx) = (cos a, sin a)
    const float s = std::sin(a), c = std::cos(a);
    const float half = baseW;

    // base is centered on (cx, cy), offset +/- perp
    const int xb1 = (int)std::lround(xtail - c * half);
    const int yb1 = (int)std::lround(ytail - s * half);
    const int xb2 = (int)std::lround(xtail + c * half);
    const int yb2 = (int)std::lround(ytail + s * half);

    spr.fillTriangle(xb1, yb1, xb2, yb2, xt, yt, col);
    // Optional: crisp edges
    // spr.drawTriangle(xb1, yb1, xb2, yb2, xt, yt, col);
  }


  static void drawSecondLine_(TFT_eSprite& spr, int cx, int cy, float a, float len, uint16_t col) {
    int xs, ys; polar_(cx, cy, a, len, xs, ys);
    spr.drawLine(cx, cy, xs, ys, col);
  }
};

} // namespace touch_panel

#endif