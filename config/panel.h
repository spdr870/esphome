#pragma once
#include "esphome.h"
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// ---- eenvoudige UI types ----
struct Rect { int x,y,w,h; };
inline bool hit(const Rect&r,int x,int y){return x>=r.x && x<r.x+r.w && y>=r.y && y<r.y+r.h;}

class Panel : public esphome::Component {
 public:
  // singleton access vanuit YAML-lambdas
  static Panel* instance() { return inst_; }

  Panel(int tft_cs, int touch_cs, int touch_irq)
  : tft_cs_(tft_cs), touch_cs_(touch_cs), touch_irq_(touch_irq),
    ts_(touch_cs_, touch_irq_) {}

  void setup() override {
    using namespace esphome;
    // SPI pins uit YAML/boards.txt: SCK=18, MISO=19, MOSI=23
    SPI.begin(18,19,23);

    pinMode(tft_cs_, OUTPUT); digitalWrite(tft_cs_, HIGH);
    pinMode(touch_cs_, OUTPUT); digitalWrite(touch_cs_, HIGH);
    if (touch_irq_ >= 0) pinMode(touch_irq_, INPUT_PULLUP);

    tft_.init();
    tft_.setRotation(1);
    tft_.setSwapBytes(true);
    tft_.invertDisplay(true);

#ifdef TFT_BL
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
#endif

    ts_.begin();
    ts_.setRotation(0);

    // layout
    status_ = {0,0,480,36};
    draw_   = {0,36,480,284};
    btn_w_ = 60; btn_h_ = 24; gap_ = 6;
    btn_white_ = {240,6,btn_w_,btn_h_};
    btn_red_   = {btn_white_.x+btn_w_+gap_,6,btn_w_,btn_h_};
    btn_blue_  = {btn_red_.x  +btn_w_+gap_,6,btn_w_,btn_h_};
    btn_green_ = {btn_blue_.x +btn_w_+gap_,6,btn_w_,btn_h_};

    current_color_ = 0xFFFF; // WHITE
    color_name_ = "WHITE";
    lx_ = ly_ = -1;
    full_redraw_ = true;

    inst_ = this;
    draw_full_();
  }

  void loop() override {
    // Incremental touch-handling, geen full-screen clears
    int16_t x,y;
    if (read_touch_(x,y)) {
      if (y < status_.h) {
        if      (hit(btn_white_, x,y)) set_color_(0xFFFF, "WHITE");
        else if (hit(btn_red_,   x,y)) set_color_(0xF800, "RED");
        else if (hit(btn_blue_,  x,y)) set_color_(0x001F, "BLUE");
        else if (hit(btn_green_, x,y)) set_color_(0x07E0, "GREEN");
        // kleine status redraw:
        draw_status_();
        lx_ = ly_ = -1;
      } else {
        // teken alleen lijnsegment
        if (lx_ >= 0) {
          tft_.drawLine(lx_, ly_, x, y, current_color_);
        } else {
          tft_.drawPixel(x, y, current_color_);
        }
        lx_ = x; ly_ = y;
      }
    } else {
      lx_ = ly_ = -1;
    }
  }

  // ---- API voor YAML/HA ----
  void clear() {
    draw_full_(); // 1x full redraw toegestaan → geen knipper tijdens tekenen
  }
  const std::string& color_name() const { return color_name_; }
  uint16_t color_rgb565() const { return current_color_; }

 private:
  // pins
  int tft_cs_, touch_cs_, touch_irq_;
  // hw
  TFT_eSPI tft_;
  XPT2046_Touchscreen ts_;

  // layout
  Rect status_, draw_;
  Rect btn_white_, btn_red_, btn_blue_, btn_green_;
  int btn_w_, btn_h_, gap_;

  // state
  int16_t lx_{-1}, ly_{-1};
  uint16_t current_color_{0xFFFF};
  std::string color_name_{"WHITE"};
  bool full_redraw_{true};

  static Panel* inst_;

  void set_color_(uint16_t c, const char* name) {
    current_color_ = c;
    color_name_ = name;
  }

  void draw_button_(const Rect&r, uint16_t fill, uint16_t stroke, const char* label) {
    tft_.fillRoundRect(r.x, r.y, r.w, r.h, 6, fill);
    tft_.drawRoundRect(r.x, r.y, r.w, r.h, 6, stroke);
    tft_.setTextDatum(MC_DATUM);
    tft_.setTextColor(stroke, fill);
    tft_.drawString(label, r.x + r.w/2, r.y + r.h/2, 2);
  }

  void draw_status_() {
    // kleine regio, geen fullscreen clear
    tft_.fillRect(status_.x, status_.y, status_.w, status_.h, TFT_BLACK);
    tft_.setTextColor(TFT_CYAN, TFT_BLACK);
    tft_.setTextDatum(TL_DATUM);
    tft_.drawString("Touch Panel", 8, 8, 2);
    tft_.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft_.drawString(("Color: " + color_name_).c_str(), 360, 8, 2);

    draw_button_(btn_white_, TFT_WHITE, TFT_DARKGREY, "W");
    draw_button_(btn_red_,   TFT_RED,   TFT_BLACK,    "R");
    draw_button_(btn_blue_,  TFT_BLUE,  TFT_BLACK,    "B");
    draw_button_(btn_green_, TFT_GREEN, TFT_BLACK,    "G");
  }

  void draw_full_() {
    tft_.fillScreen(TFT_BLACK);
    tft_.drawRect(draw_.x, draw_.y, draw_.w, draw_.h, TFT_DARKGREY);
    draw_status_();
    lx_ = ly_ = -1;
    full_redraw_ = false;
  }

  bool read_touch_(int16_t &x, int16_t &y) {
    bool pressed = false;
    if (touch_irq_ >= 0) {
      pressed = (digitalRead(touch_irq_) == LOW);
      if (!pressed) return false;
    } else {
      pressed = ts_.touched();
      if (!pressed) return false;
    }
    digitalWrite(tft_cs_, HIGH); // bus vrijmaken voor touch
    TS_Point p = ts_.getPoint();
    if (p.z < 5 || p.z > 4095) return false;

    // calibratie (zoals je Arduino sketch)
    const int RAW_X_MIN=200, RAW_X_MAX=3800;
    const int RAW_Y_MIN=200, RAW_Y_MAX=3800;
    int16_t mx = map(p.y, RAW_Y_MIN, RAW_Y_MAX, 0, 480);
    int16_t my = map(p.x, RAW_X_MIN, RAW_X_MAX, 0, 320);
    x = constrain(mx, 0, 479);
    y = constrain(my, 0, 319);
    return true;
  }
};

// static init
inline Panel* Panel::inst_ = nullptr;
