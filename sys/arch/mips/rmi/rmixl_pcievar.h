/*      $NetBSD: rmixl_pcievar.h,v 1.1.2.3 2010/01/29 00:23:34 cliff Exp $	*/

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

typedef struct rmixl_pcie_link_dispatch {
	LIST_ENTRY(rmixl_pcie_link_dispatch) next;
	int (*func)(void *);
	void *arg;
	u_int link;
	u_int bitno;
	u_int irq;
	struct evcnt count;
} rmixl_pcie_link_dispatch_t;

struct rmixl_pcie_softc;

typedef struct rmixl_pcie_link_intr {
	struct rmixl_pcie_softc *sc;
	LIST_HEAD(, rmixl_pcie_link_dispatch) dispatch;
	u_int link;
	u_int ipl;
	bool enabled;
	void *ih;			/* mips interrupt handle */
} rmixl_pcie_link_intr_t;

#define RMIXL_PCIE_NLINKS_MAX	4

typedef struct rmixl_pcie_softc {
	device_t                	sc_dev;
	struct mips_pci_chipset 	sc_pci_chipset;
	bus_space_tag_t              	sc_pcie_cfg_memt;
	bus_space_tag_t              	sc_pcie_ecfg_memt;
	bus_dma_tag_t			sc_29bit_dmat;
	bus_dma_tag_t			sc_32bit_dmat;
	bus_dma_tag_t			sc_64bit_dmat;
	rmixl_pcie_lnktab_t		sc_pcie_lnktab;
	void 			       *sc_fatal_ih;
	rmixl_pcie_link_intr_t		sc_link_intr[RMIXL_PCIE_NLINKS_MAX];
} rmixl_pcie_softc_t;

#endif  /* _MIPS_RMI_PCIE_VAR_H_ */

