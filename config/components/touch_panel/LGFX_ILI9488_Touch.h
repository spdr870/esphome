#include <unordered_map>
#include <functional>
#include <string>
#include <cmath>
#include <algorithm>
#include <mutex>
#include <atomic>

#include "esphome.h"

#include <LovyanGFX.hpp>

#ifndef LGFX_ILI9488_Touch_h
#define LGFX_ILI9488_Touch_h

namespace touch_panel {

// ---------- Minimal LGFX device for ILI9488 + XPT2046 ----------
class LGFX_ILI9488_Touch : public lgfx::LGFX_Device {
  lgfx::Panel_ILI9488 _panel;
  lgfx::Bus_SPI       _bus;
  lgfx::Touch_XPT2046 _touch;

public:
  LGFX_ILI9488_Touch(int tft_cs, int dc, int rst,
                     int sclk, int mosi, int miso,
                     int touch_cs, int touch_irq,
                     int backlight_pin = -1)
  {
    { // SPI bus config for the panel
      auto cfg = _bus.config();
      cfg.spi_host   = SPI3_HOST;      // HSPI_HOST is also fine if you prefer
      cfg.spi_mode   = 0;
      cfg.freq_write = 40000000;       // tune if needed
      cfg.freq_read  = 16000000;       // ILI9488 read can be slower
      cfg.spi_3wire  = false;
      cfg.use_lock   = true;
      cfg.dma_channel = 1;             // 1 or 2 on ESP32 (or 0 = no DMA)
      cfg.pin_sclk   = sclk;
      cfg.pin_mosi   = mosi;
      cfg.pin_miso   = miso;
      cfg.pin_dc     = dc;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    { // Panel config
      auto cfg = _panel.config();
      cfg.pin_cs   = tft_cs;
      cfg.pin_rst  = rst;              // -1 if not wired
      cfg.pin_busy = -1;

      cfg.memory_width  = 480;
      cfg.memory_height = 320;
      cfg.panel_width   = 480;
      cfg.panel_height  = 320;
      cfg.offset_x = 0;
      cfg.offset_y = 0;

      cfg.readable  = true;
      cfg.invert    = true;            // you used invertDisplay(true)
      cfg.rgb_order = false;           // BGR=false (common for ILI9488)
      cfg.dlen_16bit = false;          // SPI 8-bit
      _panel.config(cfg);
    }
    { // Optional backlight (simple on/off)
      if (backlight_pin >= 0) {
        pinMode(backlight_pin, OUTPUT);
        digitalWrite(backlight_pin, HIGH); // match your TFT_BACKLIGHT_ON
      }
    }
    { // XPT2046 touch on same bus
      auto tc = _touch.config();
      tc.spi_host   = SPI3_HOST;
      tc.freq       = 2000000;         // safe start; can push higher
      tc.pin_sclk   = sclk;
      tc.pin_mosi   = mosi;
      tc.pin_miso   = miso;
      tc.pin_cs     = touch_cs;
      tc.pin_int    = touch_irq;       // -1 if not wired
      tc.bus_shared = true;            // we share the panel bus
      _touch.config(tc);
      _panel.setTouch(&_touch);
    }

    setPanel(&_panel);
  }
};

}

#endif LGFX_ILI9488_Touch_h