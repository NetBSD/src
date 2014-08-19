/*	$NetBSD: vmparam.h,v 1.5.6.2 2014/08/20 00:03:18 tls Exp $	*/

/*
 * PlayStation 2 has one physical memory segment.
 *	0 ... kernel itself
 *	1 ... heap (continuous)
 */
#define	VM_PHYSSEG_MAX		2

#define	VM_NFREELIST		1
#define	VM_FREELIST_DEFAULT	0

#include <mips/vmparam.h>

