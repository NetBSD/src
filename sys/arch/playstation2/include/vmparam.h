/*	$NetBSD: vmparam.h,v 1.2 2001/10/19 05:47:40 shin Exp $	*/

#include <mips/vmparam.h>

/*
 * PlayStation 2 has one physical memory segment.
 *	0 ... kernel itself
 *	1 ... heap (continuous)
 */
#define	VM_PHYSSEG_MAX		2

#define	VM_NFREELIST		1
#define	VM_FREELIST_DEFAULT	0
