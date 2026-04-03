/*	$NetBSD: bus_dma.h,v 1.11 2026/04/03 14:58:01 thorpej Exp $	*/

#ifndef _NEXT68K_BUS_DMA_H_
#define	_NEXT68K_BUS_DMA_H_

#include <m68k/bus_dma.h>

/* XXX only next68k does this and it's shouldn't. */
#define	dm_xfer_len	_dm_xxx_md_field

#endif /* _NEXT68K_BUS_DMA_H_ */
