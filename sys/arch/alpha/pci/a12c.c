/* $NetBSD: a12c.c,v 1.22.2.1 2011/06/06 09:04:44 jruoho Exp $ */

/* [Notice revision 2.2]
 * Copyright (c) 1997, 1998 Avalon Computer Systems, Inc.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL AVALON OR THE CONTRIBUTORS
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

__KERNEL_RCSID(0, "$NetBSD: a12c.c,v 1.22.2.1 2011/06/06 09:04:44 jruoho Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/rpb.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <alpha/alpha/clockvar.h>

#include <alpha/pci/a12creg.h>
#include <alpha/pci/a12cvar.h>
#include <alpha/pci/pci_a12.h>

#define	A12C()	/* Generate ctags(1) key */

int	a12cmatch(struct device *, struct cfdata *, void *);
void	a12cattach(struct device *, struct device *, void *);

CFATTACH_DECL(a12c, sizeof(struct a12c_softc),
    a12cmatch, a12cattach, NULL, NULL);

extern struct cfdriver a12c_cd;

static const struct clocktime zeroct;

static void noclock_init(struct device *);
static void noclock_get(struct device *, time_t, struct clocktime *);
static void noclock_set(struct device *, struct clocktime *);

static const struct clockfns noclock_fns = {
	noclock_init, noclock_get, noclock_set
};

/* There can be only one. */

int a12cfound;
struct a12c_config a12c_configuration;

int
a12cmatch(struct device *parent, struct cfdata *match, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	return	cputype == ST_AVALON_A12
	    &&	strcmp(ma->ma_name, a12c_cd.cd_name) == 0
	    && !a12cfound;
}

void
a12c_init(struct a12c_config *ccp, int mallocsafe)
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
a12cattach(struct device *parent, struct device *self, void *aux)
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
	printf(": driver %s over logic %x\n", "$Revision: 1.22.2.1 $", 
		A12_ALL_EXTRACT(REGVAL(A12_VERS)));

	pci_a12_pickintr(ccp);
	clockattach(noclock_init, NULL);	/* XXX? */

	memset(&pba, 0, sizeof(pba));
	pba.pba_iot = 0;
	pba.pba_memt = ccp->ac_memt;
	pba.pba_dmat = &ccp->ac_dmat_direct;
	pba.pba_dmat64 = NULL;
	pba.pba_pc = &ccp->ac_pc;
	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;
	pba.pba_flags = PCI_FLAGS_MEM_OKAY |
	    PCI_FLAGS_MRL_OKAY | PCI_FLAGS_MRM_OKAY | PCI_FLAGS_MWI_OKAY;

	config_found_ia(self, "pcibus", &pba, pcibusprint);

	pba.pba_bus     = 1;
	config_found_ia(self, "a12c_xb", &pba, NULL);

	pba.pba_bus     = 2;
	config_found_ia(self, "a12c_a12dc", &pba, NULL);
}

static void noclock_init(void (*)(void *), void *)
{

	/* nothing */
}
