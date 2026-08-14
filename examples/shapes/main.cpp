// Everything the renderer can draw, without a single widget: shapes, text,
// clipping and animation straight on the window's draw list.
//
//   escape   quit

#include <exotic/draw.hpp>
#include <exotic/font.hpp>
#include <exotic/window.hpp>

#include <cmath>
#include <cstdio>
#include <exception>

int main() {
    try {
        exo::WindowDesc desc;
        desc.title = "Exotic GUI - shapes";
        desc.width = 1000;
        desc.height = 620;

        exo::Window window(desc);

        exo::Font font = exo::Font::system_ui(16.0f * window.dpi_scale());
        if (!font.valid()) {
            std::fprintf(stderr, "font: %s\n", font.error().c_str());
            return 1;
        }
        exo::Font heading = exo::Font::system_ui(28.0f * window.dpi_scale());

        const exo::Color ink = exo::Color::rgb(0xE6E9EF);
        const exo::Color dim = exo::Color::rgb(0x7C8494);
        const exo::Color accent = exo::Color::rgb(0x4C8DFF);
        const exo::Color warm = exo::Color::rgb(0xFF8A5B);
        const exo::Color green = exo::Color::rgb(0x3DD68C);

        while (window.begin_frame()) {
            if (window.key_pressed(exo::Key::Escape)) window.close();

            exo::DrawList& d = window.draw();
            const float s = window.dpi_scale();
            const auto t = static_cast<float>(window.time());

            d.text({40 * s, 32 * s}, "Exotic GUI", ink, heading);
            d.text({40 * s, 70 * s}, "shapes, text and clipping - " + std::to_string(static_cast<int>(window.fps())) + " fps",
                   dim, font);

            // Rounded rectangles with growing corner radii.
            for (int i = 0; i < 4; ++i) {
                const auto fi = static_cast<float>(i);
                const exo::Rect r{(40 + fi * 130) * s, 110 * s, 110 * s, 80 * s};
                d.rect(r, exo::Color::lerp(accent, warm, fi / 3.0f), fi * 8.0f * s);
                d.text_centered(r, std::to_string(i * 8) + "px", exo::Color::rgb(0x0E1014), font);
            }

            // Outlines, circles, lines, triangle.
            d.rect_outline({40 * s, 220 * s, 240 * s, 90 * s}, dim, 1.0f * s, 10.0f * s);
            d.text({56 * s, 250 * s}, "rect_outline()", dim, font);

            d.circle({350 * s, 265 * s}, 40 * s, accent);
            d.circle_outline({460 * s, 265 * s}, 40 * s, green, 3.0f * s);
            d.triangle({530 * s, 305 * s}, {590 * s, 225 * s}, {650 * s, 305 * s}, warm);

            for (int i = 0; i < 12; ++i) {
                const float a = t + static_cast<float>(i) * 0.5236f;
                d.line({790 * s, 265 * s},
                       {790 * s + std::cos(a) * 45 * s, 265 * s + std::sin(a) * 45 * s},
                       exo::Color::lerp(accent, warm, static_cast<float>(i) / 11.0f), 2.0f * s);
            }

            // Clipping: the circle is drawn far larger than the box that keeps it.
            const exo::Rect box{40 * s, 350 * s, 300 * s, 200 * s};
            d.rect(box, exo::Color::rgb(0x191D24), 12 * s);
            d.push_clip(box.shrunk(1.0f * s));
            d.circle({box.center().x + std::cos(t) * 90 * s, box.center().y + std::sin(t * 0.7f) * 60 * s},
                     70 * s, accent.with_alpha(200));
            d.text({box.x + 14 * s, box.y + 12 * s}, "push_clip() keeps it inside", ink, font);
            d.pop_clip();

            // Text, including the Latin-1 range of the atlas.
            d.text({380 * s, 360 * s},
                   "Immediate mode: describe the frame,\n"
                   "the library batches it.\n\n"
                   "Latin-1 is in the atlas too:\n"
                   "áéíóú ñ ¿cómo? ¡así! Ç Ü ß",
                   ink, font);

            const std::size_t vertices = d.vertices().size();
            const std::size_t batches = d.commands().size();
            d.text({40 * s, 575 * s},
                   std::to_string(vertices) + " vertices in " + std::to_string(batches) + " draw calls", dim,
                   font);

            window.end_frame();
        }
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "fatal: %s\n", error.what());
        return 1;
    }
}
