/*	$NetBSD: bios32.h,v 1.1 2001/06/19 00:20:09 fvdl Exp $	*/

/*
 * XXXfvdl paddr_t, etc, isn't right in bios32 structures, use explicit
 * sizes
 */

#ifdef _KERNEL
#include <i386/include/bios32.h>
#else
#include <i386/bios32.h>
#endif
