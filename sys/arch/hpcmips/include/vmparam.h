/*	$NetBSD: vmparam.h,v 1.1.1.1.2.2 2000/12/08 09:26:32 bouyer Exp $	*/

#include <mips/vmparam.h>

/*
 * hpcmips has one physical memory segment.
 */
#define	VM_PHYSSEG_MAX		5

#define	VM_NFREELIST		1
#define	VM_FREELIST_DEFAULT	0
