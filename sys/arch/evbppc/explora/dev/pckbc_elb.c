/*	$NetBSD: pckbc_elb.c,v 1.1 2003/03/11 10:57:57 hannken Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juergen Hannken-Illjes.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <machine/bus.h>

#include <dev/ic/i8042reg.h>
#include <dev/ic/pckbcvar.h>

#include <evbppc/explora/dev/elbvar.h>

struct pckbc_elb_softc {
	struct pckbc_softc sc_pckbc;
	int sc_irq;
};

static int	pckbc_elb_probe(struct device *, struct cfdata *, void *);
static void	pckbc_elb_attach(struct device *, struct device *, void *);
static void	pckbc_elb_intr_establish(struct pckbc_softc *, pckbc_slot_t);

CFATTACH_DECL(pckbc_elb, sizeof(struct pckbc_elb_softc),
    pckbc_elb_probe, pckbc_elb_attach, NULL, NULL);

int
pckbc_elb_probe(struct device *parent, struct cfdata *cf, void *aux)
{
	struct elb_attach_args *oaa = aux;

	if (strcmp(oaa->elb_name, cf->cf_name) != 0)
		return 0;

	return (1);
}

void
pckbc_elb_attach(struct device *parent, struct device *self, void *aux)
{
	struct pckbc_elb_softc *msc = (void *)self;
	struct pckbc_softc *sc = &msc->sc_pckbc;
	struct elb_attach_args *eaa = aux;
	struct pckbc_internal *t;

	/*
	 * Setup interrupt data.
	 */
	msc->sc_irq = eaa->elb_irq;
	sc->intr_establish = pckbc_elb_intr_establish;

	if (pckbc_is_console(eaa->elb_bt, eaa->elb_base)) {
		t = &pckbc_consdata;
		pckbc_console_attached = 1;
	} else {
		t = malloc(sizeof(struct pckbc_internal), M_DEVBUF, M_WAITOK);
		memset(t, 0, sizeof(struct pckbc_internal));
	}

	t->t_iot = eaa->elb_bt;
	bus_space_map(eaa->elb_bt, eaa->elb_base, 1, 0, &t->t_ioh_d);
	bus_space_map(eaa->elb_bt, eaa->elb_base2, 1, 0, &t->t_ioh_c);
	t->t_addr = eaa->elb_base;
	t->t_sc = sc;
	sc->id = t;

	printf("\n");

	pckbc_attach(sc);
}

static void
pckbc_elb_intr_establish(struct pckbc_softc *sc, pckbc_slot_t slot)
{
	struct pckbc_elb_softc *msc = (void *)sc;
	int irq = msc->sc_irq;

	/*
	 * We ignore slot since all slots use the same interrupt.
	 */

	if (irq >= 0)
	 	intr_establish(irq, IST_LEVEL, IPL_SERIAL, pckbcintr, sc);

	irq = -1;
}
