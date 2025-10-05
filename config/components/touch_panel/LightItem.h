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

#ifndef lightItem_h
#define lightItem_h

namespace touch_panel {

// ---------- LightItem: centered text + subtle 3D ----------
class LightItem : public BaseItem {
public:
  LightItem(const char* id, const char* label, int page=0)
  : BaseItem(id, page), label_(label) {}

  void SetState(bool on) { if (on_ != on) { on_ = on; Invalidate(); } }
  bool State() const { return on_; }

 bool RenderIfDirty(TFT_eSPI& tft, TFT_eSprite& spr) override {
    uint16_t fill   = on_ ? TFT_YELLOW : TFT_NAVY;
    uint16_t stroke = on_ ? TFT_WHITE : TFT_LIGHTGREY;
    uint16_t text = on_ ? TFT_BLACK : TFT_LIGHTGREY;

    spr.fillRoundRect(0, 0, B().w, B().h, 8, fill);
    spr.drawRoundRect(0, 0, B().w, B().h, 8, stroke);
    spr.setTextDatum(MC_DATUM);
    spr.setTextColor(text, fill);
    spr.drawString(label_.c_str(), B().w/2, B().h/2, 2);

    return true;
  }

private:
  std::string label_;
  bool on_{false};
};

} // namespace touch_panel

#endif