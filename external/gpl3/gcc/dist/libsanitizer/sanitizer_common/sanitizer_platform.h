//===-- sanitizer_platform.h ------------------------------------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Common platform macros.
//===----------------------------------------------------------------------===//

#ifndef SANITIZER_PLATFORM_H
#define SANITIZER_PLATFORM_H

#if !defined(__linux__) && !defined(__FreeBSD__) && \
  !defined(__APPLE__) && !defined(_WIN32)
# error "This operating system is not supported"
#endif

#if defined(__linux__)
# define SANITIZER_LINUX   1
#else
# define SANITIZER_LINUX   0
#endif

#if defined(__FreeBSD__)
# define SANITIZER_FREEBSD 1
#else
# define SANITIZER_FREEBSD 0
#endif

#if defined(__APPLE__)
# define SANITIZER_MAC     1
# include <TargetConditionals.h>
# if TARGET_OS_IPHONE
#  define SANITIZER_IOS    1
# else
#  define SANITIZER_IOS    0
# endif
#else
# define SANITIZER_MAC     0
# define SANITIZER_IOS     0
#endif

#if defined(_WIN32)
# define SANITIZER_WINDOWS 1
#else
# define SANITIZER_WINDOWS 0
#endif

#if defined(__ANDROID__)
# define SANITIZER_ANDROID 1
#else
# define SANITIZER_ANDROID 0
#endif

#define SANITIZER_POSIX (SANITIZER_FREEBSD || SANITIZER_LINUX || SANITIZER_MAC)

#if __LP64__ || defined(_WIN64)
#  define SANITIZER_WORDSIZE 64
#else
#  define SANITIZER_WORDSIZE 32
#endif

#if SANITIZER_WORDSIZE == 64
# define FIRST_32_SECOND_64(a, b) (b)
#else
# define FIRST_32_SECOND_64(a, b) (a)
#endif

#if defined(__x86_64__) && !defined(_LP64)
# define SANITIZER_X32 1
#else
# define SANITIZER_X32 0
#endif

// By default we allow to use SizeClassAllocator64 on 64-bit platform.
// But in some cases (e.g. AArch64's 39-bit address space) SizeClassAllocator64
// does not work well and we need to fallback to SizeClassAllocator32.
// For such platforms build this code with -DSANITIZER_CAN_USE_ALLOCATOR64=0 or
// change the definition of SANITIZER_CAN_USE_ALLOCATOR64 here.
#ifndef SANITIZER_CAN_USE_ALLOCATOR64
# if defined(__aarch64__) || defined(__mips64)
#  define SANITIZER_CAN_USE_ALLOCATOR64 0
# else
#  define SANITIZER_CAN_USE_ALLOCATOR64 (SANITIZER_WORDSIZE == 64)
# endif
#endif

// The range of addresses which can be returned my mmap.
// FIXME: this value should be different on different platforms,
// e.g. on AArch64 it is most likely (1ULL << 39). Larger values will still work
// but will consume more memory for TwoLevelByteMap.
#if defined(__aarch64__)
# define SANITIZER_MMAP_RANGE_SIZE FIRST_32_SECOND_64(1ULL << 32, 1ULL << 39)
#else
# define SANITIZER_MMAP_RANGE_SIZE FIRST_32_SECOND_64(1ULL << 32, 1ULL << 47)
#endif

// The AArch64 linux port uses the canonical syscall set as mandated by
// the upstream linux community for all new ports. Other ports may still
// use legacy syscalls.
#ifndef SANITIZER_USES_CANONICAL_LINUX_SYSCALLS
# if defined(__aarch64__) && SANITIZER_LINUX
# define SANITIZER_USES_CANONICAL_LINUX_SYSCALLS 1
# else
# define SANITIZER_USES_CANONICAL_LINUX_SYSCALLS 0
# endif
#endif

// udi16 syscalls can only be used when the following conditions are
// met:
// * target is one of arm32, x86-32, sparc32, sh or m68k
// * libc version is libc5, glibc-2.0, glibc-2.1 or glibc-2.2 to 2.15
//   built against > linux-2.2 kernel headers
// Since we don't want to include libc headers here, we check the
// target only.
#if defined(__arm__) || SANITIZER_X32 || defined(__sparc__)
#define SANITIZER_USES_UID16_SYSCALLS 1
#else
#define SANITIZER_USES_UID16_SYSCALLS 0
#endif

#ifdef __mips__
# define SANITIZER_POINTER_FORMAT_LENGTH FIRST_32_SECOND_64(8, 10)
#else
# define SANITIZER_POINTER_FORMAT_LENGTH FIRST_32_SECOND_64(8, 12)
#endif

// Assume obsolete RPC headers are available by default
#if !defined(HAVE_RPC_XDR_H) && !defined(HAVE_TIRPC_RPC_XDR_H)
# define HAVE_RPC_XDR_H (SANITIZER_LINUX && !SANITIZER_ANDROID)
# define HAVE_TIRPC_RPC_XDR_H 0
#endif

#endif // SANITIZER_PLATFORM_H
