/* $NetBSD: lock.h,v 1.4 2021/09/26 20:15:04 jmcneill Exp $ */

#ifdef __aarch64__
# ifdef _HARDKERNEL
#  ifdef SPINLOCK_BACKOFF_HOOK
#   undef SPINLOCK_BACKOFF_HOOK
#  endif
#  define SPINLOCK_BACKOFF_HOOK		asm volatile("yield" ::: "memory")
# endif
# include <sys/common_lock.h>
#elif defined(__arm__)
# include <arm/lock.h>
#endif
