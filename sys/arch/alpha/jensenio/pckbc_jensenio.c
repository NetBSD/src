/* $NetBSD: pckbc_jensenio.c,v 1.2 2001/07/12 23:25:40 thorpej Exp $ */

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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: pckbc_jensenio.c,v 1.2 2001/07/12 23:25:40 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/queue.h>
#include <sys/lock.h> 

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/ic/i8042reg.h>
#include <dev/ic/pckbcvar.h> 

#include <dev/eisa/eisavar.h>

#include <dev/isa/isavar.h>

#include <alpha/jensenio/jenseniovar.h>

struct pckbc_jensenio_softc {
	struct	pckbc_softc sc_pckbc;	/* real "pckbc" softc */

	/* Jensen-specific goo. */
	void	*sc_ih[PCKBC_NSLOTS];	/* interrupt handlers */
	int	sc_irq[PCKBC_NSLOTS];	/* Sable IRQs to use */
	eisa_chipset_tag_t sc_ec;	/* EISA chipset for registering intrs */
};

int	pckbc_jensenio_match(struct device *, struct cfdata *, void *);
void	pckbc_jensenio_attach(struct device *, struct device *, void *);

struct cfattach pckbc_jensenio_ca = {
	sizeof(struct pckbc_jensenio_softc),
	    pckbc_jensenio_match, pckbc_jensenio_attach
};

void	pckbc_jensenio_intr_establish(struct pckbc_softc *, pckbc_slot_t);

int
pckbc_jensenio_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct jensenio_attach_args *ja = aux;

	/* Always present. */
	if (strcmp(ja->ja_name, match->cf_driver->cd_name) == 0)
		return (1);

	return (0);
}

void
pckbc_jensenio_attach(struct device *parent, struct device *self, void *aux)
{
	struct pckbc_jensenio_softc *jsc = (void *)self;
	struct pckbc_softc *sc = &jsc->sc_pckbc;
	struct jensenio_attach_args *ja = aux;
	struct pckbc_internal *t;
	bus_space_handle_t ioh_d, ioh_c;

	jsc->sc_ec = ja->ja_ec;

	/*
	 * Set up IRQs.
	 */
	jsc->sc_irq[PCKBC_KBD_SLOT] = ja->ja_irq[0];
	jsc->sc_irq[PCKBC_AUX_SLOT] = ja->ja_irq[1];

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

	printf("\n");

	/* Finish off the attach. */
	pckbc_attach(sc);
}

void
pckbc_jensenio_intr_establish(struct pckbc_softc *sc, pckbc_slot_t slot)
{
	struct pckbc_jensenio_softc *jsc = (void *) sc;
	const char *intrstr;

	intrstr = eisa_intr_string(jsc->sc_ec, jsc->sc_irq[slot]);
	jsc->sc_ih[slot] = eisa_intr_establish(jsc->sc_ec, jsc->sc_irq[slot],
	    IST_EDGE, IPL_TTY, pckbcintr, sc);
	if (jsc->sc_ih[slot] == NULL) {
		printf("%s: unable to establish interrupt for %s slot",
		    sc->sc_dv.dv_xname, pckbc_slot_names[slot]);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf("%s: %s slot interrupting at %s\n", sc->sc_dv.dv_xname,
	    pckbc_slot_names[slot], intrstr);
}
