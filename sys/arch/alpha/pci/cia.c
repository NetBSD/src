/* $NetBSD: cia.c,v 1.14.2.5 1997/08/12 05:55:48 cgd Exp $ */

/*
 * Copyright Notice:
 *
 * Copyright (c) 1997 Christopher G. Demetriou.  All rights reserved.
 *
 * License:
 *
 * This License applies to this software ("Software"), created
 * by Christopher G. Demetriou ("Author").
 *
 * You may use, copy, modify and redistribute this Software without
 * charge, in either source code form, binary form, or both, on the
 * following conditions:
 *
 * 1.  (a) Binary code: (i) a complete copy of the above copyright notice
 * must be included within each copy of the Software in binary code form,
 * and (ii) a complete copy of the above copyright notice and all terms
 * of this License as presented here must be included within each copy of
 * all documentation accompanying or associated with binary code, in any
 * medium, along with a list of the software modules to which the license
 * applies.
 *
 * (b) Source Code: A complete copy of the above copyright notice and all
 * terms of this License as presented here must be included within: (i)
 * each copy of the Software in source code form, and (ii) each copy of
 * all accompanying or associated documentation, in any medium.
 *
 * 2. The following Acknowledgment must be used in communications
 * involving the Software as described below:
 *
 *      This product includes software developed by
 *      Christopher G. Demetriou for the NetBSD Project.
 *
 * The Acknowledgment must be conspicuously and completely displayed
 * whenever the Software, or any software, products or systems containing
 * the Software, are mentioned in advertising, marketing, informational
 * or publicity materials of any kind, whether in print, electronic or
 * other media (except for information provided to support use of
 * products containing the Software by existing users or customers).
 *
 * 3. The name of the Author may not be used to endorse or promote
 * products derived from this Software without specific prior written
 * permission (conditions (1) and (2) above are not considered
 * endorsement or promotion).
 *
 * 4.  This license applies to: (a) all copies of the Software, whether
 * partial or whole, original or modified, and (b) your actions, and the
 * actions of all those who may act on your behalf.  All uses not
 * expressly permitted are reserved to the Author.
 *
 * 5.  Disclaimer.  THIS SOFTWARE IS MADE AVAILABLE BY THE AUTHOR TO THE
 * PUBLIC FOR FREE AND "AS IS.''  ALL USERS OF THIS FREE SOFTWARE ARE
 * SOLELY AND ENTIRELY RESPONSIBLE FOR THEIR OWN CHOICE AND USE OF THIS
 * SOFTWARE FOR THEIR OWN PURPOSES.  BY USING THIS SOFTWARE, EACH USER
 * AGREES THAT THE AUTHOR SHALL NOT BE LIABLE FOR DAMAGES OF ANY KIND IN
 * RELATION TO ITS USE OR PERFORMANCE.
 *
 * 6.  If you have a special need for a change in one or more of these
 * license conditions, please contact the Author via electronic mail to
 *
 *     cgd@NetBSD.ORG
 *
 * or via the contact information on
 *
 *     http://www.NetBSD.ORG/People/Pages/cgd.html
 */

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

#include <machine/options.h>		/* Config options headers */
#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: cia.c,v 1.14.2.5 1997/08/12 05:55:48 cgd Exp $");
__KERNEL_COPYRIGHT(0,
    "Copyright (c) 1997 Christopher G. Demetriou.  All rights reserved.");

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

int	ciamatch __P((struct device *, struct cfdata *, void *));
void	ciaattach __P((struct device *, struct device *, void *));

struct cfattach cia_ca = {
	sizeof(struct cia_softc), ciamatch, ciaattach,
};

struct cfdriver cia_cd = {
	NULL, "cia", DV_DULL,
};

static int	ciaprint __P((void *, const char *pnp));

/* There can be only one. */
int ciafound;
struct cia_config cia_configuration;

int
ciamatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct confargs *ca = aux;

	/* Make sure that we're looking for a CIA. */
	if (strcmp(ca->ca_name, cia_cd.cd_name) != 0)
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

	ccp->cc_pass =
	    (REGVAL(CIA_CSR_CIA_REV) & CIA_REV_REV_MASK) >> CIA_REV_REV_SHIFT;
	ccp->cc_is21172 = (ccp->cc_pass >= 3);
	ccp->cc_bwx_enabled = ccp->cc_is21172 &&
	    (REGVAL(CIA_CSR_CIA_CNFG) & CIA_CNFG_IOA_BWEN);

	ccp->cc_hae_mem = REGVAL(CIA_CSR_HAE_MEM);
	ccp->cc_hae_io = REGVAL(CIA_CSR_HAE_IO);

	if (!ccp->cc_initted) {
		/* don't do these twice since they set up extents */
		ccp->cc_iot = cia_swiz_bus_io_init(ccp);
		ccp->cc_memt = cia_swiz_bus_mem_init(ccp);
		cia_swiz_pci_init(&ccp->cc_pc, ccp); /* no ext. yet */
	}
	ccp->cc_mallocsafe = mallocsafe;

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

	/* note that we've attached the chipset; can't have 2 CIAs. */
	ciafound = 1;

	/*
	 * set up the chipset's info; done once at console init time
	 * (maybe), but we must do it here as well to take care of things
	 * that need to use memory allocation.
	 */
	ccp = sc->sc_ccp = &cia_configuration;
	cia_init(ccp, 1);

	printf(": CIA pass %d (%s)", ccp->cc_pass,
	    ccp->cc_is21172 ? "21172" : "21171");
	if (ccp->cc_is21172)
	    printf(", BWX %sabled", ccp->cc_bwx_enabled ? "en" : "dis");
        printf("\n");

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

	default:
		panic("ciaattach: shouldn't be here, really...");
	}

	pba.pba_busname = "pci";
	pba.pba_iot = ccp->cc_iot;
	pba.pba_memt = ccp->cc_memt;
	pba.pba_dmat = &ccp->cc_dmat_direct;
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
