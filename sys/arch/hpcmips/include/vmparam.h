/*	$NetBSD: vmparam.h,v 1.1.1.1.2.1 2000/11/20 20:46:52 bouyer Exp $	*/

#include <mips/vmparam.h>

/*
 * hpcmips has one physical memory segment.
 */
#define	VM_PHYSSEG_MAX		5

#define	VM_NFREELIST		1
#define	VM_FREELIST_DEFAULT	0

/* pcb base */
/*#define	pcbb(p)		((u_int)(p)->p_addr) */
