/*	$NetBSD: vmparam.h,v 1.3.16.1 2014/05/18 17:45:21 rmind Exp $	*/

/*
 * PlayStation 2 has one physical memory segment.
 *	0 ... kernel itself
 *	1 ... heap (continuous)
 */
#define	VM_PHYSSEG_MAX		2

#define	VM_NFREELIST		1
#define	VM_FREELIST_DEFAULT	0

#include <mips/vmparam.h>

