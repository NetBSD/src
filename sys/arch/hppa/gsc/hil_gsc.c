/*	$NetBSD: hil_gsc.c,v 1.1.10.2 2014/08/20 00:03:04 tls Exp $	*/
/*	$OpenBSD: hil_gsc.c,v 1.5 2005/12/22 07:09:52 miod Exp $	*/
/*
 * Copyright (c) 2003, Miodrag Vallat.
 * All rights reserved.
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/cpu.h>
#include <sys/bus.h>

#include <machine/intr.h>
#include <machine/iomod.h>
#include <machine/autoconf.h>

#include <hppa/dev/cpudevs.h>
#include <hppa/gsc/gscbusvar.h>
#include <hppa/hppa/machdep.h>

#include <machine/hil_machdep.h>

#include <dev/hil/hilvar.h>

static int	hil_gsc_match(device_t, cfdata_t, void *);
static void	hil_gsc_attach(device_t, device_t, void *);

struct hil_gsc_softc {
	struct hil_softc sc_hs;
	int sc_hil_console;
	void *sc_ih;
};

CFATTACH_DECL_NEW(hil_gsc, sizeof(struct hil_gsc_softc),
    hil_gsc_match, hil_gsc_attach, NULL, NULL);

int
hil_gsc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct gsc_attach_args *ga = aux;

	if (ga->ga_type.iodc_type != HPPA_TYPE_FIO ||
	    ga->ga_type.iodc_sv_model != HPPA_FIO_HIL)
		return 0;

	return 1;
}

void
hil_gsc_attach(device_t parent, device_t self, void *aux)
{
	struct hil_gsc_softc *gsc = device_private(self);
	struct hil_softc *sc = &gsc->sc_hs;
	struct gsc_attach_args *ga = aux;
	int pagezero_cookie;

	sc->sc_dev = self;
	sc->sc_bst = ga->ga_iot;
	if (bus_space_map(ga->ga_iot, ga->ga_hpa,
	    HILMAPSIZE, 0, &sc->sc_bsh)) {
		aprint_error(": couldn't map hil controller\n");
		return;
	}

	pagezero_cookie = hppa_pagezero_map();
	gsc->sc_hil_console = ga->ga_dp.dp_mod == PAGE0->mem_kbd.pz_dp.dp_mod &&
	    memcmp(ga->ga_dp.dp_bc, PAGE0->mem_kbd.pz_dp.dp_bc, 6) == 0;
	hppa_pagezero_unmap(pagezero_cookie);

	hil_attach(sc, &gsc->sc_hil_console);

	gsc->sc_ih = hppa_intr_establish(IPL_TTY, hil_intr, sc,
	    ga->ga_ir, ga->ga_irq);

	config_interrupts(self, hil_attach_deferred);
}
