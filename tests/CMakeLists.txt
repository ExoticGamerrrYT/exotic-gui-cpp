# Deliberately framework-free: the suite is a plain executable whose exit code
# is the result. Nothing to install, nothing to fetch, runs in milliseconds.

add_executable(exotic_tests
    test_core.cpp)

target_link_libraries(exotic_tests PRIVATE exotic::gui)

set_target_properties(exotic_tests PROPERTIES FOLDER "exotic")

add_test(NAME exotic.core COMMAND exotic_tests)
