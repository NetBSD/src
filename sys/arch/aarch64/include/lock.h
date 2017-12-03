/* $NetBSD: lock.h,v 1.2.2.3 2017/12/03 11:35:44 jdolecek Exp $ */

#ifdef __aarch64__
# include <sys/common_lock.h>
#elif defined(__arm__)
# include <arm/lock.h>
#endif
