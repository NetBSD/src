/*	$NetBSD: pckbc_mainbus.c,v 1.3.2.2 2002/04/01 07:43:36 nathanw Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge and Eduardo Horvath for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
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
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/malloc.h> 
#include <sys/errno.h>
#include <sys/queue.h>

#include <machine/autoconf.h>
#include <machine/intr.h>
#include <machine/bus.h>
#include <machine/walnut.h>

#include <dev/ic/i8042reg.h>
#include <dev/ic/pckbcvar.h> 

struct pckbc_mainbus_softc {
	struct pckbc_softc sc_pckbc;

	// XXX void *sc_ih[PCKBC_NSLOTS];
	int sc_irq[PCKBC_NSLOTS];

};

static int	pckbc_mainbus_probe(struct device *, struct cfdata *, void *);
static void	pckbc_mainbus_attach(struct device *, struct device *, void *);
static void	pckbc_mainbus_intr_establish(struct pckbc_softc *, pckbc_slot_t);

struct cfattach pckbc_mainbus_ca = {
	sizeof(struct pckbc_mainbus_softc), pckbc_mainbus_probe, pckbc_mainbus_attach
};

int pckbcfound = 0;

int
pckbc_mainbus_probe(struct device *parent, struct cfdata *cf, void *aux)
{
	struct mainbus_attach_args *maa = aux;

	/* match only pckbc devices */
	if (strcmp(maa->mb_name, cf->cf_driver->cd_name) != 0)
		return 0;

	return (pckbcfound < 1);
}

struct pckbc_softc *pckbc0; /* XXX */

void
pckbc_mainbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct pckbc_mainbus_softc *msc = (void *)self;
	struct pckbc_softc *sc = &msc->sc_pckbc;
	struct mainbus_attach_args *maa = aux;
	struct pckbc_internal *t;
	bus_space_handle_t ioh_d, ioh_c;
	bus_space_tag_t iot = galaxy_make_bus_space_tag(0, 0);
	u_long addr = maa->mb_addr;
	
	/*
	 * Set up IRQs
	 */
	msc->sc_irq[PCKBC_KBD_SLOT] = maa->mb_irq;
	msc->sc_irq[PCKBC_AUX_SLOT] = maa->mb_irq + 1;	/* XXX */

	sc->intr_establish = pckbc_mainbus_intr_establish;

	if (pckbc_is_console(iot, addr)) {
		t = &pckbc_consdata;
		pckbc_console_attached = 1;
		/* t->t_cmdbyte was initialized by cnattach */
	} else {
		if (bus_space_map(iot, addr + KEY_MOUSE_DATA, 1, 0, &ioh_d) ||
		    bus_space_map(iot, addr + KEY_MOUSE_CMD, 1, 0, &ioh_c))
			panic("pckbc_attach: couldn't map");

		t = malloc(sizeof(struct pckbc_internal), M_DEVBUF, M_WAITOK);
		memset(t, 0, sizeof(struct pckbc_internal));
		t->t_iot = iot;
		t->t_ioh_d = ioh_d;
		t->t_ioh_c = ioh_c;
		t->t_addr = addr;
		t->t_cmdbyte = KC8_CPU; /* Enable ports */
	}

	t->t_sc = sc;
	sc->id = t;

	printf("\n");

	/* Finish off the attach. */
	pckbc_attach(sc);

	pckbcfound++;

	return;
}

static void
pckbc_mainbus_intr_establish(struct pckbc_softc *sc, pckbc_slot_t slot)
{
	struct pckbc_mainbus_softc *msc = (void *)sc;
	int irq = msc->sc_irq[slot];

	if (slot > PCKBC_NSLOTS) {
		printf("pckbc_mainbus_intr_establish: attempt to establish "
		    "interrupt at slot %d\n", slot);
		return;
	}
		
	intr_establish(irq, IST_LEVEL, IPL_SERIAL, pckbcintr, sc);
	printf("%s: %s slot interrupting at irq %d\n", sc->sc_dv.dv_xname,
	    pckbc_slot_names[slot], irq);

}
