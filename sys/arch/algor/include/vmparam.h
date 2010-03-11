/*	$NetBSD: vmparam.h,v 1.1.130.1 2010/03/11 15:01:56 yamt Exp $	*/

#ifndef _ALGOR_VMPARAM_H_
#define _ALGOR_VMPARAM_H_

#include <mips/vmparam.h>

/*
 * We have two freelists.  We need to squirrel away memory for ISA
 * DMA (it's limited to 8MB on the Algorithmics P-5064, for example,
 * starting at 8MB).
 */
#define	VM_FREELIST_ISADMA	1

#define	VM_PHYSSEG_MAX		2

#endif	/* !_ALGOR_VMPARAM_H_ */
