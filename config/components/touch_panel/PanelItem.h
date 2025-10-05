#include <vector>
#include <unordered_map>
#include <functional>
#include <string>
#include <cmath>
#include <algorithm>

#include "esphome.h"
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

#ifndef panelitem_h
#define panelitem_h

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

    // Now receives the panel's shared sprite. Draw into 'spr', then push.
    // Return true if something was drawn.
    virtual bool RenderIfDirty(TFT_eSPI& tft, TFT_eSprite& spr) = 0;

    virtual void Tick(uint32_t now_ms) {}
    virtual bool HitTest(int x, int y) const = 0;

    virtual bool ClearDirty() = 0;
    virtual void OnClick() = 0;
    virtual void SetOnClick(std::function<void()>) = 0;
    virtual void OnEnvUpdate(float /*t*/, float /*h*/) {}
    virtual void OnTimeUpdate(int /*hours*/, int /*minutes*/, int /*seconds*/) {}
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

}

#endif // panelitem_h