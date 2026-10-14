#include <vector>
#include <unordered_map>
#include <functional>
#include <string>
#include <cmath>
#include <algorithm>
#include <mutex>
#include <atomic>

#include "PanelItem.h"
#include "LightItem.h"
#include "ButtonItem.h"
#include "EnvItem.h"
#include "ClockItem.h"

#include "esphome.h"

// --- LovyanGFX (display + touch) ---
#include <LovyanGFX.hpp>
#include "LGFX_ILI9488_Touch.h"

#ifndef panel_h
#define panel_h

namespace touch_panel {

// Convenience color constants (avoid TFT_* macros)
static constexpr uint16_t COL_BLACK = 0x0000;
// you can add more if you used them

// ---------- Panel (grid + pages + routing) ----------
class Panel : public esphome::Component {
public:
  Panel(int tft_cs, int touch_cs, int touch_irq, int cols=3, int rows=3)
  : tft_cs_(tft_cs), touch_cs_(touch_cs), touch_irq_(touch_irq),
    // Use -D macros for DC/RST/SCLK/MOSI/MISO/BL from your build flags
    tft_(tft_cs_,
         TFT_DC,
         TFT_RST,
         TFT_SCLK,
         TFT_MOSI,
         TFT_MISO,
         touch_cs_,
         touch_irq_,
#ifdef TFT_BL
         TFT_BL
#else
        -1
#endif
    ),
    cols_(cols), rows_(rows), scratch_(&tft_) {}

  ~Panel() {
    for (auto* it : items_) delete it;
    items_.clear();
    last_bounds_.clear();
    scratch_.deleteSprite();
  }

  void setup() override {
    pinMode(tft_cs_, OUTPUT);
    digitalWrite(tft_cs_, HIGH);
    pinMode(touch_cs_, OUTPUT);
    digitalWrite(touch_cs_, HIGH);
    if (touch_irq_ >= 0) pinMode(touch_irq_, INPUT_PULLUP);

    // LovyanGFX handles SPI init via config; no SPI.begin() needed.
    tft_.init();
    tft_.setRotation(3);
    tft_.setSwapBytes(true);   // keep if your bitmaps need it

    screen_ = {0,0,480,320};
    grid_   = screen_;
    compute_grid_();

    digitalWrite(touch_cs_, HIGH);
    tft_.fillScreen(COL_BLACK);

    // Prepare shared scratch sprite (lazy-sized on first use)
    scratch_.setColorDepth(8);
    scratch_w_ = scratch_h_ = 0;

    // If you want built-in touch calibration later:
    // tft_.calibrateTouch(...); // or tft_.setTouchCalibrate(...)
  }

  void loop() override {
    power_step_();

    if (pwr_ != AWAKE) return;

    uint32_t now = millis();
    for (auto* it : items_) if (it->Page()==current_page_) it->Tick(now);

    int16_t x,y;
    if (read_touch_(x,y)) handle_touch_(x,y);

    // Reuse the shared sprite for all items on the page.
    for (auto* it : items_) {
      if (it->Page()!=current_page_) continue;
      if (!it->ClearDirty()) continue;

      const auto& b = last_bounds_[it];
      ensure_scratch_exact_(b.w, b.h);        // exact size to keep blits tight

      scratch_.fillScreen(COL_BLACK);
      it->RenderIfDirty(tft_, scratch_);

      digitalWrite(touch_cs_, HIGH);
      tft_tx([&](){
        // LGFX_Sprite::pushSprite has no src-rect overload; we sized exactly.
        scratch_.pushSprite(b.x, b.y);
      });
    }

    if (items_.empty()) {
      if ((int32_t)(now - deadline_) >= 0) {
        deadline_ = now + 100;
        tft_tx([&](){
          auto randc = lgfx::color565(random(256), random(256), random(256));
          tft_.setTextColor(randc, COL_BLACK);
          tft_.setFont(&fonts::Font4);
          tft_.drawString("Hello!", 10, 10);
        });
      }
    }
  }

  void set_button_state(const char* id, bool on) {
    for (auto* it : items_) {
      if (strcmp(it->Id(), id)==0) {
        auto btn = static_cast<ButtonItem*>(it);
        btn->SetState(on);
      }
    }
  }

  void add_button(const char* id, const char* label, int col, int row,
                  int colspan=1, int rowspan=1, int page=0) {
    add_item(new ButtonItem(id, label, page), col, row, colspan, rowspan, page);
  }

  void add_light(const char* id, const char* label, int col, int row,
                  int colspan=1, int rowspan=1, int page=0) {
    add_item(new LightItem(id, label, page), col, row, colspan, rowspan, page);
  }

  // ---------- Public API to manage items / pages ----------
  void add_item(IPanelItem* item, int col, int row, int colspan=1, int rowspan=1, int page=0) {
    Rect cell = cell_rect_(col,row,colspan,rowspan);
    item->SetBounds(cell);
    item->SetPage(page);
    items_.push_back(item);
    last_bounds_[item] = cell;
  }

  void add_paging_buttons(std::pair<int,int> prev_cell, std::pair<int,int> next_cell, int page=0) {
    auto prev = new ButtonItem("page_prev", "<", page);
    auto next = new ButtonItem("page_next", ">", page);
    add_item(prev, prev_cell.first, prev_cell.second, 1,1, page);
    add_item(next, next_cell.first, next_cell.second, 1,1, page);
    set_item_click("page_prev", [this](){ prev_page(); });
    set_item_click("page_next", [this](){ next_page(); });
  }

  void set_item_click(const char* id, std::function<void()> fn) {
    for (auto* it : items_)
      if (strcmp(it->Id(), id)==0)
        it->SetOnClick(std::move(fn));
  }

  void set_time(int hours, int minutes, int seconds) {
    for (auto* it : items_) it->OnTimeUpdate(hours, minutes, seconds);
  }

  void set_env(float t, float h) {
    for (auto* it : items_) it->OnEnvUpdate(t, h);
  }

  void next_page(){ set_page_(current_page_+1); }
  void prev_page(){ set_page_(current_page_==0 ? max_page_() : current_page_-1); }
  int  page() const { return current_page_; }

  void ready() {
    pwr_ = AWAKE;
    requestSleep_ = false;
  }

  void request_sleep(bool on) {
    requestSleep_ = on;
  }

private:
  std::mutex spi_mtx_;

  // ---------- Hardware ----------
  int tft_cs_, touch_cs_, touch_irq_;
  LGFX_ILI9488_Touch tft_;

  // Shared scratch sprite
  lgfx::LGFX_Sprite scratch_;
  int scratch_w_{0}, scratch_h_{0};

  // ---------- Layout ----------
  Rect screen_{}, grid_{};
  int cols_{3}, rows_{2};
  std::vector<Rect> cell_cache_;
  std::vector<IPanelItem*> items_;
  std::unordered_map<IPanelItem*,Rect> last_bounds_;

  int current_page_{0};

  enum PwrState { NOINIT, AWAKE, GOING_OFF_DISPOFF, GOING_OFF_SLEEPIN_WAIT, SLEEPING,
                WAKING_SLEEPOUT_WAIT, WAKING_DISPON, WAKING_SETTLE_WAIT };
  PwrState pwr_ = AWAKE;
  uint32_t deadline_ = 0;

  std::atomic<bool> requestSleep_{false};

  inline void tft_tx(std::function<void()> fn) {
    std::lock_guard<std::mutex> lk(spi_mtx_);
    tft_.waitDMA();         // finish any prior DMA first
    tft_.startWrite();
    fn();                   // do the SPI writes
    tft_.endWrite();
  }

  inline void lcd_cmd(uint8_t c) {
    std::lock_guard<std::mutex> lk(spi_mtx_);
    tft_.waitDMA();
    tft_.startWrite();
    tft_.writeCommand(c);
    tft_.endWrite();
  }

  const char* pwr_to_str(PwrState s) {
    switch (s) {
      case AWAKE: return "AWAKE";
      case GOING_OFF_DISPOFF: return "OFF_DISPOFF";
      case GOING_OFF_SLEEPIN_WAIT: return "OFF_SLEEPIN";
      case SLEEPING: return "SLEEPING";
      case WAKING_SLEEPOUT_WAIT: return "WAKE_OUT_WAIT";
      case WAKING_SETTLE_WAIT: return "WAKE_SETTLE";
      default: return "?";
    }
  }

  void power_step_() {
    const uint32_t now = millis();

    switch (pwr_) {
      case AWAKE:
        if (requestSleep_) {
          // (Optional) backlight off here
          lcd_cmd(0x28); // DISPOFF
          deadline_ = now + 20;
          pwr_ = GOING_OFF_DISPOFF;
        }
        break;

      case SLEEPING:
        if (!requestSleep_) {
          lcd_cmd(0x11); // SLEEP OUT
          deadline_ = now + 120;
          pwr_ = WAKING_SLEEPOUT_WAIT;
        }
        break;
    }

    if ((int32_t)(now - deadline_) >= 0) {
      switch (pwr_) {
        case GOING_OFF_DISPOFF:
          lcd_cmd(0x10); // SLEEP IN
          deadline_ = now + 120;
          pwr_ = GOING_OFF_SLEEPIN_WAIT;
          break;

        case GOING_OFF_SLEEPIN_WAIT:
          pwr_ = SLEEPING;
          break;

        case WAKING_SLEEPOUT_WAIT:
          lcd_cmd(0x29); // DISP ON
          deadline_ = now + 20; // settle
          pwr_ = WAKING_SETTLE_WAIT;
          break;

        case WAKING_SETTLE_WAIT:
          tft_tx([&](){ tft_.fillScreen(COL_BLACK); });
          invalidate_visible_page_();
          pwr_ = AWAKE;
          break;
      }
    }
  }

  // ---------- Grid helpers ----------
  void compute_grid_(){
    cell_cache_.clear();
    const int cw = grid_.w / cols_;
    const int ch = grid_.h / rows_;
    for (int r=0;r<rows_;++r)
      for (int c=0;c<cols_;++c)
        cell_cache_.push_back({grid_.x + c*cw, grid_.y + r*ch, cw, ch});
  }

  Rect cell_rect_(int c,int r,int cs=1,int rs=1){
    const int cw = grid_.w / cols_;
    const int ch = grid_.h / rows_;
    return {grid_.x + c*cw + 1, grid_.y + r*ch + 1, cw*cs - 2, ch*rs - 2};
  }

  // ---------- Page helpers ----------
  void set_page_(int p){
    current_page_ = (p % (max_page_()+1));
    invalidate_visible_page_();
  }

  int max_page_() const {
    int m=0; for (auto* it: items_) m = std::max(m, it->Page()); return m;
  }

  void invalidate_visible_page_(){
    for (auto* it : items_) if (it->Page()==current_page_) {
      auto itb = last_bounds_.find(it);
      if (itb != last_bounds_.end()) it->SetBounds(itb->second);
    }
  }

  // ---------- Touch handling ----------
  void handle_touch_(int x,int y){
    for (auto it = items_.rbegin(); it != items_.rend(); ++it){
      IPanelItem* item = *it;
      if (item->Page()!=current_page_) continue;
      if (item->HitTest(x,y)) { item->OnClick(); break; }
    }
  }

  bool read_touch_(int16_t &x,int16_t &y){
    bool touched = false;
    {
      tft_.waitDMA();
      // getTouch returns false when not touched; returns pixel coords when true
      touched = tft_.getTouch(&x, &y);
    }
    if (!touched) return false;

    // Clamp to screen (rotation already applied by LGFX)
    x = std::max<int16_t>(0, std::min<int16_t>(479, x));
    y = std::max<int16_t>(0, std::min<int16_t>(319, y));
    return true;
  }

  // ---------- Scratch sprite mgmt ----------
  void ensure_scratch_exact_(int w, int h){
    // Recreate if size differs to ensure tight blits
    if (w == scratch_w_ && h == scratch_h_) return;
    scratch_.deleteSprite();
    scratch_.createSprite(w, h);
    scratch_w_ = w;
    scratch_h_ = h;
  }
};

} // namespace touch_panel

#endif // panel_h
