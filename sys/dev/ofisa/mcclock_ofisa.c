/*	$NetBSD: mcclock_ofisa.c,v 1.1 2021/04/27 21:39:39 thorpej Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
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
 * OFW ISA attachment for the mc146818 real time clock.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mcclock_ofisa.c,v 1.1 2021/04/27 21:39:39 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <sys/bus.h>

#include <dev/clock_subr.h>
#include <dev/ic/mc146818var.h>

#include <dev/ofw/openfirm.h>
#include <dev/isa/isavar.h>
#include <dev/ofisa/ofisavar.h>

struct mcclock_ofisa_platform_data {
	int			year0;
	int			flags;
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "pnpPNP,b00" },
	DEVICE_COMPAT_EOL
};

static const struct mcclock_ofisa_platform_data dec_dnard_data = {
	.year0 = 1900,
	.flags = MC146818_BCD,
};

static const struct device_compatible_entry platform_data[] = {
	{ .compat = "DEC,DNARD",	.data = &dec_dnard_data, },
	DEVICE_COMPAT_EOL
};

static const struct mcclock_ofisa_platform_data *
lookup_platform_data(char *model, size_t const size)
{
	const struct device_compatible_entry *dce;

	if (OF_getprop(OF_finddevice("/"), "model", model, size) <= 0) {
		return NULL;
	}

	/* should be plenty big enough, but never hurts... */
	model[size - 1] = '\0';
	const char *cmodel = model;

	dce = device_compatible_lookup(&cmodel, 1, platform_data);
	if (dce != NULL) {
		return dce->data;
	}
	return NULL;
}

static u_int
mcclock_ofisa_read(struct mc146818_softc *sc, u_int reg)
{
	bus_space_write_1(sc->sc_bst, sc->sc_bsh, 0, reg);
	return bus_space_read_1(sc->sc_bst, sc->sc_bsh, 1);
}

static void
mcclock_ofisa_write(struct mc146818_softc *sc, u_int reg, u_int datum)
{
	bus_space_write_1(sc->sc_bst, sc->sc_bsh, 0, reg);
	bus_space_write_1(sc->sc_bst, sc->sc_bsh, 1, (uint8_t)datum);
}

static int
mcclock_ofisa_match(device_t parent, cfdata_t cf, void *aux)
{
	struct ofisa_attach_args *aa = aux;
	int rv;

	rv = of_compatible_match(aa->oba.oba_phandle, compat_data) ? 5 : 0;
#ifdef _MCCLOCK_OFISA_MD_MATCH
	if (!rv)
		rv = mcclock_ofisa_md_match(parent, cf, aux);
#endif
	return rv;
}

static void
mcclock_ofisa_attach(device_t parent, device_t self, void *aux)
{
	struct mc146818_softc *sc = device_private(self);
	struct ofisa_attach_args *aa = aux;
	const struct mcclock_ofisa_platform_data *pd;
	struct ofisa_reg_desc reg;
	char model[64];
	int n;

	sc->sc_dev = self;

	n = ofisa_reg_get(aa->oba.oba_phandle, &reg, 1);
#ifdef _MCCLOCK_OFISA_MD_REG_FIXUP
	n = mcclock_ofisa_md_reg_fixup(parent, self, aux, &reg, 1, n);
#endif
	if (n != 1) {
		aprint_error(": error getting register data\n");
		return;
	}
	if (reg.len != 2) {
		aprint_error(": weird register size (%lu, expected 2)\n",
		    (unsigned long)reg.len);
		return;
	}

	/*
	 * We don't configure interrupts here; we're only using this
	 * for the TODR.
	 */

	pd = lookup_platform_data(model, sizeof(model));

	sc->sc_bst = (reg.type == OFISA_REG_TYPE_IO) ? aa->iot : aa->memt;
	if (bus_space_map(sc->sc_bst, reg.addr, reg.len, 0, &sc->sc_bsh) != 0) {
		aprint_error(": can't map register space\n");
		return;
	}

	/*
	 * These default to 0.  We'll warn after attaching if we didn't
	 * find any platform data.
	 */
	if (pd != NULL) {
		sc->sc_year0 = pd->year0;
		sc->sc_flag = pd->flags;
	}
	sc->sc_mcread = mcclock_ofisa_read;
	sc->sc_mcwrite = mcclock_ofisa_write;

	mc146818_attach(sc);

	aprint_normal("\n");

	if (pd == NULL) {
		/*
		 * We could dynamically compute at least BCD-ness and
		 * base year by comparing against the OFW get-time
		 * call, but it hardly seems worth it right now.
		 */
		aprint_error_dev(self,
		    "WARNING: no platform data for '%s', using defaults.\n",
		    model);
	}
}

CFATTACH_DECL_NEW(mcclock_ofisa, sizeof(struct mc146818_softc),
    mcclock_ofisa_match, mcclock_ofisa_attach, NULL, NULL);
