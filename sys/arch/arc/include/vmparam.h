/*	$NetBSD: vmparam.h,v 1.5.2.1 2000/06/22 16:59:15 minoura Exp $	*/
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

#ifndef KSEG2IOBUFSIZE
#define KSEG2IOBUFSIZE	kseg2iobufsize	/* reserve PTEs for KSEG2 I/O space */

extern vsize_t kseg2iobufsize;
#endif
