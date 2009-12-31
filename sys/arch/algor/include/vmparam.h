/*	$NetBSD: vmparam.h,v 1.1.148.1 2009/12/31 00:54:08 matt Exp $	*/

#ifndef _ALGOR_VMPARAM_H_
#define _ALGOR_VMPARAM_H_

#include <mips/vmparam.h>

/*
 * We have two freelists.  We need to squirrel away memory for ISA
 * DMA (it's limited to 8MB on the Algorithmics P-5064, for example,
 * starting at 8MB).
 */
#define	VM_PHYSSEG_MAX		2
#define	VM_FREELIST_ISADMA	1


#endif	/* !_ALGOR_VMPARAM_H_ */
