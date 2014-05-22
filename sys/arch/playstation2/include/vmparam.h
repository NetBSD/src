/*	$NetBSD: vmparam.h,v 1.3.10.1 2014/05/22 11:40:03 yamt Exp $	*/

/*
 * PlayStation 2 has one physical memory segment.
 *	0 ... kernel itself
 *	1 ... heap (continuous)
 */
#define	VM_PHYSSEG_MAX		2

#define	VM_NFREELIST		1
#define	VM_FREELIST_DEFAULT	0

#include <mips/vmparam.h>

