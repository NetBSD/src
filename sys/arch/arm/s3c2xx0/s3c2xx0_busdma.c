/*	$NetBSD: s3c2xx0_busdma.c,v 1.1 2003/08/05 11:24:08 bsh Exp $ */

/* COPYRIGHT */

/*
 * bus_dma tag for s3c2xx0 CPUs
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: s3c2xx0_busdma.c,v 1.1 2003/08/05 11:24:08 bsh Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/extent.h>

#define _ARM32_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <arm/s3c2xx0/s3c2xx0var.h>

struct arm32_bus_dma_tag s3c2xx0_bus_dma = {
	NULL,			/* _ranges: set by platform specific routine */
	0,			/* _nranges */

	NULL,			/* _cookie */

	_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	_bus_dmamap_sync,
	NULL,			/* sync_post */

	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap,
};
