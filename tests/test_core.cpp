// Exotic GUI test suite.
//
// No framework on purpose: CHECK records a failure and keeps going, main()
// returns the failure count. Works identically in Debug and Release (unlike
// assert, which vanishes under NDEBUG).

#include <exotic/draw.hpp>
#include <exotic/types.hpp>
#include <exotic/version.hpp>

#include <cstdio>
#include <cstring>

namespace {

int g_checks = 0;
int g_failures = 0;

#define CHECK(expr)                                                                                          \
    do {                                                                                                     \
        ++g_checks;                                                                                          \
        if (!(expr)) {                                                                                       \
            std::printf("  FAIL  %s:%d  %s\n", __FILE__, __LINE__, #expr);                                   \
            ++g_failures;                                                                                    \
        }                                                                                                    \
    } while (false)

bool near(float a, float b, float eps = 0.001f) {
    const float d = a - b;
    return (d < 0.0f ? -d : d) <= eps;
}

void test_vec2() {
    constexpr exo::Vec2 a{3.0f, 4.0f};
    constexpr exo::Vec2 b{1.0f, 2.0f};

    static_assert(a + b == exo::Vec2{4.0f, 6.0f});
    static_assert(a - b == exo::Vec2{2.0f, 2.0f});
    static_assert(a * 2.0f == exo::Vec2{6.0f, 8.0f});
    static_assert(2.0f * a == a * 2.0f);
    static_assert(-a == exo::Vec2{-3.0f, -4.0f});

    exo::Vec2 c{0.0f, 0.0f};
    c += a;
    c -= b;
    CHECK(c == exo::Vec2(2.0f, 2.0f));
}

void test_rect() {
    constexpr exo::Rect r{10.0f, 20.0f, 100.0f, 50.0f};

    static_assert(r.right() == 110.0f);
    static_assert(r.bottom() == 70.0f);
    static_assert(r.center() == exo::Vec2{60.0f, 45.0f});

    // Half-open on the far edges, so adjacent rects never both claim a pixel.
    CHECK(r.contains({10.0f, 20.0f}));
    CHECK(r.contains({109.9f, 69.9f}));
    CHECK(!r.contains({110.0f, 45.0f}));
    CHECK(!r.contains({60.0f, 70.0f}));
    CHECK(!r.contains({9.9f, 45.0f}));

    const exo::Rect grown = r.expanded(5.0f);
    CHECK(near(grown.x, 5.0f));
    CHECK(near(grown.w, 110.0f));
    CHECK(near(r.shrunk(5.0f).w, 90.0f));

    // Overlap.
    const exo::Rect hit = r.intersected({50.0f, 0.0f, 100.0f, 100.0f});
    CHECK(near(hit.x, 50.0f));
    CHECK(near(hit.w, 60.0f));
    CHECK(near(hit.y, 20.0f));
    CHECK(near(hit.h, 50.0f));

    // Disjoint rectangles must come back empty, never negative-sized: the
    // renderer feeds these straight into glScissor.
    const exo::Rect miss = r.intersected({500.0f, 500.0f, 10.0f, 10.0f});
    CHECK(miss.empty());
    CHECK(miss.w >= 0.0f && miss.h >= 0.0f);

    CHECK(exo::Rect::bounds(1.0f, 2.0f, 5.0f, 8.0f).w == 4.0f);
    CHECK(exo::Rect().empty());
}

void test_color() {
    static_assert(exo::Color::rgb(0x2D7FF9) == exo::Color(0x2D, 0x7F, 0xF9, 255));
    static_assert(exo::Color::rgba(0x2D7FF980) == exo::Color(0x2D, 0x7F, 0xF9, 0x80));

    // The renderer uploads this straight as 4 normalised bytes: R,G,B,A.
    constexpr exo::Color c{0x11, 0x22, 0x33, 0x44};
    static_assert(c.packed() == 0x44332211u);

    CHECK(exo::Color::white().packed() == 0xFFFFFFFFu);
    CHECK(c.with_alpha(0xFF).a == 0xFF);
    CHECK(exo::Color(200, 200, 200, 100).faded(0.5f).a == 50);
    CHECK(exo::Color(255, 255, 255, 255).faded(2.0f).a == 255);

    const exo::Color mid = exo::Color::lerp(exo::Color::black(), exo::Color::white(), 0.5f);
    CHECK(mid.r == 128 && mid.g == 128 && mid.b == 128);
    CHECK(exo::Color::lerp(exo::Color::black(), exo::Color::white(), -1.0f) == exo::Color::black());
    CHECK(exo::Color::lerp(exo::Color::black(), exo::Color::white(), 9.0f) == exo::Color::white());
}

void test_version() {
    CHECK(std::strcmp(exo::version(), EXOTIC_VERSION_STRING) == 0);
    CHECK(EXOTIC_VERSION == EXOTIC_VERSION_MAJOR * 10000 + EXOTIC_VERSION_MINOR * 100 + EXOTIC_VERSION_PATCH);
    CHECK(std::strlen(exo::build_info()) > 0);
}

// The draw list is pure geometry, so all of this runs without a window.
constexpr exo::Rect kViewport{0.0f, 0.0f, 800.0f, 600.0f};

void test_draw_batching() {
    exo::DrawList list;
    list.reset(kViewport);
    CHECK(list.empty());
    CHECK(list.commands().empty());

    list.rect({10.0f, 10.0f, 100.0f, 50.0f}, exo::Color::white());
    CHECK(list.vertices().size() == 4);
    CHECK(list.indices().size() == 6);
    CHECK(list.commands().size() == 1);
    CHECK(list.commands()[0].index_count == 6);
    CHECK(list.commands()[0].texture == 0);

    // Same clip and texture: must extend the batch, not open a new one.
    list.rect({20.0f, 20.0f, 10.0f, 10.0f}, exo::Color::black());
    CHECK(list.commands().size() == 1);
    CHECK(list.commands()[0].index_count == 12);

    // A different clip has to break the batch.
    list.push_clip({0.0f, 0.0f, 50.0f, 50.0f});
    list.rect({0.0f, 0.0f, 10.0f, 10.0f}, exo::Color::white());
    CHECK(list.commands().size() == 2);
    CHECK(list.commands()[1].index_offset == 12);
    CHECK(list.commands()[1].clip == exo::Rect(0.0f, 0.0f, 50.0f, 50.0f));

    list.pop_clip();
    CHECK(list.clip() == kViewport);

    const std::size_t before = list.vertices().size();
    list.reset(kViewport);
    CHECK(before > 0 && list.empty());
    CHECK(list.vertices().empty() && list.commands().empty());
}

void test_draw_culling() {
    exo::DrawList list;
    list.reset(kViewport);

    // Nothing invisible should ever reach the GPU.
    list.rect({0.0f, 0.0f, 0.0f, 50.0f}, exo::Color::white());
    list.rect({0.0f, 0.0f, 10.0f, -5.0f}, exo::Color::white());
    list.rect({0.0f, 0.0f, 10.0f, 10.0f}, exo::Color::white().with_alpha(0));
    list.line({0.0f, 0.0f}, {0.0f, 0.0f}, exo::Color::white(), 1.0f);
    list.circle({5.0f, 5.0f}, 0.0f, exo::Color::white());
    CHECK(list.empty());

    // Clipped down to nothing: still nothing.
    list.push_clip({-100.0f, -100.0f, 10.0f, 10.0f});
    CHECK(list.clip().empty());
    list.rect({0.0f, 0.0f, 100.0f, 100.0f}, exo::Color::white());
    CHECK(list.empty());
    list.pop_clip();

    list.rect({0.0f, 0.0f, 100.0f, 100.0f}, exo::Color::white());
    CHECK(!list.empty());
}

void test_draw_shapes() {
    exo::DrawList list;
    list.reset(kViewport);

    // Rounded corners fan from the path, so more vertices than a plain quad.
    list.rect({0.0f, 0.0f, 100.0f, 100.0f}, exo::Color::white(), 12.0f);
    CHECK(list.vertices().size() > 4);
    CHECK(list.indices().size() == (list.vertices().size() - 2) * 3);

    list.reset(kViewport);
    // A polyline is one quad per segment; closed adds the segment back to the start.
    const exo::Vec2 points[] = {{0.0f, 0.0f}, {10.0f, 0.0f}, {10.0f, 10.0f}};
    list.polyline(points, exo::Color::white(), 2.0f, false);
    CHECK(list.indices().size() == 2 * 6);
    list.reset(kViewport);
    list.polyline(points, exo::Color::white(), 2.0f, true);
    CHECK(list.indices().size() == 3 * 6);

    // Clip stack nests by intersection.
    list.reset(kViewport);
    list.push_clip({100.0f, 100.0f, 200.0f, 200.0f});
    list.push_clip({0.0f, 0.0f, 150.0f, 150.0f});
    CHECK(list.clip() == exo::Rect(100.0f, 100.0f, 50.0f, 50.0f));
    list.pop_clip();
    CHECK(list.clip() == exo::Rect(100.0f, 100.0f, 200.0f, 200.0f));
    list.pop_clip();
    CHECK(list.clip() == kViewport);
    // Unbalanced pops must not underflow.
    list.pop_clip();
    CHECK(list.clip() == kViewport);
}

} // namespace

int main() {
    std::printf("%s\n", exo::build_info());

    test_vec2();
    test_rect();
    test_color();
    test_version();
    test_draw_batching();
    test_draw_culling();
    test_draw_shapes();

    std::printf("%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
