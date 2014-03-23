/*	$NetBSD: pckbc_pnpbios.c,v 1.18 2014/03/23 02:57:58 christos Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

/*
 * PNPBIOS attachment for the PC Keyboard Controller driver.
 *
 * This is a little wonky.  The keyboard controller actually
 * has 2 PNPBIOS nodes: one for the controller and the keyboard
 * interrupt, and one for the aux port (mouse) interrupt.
 *
 * For this reason, we actually attach *two* instances of this
 * driver.  After both of them have been found, then we attach
 * sub-devices.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pckbc_pnpbios.c,v 1.18 2014/03/23 02:57:58 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/queue.h>
#include <sys/bus.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <dev/ic/i8042reg.h>
#include <dev/ic/pckbcvar.h>

#include <i386/pnpbios/pnpbiosvar.h>

int	pckbc_pnpbios_match(device_t, cfdata_t, void *);
void	pckbc_pnpbios_attach(device_t, device_t, void *);

struct pckbc_pnpbios_softc {
	struct pckbc_softc sc_pckbc;

	isa_chipset_tag_t sc_ic;
	int sc_irq;
	int sc_ist;
	pckbc_slot_t sc_slot;
};

/* Save first port: */
static struct pckbc_pnpbios_softc *first;

extern struct cfdriver pckbc_cd;

CFATTACH_DECL_NEW(pckbc_pnpbios, sizeof(struct pckbc_pnpbios_softc),
    pckbc_pnpbios_match, pckbc_pnpbios_attach, NULL, NULL);

void	pckbc_pnpbios_intr_establish(struct pckbc_softc *, pckbc_slot_t);
static void pckbc_pnpbios_finish_attach(device_t);

int
pckbc_pnpbios_match(device_t parent, cfdata_t match, void *aux)
{
	struct pnpbiosdev_attach_args *aa = aux;

	if (strcmp(aa->idstr, "PNP0303") == 0 ||
	    strcmp(aa->idstr, "PNP0320") == 0)	/* Japanese 106 */
		return (1);			/* plus more? */
	if (strcmp(aa->idstr, "PNP0F13") == 0)
		return (1);

	return (0);
}

void
pckbc_pnpbios_attach(device_t parent, device_t self, void *aux)
{
	struct pckbc_pnpbios_softc *psc = device_private(self);
	struct pckbc_softc *sc = &psc->sc_pckbc;
	struct pckbc_internal *t;
	struct pnpbiosdev_attach_args *aa = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh_d, ioh_c;
	int iobase;

	sc->sc_dv = self;
	if (strncmp(aa->idstr, "PNP03", 5) == 0) {
		psc->sc_slot = PCKBC_KBD_SLOT;
	} else if (strcmp(aa->idstr, "PNP0F13") == 0) {
		psc->sc_slot = PCKBC_AUX_SLOT;
	} else {
		aprint_error(": unknown port!\n");
		panic("pckbc_pnpbios_attach: impossible");
	}

	aprint_normal(": %s port\n", pckbc_slot_names[psc->sc_slot]);

	if (pnpbios_getirqnum(aa->pbt, aa->resc, 0, &psc->sc_irq,
	    &psc->sc_ist) != 0) {
		aprint_error_dev(self, "unable to get IRQ number or type\n");
		return;
	}

	psc->sc_ic = aa->ic;

	if (!first)
		first = psc;

	if (!first->sc_pckbc.id) {

		if (pnpbios_getiobase(aa->pbt, aa->resc, 0, &iot, &iobase))
			return;

		if (pckbc_is_console(iot, iobase)) {
			t = &pckbc_consdata;
			ioh_d = t->t_ioh_d;
			ioh_c = t->t_ioh_c;
			pckbc_console_attached = 1;
			/* t->t_cmdbyte was initialized by cnattach */
		} else {
			if (pnpbios_io_map(aa->pbt, aa->resc, 0,
					   &iot, &ioh_d) ||
			    pnpbios_io_map(aa->pbt, aa->resc, 1,
			    		   &iot, &ioh_c))
				panic("pckbc_pnpbios_attach: couldn't map");

			t = malloc(sizeof(struct pckbc_internal),
			    M_DEVBUF, M_WAITOK);
			memset(t, 0, sizeof(*t));
			t->t_iot = iot;
			t->t_ioh_d = ioh_d;
			t->t_ioh_c = ioh_c;
			t->t_addr = iobase;
			t->t_cmdbyte = KC8_CPU;	/* Enable ports */
			callout_init(&t->t_cleanup, 0);
		}

		t->t_sc = &first->sc_pckbc;
		first->sc_pckbc.id = t;

		first->sc_pckbc.intr_establish = pckbc_pnpbios_intr_establish;
		config_defer(first->sc_pckbc.sc_dv,
			     pckbc_pnpbios_finish_attach);
	}
}

static void
pckbc_pnpbios_finish_attach(device_t dv)
{

	pckbc_attach(device_private(dv));
}

void
pckbc_pnpbios_intr_establish(struct pckbc_softc *sc,
    pckbc_slot_t slot)
{
	struct pckbc_pnpbios_softc *psc;
	isa_chipset_tag_t ic = NULL;
	void *rv = NULL;
	int irq, ist;
	int i;

	/*
	 * Note we're always called with sc == first.
	 */
	for (i = 0; i < pckbc_cd.cd_ndevs; i++) {
		psc = device_lookup_private(&pckbc_cd, i);
		if (psc && psc->sc_slot == slot) {
			irq = psc->sc_irq;
			ist = psc->sc_ist;
			ic = psc->sc_ic;
			break;
		}
	}
	if (i < pckbc_cd.cd_ndevs)
		rv = isa_intr_establish(ic, irq, ist, IPL_TTY, pckbcintr, sc);
	if (rv == NULL) {
		aprint_error_dev(sc->sc_dv,
		    "unable to establish interrupt for %s slot\n",
		    pckbc_slot_names[slot]);
	} else {
		aprint_normal_dev(sc->sc_dv, "using irq %d for %s slot\n",
		    irq, pckbc_slot_names[slot]);
	}
}
