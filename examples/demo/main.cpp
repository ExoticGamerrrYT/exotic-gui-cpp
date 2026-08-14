// The Exotic GUI showcase: every widget, live theme editing, drag and collapse.
//
//   drag a header    move a panel
//   click the arrow  collapse a panel
//   escape           quit

#include <exotic/exotic.hpp>

#include <cstdio>
#include <exception>
#include <string>

int main() {
    try {
        exo::Window window({.title = "Exotic GUI - demo", .width = 1180, .height = 720});

        const float scale = window.dpi_scale();
        exo::Theme theme = exo::Theme::dark();
        theme.scale(scale);

        exo::Font body = exo::Font::system_ui(theme.font_size);
        if (!body.valid()) {
            std::fprintf(stderr, "font: %s\n", body.error().c_str());
            return 1;
        }

        exo::Ui ui(window, std::move(body), theme);
        ui.set_heading_font(exo::Font::system_ui(24.0f * scale));

        // Application state. The library never owns any of this.
        bool vsync = true;
        bool wireframe = false;
        bool notifications = true;
        int quality = 1;
        int light_theme = 0;
        float volume = 0.65f;
        float rounding = theme.rounding;
        int particles = 2500;
        std::string project = "untitled";
        float progress = 0.0f;
        int clicks = 0;
        bool vsync_applied = vsync;

        while (ui.begin_frame()) {
            if (window.key_pressed(exo::Key::Escape) && !ui.wants_keyboard()) window.close();

            progress += window.delta_time() * 0.25f;
            if (progress > 1.0f) progress = 0.0f;

            ui.heading("Exotic GUI");
            ui.label("Immediate-mode widgets on GLFW and OpenGL 3.3.", ui.theme().text_dim);

            // --- controls ----------------------------------------------------
            ui.begin_panel("Controls", {24 * scale, 110 * scale, 400 * scale, 580 * scale});
            {
                ui.label("Buttons", ui.theme().text_dim);
                if (ui.button_accent("Primary")) ++clicks;
                ui.same_line();
                if (ui.button("Secondary")) ++clicks;
                ui.same_line();
                if (ui.button("Reset")) clicks = 0;
                ui.label("clicked " + std::to_string(clicks) + " times", ui.theme().text_dim);

                ui.separator();
                ui.label("Toggles", ui.theme().text_dim);
                ui.checkbox("Vertical sync", vsync);
                ui.checkbox("Wireframe", wireframe);
                ui.checkbox("Notifications", notifications);

                ui.separator();
                ui.label("Quality", ui.theme().text_dim);
                ui.radio("Low", quality, 0);
                ui.same_line();
                ui.radio("Medium", quality, 1);
                ui.same_line();
                ui.radio("High", quality, 2);

                ui.separator();
                ui.label("Values", ui.theme().text_dim);
                ui.slider("Volume", volume, 0.0f, 1.0f);
                ui.slider_int("Particles", particles, 0, 10000);
                ui.input_text("Project", project);

                ui.spacing();
                ui.progress_bar(progress, "loading " + std::to_string(static_cast<int>(progress * 100)) + "%");
            }
            ui.end_panel();

            // --- live theme editing ------------------------------------------
            ui.begin_panel("Appearance", {448 * scale, 120 * scale, 360 * scale, 300 * scale});
            {
                exo::Theme& t = ui.theme();
                ui.label("The theme is a plain struct: edit it and the", t.text_dim);
                ui.label("next frame uses it.", t.text_dim);
                ui.spacing();

                if (ui.radio("Dark", light_theme, 0)) {
                    t = exo::Theme::dark();
                    t.scale(scale);
                    rounding = t.rounding;
                }
                ui.same_line();
                if (ui.radio("Light", light_theme, 1)) {
                    t = exo::Theme::light();
                    t.scale(scale);
                    rounding = t.rounding;
                }

                ui.spacing();
                if (ui.slider("Rounding", rounding, 0.0f, 16.0f * scale, "%.0f")) t.rounding = rounding;

                float red = static_cast<float>(t.accent.r);
                float green = static_cast<float>(t.accent.g);
                float blue = static_cast<float>(t.accent.b);
                bool accent_changed = ui.slider("Accent R", red, 0.0f, 255.0f, "%.0f");
                accent_changed |= ui.slider("Accent G", green, 0.0f, 255.0f, "%.0f");
                accent_changed |= ui.slider("Accent B", blue, 0.0f, 255.0f, "%.0f");
                if (accent_changed) {
                    t.accent = {static_cast<unsigned char>(red), static_cast<unsigned char>(green),
                                static_cast<unsigned char>(blue)};
                    t.accent_hover = exo::Color::lerp(t.accent, exo::Color::white(), 0.18f);
                    t.accent_active = exo::Color::lerp(t.accent, exo::Color::black(), 0.18f);
                }
            }
            ui.end_panel();

            // --- diagnostics --------------------------------------------------
            ui.begin_panel("Diagnostics", {448 * scale, 444 * scale, 360 * scale, 146 * scale});
            {
                const exo::DrawList& list = ui.draw();
                ui.label(exo::build_info(), ui.theme().text_dim);
                ui.label(std::to_string(static_cast<int>(window.fps())) + " fps   " +
                         std::to_string(list.vertices().size()) + " vertices   " +
                         std::to_string(list.commands().size()) + " draw calls");
                ui.label(ui.wants_mouse() ? "pointer: over the interface" : "pointer: free",
                         ui.wants_mouse() ? ui.theme().success : ui.theme().text_dim);
            }
            ui.end_panel();

            // --- state readout ------------------------------------------------
            ui.begin_panel("Your state", {832 * scale, 120 * scale, 320 * scale, 260 * scale});
            {
                ui.label("Nothing below is stored by the library.", ui.theme().text_dim);
                ui.spacing();
                char volume_text[32];
                std::snprintf(volume_text, sizeof(volume_text), "volume     %.2f", volume);
                ui.label("project    " + project);
                ui.label(volume_text);
                ui.label("particles  " + std::to_string(particles));
                ui.label("quality    " + std::string(quality == 0 ? "low" : quality == 1 ? "medium" : "high"));
                ui.label(std::string("vsync      ") + (vsync ? "on" : "off"));
                ui.label(std::string("wireframe  ") + (wireframe ? "on" : "off"));
                ui.label(std::string("notify     ") + (notifications ? "on" : "off"));
            }
            ui.end_panel();

            if (vsync != vsync_applied) {
                vsync_applied = vsync;
                window.set_vsync(vsync);
            }

            ui.end_frame();
        }
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "fatal: %s\n", error.what());
        return 1;
    }
}
