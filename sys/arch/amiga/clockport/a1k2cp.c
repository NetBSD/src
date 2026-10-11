/*      $NetBSD: a1k2cp.c,v 1.6 2026/10/11 17:23:13 rkujawa Exp $ */

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
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

/* Driver for A1200 on-board clockport. */

#include <sys/cdefs.h>

#include <sys/systm.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/kmem.h>

#include <machine/cpu.h>

#include <amiga/amiga/device.h>

#include <amiga/dev/zbusvar.h>

#include <amiga/clockport/clockportvar.h>

/* #define A1K2CP_DEBUG		1 */

#define A1K2CP_BASE		0xD80001

static int	a1k2cp_match(device_t, cfdata_t, void *);
static void	a1k2cp_attach(device_t, device_t, void *);

struct a1k2cp_softc {
	device_t	sc_dev;

	struct bus_space_tag sc_bst;
	struct clockportbus_attach_args sc_cpb_aa;
};

CFATTACH_DECL_NEW(a1k2cp, sizeof(struct a1k2cp_softc),
    a1k2cp_match, a1k2cp_attach, NULL, NULL);

static int
a1k2cp_match(device_t parent, cfdata_t cf, void *aux)
{

	static int a1k2cp_matched = 0;

	if (!matchname("a1k2cp", aux))
		return 0;

	if (a1k2cp_matched)
		return 0;

	if (!is_a1200())
		return 0;

	a1k2cp_matched = 1;
	return 1;
}

static void
a1k2cp_attach(device_t parent, device_t self, void *aux)
{
	struct a1k2cp_softc *sc;

	sc = device_private(self);
	sc->sc_dev = self;

	aprint_normal(": A1200 on-board clockport\n");

	sc->sc_bst.base = (bus_addr_t) __UNVOLATILE(ztwomap(A1K2CP_BASE));
	sc->sc_bst.absm = &amiga_bus_stride_4;

	sc->sc_cpb_aa.cp_iot = &sc->sc_bst;
	sc->sc_cpb_aa.cp_intr_establish = clockport_generic_intr_establish;
	sc->sc_cpb_aa.cp_intr_disestablish =
	    clockport_generic_intr_disestablish;

#ifdef A1K2CP_DEBUG
	aprint_normal_dev(sc->sc_dev, "pa %x va %p\n", 
	    A1K2CP_BASE, (void*) sc->sc_bst.base);
#endif /* A1K2CP_DEBUG */

	config_found(sc->sc_dev, &sc->sc_cpb_aa, 0, CFARGS_NONE);
}
