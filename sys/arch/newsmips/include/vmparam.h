/*	$NetBSD: vmparam.h,v 1.3.14.1 2000/12/08 09:28:51 bouyer Exp $	*/

#include <mips/vmparam.h>

/*
 * NEWS has one physical memory segment.
 */
#define	VM_PHYSSEG_MAX		1

#define	VM_NFREELIST		1
#define	VM_FREELIST_DEFAULT	0
