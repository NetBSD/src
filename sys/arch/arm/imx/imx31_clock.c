/*	$NetBSD: imx31_clock.c,v 1.2 2010/11/15 18:19:19 bsh Exp $ */
/*
 * Copyright (c) 2009,2010  Genetec corp.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec corp.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORP. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORP.
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/evcnt.h>
#include <sys/atomic.h>
#include <sys/time.h>
#include <sys/timetc.h>

#include <sys/types.h>
#include <sys/device.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <arm/cpu.h>
#include <arm/armreg.h>
#include <arm/cpufunc.h>

#include <arm/imx/imx31reg.h>
#include <arm/imx/imx31var.h>
#if 0 /* notyet */
#include <arm/imx/imx31_ccmvar.h>
#endif
#include <arm/imx/imxclockvar.h>
#include <arm/imx/imxepitreg.h>

#include "imxccm.h"	/* if CCM driver is configured into the kernel */
#include "opt_imx31clk.h"

static int imxclock_match(device_t, struct cfdata *, void *);
static void imxclock_attach(device_t, device_t, void *);

struct imxclock_softc *epit1_sc = NULL;
struct imxclock_softc *epit2_sc = NULL;

CFATTACH_DECL_NEW(imxclock, sizeof(struct imxclock_softc),
    imxclock_match, imxclock_attach, NULL, NULL);

static int
imxclock_match(device_t parent, struct cfdata *match, void *aux)
{
	struct aips_attach_args *aipsa = aux;

	if ( (aipsa->aipsa_addr != EPIT1_BASE) &&
	     (aipsa->aipsa_addr != EPIT2_BASE) ) {
		return 0;
	}

	return 2;
}

static void
imxclock_attach(device_t parent, device_t self, void *aux)
{
	struct imxclock_softc *sc = device_private(self);
	struct aips_attach_args *aipsa = aux;

	aprint_normal("\n");

	sc->sc_dev = self;
	sc->sc_iot = aipsa->aipsa_memt;
	sc->sc_intr = aipsa->aipsa_intr;

	KASSERT((sc->sc_intr == IRQ_EPIT1) || (sc->sc_intr == IRQ_EPIT2));

	switch ( aipsa->aipsa_addr ) {
	case EPIT1_BASE:
		epit1_sc = sc;
		break;
	case EPIT2_BASE:
		epit2_sc = sc;
		break;
	default:
		panic("%s: invalid address %p", self->dv_xname, (void *)aipsa->aipsa_addr);
		break;
	}

	if (bus_space_map(aipsa->aipsa_memt, aipsa->aipsa_addr,
		aipsa->aipsa_size, 0, &sc->sc_ioh)) {
		panic("%s: Cannot map registers", device_xname(self));
	}
}

int
imxclock_get_timerfreq(struct imxclock_softc *sc)
{
#if NIMXCCM > 0
	struct imx31_clocks clk;
	imx31_get_clocks(&clk);

	return clk.ipg_clk;
#else
#ifndef	IMX31_IPGCLK_FREQ
#error	IMX31_IPGCLK_FREQ need to be defined.
#endif
	return IMX31_IPGCLK_FREQ;

#endif
}


