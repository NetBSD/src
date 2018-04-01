/*	$NetBSD: pmap.h,v 1.4 2018/04/01 04:35:05 ryo Exp $	*/

#ifdef __aarch64__
#include <aarch64/pmap.h>
#else
#include <arm/arm32/pmap.h>
#endif
