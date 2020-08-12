/*	$NetBSD: mutex.h,v 1.3 2020/08/12 13:28:46 skrll Exp $	*/

#ifdef __aarch64__
#include <aarch64/mutex.h>
#else
#include <arm/mutex.h>
#endif
