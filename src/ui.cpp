#include <exotic/ui.hpp>

#include <exotic/draw.hpp>
#include <exotic/window.hpp>

#include <cmath>
#include <cstdio>
#include <unordered_map>
#include <vector>

namespace exo {
namespace {

float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

/// One nesting level of the layout cursor: the window itself, then one per panel.
struct Layout {
    Rect content;
    Vec2 cursor;
    float indent = 0.0f;
    float row_bottom = 0.0f;
    Rect last_item;
    bool has_last = false;
};

struct PanelState {
    Rect rect;
    bool collapsed = false;
    bool placed = false;
};

struct Interaction {
    bool hovered = false;
    bool held = false;
    bool clicked = false;
};

/// Mixed into a panel id to derive the ids of its header and caret.
constexpr Id kHeaderSalt = 0x9E3779B97F4A7C15ULL;
constexpr Id kCaretSalt = 0xC2B2AE3D27D4EB4FULL;

/// Byte index of the code point before / after `at`, so editing never splits a
/// multi-byte character in half.
std::size_t previous_boundary(const std::string& text, std::size_t at) {
    if (at == 0) return 0;
    std::size_t i = at - 1;
    while (i > 0 && (static_cast<unsigned char>(text[i]) & 0xC0) == 0x80) --i;
    return i;
}

std::size_t next_boundary(const std::string& text, std::size_t at) {
    if (at >= text.size()) return text.size();
    std::size_t i = at + 1;
    while (i < text.size() && (static_cast<unsigned char>(text[i]) & 0xC0) == 0x80) ++i;
    return i;
}

} // namespace

struct Ui::Impl {
    Impl(Window& w, Font f, Theme t) : window(w), font(std::move(f)), theme(t) {}

    Window& window;
    Font font;
    Font heading_font;
    Theme theme;

    std::vector<Layout> layouts;
    std::vector<Id> id_stack;
    std::unordered_map<Id, PanelState> panels;

    Id active = 0;
    Id focus = 0;
    Id current_panel = 0;
    /// Topmost panel under the pointer, measured last frame: this is what stops
    /// a click from falling through to whatever is behind a panel.
    Id hovered_panel = 0;
    Id hovered_panel_next = 0;

    bool mouse_over_widget = false;
    bool focus_claimed = false;
    float next_width = 0.0f;

    std::size_t caret = 0;

    // --- plumbing ------------------------------------------------------------

    Layout& layout() { return layouts.back(); }

    DrawList& draw() { return window.draw(); }

    Id id_for(std::string_view text) const { return hash_id(text, id_stack.back()); }

    bool panel_owns_mouse() const { return current_panel == hovered_panel; }

    Rect item_rect(float width, float height) {
        Layout& l = layout();
        const float available = l.content.right() - l.cursor.x;
        float w = next_width > 0.0f ? next_width : (width > 0.0f ? width : available);
        if (w > available) w = available;
        if (w < 1.0f) w = 1.0f;
        next_width = 0.0f;

        const Rect r{l.cursor.x, l.cursor.y, w, height};
        l.last_item = r;
        l.has_last = true;

        const float bottom = r.bottom() > l.row_bottom ? r.bottom() : l.row_bottom;
        l.cursor = {l.content.x + l.indent, bottom + theme.spacing};
        l.row_bottom = 0.0f;
        return r;
    }

    Interaction interact(Id id, Rect r) {
        Interaction out;
        const Vec2 mouse = window.mouse();
        const bool over = panel_owns_mouse() && r.contains(mouse) && draw().clip().contains(mouse);
        if (over) mouse_over_widget = true;

        if (active == id) {
            out.held = true;
            if (!window.mouse_down()) {
                if (over) out.clicked = true;
                active = 0;
            }
        } else if (active == 0 && over && window.mouse_pressed()) {
            active = id;
            out.held = true;
        }
        out.hovered = over && (active == 0 || active == id);
        return out;
    }

    Color control_color(const Interaction& state) const {
        if (state.held) return theme.control_active;
        if (state.hovered) return theme.control_hover;
        return theme.control;
    }

    float text_baseline(Rect row) const {
        return row.y + (row.h - font.line_height()) * 0.5f;
    }

    /// Splits a widget row into "label on the left, control on the right".
    Rect split_label(Rect row, std::string_view text, float reserve_right) {
        const float wanted = text.empty() ? 0.0f : font.measure(text) + theme.spacing;
        const float label_w = wanted > row.w * 0.45f ? row.w * 0.45f : wanted;
        if (!text.empty()) {
            // Only clip when the label actually overflows its column: a clip
            // rectangle costs a batch, and most labels fit.
            const bool overflows = wanted > label_w;
            if (overflows) draw().push_clip({row.x, row.y, label_w, row.h});
            draw().text({row.x, std::round(text_baseline(row))}, text, theme.text, font);
            if (overflows) draw().pop_clip();
        }
        const float right = row.right() - reserve_right;
        return Rect::bounds(row.x + label_w, row.y, right > row.x + label_w ? right : row.x + label_w,
                            row.bottom());
    }

    // --- widgets -------------------------------------------------------------

    bool do_button(std::string_view text, bool accent) {
        const Id id = id_for(text);
        const Rect r = item_rect(font.measure(text) + theme.padding * 2.0f, theme.item_height);
        const Interaction state = interact(id, r);

        DrawList& d = draw();
        Color background;
        if (accent) {
            background = state.held ? theme.accent_active : (state.hovered ? theme.accent_hover : theme.accent);
        } else {
            background = control_color(state);
        }
        d.rect(r, background, theme.rounding);
        if (!accent) d.rect_outline(r, theme.border, theme.border_width, theme.rounding);
        d.text_centered(r, text, accent ? theme.accent_text : theme.text, font);

        if (state.hovered) window.set_cursor(Cursor::Hand);
        return state.clicked;
    }

    bool do_checkbox(std::string_view text, bool& value) {
        const Id id = id_for(text);
        const float box = theme.item_height * 0.6f;
        const Rect r = item_rect(box + theme.spacing + font.measure(text), theme.item_height);
        const Interaction state = interact(id, r);

        const bool changed = state.clicked;
        if (changed) value = !value;

        DrawList& d = draw();
        const Rect mark{r.x, r.y + (r.h - box) * 0.5f, box, box};
        const float rounding = theme.rounding * 0.6f;

        if (value) {
            d.rect(mark, state.hovered ? theme.accent_hover : theme.accent, rounding);
            const float thickness = box * 0.13f;
            const Vec2 a{mark.x + box * 0.24f, mark.y + box * 0.52f};
            const Vec2 b{mark.x + box * 0.43f, mark.y + box * 0.71f};
            const Vec2 c{mark.x + box * 0.77f, mark.y + box * 0.30f};
            d.line(a, b, theme.accent_text, thickness);
            d.line(b, c, theme.accent_text, thickness);
        } else {
            d.rect(mark, control_color(state), rounding);
            d.rect_outline(mark, theme.border, theme.border_width, rounding);
        }

        d.text({mark.right() + theme.spacing, std::round(text_baseline(r))}, text, theme.text, font);
        if (state.hovered) window.set_cursor(Cursor::Hand);
        return changed;
    }

    bool do_radio(std::string_view text, int& value, int option) {
        const Id id = id_for(text);
        const float box = theme.item_height * 0.6f;
        const Rect r = item_rect(box + theme.spacing + font.measure(text), theme.item_height);
        const Interaction state = interact(id, r);

        const bool selected = value == option;
        const bool changed = state.clicked && !selected;
        if (state.clicked) value = option;

        DrawList& d = draw();
        const Vec2 center{r.x + box * 0.5f, r.y + r.h * 0.5f};
        const float radius = box * 0.5f;
        d.circle(center, radius, selected ? (state.hovered ? theme.accent_hover : theme.accent)
                                          : control_color(state));
        if (!selected) d.circle_outline(center, radius, theme.border, theme.border_width);
        if (selected) d.circle(center, radius * 0.4f, theme.accent_text);

        d.text({r.x + box + theme.spacing, std::round(text_baseline(r))}, text, theme.text, font);
        if (state.hovered) window.set_cursor(Cursor::Hand);
        return changed;
    }

    bool do_slider(std::string_view text, float& value, float min, float max, const char* format,
                   bool integral) {
        const Id id = id_for(text);
        const Rect row = item_rect(-1.0f, theme.item_height);

        char readout[64];
        if (integral) {
            std::snprintf(readout, sizeof(readout), format, static_cast<int>(std::lround(value)));
        } else {
            std::snprintf(readout, sizeof(readout), format, static_cast<double>(value));
        }
        const float readout_w = font.measure(readout) + theme.spacing * 2.0f;

        const Rect track_area = split_label(row, text, readout_w);
        const Interaction state = interact(id, track_area);

        const float knob = theme.item_height * 0.28f;
        const float span = track_area.w - knob * 2.0f;
        bool changed = false;

        if (state.held && span > 0.0f && max > min) {
            const float t = clampf((window.mouse().x - track_area.x - knob) / span, 0.0f, 1.0f);
            float next = min + t * (max - min);
            if (integral) next = std::round(next);
            if (next != value) {
                value = next;
                changed = true;
            }
        }
        value = clampf(value, min, max);
        const float t = max > min ? (value - min) / (max - min) : 0.0f;

        DrawList& d = draw();
        const float track_h = theme.item_height * 0.2f;
        const Rect track{track_area.x, track_area.center().y - track_h * 0.5f, track_area.w, track_h};
        d.rect(track, theme.control, track_h * 0.5f);
        if (t > 0.0f) {
            d.rect({track.x, track.y, track.w * t, track.h}, theme.accent, track_h * 0.5f);
        }
        const Vec2 center{track_area.x + knob + span * t, track_area.center().y};
        d.circle(center, knob, state.held || state.hovered ? theme.accent_hover : theme.accent);
        d.circle(center, knob * 0.45f, theme.panel);

        d.text({track_area.right() + theme.spacing, std::round(text_baseline(row))}, readout, theme.text_dim,
               font);

        if (state.hovered || state.held) window.set_cursor(Cursor::ResizeH);
        return changed;
    }

    bool do_input_text(std::string_view text, std::string& value, std::size_t max_length) {
        const Id id = id_for(text);
        const Rect row = item_rect(-1.0f, theme.item_height);
        const Rect box = split_label(row, text, 0.0f);
        const Interaction state = interact(id, box);

        if (state.hovered && window.mouse_pressed()) {
            focus = id;
            caret = value.size();
            focus_claimed = true;
        }

        bool changed = false;
        const bool focused = focus == id;

        if (focused) {
            focus_claimed = true;
            if (caret > value.size()) caret = value.size();

            for (const char32_t cp : window.typed()) {
                std::string encoded;
                if (cp >= 32 && cp < 127) {
                    encoded.push_back(static_cast<char>(cp));
                } else if (cp >= 0xA0 && cp <= 0xFF) {
                    // The atlas covers Latin-1; encode it back to UTF-8.
                    encoded.push_back(static_cast<char>(0xC0 | (cp >> 6)));
                    encoded.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                }
                if (!encoded.empty() && value.size() + encoded.size() <= max_length) {
                    value.insert(caret, encoded);
                    caret += encoded.size();
                    changed = true;
                }
            }

            if (window.key_pressed(Key::Backspace) && caret > 0) {
                const std::size_t from = previous_boundary(value, caret);
                value.erase(from, caret - from);
                caret = from;
                changed = true;
            }
            if (window.key_pressed(Key::Delete) && caret < value.size()) {
                value.erase(caret, next_boundary(value, caret) - caret);
                changed = true;
            }
            if (window.key_pressed(Key::Left)) caret = previous_boundary(value, caret);
            if (window.key_pressed(Key::Right)) caret = next_boundary(value, caret);
            if (window.key_pressed(Key::Home)) caret = 0;
            if (window.key_pressed(Key::End)) caret = value.size();
            if (window.key_pressed(Key::Enter) || window.key_pressed(Key::Escape)) {
                focus = 0;
                focus_claimed = false;
            }
        }

        DrawList& d = draw();
        d.rect(box, focused ? theme.control_active : control_color(state), theme.rounding);
        d.rect_outline(box, focused ? theme.accent : theme.border, theme.border_width, theme.rounding);

        const Rect inner = box.shrunk(theme.spacing);
        const float caret_x = font.measure(std::string_view(value).substr(0, caret));
        const float overflow = focused && caret_x > inner.w ? caret_x - inner.w : 0.0f;

        d.push_clip(inner);
        const Vec2 origin{inner.x - overflow, std::round(text_baseline(row))};
        d.text(origin, value, value.empty() ? theme.text_disabled : theme.text, font);

        // 0.5s on, 0.5s off, restarting whenever the text changes.
        if (focused && (changed || std::fmod(window.time(), 1.0) < 0.5)) {
            d.rect({std::round(origin.x + caret_x), inner.y, std::round(theme.border_width),
                    font.line_height()},
                   theme.text);
        }
        d.pop_clip();

        if (state.hovered) window.set_cursor(Cursor::IBeam);
        return changed;
    }

    void do_progress(float fraction, std::string_view overlay) {
        const Rect r = item_rect(-1.0f, theme.item_height * 0.72f);
        const float t = clampf(fraction, 0.0f, 1.0f);
        const float radius = r.h * 0.5f;

        DrawList& d = draw();
        d.rect(r, theme.control, radius);
        if (t > 0.0f) {
            const float w = r.w * t;
            d.rect({r.x, r.y, w > r.h ? w : r.h, r.h}, theme.accent, radius);
        }
        if (!overlay.empty()) d.text_centered(r, overlay, theme.text, font);
    }

    bool do_begin_panel(std::string_view title, Rect initial) {
        const Id id = hash_id(title);
        PanelState& panel = panels[id];
        if (!panel.placed) {
            panel.rect = initial;
            panel.placed = true;
        }

        // Never let a panel be dragged completely off screen.
        const Vec2 screen = window.size();
        const float margin = theme.header_height * 2.0f;
        panel.rect.x = clampf(panel.rect.x, margin - panel.rect.w, screen.x - margin);
        panel.rect.y = clampf(panel.rect.y, 0.0f, screen.y - theme.header_height);

        current_panel = id;
        if (panel.rect.contains(window.mouse())) hovered_panel_next = id;

        const Rect header{panel.rect.x, panel.rect.y, panel.rect.w, theme.header_height};
        const Rect caret_box{header.x, header.y, theme.header_height, theme.header_height};

        // The caret is tested first, so it claims the press before the header
        // drag can: clicking the arrow collapses instead of moving the panel.
        const Interaction caret_state = interact(id ^ kCaretSalt, caret_box);
        if (caret_state.clicked) panel.collapsed = !panel.collapsed;

        const Interaction header_state = interact(id ^ kHeaderSalt, header);
        if (header_state.held) {
            panel.rect = panel.rect.translated(window.mouse_delta());
        }
        if (caret_state.hovered || header_state.hovered) window.set_cursor(Cursor::Hand);

        DrawList& d = draw();
        const Rect body = panel.collapsed ? header : panel.rect;
        d.rect(body, theme.panel, theme.rounding);
        d.rect(header, theme.panel_header, theme.rounding);
        if (!panel.collapsed) {
            // Square off the bottom of the header so only its top corners round.
            d.rect({header.x, header.y + theme.rounding, header.w, header.h - theme.rounding},
                   theme.panel_header);
        }
        d.rect_outline(body, theme.border, theme.border_width, theme.rounding);

        const Vec2 centre = caret_box.center();
        const float k = theme.header_height * 0.16f;
        const Color arrow = header_state.hovered ? theme.text : theme.text_dim;
        if (panel.collapsed) {
            d.triangle({centre.x - k * 0.6f, centre.y - k}, {centre.x + k * 0.9f, centre.y},
                       {centre.x - k * 0.6f, centre.y + k}, arrow);
        } else {
            d.triangle({centre.x - k, centre.y - k * 0.6f}, {centre.x + k, centre.y - k * 0.6f},
                       {centre.x, centre.y + k * 0.9f}, arrow);
        }
        d.text({caret_box.right(), std::round(header.y + (header.h - font.line_height()) * 0.5f)}, title,
               theme.text, font);

        id_stack.push_back(id);

        Layout inner;
        if (panel.collapsed) {
            // An empty clip discards anything the caller draws anyway, so a
            // forgotten `if (begin_panel(...))` cannot corrupt the frame.
            d.push_clip(Rect{});
        } else {
            const Rect region =
                Rect::bounds(panel.rect.left(), header.bottom(), panel.rect.right(), panel.rect.bottom());
            d.push_clip(region);
            inner.content = region.shrunk(theme.padding);
            inner.cursor = inner.content.pos();
        }
        layouts.push_back(inner);

        return !panel.collapsed;
    }

    void do_end_panel() {
        draw().pop_clip();
        if (layouts.size() > 1) layouts.pop_back();
        if (id_stack.size() > 1) id_stack.pop_back();
        current_panel = 0;
    }
};

// --- Ui: thin forwarders -----------------------------------------------------

Ui::Ui(Window& window, Font font, Theme theme)
    : impl_(std::make_unique<Impl>(window, std::move(font), theme)) {}

Ui::~Ui() = default;

bool Ui::begin_frame() {
    Impl& s = *impl_;
    s.window.set_clear_color(s.theme.background);
    if (!s.window.begin_frame()) return false;

    s.hovered_panel = s.hovered_panel_next;
    s.hovered_panel_next = 0;
    s.mouse_over_widget = false;
    s.focus_claimed = false;
    s.next_width = 0.0f;
    s.current_panel = 0;

    s.id_stack.clear();
    s.id_stack.push_back(kIdSeed);

    Layout root;
    root.content = Rect{0.0f, 0.0f, s.window.size().x, s.window.size().y}.shrunk(s.theme.padding);
    root.cursor = root.content.pos();
    s.layouts.clear();
    s.layouts.push_back(root);

    return true;
}

void Ui::end_frame() {
    Impl& s = *impl_;
    // A click that landed on nothing drops keyboard focus.
    if (s.window.mouse_pressed() && !s.focus_claimed) s.focus = 0;
    s.window.end_frame();
}

void Ui::same_line(float gap) {
    Layout& l = impl_->layout();
    if (!l.has_last) return;
    if (gap < 0.0f) gap = impl_->theme.spacing;
    l.row_bottom = l.row_bottom > l.last_item.bottom() ? l.row_bottom : l.last_item.bottom();
    l.cursor = {l.last_item.right() + gap, l.last_item.y};
}

void Ui::spacing(float pixels) {
    Layout& l = impl_->layout();
    l.cursor.y += pixels < 0.0f ? impl_->theme.spacing : pixels;
}

void Ui::separator() {
    Impl& s = *impl_;
    const Rect r = s.item_rect(-1.0f, s.theme.spacing);
    const float thickness = s.theme.border_width < 1.0f ? 1.0f : s.theme.border_width;
    s.draw().rect({r.x, std::round(r.center().y), r.w, thickness}, s.theme.border);
}

void Ui::indent(float pixels) {
    Layout& l = impl_->layout();
    l.indent += pixels < 0.0f ? impl_->theme.indent_width : pixels;
    l.cursor.x = l.content.x + l.indent;
}

void Ui::unindent(float pixels) {
    Layout& l = impl_->layout();
    l.indent -= pixels < 0.0f ? impl_->theme.indent_width : pixels;
    if (l.indent < 0.0f) l.indent = 0.0f;
    l.cursor.x = l.content.x + l.indent;
}

void Ui::set_next_width(float width) {
    impl_->next_width = width;
}

Vec2 Ui::cursor() const {
    return impl_->layouts.back().cursor;
}

void Ui::set_cursor(Vec2 position) {
    impl_->layout().cursor = position;
}

Rect Ui::content() const {
    return impl_->layouts.back().content;
}

void Ui::push_id(std::string_view text) {
    impl_->id_stack.push_back(hash_id(text, impl_->id_stack.back()));
}

void Ui::push_id(int value) {
    const char* bytes = reinterpret_cast<const char*>(&value);
    impl_->id_stack.push_back(hash_id(std::string_view(bytes, sizeof(value)), impl_->id_stack.back()));
}

void Ui::pop_id() {
    if (impl_->id_stack.size() > 1) impl_->id_stack.pop_back();
}

void Ui::label(std::string_view text) {
    label(text, impl_->theme.text);
}

void Ui::label(std::string_view text, Color color) {
    Impl& s = *impl_;
    const Vec2 size = s.font.measure_block(text);
    const Rect r = s.item_rect(size.x, size.y);
    s.draw().text(r.pos(), text, color, s.font);
}

void Ui::heading(std::string_view text) {
    Impl& s = *impl_;
    const Font& f = s.heading_font.valid() ? s.heading_font : s.font;
    const Vec2 size = f.measure_block(text);
    const Rect r = s.item_rect(size.x, size.y);
    s.draw().text(r.pos(), text, s.theme.text, f);
}

bool Ui::button(std::string_view text) {
    return impl_->do_button(text, false);
}

bool Ui::button_accent(std::string_view text) {
    return impl_->do_button(text, true);
}

bool Ui::checkbox(std::string_view text, bool& value) {
    return impl_->do_checkbox(text, value);
}

bool Ui::radio(std::string_view text, int& value, int option) {
    return impl_->do_radio(text, value, option);
}

bool Ui::slider(std::string_view text, float& value, float min, float max, const char* format) {
    return impl_->do_slider(text, value, min, max, format, false);
}

bool Ui::slider_int(std::string_view text, int& value, int min, int max) {
    float scratch = static_cast<float>(value);
    const bool changed = impl_->do_slider(text, scratch, static_cast<float>(min), static_cast<float>(max),
                                          "%d", true);
    value = static_cast<int>(std::lround(scratch));
    return changed;
}

bool Ui::input_text(std::string_view text, std::string& value, std::size_t max_length) {
    return impl_->do_input_text(text, value, max_length);
}

void Ui::progress_bar(float fraction, std::string_view overlay) {
    impl_->do_progress(fraction, overlay);
}

bool Ui::begin_panel(std::string_view title, Rect initial) {
    return impl_->do_begin_panel(title, initial);
}

void Ui::end_panel() {
    impl_->do_end_panel();
}

Window& Ui::window() {
    return impl_->window;
}

DrawList& Ui::draw() {
    return impl_->draw();
}

const Font& Ui::font() const {
    return impl_->font;
}

Theme& Ui::theme() {
    return impl_->theme;
}

const Theme& Ui::theme() const {
    return impl_->theme;
}

void Ui::set_heading_font(Font font) {
    impl_->heading_font = std::move(font);
}

bool Ui::wants_mouse() const {
    return impl_->mouse_over_widget || impl_->hovered_panel != 0;
}

bool Ui::wants_keyboard() const {
    return impl_->focus != 0;
}

} // namespace exo
