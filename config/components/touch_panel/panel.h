#include <vector>
#include <unordered_map>
#include <functional>
#include <string>
#include <cmath>
#include <algorithm>

#include "esphome.h"
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

namespace touch_panel {

// ---------- Geometry helpers ----------
struct Rect { int x,y,w,h; };
static inline bool hit(const Rect&r,int x,int y){return x>=r.x && x<r.x+r.w && y>=r.y && y<r.y+r.h;}

// ---------- Panel Item Interface + Base ----------
class IPanelItem {
public:
  virtual ~IPanelItem() = default;
  virtual void SetBounds(const Rect& r) = 0;
  virtual void SetPage(int page) = 0;
  virtual int  Page() const = 0;
  virtual const char* Id() const = 0;

  // Called every loop; should draw only when dirty. Return true if drawn.
  virtual bool RenderIfDirty(TFT_eSPI& tft) = 0;

  // For time-based invalidation, animations, sensors, etc.
  virtual void Tick(uint32_t now_ms) {}

  // Hit testing (default: rect bounds)
  virtual bool HitTest(int x, int y) const = 0;

  virtual void OnClick() = 0;
  virtual void SetOnClick(std::function<void()>) = 0;        // NEW
  virtual void OnEnvUpdate(float /*t*/, float /*h*/) {}       // NEW: default no-op
};

class BaseItem : public IPanelItem {
public:
  explicit BaseItem(const char* id, int page=0) : id_(id), page_(page) {}

  void SetBounds(const Rect& r) override { bounds_ = r; dirty_ = true; }
  void SetPage(int page) override { page_ = page; dirty_ = true; }
  int  Page() const override { return page_; }
  const char* Id() const override { return id_.c_str(); }

  bool HitTest(int x, int y) const override { return hit(bounds_, x, y); }

  void SetOnClick(std::function<void()> fn) { on_click_ = std::move(fn); }
  void OnClick() override { if (on_click_) on_click_(); }

protected:
  void Invalidate() { dirty_ = true; }
  bool IsDirty() const { return dirty_; }
  bool ClearDirty() { bool d = dirty_; dirty_ = false; return d; }
  const Rect& B() const { return bounds_; }

  std::string id_;
  Rect bounds_{};
  int page_{0};
  bool dirty_{true};
  std::function<void()> on_click_{};
};

// ---------- Example Items ----------
class EnvItem : public BaseItem {
public:
  EnvItem(const char* id="env", int page=0) : BaseItem(id, page) {}

  void OnEnvUpdate(float t, float h) override {
    if (isnan(t) || isnan(h)) return;
    if (fabsf(t - t_) > 0.05f || fabsf(h - h_) > 0.5f) { t_ = t; h_ = h; Invalidate(); }
  }

  bool RenderIfDirty(TFT_eSPI& tft) override {
    if (!ClearDirty()) return false;
    tft.fillRect(B().x, B().y, B().w, B().h, TFT_BLACK);
    tft.drawRect(B().x, B().y, B().w, B().h, TFT_DARKGREY);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    char buf[48];
    snprintf(buf, sizeof(buf), "Temp: %.1f°C", t_);
    tft.drawString(buf, B().x + 6, B().y + 6, 4);

    snprintf(buf, sizeof(buf), "Hum:  %.0f%%", h_);
    tft.drawString(buf, B().x + 6, B().y + 6 + 28, 4);

    return true;
  }

private:
  float t_{0}, h_{0};
};

class ButtonItem : public BaseItem {
public:
  ButtonItem(const char* id, const char* label, int page=0)
  : BaseItem(id, page), label_(label) {}

  void SetState(bool on) {
    if (on_ != on) { on_ = on; Invalidate(); }
  }
  bool State() const { return on_; }

  bool RenderIfDirty(TFT_eSPI& tft) override {
    if (!ClearDirty()) return false;
    uint16_t fill   = on_ ? TFT_GREEN : TFT_DARKGREY;
    uint16_t stroke = on_ ? TFT_WHITE : TFT_LIGHTGREY;
    tft.fillRoundRect(B().x, B().y, B().w, B().h, 8, fill);
    tft.drawRoundRect(B().x, B().y, B().w, B().h, 8, stroke);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_BLACK, fill);
    tft.drawString(label_.c_str(), B().x + B().w/2, B().y + B().h/2, 4);
    return true;
  }

private:
  std::string label_;
  bool on_{false};
};

// ---------- Panel (grid + pages + routing) ----------
class Panel : public esphome::Component {
public:
  Panel(int tft_cs, int touch_cs, int touch_irq, int cols=3, int rows=3)
  : tft_cs_(tft_cs), touch_cs_(touch_cs), touch_irq_(touch_irq),
    ts_(touch_cs_, touch_irq_), cols_(cols), rows_(rows) {}

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
    tft_.setRotation(1);
    tft_.setSwapBytes(true);
    tft_.invertDisplay(true);

    ts_.begin();
    ts_.setRotation(0);

    // Full-screen grid (no header/footer)
    screen_ = {0,0,480,320};
    grid_   = screen_;
    compute_grid_();

    // Clear screen & draw static grid borders once
    tft_.fillScreen(TFT_BLACK);
    draw_grid_frame_();
  }

  void loop() override {
    if (sleeping_) return;

    // 1) Let items update their internal states
    uint32_t now = millis();
    for (auto* it : items_) if (it->Page()==current_page_) it->Tick(now);

    // 2) Touch handling
    int16_t x,y;
    if (read_touch_(x,y)) handle_touch_(x,y);

    // 3) Render dirty items on current page
    for (auto* it : items_) if (it->Page()==current_page_) it->RenderIfDirty(tft_);
  }

  void set_button_state(const char* id, bool on) {
    for (auto* it : items_) {
      if (strcmp(it->Id(), id)==0) {
        // Only works on ButtonItem
        auto btn = static_cast<ButtonItem*>(it);
        btn->SetState(on);
      }
    }
  }

  void add_button(const char* id, const char* label, int col, int row,
                int colspan=1, int rowspan=1, int page=0) {
    add_item(new ButtonItem(id, label, page), col, row, colspan, rowspan, page);
  }
  // ---------- Public API to manage items / pages ----------
  void add_item(IPanelItem* item, int col, int row, int colspan=1, int rowspan=1, int page=0) {
    Rect cell = cell_rect_(col,row,colspan,rowspan);
    item->SetBounds(cell);
    item->SetPage(page);
    items_.push_back(item);
    last_bounds_[item] = cell;
  }

  // Optional helper: add paging buttons as normal grid items
  // Example usage (from YAML lambda):
  //   p->add_paging_buttons( /*prev*/ {0,rows-1}, /*next*/ {cols-1,rows-1}, /*page=*/0 );
  void add_paging_buttons(std::pair<int,int> prev_cell, std::pair<int,int> next_cell, int page=0) {
    auto prev = new ButtonItem("page_prev", "<", page);
    auto next = new ButtonItem("page_next", ">", page);
    add_item(prev, prev_cell.first, prev_cell.second, 1,1, page);
    add_item(next, next_cell.first, next_cell.second, 1,1, page);
    // Wire to page navigation
    set_item_click("page_prev", [this](){ prev_page(); });
    set_item_click("page_next", [this](){ next_page(); });
  }

  // Expose click handlers to YAML
  void set_item_click(const char* id, std::function<void()> fn) {
    for (auto* it : items_)
      if (strcmp(it->Id(), id)==0)
        it->SetOnClick(std::move(fn));
  }

  // Convenience: push environment values into all EnvItem instances
  void set_env(float t, float h) {
    for (auto* it : items_) it->OnEnvUpdate(t, h); 
  }

  // Page control (usable from YAML or ButtonItem callbacks)
  void next_page(){ set_page_(current_page_+1); }
  void prev_page(){ set_page_(current_page_==0 ? max_page_() : current_page_-1); }
  int  page() const { return current_page_; }

  // Sleep control (same as your original)
  void sleep(bool on) {
    if (on && !sleeping_) {
      tft_.writecommand(0x28);   // Display OFF
      delay(20);
      tft_.writecommand(0x10);   // Sleep IN
      sleeping_ = true;
    } else if (!on && sleeping_) {
      tft_.writecommand(0x11);   // Sleep OUT
      delay(120);
      tft_.writecommand(0x29);   // Display ON
      sleeping_ = false;
      // Redraw grid + mark visible items dirty
      tft_.fillScreen(TFT_BLACK);
      draw_grid_frame_();
      invalidate_visible_page_();
    }
  }

private:
  // ---------- Hardware ----------
  int tft_cs_, touch_cs_, touch_irq_;
  TFT_eSPI tft_;
  XPT2046_Touchscreen ts_;

  // ---------- Layout ----------
  Rect screen_{}, grid_{};
  int cols_{3}, rows_{2};
  std::vector<Rect> cell_cache_; // per col/row (single cell rects)
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
    return {grid_.x + c*cw, grid_.y + r*ch, cw*cs, ch*rs};
  }

  void draw_grid_frame_(){
    // Outer border
    tft_.drawRect(grid_.x, grid_.y, grid_.w, grid_.h, TFT_DARKGREY);
    // Inner cell lines
    const int cw = grid_.w / cols_;
    const int ch = grid_.h / rows_;
    for (int c=1;c<cols_;++c) {
      int x = grid_.x + c*cw;
      tft_.drawLine(x, grid_.y, x, grid_.y + grid_.h, TFT_DARKGREY);
    }
    for (int r=1;r<rows_;++r) {
      int y = grid_.y + r*ch;
      tft_.drawLine(grid_.x, y, grid_.x + grid_.w, y, TFT_DARKGREY);
    }
  }

  // ---------- Page helpers ----------
  void set_page_(int p){
    current_page_ = (p % (max_page_()+1));
    // Redraw grid frame and invalidate visible items for repaint
    draw_grid_frame_();
    invalidate_visible_page_();
  }

  int max_page_() const {
    int m=0; for (auto* it: items_) m = std::max(m, it->Page()); return m;
  }

  void invalidate_visible_page_(){
    for (auto* it : items_) if (it->Page()==current_page_) {
      // Re-set same bounds to flag dirty through SetBounds
      auto itb = last_bounds_.find(it);
      if (itb != last_bounds_.end()) it->SetBounds(itb->second);
    }
  }

  // ---------- Touch handling ----------
  void handle_touch_(int x,int y){
    // Route to topmost item (last added = top)
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
};

// ---------- Optional: convenience factory functions you may use from YAML ----------
// (Not required, but handy if you want to new() items in YAML lambdas)
//
// Example YAML usage:
//   auto p = new touch_panel::Panel(5,4,2,3,2);
//   auto clock = new touch_panel::ClockItem("clock", 0);
//   p->a

}