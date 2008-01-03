/*	$NetBSD: geodecntr.c,v 1.5 2008/01/03 04:52:54 dyoung Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank Kardel inspired from the patches to FreeBSD by Poul Henning Kamp
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: geodecntr.c,v 1.5 2008/01/03 04:52:54 dyoung Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <machine/cpu.h>
#include <sys/timetc.h>
#include <machine/bus.h>
#include <arch/i386/pci/geodevar.h>
#include <arch/i386/pci/geodereg.h>

struct  geodecntr_softc {
	struct device           sc_dev;
	struct geode_gcb_softc *sc_gcb_dev;
	struct timecounter      sc_tc;
};

static unsigned geode_get_timecount(struct timecounter *);

static int attached = 0;

static int
geodecntr_match(device_t parent, struct cfdata *match, void *aux)
{
	return !attached;
}

/*
 * attach time counter
 */
static void
geodecntr_attach(device_t parent, device_t self, void *aux)
{
	struct geodecntr_softc *sc = device_private(self);

	aprint_normal(": AMD Geode SC1100 27Mhz Counter\n");

	sc->sc_gcb_dev = device_private(parent);

	/*
	 * select 27MHz, no powerdown, no interrupt
	 */
	bus_space_write_1(sc->sc_gcb_dev->sc_iot, sc->sc_gcb_dev->sc_ioh,
			  SC1100_GCB_TMCNFG_B, SC1100_TMCNFG_TMCLKSEL);

	memset(&sc->sc_tc, 0, sizeof(sc->sc_tc));

	sc->sc_tc.tc_get_timecount = geode_get_timecount;
	sc->sc_tc.tc_poll_pps      = NULL;
	sc->sc_tc.tc_counter_mask  = 0xffffffff;
	sc->sc_tc.tc_frequency     = 27000000;
	sc->sc_tc.tc_name          = "geodecounter";
	sc->sc_tc.tc_priv          = sc;
	sc->sc_tc.tc_quality       = 1000;

	tc_init(&sc->sc_tc);

	attached = 1;
}

static int
geodecntr_detach(device_t self, int flags)
{
	struct geodecntr_softc *sc = device_private(self);

	attached = 0;
	return tc_detach(&sc->sc_tc);
}

/*
 * read counter
 */
static unsigned geode_get_timecount(struct timecounter *tc)
{
	struct geodecntr_softc *sc = (struct geodecntr_softc *)tc->tc_priv;

	return bus_space_read_4(sc->sc_gcb_dev->sc_iot, sc->sc_gcb_dev->sc_ioh,
	    SC1100_GCB_TMVALUE_L);
}

CFATTACH_DECL(geodecntr, sizeof(struct geodecntr_softc),
	      geodecntr_match, geodecntr_attach, geodecntr_detach, NULL);

