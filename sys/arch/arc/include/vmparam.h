/*	$NetBSD: vmparam.h,v 1.5 2000/01/23 21:01:59 soda Exp $	*/
/*	$OpenBSD: vmparam.h,v 1.3 1997/04/19 17:19:59 pefo Exp $	*/
/*	NetBSD: vmparam.h,v 1.5 1994/10/26 21:10:10 cgd Exp 	*/

#include <mips/vmparam.h>

/*
 * Maximum number of contigous physical memory segment.
 */
#define	VM_PHYSSEG_MAX		16

#define	VM_NFREELIST		1
#define	VM_FREELIST_DEFAULT	0

#if 0 /* changed in OpenBSD */
#define	USRTEXT		0x00400000
#endif

#if 0 /* defined in <mips/vmparam.h> in NetBSD, but not defined in OpenBSD */
#define	BTOPUSRSTACK	0x80000		/* btop(USRSTACK) */
#define	LOWPAGES	0x00001
#define	HIGHPAGES	0

#define	mapin(pte, v, pfnum, prot) \
	(*(int *)(pte) = ((pfnum) << PG_SHIFT) | (prot), MachTLBFlushAddr(v))
#endif

/* pcb base */
/*#define	pcbb(p)		((u_int)(p)->p_addr) */
