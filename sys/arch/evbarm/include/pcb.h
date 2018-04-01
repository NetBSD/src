/*	$NetBSD: pcb.h,v 1.2 2018/04/01 04:35:05 ryo Exp $	*/

#ifdef __aarch64__
#include <aarch64/pcb.h>
#else
#include <arm/pcb.h>
#endif
