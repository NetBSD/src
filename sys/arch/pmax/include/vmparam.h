/*	$NetBSD: vmparam.h,v 1.12 2000/01/09 20:09:43 simonb Exp $	*/

#ifndef _PMAX_VMPARAM_H_
#define _PMAX_VMPARAM_H_

#include <mips/vmparam.h>

/*
 * VM_PHYSSEG_MAX
 *   We have two freelists. The first 8M of RAM goes onto a lower-priority
 *   free list, since some TC boards (e.g. PixelStamp boards) are only able 
 *   to DMA into this region, and we want them to have a fighting chance of
 *   allocating their DMA memory during autoconfiguration.
 */
 
#define	VM_PHYSSEG_MAX		2

#define	VM_NFREELIST		2
#define	VM_FREELIST_DEFAULT	0
#define	VM_FREELIST_FIRST8	1

#endif	/* !_PMAX_VMPARAM_H_ */
