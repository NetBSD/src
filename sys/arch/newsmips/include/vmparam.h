/*	$NetBSD: vmparam.h,v 1.3.10.1 1999/08/06 15:10:28 chs Exp $	*/

#include <mips/vmparam.h>

/*
 * NEWS has one physical memory segment.
 */
#define	VM_PHYSSEG_MAX		1

#define	VM_NFREELIST		1
#define	VM_FREELIST_DEFAULT	0

/* pcb base */
/*#define	pcbb(p)		((u_int)(p)->p_addr) */

/*
 * Parameters for Unified Buffer Cache.
 */

#define UBC_WINSIZE 8192
#define UBC_NWINS 1024
