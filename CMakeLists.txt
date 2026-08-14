cmake_minimum_required(VERSION 3.25)

project(ExoticGui
    VERSION 0.1.0
    DESCRIPTION "Immediate-mode GUI library for modern C++"
    HOMEPAGE_URL "https://github.com/ExoticGamerrrYT/exotic-gui-cpp"
    # C is required because GLFW is a C library.
    LANGUAGES CXX C)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(PROJECT_IS_TOP_LEVEL)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
endif()

option(EXOTIC_BUILD_EXAMPLES     "Build the example applications"      ${PROJECT_IS_TOP_LEVEL})
option(EXOTIC_BUILD_TESTS        "Build the unit tests"                ${PROJECT_IS_TOP_LEVEL})
option(EXOTIC_INSTALL            "Generate the install/export targets" ${PROJECT_IS_TOP_LEVEL})
option(EXOTIC_WARNINGS_AS_ERRORS "Treat compiler warnings as errors"   OFF)

set(CMAKE_COMPILE_WARNING_AS_ERROR ${EXOTIC_WARNINGS_AS_ERRORS})

include(Dependencies)

add_subdirectory(src)

if(EXOTIC_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(EXOTIC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(EXOTIC_INSTALL)
    include(Install)
endif()

if(PROJECT_IS_TOP_LEVEL)
    message(STATUS "")
    message(STATUS "ExoticGui ${PROJECT_VERSION}")
    message(STATUS "  build type ..... ${CMAKE_BUILD_TYPE}")
    message(STATUS "  compiler ....... ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
    message(STATUS "  examples ....... ${EXOTIC_BUILD_EXAMPLES}")
    message(STATUS "  tests .......... ${EXOTIC_BUILD_TESTS}")
    message(STATUS "")
endif()
