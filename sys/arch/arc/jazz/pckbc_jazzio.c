/* $NetBSD: pckbc_jazzio.c,v 1.14.30.1 2007/07/15 13:15:35 ad Exp $ */
/* NetBSD: pckbc_isa.c,v 1.2 2000/03/23 07:01:35 thorpej Exp  */

/*
 * Copyright (c) 1998
 *	Matthias Drochner.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pckbc_jazzio.c,v 1.14.30.1 2007/07/15 13:15:35 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/queue.h>
#include <sys/lock.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <arc/jazz/pica.h>
#include <arc/jazz/jazziovar.h>

#include <dev/ic/i8042reg.h>
#include <dev/ic/pckbcvar.h>
#include <arc/jazz/pckbc_jazzioreg.h>

#define PMS_INTR 7	/* XXX - should be obtained from firmware */
#define PICA_KBCMDP	(PICA_SYS_KBD + JAZZIO_KBCMDP)

int	pckbc_jazzio_match(struct device *, struct cfdata *, void *);
void	pckbc_jazzio_attach(struct device *, struct device *, void *);
void	pckbc_jazzio_intr_establish(struct pckbc_softc *, pckbc_slot_t);

struct pckbc_jazzio_softc {
	struct pckbc_softc sc_pckbc;

	int sc_intr[PCKBC_NSLOTS];
};

CFATTACH_DECL(pckbc_jazzio, sizeof(struct pckbc_jazzio_softc),
    pckbc_jazzio_match, pckbc_jazzio_attach, NULL, NULL);

int
pckbc_jazzio_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct jazzio_attach_args *ja = aux;
	bus_space_tag_t iot = ja->ja_bust;
	bus_space_handle_t ioh_d, ioh_c;
	bus_addr_t addr = ja->ja_addr;
	int res, ok = 1;

	if (strcmp(ja->ja_name, "I8742") != 0)
		return 0;

	if (pckbc_is_console(iot, addr) == 0) {
		struct pckbc_internal t;

		if (bus_space_map(iot, addr + KBDATAP, 1, 0, &ioh_d))
			return 0;
		if (bus_space_map(iot, PICA_KBCMDP, 1, 0, &ioh_c)) {
			bus_space_unmap(iot, ioh_d, 1);
			return 0;
		}

		memset(&t, 0, sizeof(t));
		t.t_iot = iot;
		t.t_ioh_d = ioh_d;
		t.t_ioh_c = ioh_c;

		/* flush KBC */
		(void)pckbc_poll_data1(&t, PCKBC_KBD_SLOT);

		/* KBC selftest */
		if (pckbc_send_cmd(iot, ioh_c, KBC_SELFTEST) == 0) {
			ok = 0;
			goto out;
		}
		res = pckbc_poll_data1(&t, PCKBC_KBD_SLOT);
		if (res != 0x55) {
			printf("kbc selftest: %x\n", res);
			ok = 0;
		}
 out:
		bus_space_unmap(iot, ioh_d, 1);
		bus_space_unmap(iot, ioh_c, 1);
	}

	return ok;
}

void
pckbc_jazzio_attach(struct device *parent, struct device *self, void *aux)
{
	struct jazzio_attach_args *ja = aux;
	struct pckbc_jazzio_softc *jsc = (void *)self;
	struct pckbc_softc *sc = &jsc->sc_pckbc;
	struct pckbc_internal *t;
	bus_space_tag_t iot = ja->ja_bust;
	bus_space_handle_t ioh_d, ioh_c;
	bus_addr_t addr = ja->ja_addr;

	sc->intr_establish = pckbc_jazzio_intr_establish;

	/*
	 * To establish interrupt handler
	 *
	 * XXX handcrafting "aux" slot...
	 */
	jsc->sc_intr[PCKBC_KBD_SLOT] = ja->ja_intr;
	jsc->sc_intr[PCKBC_AUX_SLOT] = PMS_INTR;		/* XXX */

	if (pckbc_is_console(iot, addr)) {
		t = &pckbc_consdata;
		ioh_d = t->t_ioh_d;
		ioh_c = t->t_ioh_c;
		pckbc_console_attached = 1;
		/* t->t_cmdbyte was initialized by cnattach */
	} else {
		if (bus_space_map(iot, addr + KBDATAP, 1, 0, &ioh_d) ||
		    bus_space_map(iot, PICA_KBCMDP, 1, 0, &ioh_c))
			panic("pckbc_attach: couldn't map");

		t = malloc(sizeof(struct pckbc_internal), M_DEVBUF,
		    M_WAITOK | M_ZERO);
		t->t_iot = iot;
		t->t_ioh_d = ioh_d;
		t->t_ioh_c = ioh_c;
		t->t_addr = addr;
		t->t_cmdbyte = KC8_CPU; /* Enable ports */
		callout_init(&t->t_cleanup, 0);
	}

	t->t_sc = sc;
	sc->id = t;

	printf("\n");

	/* Finish off the attach. */
	pckbc_attach(sc);
}

void
pckbc_jazzio_intr_establish(struct pckbc_softc *sc, pckbc_slot_t slot)
{
	struct pckbc_jazzio_softc *jsc = (void *) sc;

	jazzio_intr_establish(jsc->sc_intr[slot], pckbcintr, sc);
}
