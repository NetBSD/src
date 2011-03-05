/*	$NetBSD: vmparam.h,v 1.15.8.1 2011/03/05 15:09:57 bouyer Exp $	*/

#ifndef _PMAX_VMPARAM_H_
#define _PMAX_VMPARAM_H_

#include <mips/vmparam.h>
 
/*
 * We have two freelists. The first 8M of RAM goes onto a lower-priority
 * free list, since some TC boards (e.g. PixelStamp boards) are only able 
 * to DMA into this region, and we want them to have a fighting chance of
 * allocating their DMA memory during autoconfiguration.
 */
#define	VM_PHYSSEG_MAX		16
#undef VM_NFREELIST
#define	VM_NFREELIST		2
#define	VM_FREELIST_FIRST8	1

#endif	/* !_PMAX_VMPARAM_H_ */
