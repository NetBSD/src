/*	$NetBSD: sljit_machdep.h,v 1.1.6.2 2014/08/20 00:03:25 tls Exp $	*/

/* Only 32-bit SPARCs are supported. */
#ifndef __arch64__
#include <sparc/sljit_machdep.h>
#endif
