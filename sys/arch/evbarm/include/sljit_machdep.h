/*	$NetBSD: sljit_machdep.h,v 1.1.32.1 2019/06/10 22:06:07 christos Exp $	*/

#ifdef __aarch64__
#include <aarch64/sljit_machdep.h>
#else
#include <arm/sljit_machdep.h>
#endif
