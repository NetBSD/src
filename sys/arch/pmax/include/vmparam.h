/*	$NetBSD: vmparam.h,v 1.7 1998/02/25 23:31:03 thorpej Exp $	*/

#include <mips/vmparam.h>

/*
 * DECstation has one physical memory segment.
 */
#define	VM_PHYSSEG_MAX		1

/* pcb base */
/*#define	pcbb(p)		((u_int)(p)->p_addr) */
