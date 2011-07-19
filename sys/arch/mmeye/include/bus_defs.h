/*	$NetBSD: bus_defs.h,v 1.1 2011/07/19 15:17:20 dyoung Exp $	*/

#ifndef _MMEYE_BUS_DEFS_H_
#define _MMEYE_BUS_DEFS_H_

#include <sh3/bus_defs.h>

/*
 * Dummy bus_dma(9)
 * XXX: mmeye doesn't use bus_dma.
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

#endif	/* _MMEYE_BUS_DEFS_H_ */
