/*	$NetBSD: bus.h,v 1.1.170.1 2011/06/06 09:06:14 jruoho Exp $	*/

#ifndef _MMEYE_BUS_H_
#define _MMEYE_BUS_H_

#include <sh3/bus.h>

/*
 * Dummy bus_dma(9)
 * XXXX: mmeye don't use bus_dma.
 */
typedef void *bus_dma_tag_t;

typedef struct bus_dma_segment {
	bus_addr_t ds_addr;
	bus_size_t ds_len;
} bus_dma_segment_t;

typedef struct bus_dmamap {
	bus_size_t dm_maxsegsz;
	bus_size_t dm_mapsize;
	int dm_nsegs;
	bus_dma_segment_t *dm_segs;
} *bus_dmamap_t;

#endif	/* _MMEYE_BUS_H_ */
