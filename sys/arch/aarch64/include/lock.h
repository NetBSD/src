/* $NetBSD: lock.h,v 1.2.4.1 2015/09/22 12:05:34 skrll Exp $ */

#ifdef __aarch64__
# include <sys/common_lock.h>
#elif defined(__arm__)
# include <arm/lock.h>
#endif
