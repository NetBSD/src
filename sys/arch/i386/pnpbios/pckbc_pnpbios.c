/*	$NetBSD: pckbc_pnpbios.c,v 1.2 2001/03/31 09:20:40 minoura Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/queue.h>
#include <sys/lock.h>

#include <machine/bus.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <dev/ic/i8042reg.h>
#include <dev/ic/pckbcvar.h>

#include <i386/pnpbios/pnpbiosvar.h>

int	pckbc_pnpbios_match(struct device *, struct cfdata *, void *);
void	pckbc_pnpbios_attach(struct device *, struct device *, void *);

struct pckbc_pnpbios_softc {
	struct pckbc_softc sc_pckbc;

	int sc_irq;
	int sc_ist;
	pckbc_slot_t sc_slot;

	/* Only on keyboard port... */
	void *sc_ih;
	struct pckbc_pnpbios_softc *sc_aux_port;
};

extern struct cfdriver pckbc_cd;

struct cfattach pckbc_pnpbios_ca = {
	sizeof(struct pckbc_pnpbios_softc), pckbc_pnpbios_match,
	    pckbc_pnpbios_attach,
};

void	pckbc_pnpbios_intr_establish(struct pckbc_softc *, pckbc_slot_t);

int
pckbc_pnpbios_match(struct device *parent,
    struct cfdata *match,
    void *aux)
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
pckbc_pnpbios_attach(struct device *parent,
    struct device *self,
    void *aux)
{
	struct pckbc_pnpbios_softc *psc = (void *) self, *peer_psc, *kbd_psc;
	struct pckbc_softc *sc = &psc->sc_pckbc;
	struct pckbc_internal *t;
	struct pnpbiosdev_attach_args *aa = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh_d, ioh_c;
	pckbc_slot_t peer;
	int iobase, i;

	if (strncmp(aa->idstr, "PNP03", 5) == 0) {
		psc->sc_slot = PCKBC_KBD_SLOT;
		peer = PCKBC_AUX_SLOT;
	} else if (strcmp(aa->idstr, "PNP0F13") == 0) {
		psc->sc_slot = PCKBC_AUX_SLOT;
		peer = PCKBC_KBD_SLOT;
	} else {
		printf(": unknown port!\n");
		panic("pckbc_pnpbios_attach: impossible");
	}

	printf(": %s port\n", pckbc_slot_names[psc->sc_slot]);

	if (pnpbios_getirqnum(aa->pbt, aa->resc, 0, &psc->sc_irq,
	    &psc->sc_ist) != 0) {
		printf("%s: unable to get IRQ number or type\n",
		    sc->sc_dv.dv_xname);
		return;
	}

	if (psc->sc_slot == PCKBC_KBD_SLOT) {
		if (pnpbios_getiobase(aa->pbt, aa->resc, 0, &iot, &iobase)) {
			printf("%s: can't get iobase\n", sc->sc_dv.dv_xname);
			return;
		}

		sc->intr_establish = pckbc_pnpbios_intr_establish;

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
			callout_init(&t->t_cleanup);
		}

		t->t_sc = sc;
		sc->id = t;
	}

	for (i = 0; i < pckbc_cd.cd_ndevs; i++) {
		peer_psc = pckbc_cd.cd_devs[i];
		if (peer_psc != NULL &&
		    peer_psc != psc &&
		    peer_psc->sc_pckbc.sc_dv.dv_parent ==
		     psc->sc_pckbc.sc_dv.dv_parent)
			break;
		peer_psc = NULL;
	}

	if (peer_psc != NULL) {
		/*
		 * Have both -- finish attaching the one marked "keyboard".
		 */
		if (psc->sc_slot == PCKBC_KBD_SLOT) {
			kbd_psc = psc;
			kbd_psc->sc_aux_port = peer_psc;
		} else {
			kbd_psc = peer_psc;
			kbd_psc->sc_aux_port = psc;
		}
		pckbc_attach(&kbd_psc->sc_pckbc);
	}
}

void
pckbc_pnpbios_intr_establish(struct pckbc_softc *sc,
    pckbc_slot_t slot)
{
	struct pckbc_pnpbios_softc *psc = (void *) sc;
	void *rv;
	int irq, ist;

	/*
	 * Note we're always called with the keyboard slot.
	 */

	if (slot == PCKBC_KBD_SLOT) {
		irq = psc->sc_irq;
		ist = psc->sc_ist;
	} else {
		irq = psc->sc_aux_port->sc_irq;
		ist = psc->sc_aux_port->sc_ist;
	}

	rv = isa_intr_establish(0/*XXX*/, irq, ist, IPL_TTY, pckbcintr, sc);
	if (rv == NULL) {
		printf("%s: unable to establish interrupt for %s slot\n",
		    sc->sc_dv.dv_xname, pckbc_slot_names[slot]);
	} else {
		printf("%s: using irq %d for %s slot\n", sc->sc_dv.dv_xname,
		    irq, pckbc_slot_names[slot]);
	}
}
