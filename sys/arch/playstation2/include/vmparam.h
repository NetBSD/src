/*	$NetBSD: vmparam.h,v 1.1 2001/10/16 15:38:52 uch Exp $	*/

#include <mips/vmparam.h>

/*
 * PlayStation2 has one physical memory segment.
 *	0 ... kernel itself
 *	1 ... heap (continuous)
 */
#define	VM_PHYSSEG_MAX		2

#define	VM_NFREELIST		1
#define	VM_FREELIST_DEFAULT	0
