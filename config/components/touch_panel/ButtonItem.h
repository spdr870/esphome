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

#ifndef buttonItem_h
#define buttonItem_h

namespace touch_panel {

class ButtonItem : public BaseItem {
public:
  ButtonItem(const char* id, const char* label, int page=0)
  : BaseItem(id, page), label_(label) {}

  void SetState(bool on) {
    if (on_ != on) { on_ = on; Invalidate(); }
  }
  bool State() const { return on_; }

  bool RenderIfDirty(TFT_eSPI& tft, TFT_eSprite& spr) override {
    uint16_t fill   = on_ ? TFT_GREEN : TFT_DARKGREY;
    uint16_t stroke = on_ ? TFT_WHITE : TFT_LIGHTGREY;

    spr.fillRoundRect(0, 0, B().w, B().h, 8, fill);
    spr.drawRoundRect(0, 0, B().w, B().h, 8, stroke);
    spr.setTextDatum(MC_DATUM);
    spr.setTextColor(TFT_BLACK, fill);
    spr.drawString(label_.c_str(), B().w/2, B().h/2, 4);

    return true;
  }

private:
  std::string label_;
  bool on_{false};
};


} // namespace touch_panel

#endif