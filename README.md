<div align="center">

# Exotic GUI

**An immediate-mode GUI library for modern C++.**

[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/build-CMake%20%2B%20Ninja-064F8C?logo=cmake&logoColor=white)](https://cmake.org)
[![MSVC](https://img.shields.io/badge/toolchain-MSVC-5C2D91?logo=visualstudio&logoColor=white)](https://visualstudio.microsoft.com)
[![License: MIT](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

</div>

---

> **Status: 0.1.0 in development.** The core value types and the build system
> are in place; windowing, the renderer and the widget set land next.

## What it is

A small, batteries-included GUI toolkit for C++ applications and tools. You
describe the interface every frame with ordinary code, the library turns it
into geometry and draws it in a single batched pass.

```cpp
if (ui.button("Save")) save();
```

No widget tree to keep in sync, no callbacks, no code generation.

## Design decisions

| Area | Choice | Why |
| --- | --- | --- |
| Windowing | [GLFW 3.4](https://www.glfw.org) | Zero raw Win32 in this codebase. Windows, contexts, input and cursors for free. |
| Rendering | OpenGL 3.3 core | Available everywhere, one batched draw pass per frame. |
| Text | [stb_truetype](https://github.com/nothings/stb) | One header, no font-service dependency, packed into a single atlas. |
| Dependencies | CMake `FetchContent`, pinned | `git clone` then build. Nothing to install by hand. |
| Model | Immediate mode | No retained widget tree to fall out of sync with your data. |

## Requirements

* Windows 10/11
* Visual Studio 2022+ with the **Desktop development with C++** workload
* CMake 3.25+ and Ninja (both ship with Visual Studio)

## Build

```powershell
git clone https://github.com/ExoticGamerrrYT/exotic-gui-cpp.git
cd exotic-gui
.\scripts\build.ps1
```

The scripts load the MSVC environment themselves, so a plain PowerShell prompt
is enough.

| Script | Purpose |
| --- | --- |
| `scripts\vsdev.ps1` | Load the MSVC x64 developer environment. |
| `scripts\build.ps1` | Configure + build (`-Config debug\|release`, `-Target`, `-Clean`, `-Fresh`). |
| `scripts\test.ps1` | Build and run the test suite through CTest. |
| `scripts\format.ps1` | Run clang-format over the tree (`-Check` to verify only). |
| `scripts\clean.ps1` | Delete `build/` and `dist/`. |

CMake presets are available too:

```powershell
cmake --preset ninja-release
cmake --build --preset ninja-release
ctest --preset ninja-release
```

## Layout

```
exotic-gui/
├── cmake/         CMake modules: dependencies, install/export rules
├── include/exotic Public headers - the entire API surface
├── src/           Implementation, private headers
├── tests/         Framework-free test suite driven by CTest
├── scripts/       PowerShell developer scripts
└── CMakePresets.json
```

## Using it from CMake

```cmake
include(FetchContent)
FetchContent_Declare(exotic_gui
    GIT_REPOSITORY https://github.com/ExoticGamerrrYT/exotic-gui-cpp.git
    GIT_TAG        v0.1.0)
FetchContent_MakeAvailable(exotic_gui)

target_link_libraries(my_app PRIVATE exotic::gui)
```

Or install it and use `find_package(ExoticGui 0.1 REQUIRED)`.

## Versioning

[Semantic Versioning](https://semver.org). Before 1.0 the minor version may
break the API; the patch version never will.

## License

MIT - see [LICENSE](LICENSE).
