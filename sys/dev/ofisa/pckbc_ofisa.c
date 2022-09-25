/* $NetBSD: pckbc_ofisa.c,v 1.20 2022/09/25 17:29:33 thorpej Exp $ */

/*
 * Copyright (c) 1998
 *	Matthias Drochner.  All rights reserved.
 * Copyright (c) 2001
 *	Matt Thomas.  All rights reserved.
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
__KERNEL_RCSID(0, "$NetBSD: pckbc_ofisa.c,v 1.20 2022/09/25 17:29:33 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/errno.h>
#include <sys/queue.h>
#include <sys/bus.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <dev/ofw/openfirm.h>
#include <dev/ofisa/ofisavar.h>

#include <dev/ic/i8042reg.h>
#include <dev/ic/pckbcvar.h>

static int pckbc_ofisa_match (device_t, cfdata_t, void *);
static void pckbc_ofisa_attach (device_t, device_t, void *);

struct pckbc_ofisa_softc {
	struct pckbc_softc sc_pckbc;

	isa_chipset_tag_t sc_ic;
	struct ofisa_intr_desc sc_intr[PCKBC_NSLOTS];
};

CFATTACH_DECL_NEW(pckbc_ofisa, sizeof(struct pckbc_ofisa_softc),
    pckbc_ofisa_match, pckbc_ofisa_attach, NULL, NULL);

static void pckbc_ofisa_intr_establish (struct pckbc_softc *, pckbc_slot_t);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "INTC,80c42" },
	DEVICE_COMPAT_EOL
};

static const struct device_compatible_entry port_compat_data[] = {
	{ .compat = "pnpPNP,303",	.value = PCKBC_KBD_SLOT },
	{ .compat = "pnpPNP,f03",	.value = PCKBC_AUX_SLOT },
	DEVICE_COMPAT_EOL
};

static int
pckbc_ofisa_match(device_t parent, cfdata_t match, void *aux)
{
	struct ofisa_attach_args *aa = aux;

	return of_compatible_match(aa->oba.oba_phandle, compat_data) ? 5 : 0;
}

static void
pckbc_ofisa_attach(device_t parent, device_t self, void *aux)
{
	struct pckbc_ofisa_softc *osc = device_private(self);
	struct pckbc_softc *sc = &osc->sc_pckbc;
	struct ofisa_attach_args *aa = aux;
	const struct device_compatible_entry *dce;
	struct pckbc_internal *t;
	bus_space_tag_t iot;
	bus_space_handle_t ioh_d, ioh_c;
	struct ofisa_reg_desc regs[2];
	int phandle;
	int n;

	sc->sc_dv = self;
	osc->sc_ic = aa->ic;
	iot = aa->iot;

	phandle = OF_child(aa->oba.oba_phandle);
	while (phandle != 0) {
		dce = of_compatible_lookup(phandle, port_compat_data);
		if (dce != NULL) {
			ofisa_intr_get(phandle, &osc->sc_intr[dce->value], 1);
		}
		phandle = OF_peer(phandle);
	}

	sc->intr_establish = pckbc_ofisa_intr_establish;

	if (pckbc_is_console(iot, IO_KBD)) {
		t = &pckbc_consdata;
		ioh_d = t->t_ioh_d;
		ioh_c = t->t_ioh_c;
		pckbc_console_attached = 1;
		/* t->t_cmdbyte was initialized by cnattach */
	} else {
		n = ofisa_reg_get(aa->oba.oba_phandle, regs, 2);
		if (n != 2 || regs[0].type != OFISA_REG_TYPE_IO || regs[1].type != OFISA_REG_TYPE_IO
		    || bus_space_map(iot, regs[0].addr, regs[0].len, 0, &ioh_d)
		    || bus_space_map(iot, regs[1].addr, regs[1].len, 0, &ioh_c))
			panic("pckbc_attach: couldn't map");

		t = kmem_zalloc(sizeof(*t), KM_SLEEP);
		t->t_iot = iot;
		t->t_ioh_d = ioh_d;
		t->t_ioh_c = ioh_c;
		t->t_addr = regs[0].addr;
		t->t_cmdbyte = KC8_CPU; /* Enable ports */
		callout_init(&t->t_cleanup, 0);
	}

	t->t_sc = sc;
	sc->id = t;

	aprint_normal("\n");

	/* Finish off the attach. */
	pckbc_attach(sc);
}

static void
pckbc_ofisa_intr_establish(struct pckbc_softc *sc, pckbc_slot_t slot)
{
	struct pckbc_ofisa_softc *osc = (void *) sc;
	void *rv;

	rv = isa_intr_establish(osc->sc_ic, osc->sc_intr[slot].irq, osc->sc_intr[slot].share,
	    IPL_TTY, pckbcintr, sc);
	if (rv == NULL) {
		aprint_error_dev(sc->sc_dv,
		    "unable to establish interrupt for %s slot\n",
		    pckbc_slot_names[slot]);
	} else {
		aprint_normal_dev(sc->sc_dv, "using irq %d for %s slot\n",
		    osc->sc_intr[slot].irq, pckbc_slot_names[slot]);
	}
}
