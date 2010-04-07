/*      $NetBSD: rmixl_pcixvar.h,v 1.1.2.1 2010/04/07 19:25:48 cliff Exp $	*/

#ifndef _MIPS_RMI_PCIX_VAR_H_
#define _MIPS_RMI_PCIX_VAR_H_

#include <dev/pci/pcivar.h>

typedef struct rmixl_pcix_dispatch {
	LIST_ENTRY(rmixl_pcix_dispatch) next;
	int (*func)(void *);
	void *arg;
	u_int bitno;
	u_int irq;
	struct evcnt count;
	char count_name[32];
} rmixl_pcix_dispatch_t;

struct rmixl_pcix_softc;

typedef struct rmixl_pcix_intr {
	struct rmixl_pcix_softc *sc;
	LIST_HEAD(, rmixl_pcix_dispatch) dispatch;
	u_int intrpin;
	u_int ipl;
	bool enabled;
	void *ih;			/* mips interrupt handle */
} rmixl_pcix_intr_t;

#define RMIXL_PCIX_NINTR	4	/* PCI INT[A,B,C,D] */

typedef struct rmixl_pcix_softc {
	device_t                	sc_dev;
	struct mips_pci_chipset 	sc_pci_chipset;
	bus_space_tag_t              	sc_pci_cfg_memt;
	bus_space_tag_t              	sc_pci_ecfg_memt;
	bus_dma_tag_t			sc_29bit_dmat;
	bus_dma_tag_t			sc_32bit_dmat;
	bus_dma_tag_t			sc_64bit_dmat;
	int				sc_tmsk;
	void 			       *sc_fatal_ih;
	void 			       *sc_ih;
	rmixl_pcix_intr_t		sc_intr[RMIXL_PCIX_NINTR];
	bool				sc_intr_init_done;	
} rmixl_pcix_softc_t;


extern void rmixl_physaddr_init_pcix(struct extent *);

#endif  /* _MIPS_RMI_PCIX_VAR_H_ */

