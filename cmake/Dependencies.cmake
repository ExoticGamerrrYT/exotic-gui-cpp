# Third-party dependencies.
#
# Everything is fetched and pinned at configure time - no submodules, no system
# packages, nothing to install by hand. Both dependencies are permissively
# licensed (GLFW: zlib/libpng, stb: MIT/public domain).

include(FetchContent)

# GLFW 3.4 declares compatibility with CMake 3.1, which CMake 4.x refuses to
# honour. This tells CMake to treat such projects as if they had requested 3.5.
# Our own project requires 3.25, so this only ever affects dependencies.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# --- GLFW: window creation, OpenGL context and input -------------------------
# This is why the library never touches the raw Win32 API.
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
# GLFW is linked privately, but a static ExoticGui still needs its symbols at
# link time, so it has to travel with the install/export set.
set(GLFW_INSTALL ${EXOTIC_INSTALL} CACHE BOOL "" FORCE)

FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE
    SYSTEM)

# --- stb_truetype: glyph rasterisation ---------------------------------------
# Header-only, and the repository ships no usable CMakeLists, so point
# SOURCE_SUBDIR at a directory that does not exist to skip add_subdirectory().
FetchContent_Declare(stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        2c980bb59875b0d32144a71867fbdebb2f77cd20
    SOURCE_SUBDIR  headers-only-no-cmake
    SYSTEM)

FetchContent_MakeAvailable(glfw stb)

unset(CMAKE_POLICY_VERSION_MINIMUM)

# Exposed as a plain include path rather than an INTERFACE target: stb is
# header-only, so it imposes no link requirement on consumers and must not end
# up in our exported target set.
set(EXOTIC_STB_INCLUDE_DIR "${stb_SOURCE_DIR}")

foreach(dep_target IN ITEMS glfw update_mappings)
    if(TARGET ${dep_target})
        set_target_properties(${dep_target} PROPERTIES FOLDER "third-party")
    endif()
endforeach()
