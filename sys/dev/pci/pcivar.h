/*	$NetBSD: pcivar.h,v 1.50 2002/05/16 01:01:30 thorpej Exp $	*/

/*
 * Copyright (c) 1996, 1997 Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1994 Charles M. Hannum.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Charles M. Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _DEV_PCI_PCIVAR_H_
#define	_DEV_PCI_PCIVAR_H_

/*
 * Definitions for PCI autoconfiguration.
 *
 * This file describes types and functions which are used for PCI
 * configuration.  Some of this information is machine-specific, and is
 * provided by pci_machdep.h.
 */

#include <sys/device.h>
#include <machine/bus.h>
#include <dev/pci/pcireg.h>

/*
 * Structures and definitions needed by the machine-dependent header.
 */
typedef u_int32_t pcireg_t;		/* configuration space register XXX */
struct pcibus_attach_args;
struct pci_softc;

#ifdef _KERNEL
/*
 * Machine-dependent definitions.
 */
#include <machine/pci_machdep.h>

/*
 * PCI bus attach arguments.
 */
struct pcibus_attach_args {
	char		*pba_busname;	/* XXX should be common */
	bus_space_tag_t pba_iot;	/* pci i/o space tag */
	bus_space_tag_t pba_memt;	/* pci mem space tag */
	bus_dma_tag_t pba_dmat;		/* DMA tag */
	pci_chipset_tag_t pba_pc;
	int		pba_flags;	/* flags; see below */

	int		pba_bus;	/* PCI bus number */

	/*
	 * Pointer to the pcitag of our parent bridge.  If there is no
	 * parent bridge, then we assume we are a root bus.
	 */
	pcitag_t	*pba_bridgetag;

	/*
	 * Interrupt swizzling information.  These fields
	 * are only used by secondary busses.
	 */
	u_int		pba_intrswiz;	/* how to swizzle pins */
	pcitag_t	pba_intrtag;	/* intr. appears to come from here */
};

/*
 * PCI device attach arguments.
 */
struct pci_attach_args {
	bus_space_tag_t pa_iot;		/* pci i/o space tag */
	bus_space_tag_t pa_memt;	/* pci mem space tag */
	bus_dma_tag_t pa_dmat;		/* DMA tag */
	pci_chipset_tag_t pa_pc;
	int		pa_flags;	/* flags; see below */

	u_int		pa_bus;
	u_int		pa_device;
	u_int		pa_function;
	pcitag_t	pa_tag;
	pcireg_t	pa_id, pa_class;

	/*
	 * Interrupt information.
	 *
	 * "Intrline" is used on systems whose firmware puts
	 * the right routing data into the line register in
	 * configuration space.  The rest are used on systems
	 * that do not.
	 */
	u_int		pa_intrswiz;	/* how to swizzle pins if ppb */
	pcitag_t	pa_intrtag;	/* intr. appears to come from here */
	pci_intr_pin_t	pa_intrpin;	/* intr. appears on this pin */
	pci_intr_line_t	pa_intrline;	/* intr. routing information */
};

/*
 * Flags given in the bus and device attachment args.
 */
#define	PCI_FLAGS_IO_ENABLED	0x01		/* I/O space is enabled */
#define	PCI_FLAGS_MEM_ENABLED	0x02		/* memory space is enabled */
#define	PCI_FLAGS_MRL_OKAY	0x04		/* Memory Read Line okay */
#define	PCI_FLAGS_MRM_OKAY	0x08		/* Memory Read Multiple okay */
#define	PCI_FLAGS_MWI_OKAY	0x10		/* Memory Write and Invalidate
						   okay */

/*
 * PCI device 'quirks'.
 *
 * In general strange behaviour which can be handled by a driver (e.g.
 * a bridge's inability to pass a type of access correctly) should be.
 * The quirks table should only contain information which impacts
 * the operation of the MI PCI code and which can't be pushed lower
 * (e.g. because it's unacceptable to require a driver to be present
 * for the information to be known).
 */
struct pci_quirkdata {
	pci_vendor_id_t		vendor;		/* Vendor ID */
	pci_product_id_t	product;	/* Product ID */
	int			quirks;		/* quirks; see below */
};
#define	PCI_QUIRK_MULTIFUNCTION		1

#include "locators.h"

struct pci_softc {
	struct device sc_dev;
	bus_space_tag_t sc_iot, sc_memt;
	bus_dma_tag_t sc_dmat; 
	pci_chipset_tag_t sc_pc; 
	int sc_bus, sc_maxndevs;
	pcitag_t *sc_bridgetag;
	u_int sc_intrswiz;
	pcitag_t sc_intrtag;
	int sc_flags;
};

extern struct cfdriver pci_cd;

/*
 * Locators devices that attach to 'pcibus', as specified to config.
 */
#define	pcibuscf_bus		cf_loc[PCIBUSCF_BUS]
#define	PCIBUS_UNK_BUS		PCIBUSCF_BUS_DEFAULT	/* wildcarded 'bus' */

/*
 * Locators for PCI devices, as specified to config.
 */
#define	pcicf_dev		cf_loc[PCICF_DEV]
#define	PCI_UNK_DEV		PCICF_DEV_DEFAULT	/* wildcarded 'dev' */

#define	pcicf_function		cf_loc[PCICF_FUNCTION]
#define	PCI_UNK_FUNCTION	PCICF_FUNCTION_DEFAULT /* wildcarded 'function' */

/*
 * Configuration space access and utility functions.  (Note that most,
 * e.g. make_tag, conf_read, conf_write are declared by pci_machdep.h.)
 */
pcireg_t pci_mapreg_type __P((pci_chipset_tag_t, pcitag_t, int));
int	pci_mapreg_info __P((pci_chipset_tag_t, pcitag_t, int, pcireg_t,
	    bus_addr_t *, bus_size_t *, int *));
int	pci_mapreg_map __P((struct pci_attach_args *, int, pcireg_t, int,
	    bus_space_tag_t *, bus_space_handle_t *, bus_addr_t *,
	    bus_size_t *));

int pci_get_capability __P((pci_chipset_tag_t, pcitag_t, int,
			    int *, pcireg_t *));

/*
 * Helper functions for autoconfiguration.
 */
int	pci_enumerate_bus_generic(struct pci_softc *,
	    int (*)(struct pci_attach_args *), struct pci_attach_args *);
int	pci_probe_device(struct pci_softc *, pcitag_t tag,
	    int (*)(struct pci_attach_args *), struct pci_attach_args *);
void	pci_devinfo __P((pcireg_t, pcireg_t, int, char *));
void	pci_conf_print __P((pci_chipset_tag_t, pcitag_t,
	    void (*)(pci_chipset_tag_t, pcitag_t, const pcireg_t *)));
const struct pci_quirkdata *
	pci_lookup_quirkdata __P((pci_vendor_id_t, pci_product_id_t));

/*
 * Helper functions for user access to the PCI bus.
 */
struct proc;
int	pci_devioctl __P((pci_chipset_tag_t, pcitag_t, u_long, caddr_t,
	    int flag, struct proc *));

/*
 * Misc.
 */
char   *pci_findvendor __P((pcireg_t));
int	pci_find_device(struct pci_attach_args *pa,
			int (*match)(struct pci_attach_args *));

#endif /* _KERNEL */

#endif /* _DEV_PCI_PCIVAR_H_ */
