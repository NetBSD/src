/*	$NetBSD: bus.h,v 1.1.2.1 1999/01/30 21:58:43 scw Exp $ */

#include <machine/bus_space.h>

#if 0	/* XXXSCW: No Bus DMA yet */
#include <machine/bus_dma.h>
#endif

#define generic_btop(x) m68k_btop(x)
