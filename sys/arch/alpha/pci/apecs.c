/* $NetBSD: apecs.c,v 1.36 1999/11/04 19:15:22 thorpej Exp $ */

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

#include "opt_dec_2100_a50.h"
#include "opt_dec_eb64plus.h"
#include "opt_dec_1000a.h"
#include "opt_dec_1000.h"

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: apecs.c,v 1.36 1999/11/04 19:15:22 thorpej Exp $");

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
#include <alpha/pci/apecsreg.h>
#include <alpha/pci/apecsvar.h>
#ifdef DEC_2100_A50
#include <alpha/pci/pci_2100_a50.h>
#endif
#ifdef DEC_EB64PLUS
#include <alpha/pci/pci_eb64plus.h>
#endif
#ifdef DEC_1000A
#include <alpha/pci/pci_1000a.h>
#endif
#ifdef DEC_1000
#include <alpha/pci/pci_1000.h>
#endif

int	apecsmatch __P((struct device *, struct cfdata *, void *));
void	apecsattach __P((struct device *, struct device *, void *));

struct cfattach apecs_ca = {
	sizeof(struct apecs_softc), apecsmatch, apecsattach,
};

extern struct cfdriver apecs_cd;

static int	apecsprint __P((void *, const char *pnp));

/* There can be only one. */
int apecsfound;
struct apecs_config apecs_configuration;

int
apecsmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;

	/* Make sure that we're looking for an APECS. */
	if (strcmp(ma->ma_name, apecs_cd.cd_name) != 0)
		return (0);

	if (apecsfound)
		return (0);

	return (1);
}

/*
 * Set up the chipset's function pointers.
 */
void
apecs_init(acp, mallocsafe)
	struct apecs_config *acp;
	int mallocsafe;
{
	acp->ac_comanche_pass2 =
	    (REGVAL(COMANCHE_ED) & COMANCHE_ED_PASS2) != 0;
	acp->ac_memwidth =
	    (REGVAL(COMANCHE_GCR) & COMANCHE_GCR_WIDEMEM) != 0 ? 128 : 64;
	acp->ac_epic_pass2 =
	    (REGVAL(EPIC_DCSR) & EPIC_DCSR_PASS2) != 0;

	acp->ac_haxr1 = REGVAL(EPIC_HAXR1);
	acp->ac_haxr2 = REGVAL(EPIC_HAXR2);

	if (!acp->ac_initted) {
		/* don't do these twice since they set up extents */
		apecs_bus_io_init(&acp->ac_iot, acp);
		apecs_bus_mem_init(&acp->ac_memt, acp);
	}
	acp->ac_mallocsafe = mallocsafe;

	apecs_pci_init(&acp->ac_pc, acp);

	acp->ac_initted = 1;
}

void
apecsattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct apecs_softc *sc = (struct apecs_softc *)self;
	struct apecs_config *acp;
	struct pcibus_attach_args pba;

	/* note that we've attached the chipset; can't have 2 APECSes. */
	apecsfound = 1;

	/*
	 * set up the chipset's info; done once at console init time
	 * (maybe), but doesn't hurt to do twice.
	 */
	acp = sc->sc_acp = &apecs_configuration;
	apecs_init(acp, 1);

	apecs_dma_init(acp);

	printf(": DECchip %s Core Logic chipset\n",
	    acp->ac_memwidth == 128 ? "21072" : "21071");
	printf("%s: DC21071-CA pass %d, %d-bit memory bus\n",
	    self->dv_xname, acp->ac_comanche_pass2 ? 2 : 1, acp->ac_memwidth);
	printf("%s: DC21071-DA pass %d\n", self->dv_xname,
	    acp->ac_epic_pass2 ? 2 : 1);
	/* XXX print bcache size */

	if (!acp->ac_epic_pass2)
		printf("WARNING: 21071-DA NOT PASS2... NO BETS...\n");

	switch (cputype) {
#ifdef DEC_2100_A50
	case ST_DEC_2100_A50:
		pci_2100_a50_pickintr(acp);
		break;
#endif

#ifdef DEC_EB64PLUS
	case ST_EB64P:
		pci_eb64plus_pickintr(acp);
		break;
#endif

#ifdef DEC_1000A
	case ST_DEC_1000A:
		pci_1000a_pickintr(acp, &acp->ac_iot, &acp->ac_memt,
			&acp->ac_pc);
		break;
#endif

#ifdef DEC_1000
	case ST_DEC_1000:
		pci_1000_pickintr(acp, &acp->ac_iot, &acp->ac_memt,
			&acp->ac_pc);
		break;
#endif

	default:
		panic("apecsattach: shouldn't be here, really...");
	}

	pba.pba_busname = "pci";
	pba.pba_iot = &acp->ac_iot;
	pba.pba_memt = &acp->ac_memt;
	pba.pba_dmat =
	    alphabus_dma_get_tag(&acp->ac_dmat_direct, ALPHA_BUS_PCI);
	pba.pba_pc = &acp->ac_pc;
	pba.pba_bus = 0;
	pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED |
	    PCI_FLAGS_MRL_OKAY | PCI_FLAGS_MRM_OKAY | PCI_FLAGS_MWI_OKAY;
	config_found(self, &pba, apecsprint);
}

static int
apecsprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	register struct pcibus_attach_args *pba = aux;

	/* only PCIs can attach to APECSes; easy. */
	if (pnp)
		printf("%s at %s", pba->pba_busname, pnp);
	printf(" bus %d", pba->pba_bus);
	return (UNCONF);
}
