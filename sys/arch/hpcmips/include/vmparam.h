/*	$NetBSD: vmparam.h,v 1.2 2000/02/21 13:46:04 shin Exp $	*/

#include <mips/vmparam.h>

/*
 * hpcmips has one physical memory segment.
 */
#define	VM_PHYSSEG_MAX		5

#define	VM_NFREELIST		1
#define	VM_FREELIST_DEFAULT	0

/* pcb base */
/*#define	pcbb(p)		((u_int)(p)->p_addr) */
