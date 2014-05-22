// RUN: %clang_cc1 -fsyntax-only -ffreestanding %s -verify
// RUN: %clang_cc1 -fsyntax-only -ffreestanding -fno-lax-vector-conversions %s -verify
// RUN: %clang_cc1 -fsyntax-only -ffreestanding -x c++ %s -verify
// expected-no-diagnostics

#if defined(i386) || defined(__x86_64__)

// Pretend to enable all features.
#ifndef __3dNOW__
#define __3dNOW__
#endif
#ifndef __BMI__
#define __BMI__
#endif
#ifndef __BMI2__
#define __BMI2__
#endif
#ifndef __LZCNT__
#define __LZCNT__
#endif
#ifndef __POPCNT__
#define __POPCNT__
#endif
#ifndef __RDSEED__
#define __RDSEED__
#endif
#ifndef __PRFCHW__
#define __PRFCHW__
#endif
#ifndef __SSE4A__
#define __SSE4A__
#endif
#ifndef __FMA4__
#define __FMA4__
#endif
#ifndef __XOP__
#define __XOP__
#endif
#ifndef __F16C__
#define __F16C__
#endif
#ifndef __MMX__
#define __MMX__
#endif
#ifndef __SSE__
#define __SSE__
#endif
#ifndef __SSE2__
#define __SSE2__
#endif
#ifndef __SSE3__
#define __SSE3__
#endif
#ifndef __SSSE3__
#define __SSSE3__
#endif
#ifndef __SSE4_1__
#define __SSE4_1__
#endif
#ifndef __SSE4_2__
#define __SSE4_2__
#endif
#ifndef __AES__
#define __AES__
#endif
#ifndef __AVX__
#define __AVX__
#endif
#ifndef __AVX2__
#define __AVX2__
#endif
#ifndef __BMI__
#define __BMI__
#endif
#ifndef __BMI2__
#define __BMI2__
#endif
#ifndef __LZCNT__
#define __LZCNT__
#endif
#ifndef __FMA__
#define __FMA__
#endif
#ifndef __RDRND__
#define __RDRND__
#endif

// Now include the metaheader that includes all x86 intrinsic headers.
#include <x86intrin.h>

#endif
