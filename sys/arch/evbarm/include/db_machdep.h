/*	$NetBSD: db_machdep.h,v 1.2 2018/04/01 04:35:04 ryo Exp $	*/

#ifdef __aarch64__
#include <aarch64/db_machdep.h>
#else
#include <arm/db_machdep.h>
#endif
