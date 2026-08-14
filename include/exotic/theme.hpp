// Exotic GUI - colours and metrics.
//
// A plain struct: change any field at any time and the next frame uses it.

#pragma once

#include <exotic/types.hpp>

namespace exo {

struct Theme {
    // Surfaces.
    Color background;
    Color panel;
    Color panel_header;
    Color border;

    // Text.
    Color text;
    Color text_dim;
    Color text_disabled;

    // Controls.
    Color control;
    Color control_hover;
    Color control_active;

    // Emphasis.
    Color accent;
    Color accent_hover;
    Color accent_active;
    Color accent_text;

    // Status.
    Color success;
    Color warning;
    Color danger;

    // Metrics, in pixels. scale() multiplies all of them for high-DPI screens.
    float rounding = 6.0f;
    float border_width = 1.0f;
    float padding = 12.0f;
    float spacing = 8.0f;
    float item_height = 30.0f;
    float indent_width = 18.0f;
    float header_height = 32.0f;
    float font_size = 15.0f;

    static Theme dark();
    static Theme light();

    /// Multiplies every metric by `factor`. Colours are untouched.
    void scale(float factor);
};

} // namespace exo
