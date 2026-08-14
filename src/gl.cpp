#include "gl.hpp"

namespace exo::gl {

#define EXO_GL_DEFINE(ret, name, params) PFN_##name name = nullptr;
EXO_GL_FUNCTIONS(EXO_GL_DEFINE)
#undef EXO_GL_DEFINE

bool load(ProcLoader loader, const char** missing) {
    if (missing) *missing = nullptr;
    if (!loader) return false;

    bool ok = true;

#define EXO_GL_LOAD(ret, name, params)                                                                       \
    name = reinterpret_cast<PFN_##name>(loader(#name));                                                      \
    if (!name && ok) {                                                                                       \
        ok = false;                                                                                          \
        if (missing) *missing = #name;                                                                       \
    }

    EXO_GL_FUNCTIONS(EXO_GL_LOAD)
#undef EXO_GL_LOAD

    return ok;
}

} // namespace exo::gl
