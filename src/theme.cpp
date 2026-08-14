#include <exotic/theme.hpp>

namespace exo {

Theme Theme::dark() {
    Theme t;
    t.background = Color::rgb(0x0E1014);
    t.panel = Color::rgb(0x171A21);
    t.panel_header = Color::rgb(0x1E222B);
    t.border = Color::rgb(0x2A2F3A);

    t.text = Color::rgb(0xE6E9EF);
    t.text_dim = Color::rgb(0x8C94A6);
    t.text_disabled = Color::rgb(0x555C6B);

    t.control = Color::rgb(0x232833);
    t.control_hover = Color::rgb(0x2C3340);
    t.control_active = Color::rgb(0x363E4D);

    t.accent = Color::rgb(0x4C8DFF);
    t.accent_hover = Color::rgb(0x6BA1FF);
    t.accent_active = Color::rgb(0x3A78E0);
    t.accent_text = Color::rgb(0xFFFFFF);

    t.success = Color::rgb(0x3DD68C);
    t.warning = Color::rgb(0xFFC55C);
    t.danger = Color::rgb(0xFF6B6B);
    return t;
}

Theme Theme::light() {
    Theme t;
    t.background = Color::rgb(0xF4F5F7);
    t.panel = Color::rgb(0xFFFFFF);
    t.panel_header = Color::rgb(0xEDEFF3);
    t.border = Color::rgb(0xD8DCE3);

    t.text = Color::rgb(0x1A1D23);
    t.text_dim = Color::rgb(0x5C6472);
    t.text_disabled = Color::rgb(0xA2A9B5);

    t.control = Color::rgb(0xE9ECF1);
    t.control_hover = Color::rgb(0xDFE3EA);
    t.control_active = Color::rgb(0xD2D8E1);

    t.accent = Color::rgb(0x2D7FF9);
    t.accent_hover = Color::rgb(0x4A91FA);
    t.accent_active = Color::rgb(0x1F6AD8);
    t.accent_text = Color::rgb(0xFFFFFF);

    t.success = Color::rgb(0x1CA96C);
    t.warning = Color::rgb(0xD9962B);
    t.danger = Color::rgb(0xE0483F);
    return t;
}

void Theme::scale(float factor) {
    if (factor <= 0.0f) return;
    rounding *= factor;
    border_width *= factor;
    padding *= factor;
    spacing *= factor;
    item_height *= factor;
    indent_width *= factor;
    header_height *= factor;
    font_size *= factor;
}

} // namespace exo
