/*	$NetBSD: pci_machdep.c,v 1.2 2002/03/18 01:21:13 simonb Exp $	*/

#include <sys/param.h>
#include <sys/device.h>

#define _MIPS_BUS_DMA_PRIVATE
#include <machine/bus.h>

/*
 * PCI doesn't have any special needs; just use
 * the generic versions of these functions.
 */
struct mips_bus_dma_tag pci_bus_dma_tag = {
	NULL,	/* cookie */
	0,	/* _wbase */
	0,	/* _physbase */
	0,	/* _wsize */
	_bus_dmamap_create, 
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	_bus_dmamap_sync,
	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap,
};
