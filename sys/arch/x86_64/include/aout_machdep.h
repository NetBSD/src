/*	$NetBSD: aout_machdep.h,v 1.1 2001/06/19 00:20:09 fvdl Exp $	*/

#ifndef _X86_64_AOUT_H_
#define _X86_64_AOUT_H

/*
 * Only needed for 32 bit binaries in compatibility mode.
 */
#ifdef _KERNEL
#include <i386/include/aout_machdep.h>
#else
#include <i386/aout_machdep.h>
#endif

#endif  /* _X86_64_AOUT_H */
