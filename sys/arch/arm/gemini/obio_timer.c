/*	$NetBSD: obio_timer.c,v 1.3 2008/11/11 19:54:38 cliff Exp $	*/

/* adapted from:
 *	NetBSD: obio_mputmr.c,v 1.3 2008/08/27 11:03:10 matt Exp
 */

/*
 * Based on omap_mputmr.c
 * Based on i80321_timer.c and arch/arm/sa11x0/sa11x0_ost.c
 *
 * Copyright (c) 1997 Mark Brinicombe.
 * Copyright (c) 1997 Causality Limited.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by IWAMOTO Toshihiro and Ichiro FUKUHARA.
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
 *
 * Copyright (c) 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright (c) 2007 Microsoft
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Microsoft
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTERS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: obio_timer.c,v 1.3 2008/11/11 19:54:38 cliff Exp $");

#include "opt_cpuoptions.h"
#include "opt_gemini.h"
#include "locators.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/device.h>

#include <dev/clock_subr.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <arm/gemini/gemini_reg.h>
#include <arm/gemini/gemini_obiovar.h>
#include <arm/gemini/gemini_timervar.h>

#if STATHZ != HZ
# error system clock HZ and stat clock STATHZ must be same
#endif


#ifndef GEMINI_TIMER_CLOCK_FREQ
# error Specify the timer frequency in Hz with option GEMINI_TIMER_CLOCK_FREQ 
#endif

static int	obiotimer_match(device_t, struct cfdata *, void *);
static void	obiotimer_attach(device_t, device_t, void *);

struct geminitmr_softc xsc;



typedef struct {
	uint       timerno;
	bus_addr_t addr;
	uint       intr;
} obiotimer_instance_t;

/* XXX
 * this table can be used to match the GP Timers
 * until we use config(8) locators to distinguish between
 * gemini "sub-timers".
 */
#define GPT_ENTRY(n, i) { \
		.timerno = (n), \
		.addr = GEMINI_TIMER_BASE, \
		.intr = i, \
	}
static const obiotimer_instance_t obiotimer_instance_tab[] = {
	GPT_ENTRY(1, 14),
	GPT_ENTRY(2, 15),
	GPT_ENTRY(3, 16),
};
#undef	GPT_ENTRY
#define GPTIMER_INSTANCE_CNT	__arraycount(obiotimer_instance_tab)

static const obiotimer_instance_t *
		obiotimer_lookup(struct obio_attach_args *);
static void	obiotimer_enable(struct geminitmr_softc *,
			struct obio_attach_args *,
			const obiotimer_instance_t *);

static int	obiotimer_match(device_t, struct cfdata *, void *);
static void	obiotimer_attach(device_t, device_t, void *);


CFATTACH_DECL_NEW(obiotimer, sizeof(struct geminitmr_softc),
    obiotimer_match, obiotimer_attach, NULL, NULL);


static int
obiotimer_match(device_t parent, struct cfdata *match, void *aux)
{
	struct obio_attach_args *obio = aux;

	if ((obio->obio_addr == OBIOCF_ADDR_DEFAULT)
	||  (obio->obio_intr == OBIOCF_INTR_DEFAULT))
		panic("geminitmr must have addr and intr specified in config.");

	if (obiotimer_lookup(obio) == NULL)
		return 0;

	return 1;
}

void
obiotimer_attach(device_t parent, device_t self, void *aux)
{
	struct geminitmr_softc *sc = device_private(self);
	struct obio_attach_args *obio = aux;
	const obiotimer_instance_t *ip;
	static int once=1;

	ip = obiotimer_lookup(obio);
	if (ip == NULL)
		panic("%s: bad lookup", device_xname(self));
			/* should not fail since we already matched */

	sc->sc_timerno = ip->timerno;
	sc->sc_iot = obio->obio_iot;
	sc->sc_intr = obio->obio_intr;
	sc->sc_addr = obio->obio_addr;
	sc->sc_size = (obio->obio_size == OBIOCF_SIZE_DEFAULT)
		? (GEMINI_TIMER_INTRMASK + 4)
		: obio->obio_size;

	if (bus_space_map(sc->sc_iot, sc->sc_addr, sc->sc_size, 0, &sc->sc_ioh))
		panic("%s: Cannot map registers", device_xname(self));

	obiotimer_enable(sc, obio, obiotimer_lookup(obio));
	aprint_normal("\n");
	aprint_naive("\n");

	if (once) {
		once = 0;
		bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			GEMINI_TIMER_TMCR, 0);
		bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			GEMINI_TIMER_INTRMASK, (uint32_t)~TIMER_INTRMASK_Resv);
		bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			GEMINI_TIMER_INTRSTATE, 0);
	}

	switch (sc->sc_timerno) {
	case 1:
#ifndef GEMINI_SLAVE
		/*
		 * timer #1 is the combined system clock and stat clock
		 * for the Master or Single Gemini CPU
		 * it gets started later
		 */
		profhz = stathz = hz;
		stat_sc = clock_sc = sc;
#endif
		break;
	case 2:
#ifdef GEMINI_SLAVE
		/*
		 * timer #2 is the combined system clock and stat clock
		 * for the Slave Gemini CPU
		 * it gets started later
		 */
		profhz = stathz = hz;
		stat_sc = clock_sc = sc;
#endif
		break;
	case 3:
		/*
		 * Timer #3 is used for microtime reference clock and delay()
		 * autoloading, non-interrupting, just wraps around
		 * we start it now to make delay() available
		 */
		ref_sc = sc;
		gemini_microtime_init();
		break;
	default:
		panic("bad gemini timer number %d\n", sc->sc_timerno);
		break;
	}
}

static const obiotimer_instance_t *
obiotimer_lookup(struct obio_attach_args *obio)
{
	const obiotimer_instance_t *ip;
	uint i;

	for (i = 0, ip = obiotimer_instance_tab;
	     i < GPTIMER_INSTANCE_CNT; i++, ip++) {
		if (ip->addr == obio->obio_addr && ip->intr == obio->obio_intr)
			return ip;
	}

	return NULL;
}

void
obiotimer_enable(
	struct geminitmr_softc *sc,
	struct obio_attach_args *obio,
	const obiotimer_instance_t *ip)
{
	/* nothing to do */
}
