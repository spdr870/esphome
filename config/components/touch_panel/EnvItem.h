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

#ifndef envItem_h
#define envItem_h

namespace touch_panel {

class EnvItem : public BaseItem {
public:
  EnvItem(const char* id="env", int page=0) : BaseItem(id, page) {}

  void OnEnvUpdate(float t, float h) override {
    if (isnan(t) || isnan(h)) return;
    if (fabsf(t - t_) > 0.05f || fabsf(h - h_) > 0.5f) {
      t_ = t; h_ = h; Invalidate();
    }
  }

  bool RenderIfDirty(TFT_eSPI& tft, TFT_eSprite& spr) override {
    // --- layout ---
    const int w = B().w, h = B().h;
    const int pad = 6;

    // --- palette / colors (subtle theme) ---
    const uint16_t bg        = TFT_BLACK;
    const uint16_t cardFill  = 0x2104;      // deep grey
    const uint16_t ink       = TFT_SILVER;  // text
    const uint16_t tickDim   = 0x7BEF;

    const uint16_t thermoOut = tickDim;
    const uint16_t thermoFill= 0xF900;      // red
    const uint16_t dropOut   = tickDim;
    const uint16_t dropFill  = 0x003F;      // blue

    // --- thermometer icon (top row) ---
    // Normalize temp to [-10..40]°C range for fill level
    const float tMin = -10.0f, tMax = 40.0f;
    float tPct = (t_ - tMin) / (tMax - tMin);
    if (tPct < 0) tPct = 0; if (tPct > 1) tPct = 1;

    int iconX = pad + 7;
    int iconY = pad + 2;
    drawThermometer_(spr, iconX, iconY, /*stemH*/28, /*stemW*/10, /*bulbR*/8, thermoOut, thermoFill, tPct);

    // temperature text
    spr.setTextDatum(TL_DATUM);
    spr.setTextColor(ink, bg);
    static int tpad = 0;
    if (!tpad) tpad = spr.textWidth("-00.0°C", 4);
    spr.setTextPadding(tpad);
    char buf[32];
    snprintf(buf, sizeof(buf), "%.1f°C", t_);
    spr.drawString(buf, iconX + 30, pad + 11, 4);

    // --- droplet icon (bottom row) ---
    float hPct = h_ / 100.0f;
    if (hPct < 0) hPct = 0; if (hPct > 1) hPct = 1;

    int dropX = pad - 3;
    int dropY = h/2 + pad;
    drawDroplet_(spr, dropX, dropY, /*size*/34, dropOut, dropFill, hPct);

    // humidity text
    static int hpad_txt = 0;
    if (!hpad_txt) hpad_txt = spr.textWidth("100%", 4);
    spr.setTextPadding(hpad_txt);
    snprintf(buf, sizeof(buf), "%.0f%%", h_);
    spr.drawString(buf, iconX + 30, h/2 + pad + 6, 4);

    return true;
  }

private:
  float t_{0}, h_{0};

  // --- ICON HELPERS ---

  // Simple thermometer with outline + variable-level fill.
  // (x,y) is top-left of the icon bounding box.
  static void drawThermometer_(TFT_eSprite& spr, int x, int y, int stemH, int stemW, int bulbR,
                               uint16_t outlineCol, uint16_t fillCol, float pct)
  {
    // Geometry
    const int cx = x + bulbR;                    // center X
    const int stemX = cx - stemW/2;
    const int stemY = y + 2;
    const int stemHclamped = stemH;
    const int bulbCx = cx;
    const int bulbCy = stemY + stemHclamped + bulbR - 1;

    // Outline
    spr.drawRoundRect(stemX, stemY, stemW, stemHclamped, stemW/2, outlineCol);
    spr.drawCircle(bulbCx, bulbCy-2, bulbR, outlineCol);

    // Fill level (from bulb up into stem)
    int level = (int)std::round(pct * (stemHclamped - 2));
    if (level < 0) level = 0;
    // bulb fill
    spr.fillCircle(bulbCx, bulbCy-2, bulbR-2, fillCol);
    // stem fill (upwards)
    spr.fillRect(stemX + 2, stemY + stemHclamped - level, stemW - 4, level, fillCol);

    // small highlight
    spr.drawLine(stemX + stemW - 3, stemY + 3, stemX + stemW - 3, stemY + stemHclamped - 3, 0xC618);
  }

  // Minimal droplet: circle + top “teardrop” tip; fill level mask using a rect.
  // (x,y) is top-left of the icon box; size ~ height.
  static void drawDroplet_(TFT_eSprite& spr, int x, int y, int size,
                           uint16_t outlineCol, uint16_t fillCol, float pct)
  {
    const int w = size, h = size;
    const int cx = x + w/2;
    const int cy = y + h/2 + 2;
    const int r  = w/3;

    // Base filled shape: circle + triangle tip
    spr.fillCircle(cx, cy, r, fillCol);
    // tip triangle (points upward)
    spr.fillTriangle(cx, cy - r - (r/2),  cx - r/2, cy - r/4,  cx + r/2, cy - r/4, fillCol);

    // “level” cut: draw a rectangle in background color above the current level to simulate liquid level.
    // We assume sprite background is already the card color; sample a pixel for safety.
    uint16_t bg = spr.readPixel(0,0);
    int levelY = y + (int)std::round((1.0f - pct) * (h - 2)); // higher pct => lower cut
    spr.fillRect(x, y, w, std::max(0, levelY - y), bg);

    // Outline over top
    spr.drawCircle(cx, cy, r, outlineCol);
    spr.drawLine(cx, cy - r - (r/2), cx - r/2, cy - r/4, outlineCol);
    spr.drawLine(cx, cy - r - (r/2), cx + r/2, cy - r/4, outlineCol);
    spr.drawLine(cx - r/2, cy - r/4, cx + r/2, cy - r/4, outlineCol);

    // small inner highlight
    spr.drawLine(cx - r/3, cy - r/3, cx - r/6, cy - r/2, 0xC618);
  }
};

} // namespace touch_panel

#endif