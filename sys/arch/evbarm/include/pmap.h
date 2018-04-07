/*	$NetBSD: pmap.h,v 1.3.30.1 2018/04/07 04:12:13 pgoyette Exp $	*/

#ifdef __aarch64__
#include <aarch64/pmap.h>
#else
#include <arm/arm32/pmap.h>
#endif
