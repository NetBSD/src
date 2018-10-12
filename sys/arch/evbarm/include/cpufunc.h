/* $NetBSD: cpufunc.h,v 1.1 2018/10/12 22:09:04 jmcneill Exp $ */

#ifdef __aarch64__
#include <aarch64/cpufunc.h>
#else
#include <arm/cpufunc.h>
#endif
