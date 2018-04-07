/*	$NetBSD: frame.h,v 1.2.30.1 2018/04/07 04:12:13 pgoyette Exp $	*/

#ifdef __aarch64__
#include <aarch64/frame.h>
#else
#include <arm/arm32/frame.h>
#endif
