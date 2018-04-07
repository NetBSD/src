/*	$NetBSD: lock.h,v 1.1.32.1 2018/04/07 04:12:13 pgoyette Exp $	*/

#ifdef __aarch64__
#include <aarch64/lock.h>
#else
#include <arm/lock.h>
#endif
