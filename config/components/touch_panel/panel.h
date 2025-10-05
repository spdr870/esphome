#include <vector>
#include <unordered_map>
#include <functional>
#include <string>
#include <cmath>
#include <algorithm>

#include "PanelItem.h"
#include "LightItem.h"
#include "ButtonItem.h"
#include "EnvItem.h"
#include "ClockItem.h"

#include "esphome.h"
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

namespace touch_panel {

#ifndef panel_h
#define panel_h

// ---------- Panel (grid + pages + routing) ----------
class Panel : public esphome::Component {
public:
  Panel(int tft_cs, int touch_cs, int touch_irq, int cols=3, int rows=3)
  : tft_cs_(tft_cs), touch_cs_(touch_cs), touch_irq_(touch_irq),
    ts_(touch_cs_, touch_irq_), cols_(cols), rows_(rows), scratch_(&tft_) {}

  void setup() override {
    SPI.begin(18,19,23);
    pinMode(tft_cs_, OUTPUT);   digitalWrite(tft_cs_, HIGH);
    pinMode(touch_cs_, OUTPUT); digitalWrite(touch_cs_, HIGH);
    if (touch_irq_ >= 0) pinMode(touch_irq_, INPUT_PULLUP);

#ifdef TFT_BL
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
#endif

    tft_.init();
    tft_.setRotation(3);
    tft_.setSwapBytes(true);
    tft_.invertDisplay(true);

    ts_.begin();
    ts_.setRotation(2);

    screen_ = {0,0,480,320};
    grid_   = screen_;
    compute_grid_();

    tft_.fillScreen(TFT_BLACK);

    // Prepare shared scratch sprite (lazy sized on first use)
    scratch_.setColorDepth(8);
    scratch_w_ = scratch_h_ = 0;
  }

  void loop() override {
    if (sleeping_) return;

    uint32_t now = millis();
    for (auto* it : items_) if (it->Page()==current_page_) it->Tick(now);

    int16_t x,y;
    if (read_touch_(x,y)) handle_touch_(x,y);

    // Reuse the shared sprite for all items on the page.
    for (auto* it : items_) {
      if (it->Page()!=current_page_) continue;

      if (!it->ClearDirty()) continue;

      // Ensure sprite is at least the item's size; only grow (rare).
      const auto& b = last_bounds_[it];
      ensure_scratch_(b.w, b.h);

      // Clear the scratch area once per item before it draws.
      scratch_.fillSprite(TFT_BLACK);

      // Let the item render into the shared sprite and push.
      it->RenderIfDirty(tft_, scratch_);
      
      tft_.startWrite();
      scratch_.pushSprite(b.x, b.y, 0, 0, b.w, b.h);
      tft_.endWrite();
    }

    if (items_.empty()) {
      tft_.setTextColor(tft_.color565(random(256), random(256), random(256)), TFT_BLACK);
      tft_.drawString("Hello!", 10, 10, 4);
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

  void sleep(bool on) {
    if (on && !sleeping_) {
      tft_.writecommand(0x28);
      delay(20);
      tft_.writecommand(0x10);
      sleeping_ = true;
    } else if (!on && sleeping_) {
      tft_.writecommand(0x11);
      delay(120);
      tft_.writecommand(0x29);
      sleeping_ = false;
      tft_.fillScreen(TFT_BLACK);
      invalidate_visible_page_();
    }
  }

private:
  // ---------- Hardware ----------
  int tft_cs_, touch_cs_, touch_irq_;
  TFT_eSPI tft_;
  XPT2046_Touchscreen ts_;

  // Shared scratch sprite
  TFT_eSprite scratch_;
  int scratch_w_{0}, scratch_h_{0};

  // ---------- Layout ----------
  Rect screen_{}, grid_{};
  int cols_{3}, rows_{2};
  std::vector<Rect> cell_cache_;
  std::vector<IPanelItem*> items_;
  std::unordered_map<IPanelItem*,Rect> last_bounds_;

  int current_page_{0};
  bool sleeping_{false};

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
    bool pressed=false;
    if (touch_irq_>=0){ pressed=(digitalRead(touch_irq_)==LOW); if(!pressed) return false; }
    else { pressed=ts_.touched(); if(!pressed) return false; }
    digitalWrite(tft_cs_, HIGH);
    TS_Point p=ts_.getPoint();
    if (p.z<5 || p.z>4095) return false;
    const int RX0=200,RX1=3800, RY0=200,RY1=3800;
    int16_t mx = map(p.y, RY0, RY1, 0, 480);
    int16_t my = map(p.x, RX0, RX1, 0, 320);
    x = std::max<int16_t>(0, std::min<int16_t>(479, mx));
    y = std::max<int16_t>(0, std::min<int16_t>(319, my));
    return true;
  }

  // ---------- Scratch sprite mgmt ----------
  void ensure_scratch_(int w, int h){
    // Only grow; reuse if current is big enough.
    if (w <= scratch_w_ && h <= scratch_h_) return;
    scratch_.deleteSprite();
    scratch_.createSprite(w, h);
    scratch_w_ = w;
    scratch_h_ = h;
  }
};

} // namespace touch_panel

#endif // panel_h