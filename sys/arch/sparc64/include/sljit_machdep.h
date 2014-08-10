/*	$NetBSD: sljit_machdep.h,v 1.1.2.2 2014/08/10 06:54:08 tls Exp $	*/

/* Only 32-bit SPARCs are supported. */
#ifndef __arch64__
#include <sparc/sljit_machdep.h>
#endif
