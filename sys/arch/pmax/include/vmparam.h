/*	$NetBSD: vmparam.h,v 1.9.2.1 2000/11/20 20:20:32 bouyer Exp $	*/

#ifndef _PMAX_VMPARAM_H_
#define _PMAX_VMPARAM_H_

#include <mips/vmparam.h>
 
/*
 * We have two freelists. The first 8M of RAM goes onto a lower-priority
 * free list, since some TC boards (e.g. PixelStamp boards) are only able 
 * to DMA into this region, and we want them to have a fighting chance of
 * allocating their DMA memory during autoconfiguration.
 */
#define	VM_NFREELIST		2
#define	VM_FREELIST_DEFAULT	0
#define	VM_FREELIST_FIRST8	1

#define	VM_PHYSSEG_MAX		2

#endif	/* !_PMAX_VMPARAM_H_ */
