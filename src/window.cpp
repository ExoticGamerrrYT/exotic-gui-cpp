#include <exotic/window.hpp>

#include <exotic/draw.hpp>

#include "gl.hpp"
#include "renderer.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <array>
#include <cstdint>
#include <stdexcept>

namespace exo {
namespace {

constexpr int kKeyCount = GLFW_KEY_LAST + 1;
constexpr int kButtonCount = 3;
constexpr int kCursorCount = 6;

// Key values are GLFW codes so that lookups are plain array indexing. If GLFW
// ever renumbers, this stops the build instead of silently mixing up keys.
static_assert(static_cast<int>(Key::Space) == GLFW_KEY_SPACE);
static_assert(static_cast<int>(Key::Num9) == GLFW_KEY_9);
static_assert(static_cast<int>(Key::Z) == GLFW_KEY_Z);
static_assert(static_cast<int>(Key::Backspace) == GLFW_KEY_BACKSPACE);
static_assert(static_cast<int>(Key::F12) == GLFW_KEY_F12);
static_assert(static_cast<int>(Key::RightAlt) == GLFW_KEY_RIGHT_ALT);
static_assert(static_cast<int>(Key::RightAlt) < kKeyCount);
static_assert(static_cast<int>(MouseButton::Middle) == GLFW_MOUSE_BUTTON_MIDDLE);

int g_live_windows = 0;
std::string g_last_glfw_error;

void on_glfw_error(int code, const char* description) {
    g_last_glfw_error = description ? description : "unknown error";
    g_last_glfw_error += " (" + std::to_string(code) + ")";
}

int to_glfw_cursor(Cursor cursor) {
    switch (cursor) {
    case Cursor::IBeam: return GLFW_IBEAM_CURSOR;
    case Cursor::Hand: return GLFW_POINTING_HAND_CURSOR;
    case Cursor::ResizeH: return GLFW_RESIZE_EW_CURSOR;
    case Cursor::ResizeV: return GLFW_RESIZE_NS_CURSOR;
    case Cursor::ResizeAll: return GLFW_RESIZE_ALL_CURSOR;
    case Cursor::Arrow: break;
    }
    return GLFW_ARROW_CURSOR;
}

float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

} // namespace

struct Window::Impl {
    GLFWwindow* handle = nullptr;
    WindowDesc desc;

    Vec2 fb_size;
    float scale = 1.0f;

    double last_time = 0.0;
    float dt = 1.0f / 60.0f;
    float fps = 60.0f;

    Vec2 mouse;
    Vec2 mouse_previous;
    Vec2 mouse_delta;
    bool mouse_seen = false;
    float scroll = 0.0f;

    std::array<std::uint8_t, kKeyCount> key_is_down{};
    std::array<std::uint8_t, kKeyCount> key_went_down{};
    std::array<std::uint8_t, kButtonCount> button_is_down{};
    std::array<std::uint8_t, kButtonCount> button_went_down{};
    std::array<std::uint8_t, kButtonCount> button_went_up{};
    std::u32string typed;

    std::array<GLFWcursor*, kCursorCount> cursors{};
    Cursor cursor = Cursor::Arrow;

    DrawList draw_list;
    std::unique_ptr<Renderer> renderer;

    static Impl& of(GLFWwindow* w) { return *static_cast<Impl*>(glfwGetWindowUserPointer(w)); }
};

Window::Window(const WindowDesc& desc) : impl_(std::make_unique<Impl>()) {
    Impl& self = *impl_;
    self.desc = desc;

    if (g_live_windows == 0) {
        glfwSetErrorCallback(on_glfw_error);
        if (!glfwInit()) throw std::runtime_error("exo::Window: glfwInit failed: " + g_last_glfw_error);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, desc.msaa_samples);
    glfwWindowHint(GLFW_RESIZABLE, desc.resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_MAXIMIZED, desc.maximized ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

    self.handle = glfwCreateWindow(desc.width, desc.height, desc.title.c_str(), nullptr, nullptr);
    if (!self.handle) {
        if (g_live_windows == 0) glfwTerminate();
        throw std::runtime_error("exo::Window: could not create window: " + g_last_glfw_error +
                                 ". An OpenGL 3.3 capable driver is required.");
    }
    ++g_live_windows;

    glfwMakeContextCurrent(self.handle);
    glfwSwapInterval(desc.vsync ? 1 : 0);

    const char* missing = nullptr;
    const auto loader = [](const char* name) -> void* {
        return reinterpret_cast<void*>(glfwGetProcAddress(name));
    };
    if (!gl::load(loader, &missing)) {
        throw std::runtime_error(std::string("exo::Window: OpenGL 3.3 entry point unavailable: ") +
                                 (missing ? missing : "?"));
    }

    self.renderer = std::make_unique<Renderer>();
    if (!self.renderer->valid()) {
        throw std::runtime_error("exo::Window: renderer setup failed: " + self.renderer->error());
    }

    for (int i = 0; i < kCursorCount; ++i) {
        self.cursors[static_cast<std::size_t>(i)] =
            glfwCreateStandardCursor(to_glfw_cursor(static_cast<Cursor>(i)));
    }

    glfwSetWindowUserPointer(self.handle, &self);

    glfwSetKeyCallback(self.handle, [](GLFWwindow* w, int key, int, int action, int) {
        if (key < 0 || key >= kKeyCount) return;
        Impl& s = Impl::of(w);
        const auto index = static_cast<std::size_t>(key);
        s.key_is_down[index] = action != GLFW_RELEASE;
        if (action != GLFW_RELEASE) s.key_went_down[index] = 1;
    });

    glfwSetCharCallback(self.handle, [](GLFWwindow* w, unsigned int codepoint) {
        Impl::of(w).typed.push_back(static_cast<char32_t>(codepoint));
    });

    glfwSetMouseButtonCallback(self.handle, [](GLFWwindow* w, int button, int action, int) {
        if (button < 0 || button >= kButtonCount) return;
        Impl& s = Impl::of(w);
        const auto index = static_cast<std::size_t>(button);
        if (action == GLFW_PRESS) {
            s.button_is_down[index] = 1;
            s.button_went_down[index] = 1;
        } else if (action == GLFW_RELEASE) {
            s.button_is_down[index] = 0;
            s.button_went_up[index] = 1;
        }
    });

    glfwSetScrollCallback(self.handle,
                          [](GLFWwindow* w, double, double y) { Impl::of(w).scroll += static_cast<float>(y); });

    int fw = 0;
    int fh = 0;
    glfwGetFramebufferSize(self.handle, &fw, &fh);
    self.fb_size = {static_cast<float>(fw), static_cast<float>(fh)};

    float sx = 1.0f;
    float sy = 1.0f;
    glfwGetWindowContentScale(self.handle, &sx, &sy);
    self.scale = sx > 0.0f ? sx : 1.0f;

    self.last_time = glfwGetTime();

    if (desc.msaa_samples > 0) gl::glEnable(gl::GL_MULTISAMPLE);
    gl::glDisable(gl::GL_DEPTH_TEST);
    gl::glDisable(gl::GL_CULL_FACE);
    gl::glEnable(gl::GL_BLEND);
    gl::glBlendFuncSeparate(gl::GL_SRC_ALPHA, gl::GL_ONE_MINUS_SRC_ALPHA, gl::GL_ONE,
                            gl::GL_ONE_MINUS_SRC_ALPHA);
}

Window::~Window() {
    Impl& self = *impl_;
    for (GLFWcursor* cursor : self.cursors) {
        if (cursor) glfwDestroyCursor(cursor);
    }
    if (self.handle) {
        glfwDestroyWindow(self.handle);
        if (--g_live_windows == 0) glfwTerminate();
    }
}

bool Window::begin_frame() {
    Impl& self = *impl_;

    // Per-frame edges are cleared before polling so that the callbacks below
    // fill them in for exactly one frame.
    self.typed.clear();
    self.scroll = 0.0f;
    self.key_went_down.fill(0);
    self.button_went_down.fill(0);
    self.button_went_up.fill(0);

    glfwPollEvents();

    if (glfwWindowShouldClose(self.handle)) return false;

    const double now = glfwGetTime();
    self.dt = clampf(static_cast<float>(now - self.last_time), 0.0f, 0.1f);
    self.last_time = now;
    if (self.dt > 0.0f) self.fps += (1.0f / self.dt - self.fps) * 0.1f;

    int fw = 0;
    int fh = 0;
    int ww = 0;
    int wh = 0;
    glfwGetFramebufferSize(self.handle, &fw, &fh);
    glfwGetWindowSize(self.handle, &ww, &wh);
    self.fb_size = {static_cast<float>(fw), static_cast<float>(fh)};

    float sx = 1.0f;
    float sy = 1.0f;
    glfwGetWindowContentScale(self.handle, &sx, &sy);
    self.scale = sx > 0.0f ? sx : 1.0f;

    // The cursor is reported in window points; everything else in this library
    // works in framebuffer pixels. On Windows the ratio is 1.
    double mx = 0.0;
    double my = 0.0;
    glfwGetCursorPos(self.handle, &mx, &my);
    const float px = ww > 0 ? static_cast<float>(fw) / static_cast<float>(ww) : 1.0f;
    const float py = wh > 0 ? static_cast<float>(fh) / static_cast<float>(wh) : 1.0f;
    self.mouse = {static_cast<float>(mx) * px, static_cast<float>(my) * py};
    self.mouse_delta = self.mouse_seen ? self.mouse - self.mouse_previous : Vec2{};
    self.mouse_previous = self.mouse;
    self.mouse_seen = true;

    set_cursor(Cursor::Arrow);

    gl::glViewport(0, 0, fw, fh);
    const Color c = self.desc.clear_color;
    gl::glClearColor(static_cast<float>(c.r) / 255.0f, static_cast<float>(c.g) / 255.0f,
                     static_cast<float>(c.b) / 255.0f, static_cast<float>(c.a) / 255.0f);
    gl::glClear(gl::GL_COLOR_BUFFER_BIT);

    self.draw_list.reset({0.0f, 0.0f, self.fb_size.x, self.fb_size.y});

    return true;
}

void Window::end_frame() {
    Impl& self = *impl_;
    self.renderer->render(self.draw_list, self.fb_size);
    glfwSwapBuffers(self.handle);
}

DrawList& Window::draw() {
    return impl_->draw_list;
}

void Window::close() {
    glfwSetWindowShouldClose(impl_->handle, GLFW_TRUE);
}

bool Window::should_close() const {
    return glfwWindowShouldClose(impl_->handle) != 0;
}

void Window::set_title(std::string_view title) {
    impl_->desc.title = title;
    glfwSetWindowTitle(impl_->handle, impl_->desc.title.c_str());
}

void Window::set_clear_color(Color color) {
    impl_->desc.clear_color = color;
}

void Window::set_vsync(bool enabled) {
    impl_->desc.vsync = enabled;
    glfwSwapInterval(enabled ? 1 : 0);
}

Vec2 Window::size() const {
    return impl_->fb_size;
}

float Window::dpi_scale() const {
    return impl_->scale;
}

float Window::delta_time() const {
    return impl_->dt;
}

double Window::time() const {
    return glfwGetTime();
}

float Window::fps() const {
    return impl_->fps;
}

Vec2 Window::mouse() const {
    return impl_->mouse;
}

Vec2 Window::mouse_delta() const {
    return impl_->mouse_delta;
}

float Window::scroll() const {
    return impl_->scroll;
}

bool Window::mouse_down(MouseButton button) const {
    return impl_->button_is_down[static_cast<std::size_t>(button)] != 0;
}

bool Window::mouse_pressed(MouseButton button) const {
    return impl_->button_went_down[static_cast<std::size_t>(button)] != 0;
}

bool Window::mouse_released(MouseButton button) const {
    return impl_->button_went_up[static_cast<std::size_t>(button)] != 0;
}

bool Window::key_down(Key key) const {
    const int index = static_cast<int>(key);
    return index >= 0 && index < kKeyCount && impl_->key_is_down[static_cast<std::size_t>(index)] != 0;
}

bool Window::key_pressed(Key key) const {
    const int index = static_cast<int>(key);
    return index >= 0 && index < kKeyCount && impl_->key_went_down[static_cast<std::size_t>(index)] != 0;
}

bool Window::shift() const {
    return key_down(Key::LeftShift) || key_down(Key::RightShift);
}

bool Window::ctrl() const {
    return key_down(Key::LeftControl) || key_down(Key::RightControl);
}

bool Window::alt() const {
    return key_down(Key::LeftAlt) || key_down(Key::RightAlt);
}

const std::u32string& Window::typed() const {
    return impl_->typed;
}

void Window::set_cursor(Cursor cursor) {
    Impl& self = *impl_;
    const auto index = static_cast<std::size_t>(cursor);
    if (self.cursor == cursor || index >= self.cursors.size()) return;
    self.cursor = cursor;
    glfwSetCursor(self.handle, self.cursors[index]);
}

void* Window::native_handle() const {
    return impl_->handle;
}

} // namespace exo
