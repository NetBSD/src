/*	$NetBSD: setjmp.h,v 1.2.2.1 2019/06/10 22:06:07 christos Exp $	*/

#ifdef __aarch64__
#include <aarch64/setjmp.h>
#else
#include <arm/setjmp.h>
#endif
