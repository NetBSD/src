/*	$NetBSD: vmparam.h,v 1.3 2000/12/01 17:57:44 tsutsui Exp $	*/

#include <mips/vmparam.h>

/*
 * hpcmips has one physical memory segment.
 */
#define	VM_PHYSSEG_MAX		5

#define	VM_NFREELIST		1
#define	VM_FREELIST_DEFAULT	0
