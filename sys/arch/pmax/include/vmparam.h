/*	$NetBSD: vmparam.h,v 1.10 2000/01/09 14:28:54 ad Exp $	*/

#ifndef _PMAX_VMPARAM_H_
#define _PMAX_VMPARAM_H_	1

#include <mips/vmparam.h>

/*
 * VM_PHYSSEG_MAX
 *   We have two freelists. The first 8M of RAM goes onto a lower-priority
 *   free list, since some TC boards (e.g. PixelStamp boards) are only able 
 *   to DMA into this region, and we want them to have a fighting chance of
 *   allocating their DMA memory during autoconfiguration.
 *
 * VM_PHYSSEG_NOADD
 *   We can't add RAM after vm_mem_init.
 */
 
#define	VM_PHYSSEG_MAX		2
#define	VM_PHYSSEG_STRAT	VM_PSTRAT_BSEARCH
#define	VM_PHYSSEG_NOADD	1

#define	VM_NFREELIST		2
#define	VM_FREELIST_DEFAULT	0
#define	VM_FREELIST_FIRST8	1

#endif	/* !_PMAX_VMPARAM_H_ */
