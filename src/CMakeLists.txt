configure_file(
    "${PROJECT_SOURCE_DIR}/include/exotic/version.hpp.in"
    "${PROJECT_BINARY_DIR}/generated/exotic/version.hpp"
    @ONLY)

add_library(exotic_gui STATIC
    core.cpp
    draw.cpp
    font.cpp
    gl.cpp
    renderer.cpp
    theme.cpp
    ui.cpp
    window.cpp)

add_library(exotic::gui ALIAS exotic_gui)

target_sources(exotic_gui
    PUBLIC
        FILE_SET public_headers TYPE HEADERS
            BASE_DIRS "${PROJECT_SOURCE_DIR}/include"
            FILES
                "${PROJECT_SOURCE_DIR}/include/exotic/draw.hpp"
                "${PROJECT_SOURCE_DIR}/include/exotic/exotic.hpp"
                "${PROJECT_SOURCE_DIR}/include/exotic/font.hpp"
                "${PROJECT_SOURCE_DIR}/include/exotic/theme.hpp"
                "${PROJECT_SOURCE_DIR}/include/exotic/types.hpp"
                "${PROJECT_SOURCE_DIR}/include/exotic/ui.hpp"
                "${PROJECT_SOURCE_DIR}/include/exotic/window.hpp"
        FILE_SET generated_headers TYPE HEADERS
            BASE_DIRS "${PROJECT_BINARY_DIR}/generated"
            FILES
                "${PROJECT_BINARY_DIR}/generated/exotic/version.hpp")

target_compile_features(exotic_gui PUBLIC cxx_std_20)

target_link_libraries(exotic_gui PRIVATE glfw)

target_include_directories(exotic_gui SYSTEM PRIVATE "${EXOTIC_STB_INCLUDE_DIR}")

target_compile_definitions(exotic_gui PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)

if(MSVC)
    target_compile_options(exotic_gui PRIVATE /W4 /permissive- /utf-8 /Zc:__cplusplus)
    # We use the standard C library on purpose (std::getenv); MSVC's "unsafe"
    # deprecations only point at Microsoft-specific replacements.
    target_compile_definitions(exotic_gui PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(exotic_gui PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(exotic_gui PROPERTIES
    FOLDER "exotic"
    OUTPUT_NAME "exotic-gui"
    DEBUG_POSTFIX "d")
