/*	$NetBSD: imx51_clock.c,v 1.3.2.2 2014/08/20 00:02:46 tls Exp $ */
/*
 * Copyright (c) 2009  Genetec corp.  All rights reserved.
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
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imx51_clock.c,v 1.3.2.2 2014/08/20 00:02:46 tls Exp $");

#include "opt_imx.h"
#include "opt_imx51clk.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/evcnt.h>
#include <sys/atomic.h>
#include <sys/time.h>
#include <sys/timetc.h>

#include <sys/types.h>
#include <sys/device.h>

#include <dev/clock_subr.h>

#include <machine/intr.h>
#include <sys/bus.h>

#include <arm/cpufunc.h>

#include <arm/imx/imx51reg.h>
#include <arm/imx/imx51var.h>
#include <arm/imx/imxepitreg.h>
#include <arm/imx/imx51_ccmvar.h>
#include <arm/imx/imxclockvar.h>

#include "imxccm.h"	/* if CCM driver is configured into the kernel */



static int imxclock_match(device_t, struct cfdata *, void *);
static void imxclock_attach(device_t, device_t, void *);

struct imxclock_softc *epit1_sc = NULL;
struct imxclock_softc *epit2_sc = NULL;

CFATTACH_DECL_NEW(imxclock, sizeof(struct imxclock_softc),
    imxclock_match, imxclock_attach, NULL, NULL);

static int
imxclock_match(device_t parent, struct cfdata *match, void *aux)
{
	struct axi_attach_args *aa = aux;

	if ( (aa->aa_addr != EPIT1_BASE) &&
	     (aa->aa_addr != EPIT2_BASE) ) {
		return 0;
	}

	return 2;
}

static void
imxclock_attach(device_t parent, device_t self, void *aux)
{
	struct imxclock_softc *sc = device_private(self);
	struct axi_attach_args *aa = aux;

	aprint_normal("\n");

	sc->sc_dev = self;
	sc->sc_iot = aa->aa_iot;
	sc->sc_intr = aa->aa_irq;

	switch ( aa->aa_addr ) {
	case EPIT1_BASE:
		epit1_sc = sc;
		break;
	case EPIT2_BASE:
		epit2_sc = sc;
		break;
	default:
		panic("%s: invalid address %p", device_xname(self), (void *)aa->aa_addr);
		break;
	}

	if (bus_space_map(aa->aa_iot, aa->aa_addr, aa->aa_size, 0, &sc->sc_ioh))
		panic("%s: Cannot map registers", device_xname(self));

	sc->sc_clksrc = EPITCR_CLKSRC_IPG;
}

int
imxclock_get_timerfreq(struct imxclock_softc *sc)
{
	unsigned int ipg_freq;
#if NIMXCCM > 0
	ipg_freq = imx51_get_clock(IMX51CLK_IPG_CLK_ROOT);
#else
#ifndef	IMX51_IPGCLK_FREQ
#error	IMX51_IPGCLK_FREQ need to be defined.
#endif
	ipg_freq = IMX51_IPGCLK_FREQ;
#endif

	return ipg_freq;
}

