/*	$NetBSD: vmparam.h,v 1.1 2001/05/28 16:22:20 thorpej Exp $	*/

#ifndef _ALGOR_VMPARAM_H_
#define _ALGOR_VMPARAM_H_

#include <mips/vmparam.h>

/*
 * We have two freelists.  We need to squirrel away memory for ISA
 * DMA (it's limited to 8MB on the Algorithmics P-5064, for example,
 * starting at 8MB).
 */
#define	VM_NFREELIST		2
#define	VM_FREELIST_DEFAULT	0
#define	VM_FREELIST_ISADMA	1

#define	VM_PHYSSEG_MAX		2

#endif	/* !_ALGOR_VMPARAM_H_ */
