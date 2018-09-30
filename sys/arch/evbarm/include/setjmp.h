/*	$NetBSD: setjmp.h,v 1.1.32.2 2018/09/30 01:45:41 pgoyette Exp $	*/

#ifdef __aarch64__
#include <aarch64/setjmp.h>
#else
#include <arm/setjmp.h>
#endif
