/*	$NetBSD: vmparam.h,v 1.2 1998/02/25 23:27:20 thorpej Exp $	*/

#include <mips/vmparam.h>

/*
 * NEWS has one physical memory segment.
 */
#define	VM_PHYSSEG_MAX		1

/* pcb base */
/*#define	pcbb(p)		((u_int)(p)->p_addr) */
