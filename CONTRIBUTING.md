# Contributing

Thanks for taking a look. This is a small library and it intends to stay small.

## Before you write code

Open an issue first for anything larger than a bug fix. The fastest way to get
a pull request rejected is to add a feature the library deliberately does not
have - see the *Limitations* section of the README, which lists what is missing
on purpose as much as by omission.

## Getting set up

```powershell
git clone https://github.com/ExoticGamerrrYT/exotic-gui-cpp.git
cd exotic-gui-cpp
.\scripts\build.ps1 -Config debug
.\scripts\test.ps1
```

You need Visual Studio 2022+ with the C++ workload. Everything else is fetched
by CMake.

## Ground rules

- **Debug builds must be warning-free.** The `ninja-debug` preset compiles at
  `/W4` with warnings as errors. If a third-party header warns, silence it
  around the include, not globally.
- **Run `scripts\format.ps1`** before committing. The `.clang-format` in the
  repository is the only style authority; do not reformat unrelated lines.
- **Add a check for new logic.** `tests/test_core.cpp` is a plain executable -
  add a `CHECK` next to the ones already there. Anything that can be tested
  without a window should be.
- **No new dependencies** without a very good reason. Two is already two more
  than zero.
- **Public headers stay clean**: no GLFW, no OpenGL, no stb. Implementation
  types live behind a pimpl.

## Commit messages and branches

Branch names are `feat/...`, `fix/...`, `docs/...` or `chore/...`. Commits
follow [Conventional Commits](https://www.conventionalcommits.org):

```
feat(ui): add a colour picker widget

Explain why, not what. The diff already says what.
```

## Pull requests

Say what changed and, more importantly, *why the design is what it is*. If you
made a trade-off, name it. If you left something out on purpose, say so.

Every pull request should state how it was verified: which builds, which tests,
and - for anything interactive - what you actually clicked.
