/*	$NetBSD: plumisa_machdep.c,v 1.2 2001/09/16 05:32:20 uch Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
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

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/bus.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isareg.h>

#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/dev/plumvar.h>
#include <hpcmips/dev/plumicuvar.h>
#include <hpcmips/dev/plumiobusvar.h>

#include "locators.h"

int	plumisabprint(void *, const char *);
int	plumisabmatch(struct device *, struct cfdata *, void *);
void	plumisabattach(struct device *, struct device *, void *);

struct plumisab_softc {
	struct device sc_dev;
	plum_chipset_tag_t sc_pc;
	bus_space_tag_t sc_iot;
	int sc_irq;
	void *sc_ih;
};

struct cfattach plumisab_ca = {
	sizeof(struct plumisab_softc), plumisabmatch, plumisabattach
};

int
plumisabmatch(struct device *parent, struct cfdata *match, void *aux)
{
	struct plumiobus_attach_args *pba = aux;
	platid_mask_t mask;
    
	if (strcmp(pba->pba_busname, match->cf_driver->cd_name)) {
		return (0);
	}

	if (match->cf_loc[PLUMIOBUSIFCF_PLATFORM] == 
	    PLUMIOBUSIFCF_PLATFORM_DEFAULT) {
		return (1);
	}

	mask = PLATID_DEREF(match->cf_loc[PLUMIOBUSIFCF_PLATFORM]);
	if (platid_match(&platid, &mask)) {
		return (2);
	}

	return (0);
}

void
plumisabattach(struct device *parent, struct device *self, void *aux)
{
	struct plumiobus_attach_args *pba = aux;
	struct plumisab_softc *sc = (void*)self;
	struct isabus_attach_args iba;
    
	printf("\n");
	sc->sc_pc = pba->pba_pc;
	sc->sc_iot = pba->pba_iot;
	sc->sc_irq = pba->pba_irq;
	printf(" base=%#x irq=%d\n", sc->sc_iot->t_base, sc->sc_irq);
#if 0
	/* Reset I/O bus */
	plum_power_ioreset(sc->sc_pc);
#endif
	/* Dump I/O port */
	if (0) {
		bus_space_handle_t ioh;
		int i;
		bus_space_map(sc->sc_iot, 0, 0x400, 0, &ioh);
		for(i = 0; i < 0x400; i += 4) {
			printf("[%03x]%02x", i, bus_space_read_1(sc->sc_iot, ioh, i + 3));
			printf("%02x", bus_space_read_1(sc->sc_iot, ioh, i + 2));
			printf("%02x", bus_space_read_1(sc->sc_iot, ioh, i + 1));
			printf("%02x", bus_space_read_1(sc->sc_iot, ioh, i + 0));
		}
		bus_space_unmap(sc->sc_iot, ioh, 0x400);
	}

	iba.iba_busname = "isa";
	iba.iba_ic	= sc;
	/* Plum ISA-bus don't have memory space! */
	/* Plum ISA port space */
	iba.iba_iot     = sc->sc_iot;
	config_found(self, &iba, plumisabprint);
}

int
plumisabprint(void *aux, const char *pnp)
{

	return (pnp ? QUIET : UNCONF);
}

void
isa_attach_hook(struct device *parent, struct device *self,
    struct isabus_attach_args *iba)
{

}

void *
isa_intr_establish(isa_chipset_tag_t ic, int irq, int type, int level,
    int (*ih_fun)(void *), void *ih_arg)
{
	struct plumisab_softc *sc = (void*)ic;
	
	sc->sc_ih = plum_intr_establish(sc->sc_pc, sc->sc_irq, type, level,
					ih_fun, ih_arg);
	return (sc->sc_ih);
}

void
isa_intr_disestablish(isa_chipset_tag_t ic, void *arg)
{
	struct plumisab_softc *sc = (void*)ic;

	plum_intr_disestablish(sc->sc_pc, arg);
}

int
isa_intr_alloc(isa_chipset_tag_t ic, int mask, int type, int *irq)
{
	struct plumisab_softc *sc = (void*)ic;
	
	*irq = sc->sc_irq;
	
	return (0);
}
