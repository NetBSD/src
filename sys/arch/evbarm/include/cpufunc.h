/* $NetBSD: cpufunc.h,v 1.1.2.2 2018/10/20 06:58:27 pgoyette Exp $ */

#ifdef __aarch64__
#include <aarch64/cpufunc.h>
#else
#include <arm/cpufunc.h>
#endif
