/* $NetBSD: sysarch.h,v 1.1 2014/08/10 05:47:38 matt Exp $ */

#ifdef __aarch64__
/* nothing */
#elif defined(__arm__)
#include <arm/sysarch.h>
#endif
