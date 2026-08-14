// Smallest possible Exotic GUI program: open a window, read input, close it.
//
//   left click   cycle the background colour
//   space        toggle vsync
//   escape       quit

#include <exotic/version.hpp>
#include <exotic/window.hpp>

#include <cstdio>
#include <exception>

int main() {
    try {
        exo::WindowDesc desc;
        desc.title = "Exotic GUI - hello window";
        desc.width = 960;
        desc.height = 540;

        exo::Window window(desc);
        std::printf("%s\n", exo::build_info());
        std::printf("window %.0fx%.0f at %.0f%% scale\n", window.size().x, window.size().y,
                    window.dpi_scale() * 100.0f);

        constexpr exo::Color palette[] = {exo::Color::rgb(0x0E1014), exo::Color::rgb(0x1B2A41),
                                          exo::Color::rgb(0x2B1B41), exo::Color::rgb(0x123524)};
        int index = 0;
        bool vsync = true;
        double next_report = 0.0;

        while (window.begin_frame()) {
            if (window.key_pressed(exo::Key::Escape)) window.close();

            if (window.mouse_pressed()) {
                index = (index + 1) % 4;
                window.set_clear_color(palette[index]);
            }

            if (window.key_pressed(exo::Key::Space)) {
                vsync = !vsync;
                window.set_vsync(vsync);
            }

            if (window.time() >= next_report) {
                next_report = window.time() + 1.0;
                std::printf("%6.1f fps | mouse %4.0f,%-4.0f | vsync %s\n", window.fps(), window.mouse().x,
                            window.mouse().y, vsync ? "on" : "off");
            }

            window.end_frame();
        }
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "fatal: %s\n", error.what());
        return 1;
    }
}
