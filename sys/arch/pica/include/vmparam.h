/*	$NetBSD: vmparam.h,v 1.3 1998/02/25 23:30:11 thorpej Exp $	*/

#include <mips/vmparam.h>

/*
 * PICA has one physical memory segment.
 */
#define	VM_PHYSSEG_MAX		1

/* pcb base */
/*#define	pcbb(p)		((u_int)(p)->p_addr) */
