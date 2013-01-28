/*	$NetBSD: empm.c,v 1.1 2013/01/28 14:44:37 rkujawa Exp $ */

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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
 * Power management on Elbox Mediator 1200 SX and TX.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <amiga/pci/empbreg.h>
#include <amiga/pci/empmvar.h>

static int	empm_match(device_t, cfdata_t, void *);
static void	empm_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(empm, sizeof(struct empm_softc),
    empm_match, empm_attach, NULL, NULL);

static int
empm_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
empm_attach(device_t parent, device_t self, void *aux)
{
	struct empm_softc *sc;
	struct empm_attach_args *aa;

	sc = device_private(self); 
	aa = aux;

	sc->sc_dev = self;

	aprint_normal(": ELBOX Mediator 1200 SX/TX Power Manager\n");

	sc->setup_area_t = aa->setup_area_t;
	
	if (bus_space_map(sc->setup_area_t, EMPB_PM_OFF, 1, 0,
	    &sc->powermgr_h))
		aprint_error_dev(self, "couldn't map power manager register\n");

}

void
empm_power_off(struct empm_softc *sc)
{
	aprint_normal_dev(sc->sc_dev, "trying soft power-off...\n");

	bus_space_write_1(sc->setup_area_t, sc->powermgr_h, 0, 0);
}

