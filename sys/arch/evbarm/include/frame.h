/*	$NetBSD: frame.h,v 1.3 2018/04/01 04:35:05 ryo Exp $	*/

#ifdef __aarch64__
#include <aarch64/frame.h>
#else
#include <arm/arm32/frame.h>
#endif
