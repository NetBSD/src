/*	$NetBSD: vmparam.h,v 1.9 1999/05/07 18:04:37 thorpej Exp $	*/

#include <mips/vmparam.h>

/*
 * DECstation has one physical memory segment.
 */
#define	VM_PHYSSEG_MAX		2	/* 2 free lists */
#define	VM_PHYSSEG_STRAT	VM_PSTRAT_BSEARCH
#define	VM_PHYSSEG_NOADD		/* can't add RAM after vm_mem_init */

#define	VM_NFREELIST		2
#define	VM_FREELIST_DEFAULT	0
#define	VM_FREELIST_FIRST8	1

/* pcb base */
/*#define	pcbb(p)		((u_int)(p)->p_addr) */
