/* $NetBSD: a12c.c,v 1.1 1998/01/29 21:42:50 ross Exp $ */

/* [Notice revision 2.0]
 * Copyright (c) 1997 Avalon Computer Systems, Inc.
 * All rights reserved.
 *
 * Author: Ross Harvey
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright and
 *    author notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Avalon Computer Systems, Inc. nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. This copyright will be assigned to The NetBSD Foundation on
 *    1/1/2000 unless these terms (including possibly the assignment
 *    date) are updated in writing by Avalon prior to the latest specified
 *    assignment date.
 *
 * THIS SOFTWARE IS PROVIDED BY AVALON COMPUTER SYSTEMS, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_avalon_a12.h"		/* Config options headers */
#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: a12c.c,v 1.1 1998/01/29 21:42:50 ross Exp $");

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
#include <alpha/pci/a12creg.h>
#include <alpha/pci/a12cvar.h>
#include <alpha/pci/pci_a12.h>

int	a12cmatch __P((struct device *, struct cfdata *, void *));
void	a12cattach __P((struct device *, struct device *, void *));

struct cfattach a12c_ca = {
	sizeof(struct a12c_softc), a12cmatch, a12cattach,
};

extern struct cfdriver a12c_cd;

static int	a12cprint __P((void *, const char *pnp));

static void hrh_noclock(void);

/* There can be only one. */
int a12cfound;
struct a12c_config a12c_configuration;

int
a12cmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct confargs *ca = aux;

	/* Make sure that we're looking for an A12C. */
	if (strcmp(ca->ca_name, a12c_cd.cd_name) != 0)
		return (0);

	if (a12cfound)
		return (0);

	return (1);
}

/*
 * Set up the chipset's function pointers.
 */
void
a12c_init(ccp, mallocsafe)
	struct a12c_config *ccp;
	int mallocsafe;
{

	if (!ccp->ac_initted) {
		/* someday these may allocate memory, do once only */

		ccp->ac_iot  = 0;
		ccp->ac_memt = a12c_bus_mem_init(ccp);
	}
	ccp->ac_mallocsafe = mallocsafe;

	a12c_pci_init(&ccp->ac_pc, ccp);

	a12c_dma_init(ccp);

	ccp->ac_initted = 1;
}

void
a12cattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct a12c_softc *sc = (struct a12c_softc *)self;
	struct a12c_config *ccp;
	struct pcibus_attach_args pba;

	/* note that we've attached the chipset; can't have 2 A12Cs. */
	a12cfound = 1;

	/*
	 * set up the chipset's info; done once at console init time
	 * (maybe), but we must do it here as well to take care of things
	 * that need to use memory allocation.
	 */
	ccp = sc->sc_ccp = &a12c_configuration;
	a12c_init(ccp, 1);

	/* XXX print chipset information */
	printf("\n");

	switch (hwrpb->rpb_type) {

#ifdef AVALON_A12
	case ST_AVALON_A12:
		pci_a12_pickintr(ccp);
#ifdef EVCNT_COUNTERS
		evcnt_attach(self, "intr", &a12_intr_evcnt);
#endif
		break;
#endif

	default:
		panic("a12cattach: shouldn't be here, really...");
	}

	pba.pba_busname = "pci";
	pba.pba_iot = 0;
	pba.pba_memt = ccp->ac_memt;
	pba.pba_dmat = &ccp->ac_dmat_direct;
	pba.pba_pc = &ccp->ac_pc;
	pba.pba_bus = 0;
	pba.pba_flags = PCI_FLAGS_MEM_ENABLED;
	config_found(self, &pba, a12cprint);

	hrh_noclock();
}

static int
a12cprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	register struct pcibus_attach_args *pba = aux;

	/* can attach xbar or pci to a12c */
	if (pnp)
		printf("%s at %s", pba->pba_busname, pnp);
	printf(" bus %d", pba->pba_bus);
	return (UNCONF);
}

#include <dev/dec/clockvar.h>

static const struct clocktime zeroct;

static void noclock_init(struct device *);
static void noclock_get(struct device *, time_t, struct clocktime *);
static void noclock_set(struct device *, struct clocktime *);

static void noclock_init(struct device *dev) {
	dev = dev;
}

static void noclock_get(struct device *dev, time_t t, struct clocktime *ct)
{
	*ct = zeroct;
}

static void noclock_set(struct device *dev, struct clocktime *ct)
{
	if(dev!=NULL)
		*ct = *ct;
}

static const struct clockfns noclock_fns = {
	noclock_init, noclock_get, noclock_set
};

static void
hrh_noclock(void) {
	extern const struct clockfns *clockfns;

	clockfns = &noclock_fns;
}
