/* $NetBSD: cpu_counter.h,v 1.2 2018/04/01 04:35:04 ryo Exp $ */

#ifdef __aarch64__
#include <aarch64/cpu_counter.h>
#else
#include <arm/cpu_counter.h>
#endif
