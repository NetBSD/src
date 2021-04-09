//===-- sanitizer_syscall_generic.inc ---------------------------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Generic implementations of internal_syscall* and internal_iserror.
//
//===----------------------------------------------------------------------===//

// NetBSD uses libc calls directly
#if SANITIZER_NETBSD
# define SYS_stat SYS___stat50
# define SYS_lstat SYS___lstat50
# define SYS_fstat SYS___fstat50
# define SYS_gettimeofday SYS___gettimeofday50
# define SYS_wait4 SYS___wait450
# define SYS_getdents SYS___getdents30
# define SYS_sigaltstack SYS___sigaltstack14
# define SYS_sigprocmask SYS___sigprocmask14
# define SYS_nanosleep SYS___nanosleep50
#else

#if SANITIZER_FREEBSD || SANITIZER_MAC || SANITIZER_OPENBSD || SANITIZER_SOLARIS
# define SYSCALL(name) SYS_ ## name
#else
# define SYSCALL(name) __NR_ ## name
#endif

#if defined(__x86_64__) && (SANITIZER_FREEBSD || SANITIZER_MAC)
# define internal_syscall __syscall
# else
# define internal_syscall syscall
#endif

#endif

bool internal_iserror(uptr retval, int *rverrno) {
  if (retval == (uptr)-1) {
    if (rverrno)
      *rverrno = errno;
    return true;
  } else {
    return false;
  }
}
