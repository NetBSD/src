/* $NetBSD: cia.c,v 1.39 1998/06/05 02:13:42 thorpej Exp $ */

/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include "opt_dec_eb164.h"
#include "opt_dec_kn20aa.h"
#include "opt_dec_550.h"

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: cia.c,v 1.39 1998/06/05 02:13:42 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <vm/vm.h>

#include <machine/autoconf.h>
#include <machine/rpb.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <alpha/pci/ciareg.h>
#include <alpha/pci/ciavar.h>
#ifdef DEC_KN20AA
#include <alpha/pci/pci_kn20aa.h>
#endif
#ifdef DEC_EB164
#include <alpha/pci/pci_eb164.h>
#endif
#ifdef DEC_550
#include <alpha/pci/pci_550.h>
#endif

int	ciamatch __P((struct device *, struct cfdata *, void *));
void	ciaattach __P((struct device *, struct device *, void *));

struct cfattach cia_ca = {
	sizeof(struct cia_softc), ciamatch, ciaattach,
};

extern struct cfdriver cia_cd;

static int	ciaprint __P((void *, const char *pnp));

/* There can be only one. */
int ciafound;
struct cia_config cia_configuration;

/*
 * This determines if we attempt to use BWX for PCI bus and config space
 * access.  Some systems, notably with Pyxis, don't fare so well unless
 * BWX is used.
 */
#ifndef CIA_USE_BWX
#define	CIA_USE_BWX	1
#endif

int	cia_use_bwx = CIA_USE_BWX;

int
ciamatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;

	/* Make sure that we're looking for a CIA. */
	if (strcmp(ma->ma_name, cia_cd.cd_name) != 0)
		return (0);

	if (ciafound)
		return (0);

	return (1);
}

/*
 * Set up the chipset's function pointers.
 */
void
cia_init(ccp, mallocsafe)
	struct cia_config *ccp;
	int mallocsafe;
{

	ccp->cc_hae_mem = REGVAL(CIA_CSR_HAE_MEM);
	ccp->cc_hae_io = REGVAL(CIA_CSR_HAE_IO);
	ccp->cc_rev = REGVAL(CIA_CSR_REV) & REV_MASK;

	/*
	 * Determine if we have a Pyxis.  Only two systypes can
	 * have this: the EB164 systype (AlphaPC164LX and AlphaPC164SX)
	 * and the DEC_550 systype (Miata).
	 */
	if ((hwrpb->rpb_type == ST_EB164 &&
	     (hwrpb->rpb_variation & SV_ST_MASK) >= SV_ST_ALPHAPC164LX_400) ||
	    hwrpb->rpb_type == ST_DEC_550)
		ccp->cc_flags |= CCF_ISPYXIS;

	/*
	 * Revisions >= 2 have the CNFG register.
	 */
	if (ccp->cc_rev >= 2)
		ccp->cc_cnfg = REGVAL(CIA_CSR_CNFG);
	else
		ccp->cc_cnfg = 0;

	/*
	 * Use BWX iff:
	 *
	 *	- It hasn't been disbled by the user,
	 *	- it's enabled in CNFG,
	 *	- we're implementation version ev5,
	 *	- BWX is enabled in the CPU's capabilities mask (yes,
	 *	  the bit is really cleared if the capability exists...)
	 */
	if (cia_use_bwx != 0 &&
	    (ccp->cc_cnfg & CNFG_BWEN) != 0 &&
	    alpha_implver() == ALPHA_IMPLVER_EV5 &&
	    alpha_amask(ALPHA_AMASK_BWX) == 0) {
		u_int32_t ctrl;

		ccp->cc_flags |= CCF_USEBWX;

		/*
		 * For whatever reason, the firmware seems to enable PCI
		 * loopback mode if it also enables BWX.  Make sure it's
		 * enabled if we have an old, buggy firmware rev.
		 */
		alpha_mb();
		ctrl = REGVAL(CIA_CSR_CTRL);
		if ((ctrl & CTRL_PCI_LOOP_EN) == 0) {
			REGVAL(CIA_CSR_CTRL) = ctrl | CTRL_PCI_LOOP_EN;
			alpha_mb();
		}
	}

	if (!ccp->cc_initted) {
		/* don't do these twice since they set up extents */
		if (ccp->cc_flags & CCF_USEBWX) {
			cia_bwx_bus_io_init(&ccp->cc_iot, ccp);
			cia_bwx_bus_mem_init(&ccp->cc_memt, ccp);
		} else {
			cia_swiz_bus_io_init(&ccp->cc_iot, ccp);
			cia_swiz_bus_mem_init(&ccp->cc_memt, ccp);
		}
	}
	ccp->cc_mallocsafe = mallocsafe;

	cia_pci_init(&ccp->cc_pc, ccp);

	cia_dma_init(ccp);

	ccp->cc_initted = 1;
}

void
ciaattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct cia_softc *sc = (struct cia_softc *)self;
	struct cia_config *ccp;
	struct pcibus_attach_args pba;
	char bits[64];

	/* note that we've attached the chipset; can't have 2 CIAs. */
	ciafound = 1;

	/*
	 * set up the chipset's info; done once at console init time
	 * (maybe), but we must do it here as well to take care of things
	 * that need to use memory allocation.
	 */
	ccp = sc->sc_ccp = &cia_configuration;
	cia_init(ccp, 1);

	printf(": DECchip 2117x Core Logic Chipset (%s), pass %d\n",
	    (ccp->cc_flags & CCF_ISPYXIS) ? "Pyxis" : "ALCOR/ALCOR2",
	    ccp->cc_rev + 1);
	if (ccp->cc_cnfg)
		printf("%s: extended capabilities: %s\n", self->dv_xname,
		    bitmask_snprintf(ccp->cc_cnfg, CIA_CSR_CNFG_BITS,
		    bits, sizeof(bits)));
#if 1
	if (ccp->cc_flags & CCF_USEBWX)
		printf("%s: using BWX for PCI config and device access\n",
		    self->dv_xname);
#endif

	switch (hwrpb->rpb_type) {
#ifdef DEC_KN20AA
	case ST_DEC_KN20AA:
		pci_kn20aa_pickintr(ccp);
#ifdef EVCNT_COUNTERS
		evcnt_attach(self, "intr", &kn20aa_intr_evcnt);
#endif
		break;
#endif

#ifdef DEC_EB164
	case ST_EB164:
		pci_eb164_pickintr(ccp);
#ifdef EVCNT_COUNTERS
		evcnt_attach(self, "intr", &eb164_intr_evcnt);
#endif
		break;
#endif

#ifdef DEC_550
	case ST_DEC_550:
		pci_550_pickintr(ccp);
#ifdef EVCNT_COUNTERS
		evcnt_attach(self, "intr", &dec_550_intr_evcnt);
#endif
		break;
#endif

	default:
		panic("ciaattach: shouldn't be here, really...");
	}

	pba.pba_busname = "pci";
	pba.pba_iot = &ccp->cc_iot;
	pba.pba_memt = &ccp->cc_memt;
	pba.pba_dmat =
	    alphabus_dma_get_tag(&ccp->cc_dmat_direct, ALPHA_BUS_PCI);
	pba.pba_pc = &ccp->cc_pc;
	pba.pba_bus = 0;
	pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED;
	config_found(self, &pba, ciaprint);
}

static int
ciaprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	register struct pcibus_attach_args *pba = aux;

	/* only PCIs can attach to CIAs; easy. */
	if (pnp)
		printf("%s at %s", pba->pba_busname, pnp);
	printf(" bus %d", pba->pba_bus);
	return (UNCONF);
}
