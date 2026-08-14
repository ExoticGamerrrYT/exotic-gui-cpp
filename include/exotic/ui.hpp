// Exotic GUI - immediate-mode widgets.
//
// There is no widget tree. You call a function, it draws itself and tells you
// what the user did:
//
//     if (ui.button("Save")) save();
//     ui.slider("Volume", volume, 0.0f, 1.0f);
//
// Widget identity comes from the label text, so two widgets with the same
// label inside the same panel share state. push_id() separates them.

#pragma once

#include <exotic/font.hpp>
#include <exotic/theme.hpp>
#include <exotic/types.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace exo {

class DrawList;
class Window;

/// Widget identity: a hash of the label mixed with the enclosing scope.
using Id = std::uint64_t;

inline constexpr Id kIdSeed = 0xcbf29ce484222325ULL;

/// FNV-1a. Stable across runs and platforms, which is what makes widget state
/// survive from frame to frame.
constexpr Id hash_id(std::string_view text, Id seed = kIdSeed) {
    Id hash = seed;
    for (const char c : text) {
        hash ^= static_cast<Id>(static_cast<unsigned char>(c));
        hash *= 0x100000001B3ULL;
    }
    return hash;
}

/// The context you keep alive for the lifetime of your application.
///
///     exo::Window window;
///     exo::Ui ui(window, exo::Font::system_ui(15.0f));
///     while (ui.begin_frame()) {
///         if (ui.button("Quit")) window.close();
///         ui.end_frame();
///     }
class Ui {
public:
    /// Takes ownership of `font`; it must already be valid.
    Ui(Window& window, Font font, Theme theme = Theme::dark());
    ~Ui();

    Ui(const Ui&) = delete;
    Ui& operator=(const Ui&) = delete;

    /// Starts the window's frame and resets layout. False when the user quits.
    bool begin_frame();
    void end_frame();

    // --- layout --------------------------------------------------------------

    /// Puts the next widget to the right of the previous one.
    void same_line(float gap = -1.0f);
    /// Vertical gap. Negative uses Theme::spacing.
    void spacing(float pixels = -1.0f);
    /// Horizontal rule.
    void separator();
    void indent(float pixels = -1.0f);
    void unindent(float pixels = -1.0f);
    /// Width for the next widget only. Negative or zero means "auto".
    void set_next_width(float width);

    Vec2 cursor() const;
    void set_cursor(Vec2 position);
    /// Region the current panel (or the window) lays widgets out in.
    Rect content() const;

    // --- identity ------------------------------------------------------------

    /// Scopes widget ids, so the same label can be used in a loop.
    void push_id(std::string_view text);
    void push_id(int value);
    void pop_id();

    // --- widgets -------------------------------------------------------------

    void label(std::string_view text);
    void label(std::string_view text, Color color);
    /// Larger, full-strength text. Uses the heading font when one is set.
    void heading(std::string_view text);

    bool button(std::string_view text);
    /// Button drawn in the accent colour, for the primary action.
    bool button_accent(std::string_view text);
    bool checkbox(std::string_view text, bool& value);
    bool radio(std::string_view text, int& value, int option);
    bool slider(std::string_view text, float& value, float min, float max, const char* format = "%.2f");
    bool slider_int(std::string_view text, int& value, int min, int max);
    /// Single-line text field. Returns true whenever the text changes.
    bool input_text(std::string_view text, std::string& value, std::size_t max_length = 128);
    void progress_bar(float fraction, std::string_view overlay = {});

    // --- panels --------------------------------------------------------------

    /// Draggable, collapsible panel. `initial` is used the first time only -
    /// after that the position the user dragged it to wins. Returns false when
    /// collapsed: skip the contents, but always call end_panel().
    bool begin_panel(std::string_view title, Rect initial);
    void end_panel();

    // --- access --------------------------------------------------------------

    Window& window();
    DrawList& draw();
    const Font& font() const;
    Theme& theme();
    const Theme& theme() const;
    /// Optional larger font used by heading().
    void set_heading_font(Font font);

    /// True while the pointer is over any widget - useful to stop a 3D scene
    /// underneath from reacting to the same click.
    bool wants_mouse() const;
    /// True while a text field has keyboard focus.
    bool wants_keyboard() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace exo
