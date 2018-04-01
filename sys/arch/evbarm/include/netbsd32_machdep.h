/* $NetBSD: netbsd32_machdep.h,v 1.2 2018/04/01 04:35:05 ryo Exp $ */

#ifdef __aarch64__
#include <aarch64/netbsd32_machdep.h>
#else
#include <arm/netbsd32_machdep.h>
#endif
