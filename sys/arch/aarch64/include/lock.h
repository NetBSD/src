/* $NetBSD: lock.h,v 1.3 2015/06/26 18:27:52 matt Exp $ */

#ifdef __aarch64__
# include <sys/common_lock.h>
#elif defined(__arm__)
# include <arm/lock.h>
#endif
