/*	$NetBSD: vmparam.h,v 1.1.1.1 1999/09/16 12:23:24 takemura Exp $	*/

#include <mips/vmparam.h>

/*
 * hpcmips has one physical memory segment.
 */
#define	VM_PHYSSEG_MAX		1

#define	VM_NFREELIST		1
#define	VM_FREELIST_DEFAULT	0

/* pcb base */
/*#define	pcbb(p)		((u_int)(p)->p_addr) */
