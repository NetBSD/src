/* $NetBSD: cpu_counter.h,v 1.1.44.1 2018/04/07 04:12:13 pgoyette Exp $ */

#ifdef __aarch64__
#include <aarch64/cpu_counter.h>
#else
#include <arm/cpu_counter.h>
#endif
