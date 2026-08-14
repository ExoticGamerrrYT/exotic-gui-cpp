# Changelog

All notable changes to this project are documented here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).
Before 1.0, the minor version may break the API; the patch version never will.

## [Unreleased]

## [0.1.0] - 2026-08-14

First release. Everything below is new.

### Added

**Windowing and input**

- `exo::Window`: RAII window with an OpenGL 3.3 core context, backed by GLFW so
  the library contains no raw Win32 code. GLFW is hidden behind a pimpl and
  never appears in a public header.
- Frame loop: `begin_frame()` polls, times, resizes and clears; `end_frame()`
  renders and presents.
- Mouse position, delta and wheel; press and release edges collected in
  callbacks so fast clicks are never dropped; key down/pressed with modifier
  helpers; typed Unicode code points; the standard cursor shapes.
- High-DPI aware through `dpi_scale()`.

**Rendering**

- `exo::DrawList`: rounded rectangles, outlines, lines, circles, triangles,
  convex polygons, polylines, text, and a path API for everything else. Pure
  CPU geometry, so it is unit tested without a window.
- Nested clip stack that maps onto `glScissor`.
- Batching: draw calls merge while the clip rectangle and texture are unchanged.
- OpenGL 3.3 backend with one VAO, one streamed vertex buffer and one shader.
  Untextured geometry samples a 1x1 white texture so shapes and glyphs share
  the same pipeline.
- A ~45-entry OpenGL loader, so there is no dependency on glad, GLEW, OpenGL
  headers or an import library.

**Text**

- `exo::Font`: a TrueType face packed into a single 8-bit atlas with
  stb_truetype, 2x2 oversampled, covering ASCII and Latin-1.
- Metrics, `measure()`, `measure_block()`, and `system_ui()` to pick a face off
  the machine.

**Widgets**

- `exo::Ui` with `label`, `heading`, `button`, `button_accent`, `checkbox`,
  `radio`, `slider`, `slider_int`, `input_text`, `progress_bar`, `separator`.
- Draggable, collapsible panels that remember their position.
- Cursor-based layout: `same_line`, `spacing`, `indent`/`unindent`,
  `set_next_width`.
- `push_id`/`pop_id` to scope repeated labels inside loops.
- Clicks cannot fall through a panel onto what is behind it.
- Text editing keeps the caret on UTF-8 boundaries.
- `wants_mouse()` / `wants_keyboard()` so the host application can ignore input
  the interface already consumed.

**Theming**

- `exo::Theme`: a plain struct of colours and metrics with `dark()` and
  `light()` presets, and `scale()` for high-DPI. Assign a field and the next
  frame uses it.

**Project**

- CMake 3.25 build with presets for Ninja + MSVC, dependencies pinned through
  `FetchContent`, and install/export rules for `find_package(ExoticGui)`.
- PowerShell scripts: `vsdev`, `build`, `run`, `test`, `format`, `package`,
  `clean`.
- Examples: `hello_window`, `shapes`, `demo`.
- Framework-free test suite wired into CTest.

[Unreleased]: https://github.com/ExoticGamerrrYT/exotic-gui-cpp/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/ExoticGamerrrYT/exotic-gui-cpp/releases/tag/v0.1.0
