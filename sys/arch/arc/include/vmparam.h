/*	$NetBSD: vmparam.h,v 1.4 2000/01/23 20:08:42 soda Exp $	*/

#include <mips/vmparam.h>

/*
 * PICA has one physical memory segment.
 */
#define	VM_PHYSSEG_MAX		1

/* pcb base */
/*#define	pcbb(p)		((u_int)(p)->p_addr) */
