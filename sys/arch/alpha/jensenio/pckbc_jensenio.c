/* $NetBSD: pckbc_jensenio.c,v 1.12 2014/03/21 16:39:29 christos Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: pckbc_jensenio.c,v 1.12 2014/03/21 16:39:29 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/queue.h>
#include <sys/intr.h>
#include <sys/bus.h>

#include <dev/ic/i8042reg.h>
#include <dev/ic/pckbcvar.h>

#include <dev/eisa/eisavar.h>

#include <dev/isa/isavar.h>

#include <alpha/jensenio/jenseniovar.h>

struct pckbc_jensenio_intrcookie {
	struct pckbc_softc *ic_sc;
	struct evcnt ic_ev;
	u_long ic_vector;
	char ic_vecstr[8];
};

struct pckbc_jensenio_softc {
	struct	pckbc_softc sc_pckbc;	/* real "pckbc" softc */

	/* Jensen-specific goo. */
	struct pckbc_jensenio_intrcookie sc_ic[PCKBC_NSLOTS];
};

int	pckbc_jensenio_match(device_t, cfdata_t, void *);
void	pckbc_jensenio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(pckbc_jensenio, sizeof(struct pckbc_jensenio_softc),
    pckbc_jensenio_match, pckbc_jensenio_attach, NULL, NULL);

void	pckbc_jensenio_intr_establish(struct pckbc_softc *, pckbc_slot_t);
void	pckbc_jensenio_intr(void *, u_long);

int
pckbc_jensenio_match(device_t parent, cfdata_t match, void *aux)
{
	struct jensenio_attach_args *ja = aux;

	/* Always present. */
	if (strcmp(ja->ja_name, match->cf_name) == 0)
		return (1);

	return (0);
}

void
pckbc_jensenio_attach(device_t parent, device_t self, void *aux)
{
	struct pckbc_jensenio_softc *jsc = device_private(self);
	struct pckbc_softc *sc = &jsc->sc_pckbc;
	struct jensenio_attach_args *ja = aux;
	struct pckbc_internal *t;
	bus_space_handle_t ioh_d, ioh_c;

	sc->sc_dv = self;

	/*
	 * Set up IRQs.
	 */
	jsc->sc_ic[PCKBC_KBD_SLOT].ic_vector = ja->ja_irq[0];
	jsc->sc_ic[PCKBC_AUX_SLOT].ic_vector = ja->ja_irq[1];

	sc->intr_establish = pckbc_jensenio_intr_establish;

	if (pckbc_is_console(ja->ja_iot, ja->ja_ioaddr)) {
		t = &pckbc_consdata;
		pckbc_console_attached = 1;
		/* t->t_cmdbyte was initialized by cnattach */
	} else {
		if (bus_space_map(ja->ja_iot, ja->ja_ioaddr + KBDATAP,
				  1, 0, &ioh_d) != 0 ||
		    bus_space_map(ja->ja_iot, ja->ja_ioaddr + KBCMDP,
				  1, 0, &ioh_c) != 0)
			panic("pckbc_jensenio_attach: couldn't map");

		t = malloc(sizeof(struct pckbc_internal), M_DEVBUF, M_WAITOK);
		memset(t, 0, sizeof(struct pckbc_internal));
		t->t_iot = ja->ja_iot;
		t->t_ioh_d = ioh_d;
		t->t_ioh_c = ioh_c;
		t->t_addr = ja->ja_ioaddr;
		t->t_cmdbyte = KC8_CPU; /* Enable ports */
	}

	t->t_sc = sc;
	sc->id = t;

	aprint_normal("\n");

	/* Finish off the attach. */
	pckbc_attach(sc);
}

void
pckbc_jensenio_intr_establish(struct pckbc_softc *sc, pckbc_slot_t slot)
{
	struct pckbc_jensenio_softc *jsc = (void *) sc;

	jsc->sc_ic[slot].ic_sc = sc;

	scb_set(jsc->sc_ic[slot].ic_vector, pckbc_jensenio_intr,
	    &jsc->sc_ic[slot], IPL_VM);
	aprint_normal_dev(sc->sc_dv, "%s slot interrupting at vector 0x%lx\n",
	    pckbc_slot_names[slot], jsc->sc_ic[slot].ic_vector);

	snprintf(jsc->sc_ic[slot].ic_vecstr, sizeof(jsc->sc_ic[slot].ic_vecstr),
	    "0x%lx", jsc->sc_ic[slot].ic_vector);
	evcnt_attach_dynamic(&jsc->sc_ic[slot].ic_ev, EVCNT_TYPE_INTR,
	    NULL, "vector", jsc->sc_ic[slot].ic_vecstr);
}

void
pckbc_jensenio_intr(void *arg, u_long vec)
{
	struct pckbc_jensenio_intrcookie *ic = arg;

	ic->ic_ev.ev_count++;
	(void) pckbcintr(ic->ic_sc);
}
