/*	$NetBSD: setjmp.h,v 1.2 2018/04/01 04:35:05 ryo Exp $	*/

#ifdef __aarch64__
#include <arch64/setjmp.h>
#else
#include <arm/setjmp.h>
#endif
