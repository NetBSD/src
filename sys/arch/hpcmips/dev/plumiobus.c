/*	$NetBSD: plumiobus.c,v 1.2.2.1 1999/12/27 18:32:02 wrstuden Exp $ */

/*
 * Copyright (c) 1999, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include "opt_tx39_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/dev/plumvar.h>
#include <hpcmips/dev/plumicuvar.h>
#include <hpcmips/dev/plumpowervar.h>
#include <hpcmips/dev/plumiobusreg.h>
#include <hpcmips/dev/plumiobusvar.h>

#include "locators.h"

int	plumiobus_match __P((struct device*, struct cfdata*, void*));
void	plumiobus_attach __P((struct device*, struct device*, void*));
int	plumiobus_print __P((void*, const char*));
int	plumiobus_search __P((struct device*, struct cfdata*, void*));

struct plumisa_resource {
	int		pr_irq;
	bus_space_tag_t	pr_iot;
	int		pr_enabled;
};

struct plumiobus_softc {
	struct	device		sc_dev;
	plum_chipset_tag_t	sc_pc;
	bus_space_tag_t		sc_regt;
	bus_space_handle_t	sc_regh;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	struct plumisa_resource	sc_isa[PLUM_IOBUS_IO5CSMAX];
};

struct cfattach plumiobus_ca = {
	sizeof(struct plumiobus_softc), plumiobus_match, plumiobus_attach
};

void	plumiobus_dump __P((struct plumiobus_softc*));
bus_space_tag_t __plumiobus_subregion __P((bus_space_tag_t, bus_addr_t, bus_size_t));

int
plumiobus_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return 1;
}

void
plumiobus_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct plum_attach_args *pa = aux;
	struct plumiobus_softc *sc = (void*)self;
	struct plumisa_resource *pr;

	sc->sc_pc	= pa->pa_pc;
	sc->sc_regt	= pa->pa_regt;
	sc->sc_iot	= pa->pa_iot;

	if (bus_space_map(sc->sc_regt, PLUM_IOBUS_REGBASE, 
			  PLUM_IOBUS_REGSIZE, 0, &sc->sc_regh)) {
		printf(": register map failed.\n");
		return;
	}
	printf("\n");
	plum_power_establish(sc->sc_pc, PLUM_PWR_IO5);
	
	/* Address space <-> IRQ mapping */
	pr = &sc->sc_isa[IO5CS0];
	pr->pr_irq = PLUM_INT_EXT5IO0;
	pr->pr_iot = __plumiobus_subregion(
		sc->sc_iot, 
		PLUM_IOBUS_IOBASE + PLUM_IOBUS_IO5CS0BASE,
		PLUM_IOBUS_IO5SIZE);

	pr = &sc->sc_isa[IO5CS1];
	pr->pr_irq = PLUM_INT_EXT5IO1;
	pr->pr_iot = __plumiobus_subregion(
		sc->sc_iot, 
		PLUM_IOBUS_IOBASE + PLUM_IOBUS_IO5CS1BASE,
		PLUM_IOBUS_IO5SIZE);

	pr = &sc->sc_isa[IO5CS2];
	pr->pr_irq = PLUM_INT_EXT5IO2;
	pr->pr_iot = __plumiobus_subregion(
		sc->sc_iot, 
		PLUM_IOBUS_IOBASE + PLUM_IOBUS_IO5CS2BASE,
		PLUM_IOBUS_IO5SIZE);

	pr = &sc->sc_isa[IO5CS3];
	pr->pr_irq = PLUM_INT_EXT5IO3;
	pr->pr_iot = __plumiobus_subregion(
		sc->sc_iot, 
		PLUM_IOBUS_IOBASE + PLUM_IOBUS_IO5CS3BASE,
		PLUM_IOBUS_IO5SIZE);

	pr = &sc->sc_isa[IO5CS4];
	pr->pr_irq = PLUM_INT_EXT3IO0; /* XXX */
	pr->pr_iot = __plumiobus_subregion(
		sc->sc_iot, 
		PLUM_IOBUS_IOBASE + PLUM_IOBUS_IO5CS4BASE,
		PLUM_IOBUS_IO5SIZE);


	pr = &sc->sc_isa[IO5NCS];
	pr->pr_irq = PLUM_INT_EXT3IO1;
	pr->pr_iot = __plumiobus_subregion(
		sc->sc_iot, 
		PLUM_IOBUS_IOBASE + PLUM_IOBUS_IO5CS5BASE,
		PLUM_IOBUS_IO5SIZE);


	plumiobus_dump(sc);

	config_search(plumiobus_search, self, plumiobus_print);
}

/* XXX something kludge */
bus_space_tag_t
__plumiobus_subregion(t, ofs, size)
	bus_space_tag_t t;
	bus_addr_t ofs;
	bus_size_t size;
{
	struct hpcmips_bus_space *hbs;
	
	if (!(hbs = malloc(sizeof(struct hpcmips_bus_space), 
			   M_DEVBUF, M_NOWAIT))) {
		panic ("__plumiobus_subregion: no memory.");
	}
	*hbs = *t;
	hbs->t_base += ofs;
	hbs->t_size = size;
	
	return hbs;
}

int
plumiobus_search(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct plumiobus_softc *sc = (void*)parent;
	struct plumiobus_attach_args pba;
	int slot;
	
	/* Disallow wildcarded IO5CS slot */
	if (cf->cf_loc[PLUMIOBUSIFCF_SLOT] == PLUMIOBUSIFCF_SLOT_DEFAULT) {
		printf("plumiobus_search: wildcarded slot, skipping\n");
		return 0;
	}
	slot = pba.pba_slot = cf->cf_loc[PLUMIOBUSIFCF_SLOT];

	pba.pba_pc	= sc->sc_pc;
	pba.pba_iot	= sc->sc_isa[slot].pr_iot;
	pba.pba_irq	= sc->sc_isa[slot].pr_irq;
	pba.pba_busname	= "plumisab";
	
	if (!(sc->sc_isa[slot].pr_enabled) && /* not attached slot */
	    (*cf->cf_attach->ca_match)(parent, cf, &pba)) {
		config_attach(parent, cf, &pba, plumiobus_print);
		sc->sc_isa[slot].pr_enabled = 1;
	}

	return 0;
}

int
plumiobus_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	return pnp ? QUIET : UNCONF;
}

void
plumiobus_dump(sc)
	struct plumiobus_softc *sc;
{
	bus_space_tag_t regt = sc->sc_regt;
	bus_space_handle_t regh = sc->sc_regh;
	plumreg_t reg;
	int i, wait;
	
	reg = plum_conf_read(regt, regh, PLUM_IOBUS_IOXBSZ_REG);
	printf("8bit port:");
	for (i = 0; i < 6; i++) {
		if (reg & (1 << i)) {
			printf(" IO5CS%d", i);
		}
	}
	printf("\n");

	reg = PLUM_IOBUS_IOXCCNT_MASK &
		plum_conf_read(regt, regh, PLUM_IOBUS_IOXCCNT_REG);
	printf(" # of wait to become from the access begining: %d clock\n",
	       reg + 1);
	reg = plum_conf_read(regt, regh, PLUM_IOBUS_IOXACNT_REG);
	printf(" # of wait in access clock: ");
	for (i = 0; i < 5; i++) {
		wait = (reg >> (i * PLUM_IOBUS_IOXACNT_SHIFT))
			& PLUM_IOBUS_IOXACNT_MASK;
		printf("[CS%d:%d] ", i, wait + 1);
	}
	printf("\n");

	reg = PLUM_IOBUS_IOXSCNT_MASK &
		plum_conf_read(regt, regh, PLUM_IOBUS_IOXSCNT_REG);
	printf(" # of wait during access by I/O bus : %d clock\n", reg + 1);
	
	reg = plum_conf_read(regt, regh, PLUM_IOBUS_IDEMODE_REG);
	if (reg & PLUM_IOBUS_IDEMODE) {
		printf("IO5CS3,4 IDE mode\n");
	}
	
}

