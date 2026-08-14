#include <exotic/version.hpp>

namespace exo {

const char* version() noexcept {
    return EXOTIC_VERSION_STRING;
}

const char* build_info() noexcept {
    return "Exotic GUI " EXOTIC_VERSION_STRING
#if defined(_MSC_VER)
           " | MSVC"
#elif defined(__clang__)
           " | Clang"
#elif defined(__GNUC__)
           " | GCC"
#endif
#if defined(NDEBUG)
           " | Release"
#else
           " | Debug"
#endif
           " | " __DATE__;
}

} // namespace exo
