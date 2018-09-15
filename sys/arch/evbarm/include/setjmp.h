/*	$NetBSD: setjmp.h,v 1.3 2018/09/15 15:22:06 jakllsch Exp $	*/

#ifdef __aarch64__
#include <aarch64/setjmp.h>
#else
#include <arm/setjmp.h>
#endif
