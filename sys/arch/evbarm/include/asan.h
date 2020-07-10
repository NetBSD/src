/*	$NetBSD: asan.h,v 1.2 2020/07/10 12:25:10 skrll Exp $	*/

#ifdef __aarch64__
#include <aarch64/asan.h>
#endif

#ifdef __arm__
#include <arm/asan.h>
#endif
