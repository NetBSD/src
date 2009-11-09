/*      $NetBSD: rmixl_pcievar.h,v 1.1.2.1 2009/11/09 10:17:06 cliff Exp $	*/

#ifndef _MIPS_RMI_PCIE_VAR_H_
#define _MIPS_RMI_PCIE_VAR_H_

#include <dev/pci/pcivar.h>

typedef enum rmixl_pcie_lnkcfg_mode {
	LCFG_NO=0,		/* placeholder */
	LCFG_EP,		/* end point */
	LCFG_RC,		/* root complex */
} rmixl_pcie_lnkcfg_mode_t;

typedef struct rmixl_pcie_lnkcfg {
	rmixl_pcie_lnkcfg_mode_t mode;
	u_int lanes;
} rmixl_pcie_lnkcfg_t;

typedef struct rmixl_pcie_lnktab {
	u_int ncfgs;
	const char *str;
	const rmixl_pcie_lnkcfg_t *cfg;
} rmixl_pcie_lnktab_t;

struct rmixl_pcie_softc {
	device_t                	sc_dev;
	struct mips_pci_chipset 	sc_pci_chipset;
	bus_addr_t              	sc_pcie_cfg_pbase;
	bus_addr_t              	sc_pcie_ecfg_pbase;
	bus_dma_tag_t			sc_29bit_dmat;
	bus_dma_tag_t			sc_32bit_dmat;
	bus_dma_tag_t			sc_64bit_dmat;
	rmixl_pcie_lnktab_t		sc_pcie_lnktab;
};

#endif  /* _MIPS_RMI_PCIE_VAR_H_ */

