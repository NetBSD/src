/*	$NetBSD: sljit_machdep.h,v 1.1.30.1 2018/09/06 06:55:31 pgoyette Exp $	*/

#ifdef __aarch64__
#include <aarch64/sljit_machdep.h>
#else
#include <arm/sljit_machdep.h>
#endif
