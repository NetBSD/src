/*	$NetBSD: pci_machdep.c,v 1.4.100.1 2011/03/05 20:50:14 rmind Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pci_machdep.c,v 1.4.100.1 2011/03/05 20:50:14 rmind Exp $");

#include <sys/param.h>
#include <sys/device.h>

#define _MIPS_BUS_DMA_PRIVATE
#include <machine/bus.h>

/*
 * PCI doesn't have any special needs; just use
 * the generic versions of these functions.
 */
struct mips_bus_dma_tag pci_bus_dma_tag = {
	._dmamap_ops = _BUS_DMAMAP_OPS_INITIALIZER,
	._dmamem_ops = _BUS_DMAMEM_OPS_INITIALIZER,
	._dmatag_ops = _BUS_DMATAG_OPS_INITIALIZER,
};
