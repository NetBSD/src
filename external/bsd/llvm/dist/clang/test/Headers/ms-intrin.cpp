// RUN: %clang_cc1 -triple i386-pc-win32 -target-cpu pentium4 \
// RUN:     -fms-extensions -fms-compatibility -fmsc-version=1700 \
// RUN:     -ffreestanding -fsyntax-only -Werror \
// RUN:     -isystem %S/Inputs/include %s

// Intrin.h needs size_t, but -ffreestanding prevents us from getting it from
// stddef.h.  Work around it with this typedef.
typedef __SIZE_TYPE__ size_t;

#include <Intrin.h>

// Use some C++ to make sure we closed the extern "C" brackets.
template <typename T>
void foo(T V) {}
