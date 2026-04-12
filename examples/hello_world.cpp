/// @file hello_world.cpp
/// @brief Minimal example — creates and runs a core::App.

#include "core/app.h"

int main() {
    core::App app({.name = "HelloWorld", .log_level = 1});
    return app.run();
}
