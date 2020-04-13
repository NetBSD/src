#!/usr/bin/env bash

# TravisCI workaround to use new clang-format while avoiding painful aliasing
# into the subshell
if which clang-format-8; then
    clang-format-8 --verbose -style=file -i **/*.c **/*.h **/*.cpp
else
    clang-format --verbose -style=file -i **/*.c **/*.h **/*.cpp
fi

