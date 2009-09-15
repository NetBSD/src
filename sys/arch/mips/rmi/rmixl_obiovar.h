/*	$NetBSD: rmixl_obiovar.h,v 1.1.2.3 2009/09/15 02:32:02 cliff Exp $	*/

#ifndef _MIPS_RMIXL_OBIOVAR_H_
#define _MIPS_RMIXL_OBIOVAR_H_

#include <dev/pci/pcivar.h>
#include <mips/pci_machdep.h>

struct obio_attach_args {
	bus_space_tag_t	obio_el_bst;
	bus_space_tag_t	obio_eb_bst;
	bus_addr_t	obio_addr;
	bus_size_t	obio_size;
	int		obio_intr;
	unsigned int	obio_mult;
	bus_dma_tag_t	obio_dmat;
};

typedef struct obio_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_el_bst;
	bus_space_tag_t		sc_eb_bst;
	bus_dma_tag_t		sc_lt4G_dmat;
	bus_dma_tag_t		sc_ge4G_dmat;
	bus_addr_t		sc_base;
	bus_size_t		sc_size;

	/* Bus space, DMA, and PCI tags for the PCI bus. */
	bus_space_handle_t	sc_pcicfg_ioh;
#ifdef NOTYET
	struct XXX_bus_dma_tag sc_pci_dmat;
	struct XXX_pci_chipset sc_pci_chipset;
#endif
} obio_softc_t;

extern struct mips_bus_space   rmixl_bus_mbst;
extern struct mips_bus_dma_tag rmixl_bus_mdt;


#endif /* _MIPS_OMAP_RMIXL_OBIOVAR_H_ */
