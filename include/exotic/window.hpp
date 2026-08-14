// Exotic GUI - window, OpenGL context and input.
//
// Backed by GLFW, so this library contains no raw Win32 code. Everything is
// measured in framebuffer pixels; use dpi_scale() to size your content on
// high-DPI displays.

#pragma once

#include <exotic/types.hpp>

#include <memory>
#include <string>
#include <string_view>

namespace exo {

/// Mouse buttons, in GLFW order.
enum class MouseButton : int { Left = 0, Right = 1, Middle = 2 };

/// Shape of the mouse pointer.
enum class Cursor : int { Arrow, IBeam, Hand, ResizeH, ResizeV, ResizeAll };

/// Keyboard keys. The values are GLFW key codes on purpose: the input tables
/// are indexed directly by them, which keeps the hot path a plain array lookup.
enum class Key : int {
    Space = 32,
    Apostrophe = 39,
    Comma = 44,
    Minus = 45,
    Period = 46,
    Slash = 47,
    Num0 = 48, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    Semicolon = 59,
    Equal = 61,
    A = 65, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    LeftBracket = 91,
    Backslash = 92,
    RightBracket = 93,
    Escape = 256,
    Enter = 257,
    Tab = 258,
    Backspace = 259,
    Insert = 260,
    Delete = 261,
    Right = 262,
    Left = 263,
    Down = 264,
    Up = 265,
    PageUp = 266,
    PageDown = 267,
    Home = 268,
    End = 269,
    CapsLock = 280,
    F1 = 290, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    KeypadEnter = 335,
    LeftShift = 340,
    LeftControl = 341,
    LeftAlt = 342,
    LeftSuper = 343,
    RightShift = 344,
    RightControl = 345,
    RightAlt = 346,
};

/// Everything you can decide before the window exists.
struct WindowDesc {
    std::string title = "Exotic GUI";
    int width = 1280;
    int height = 720;
    bool vsync = true;
    bool resizable = true;
    bool maximized = false;
    /// 0 disables multisampling; 4 keeps rounded corners smooth.
    int msaa_samples = 4;
    Color clear_color = Color::rgb(0x0E1014);
};

/// A window, its OpenGL context and one frame of input state.
///
///     exo::Window window;
///     while (window.begin_frame()) {
///         window.end_frame();
///     }
class Window {
public:
    explicit Window(const WindowDesc& desc = {});
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    /// Polls input, clears the framebuffer, starts a frame.
    /// Returns false once the user has asked to close the window.
    bool begin_frame();

    /// Presents the frame.
    void end_frame();

    void close();
    bool should_close() const;
    void set_title(std::string_view title);
    void set_clear_color(Color color);
    void set_vsync(bool enabled);

    /// Framebuffer size in pixels - the coordinate system of every draw call.
    Vec2 size() const;
    /// Ratio between pixels and OS-reported points (1.0, 1.25, 1.5, 2.0, ...).
    float dpi_scale() const;
    /// Seconds since the previous frame, clamped to a sane maximum.
    float delta_time() const;
    /// Seconds since the window was created.
    double time() const;
    /// Smoothed frames per second.
    float fps() const;

    Vec2 mouse() const;
    Vec2 mouse_delta() const;
    /// Vertical wheel movement during this frame, in notches.
    float scroll() const;
    bool mouse_down(MouseButton button = MouseButton::Left) const;
    bool mouse_pressed(MouseButton button = MouseButton::Left) const;
    bool mouse_released(MouseButton button = MouseButton::Left) const;

    bool key_down(Key key) const;
    /// True on the frame a key goes down, and again while it auto-repeats.
    bool key_pressed(Key key) const;
    bool shift() const;
    bool ctrl() const;
    bool alt() const;
    /// Unicode code points typed this frame, already filtered for control keys.
    const std::u32string& typed() const;

    /// Applied for the current frame only; resets to Arrow on the next one.
    void set_cursor(Cursor cursor);

    /// The underlying GLFWwindow*, for anything this API does not cover.
    void* native_handle() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace exo
