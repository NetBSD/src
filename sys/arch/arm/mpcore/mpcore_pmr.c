/*	$NetBSD: mpcore_pmr.c,v 1.1.6.2 2011/06/06 09:05:05 jruoho Exp $ */

/*
 * Copyright (c) 2010, 2011 Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * MPCore Private Memory Region
 *  which contains:
 *     System Controller Unit
 *     Distributed Interrupt Controller
 *     Private Timers and watchdogs
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mpcore_pmr.c,v 1.1.6.2 2011/06/06 09:05:05 jruoho Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/evcnt.h>
#include <sys/device.h>
#include <sys/atomic.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <uvm/uvm_extern.h>

#include <arm/cpu.h>
#include <arm/armreg.h>
#include <arm/cpufunc.h>
#include <arm/pic/picvar.h>

#include <machine/autoconf.h>
#include <machine/bus.h>

#include <arm/mpcore/mpcorereg.h>
#include <arm/mpcore/mpcorevar.h>

struct pmr_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
};

static int pmr_match(device_t, cfdata_t, void *);
static void pmr_attach(device_t, device_t, void *);
static int pmr_search(device_t, cfdata_t, const int *, void *);

CFATTACH_DECL_NEW(mpcorepmr, sizeof(struct pmr_softc),
    pmr_match, pmr_attach, NULL, NULL);

static int
pmr_match(device_t parent, cfdata_t cf, void *aux)
{
	if (strcmp(cf->cf_name, "mpcorepmr") == 0)
		return 1;

	return 0;
}

static void
pmr_attach(device_t parent, device_t self, void *aux)
{
	struct axi_attach_args * const aa = aux;
	struct pmr_softc *sc = device_private(self);

	aprint_normal(": Private Memory Region\n");
	aprint_naive("\n");


	sc->sc_iot = aa->aa_iot;
	if (bus_space_map(sc->sc_iot, aa->aa_addr, MPCORE_PMR_SIZE, 0,
			  &sc->sc_ioh)) {
		aprint_error_dev(self, "can't map");
		return;
	}

	config_search_ia(pmr_search, self, "mpcorepmr", NULL);
}

static int
pmr_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct pmr_softc *sc = device_private(parent);
	struct pmr_attach_args pa;

	pa.pa_iot = sc->sc_iot;
	pa.pa_ioh = sc->sc_ioh;
	pa.pa_irq = cf->cf_loc[MPCOREPMRCF_IRQ];

	if (config_match(parent, cf, &pa) > 0)
		config_attach(parent, cf, &pa, NULL);

	return 0;
}
