/*	$NetBSD: bus.h,v 1.2 1999/02/14 17:54:29 scw Exp $ */

#include <machine/bus_space.h>

#if 0	/* XXXSCW: No Bus DMA yet */
#include <machine/bus_dma.h>
#endif

#define generic_btop(x) m68k_btop(x)
