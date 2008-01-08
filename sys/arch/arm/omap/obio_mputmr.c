/* $NetBSD: obio_mputmr.c,v 1.1.2.6 2008/01/08 07:16:27 matt Exp $ */

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
 * Copyright (c) 2007 Danger Inc.
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
 *	This product includes software developed by Danger Inc.
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
__KERNEL_RCSID(0, "$NetBSD: obio_mputmr.c,v 1.1.2.6 2008/01/08 07:16:27 matt Exp $");

#include "opt_omap.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/device.h>

#include <dev/clock_subr.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <arm/omap/omap2430obiovar.h>

#include <arm/omap/omap2_mputmrvar.h>
#include <arm/omap/omap2_mputmrreg.h>


#include <arm/omap/omap2430reg.h>

#ifndef OMAP_MPU_TIMER_CLOCK_FREQ
#error Specify the timer frequency in Hz with the OMAP_MPU_TIMER_CLOCK_FREQ option.
#endif

typedef struct {
	uint       gptn;
	bus_addr_t addr;
	uint       intr;
	uint32_t   clksel2;
	uint32_t   fclken1;
	uint32_t   iclken1;
} gptimer_instance_t;

/* XXX
 * this table can be used to initialize the GP Timers
 * until we use config(8) locators for CLKSEL2 values, you may want to edit here.
 */
#define GPT_ENTRY(n) { \
		.gptn = (n), \
		.addr = GPT ## n ## _BASE, \
		.intr = IRQ_ ## GPT ## n, \
		.clksel2 = OMAP2_CM_CLKSEL2_CORE_GPTn(n, \
		    CLKSEL2_CORE_GPT_SYS_CLK), \
		.fclken1 = OMAP2_CM_FCLKEN1_CORE_EN_GPT ## n, \
		.iclken1 = OMAP2_CM_ICLKEN1_CORE_EN_GPT ## n, \
	}
static const gptimer_instance_t gptimer_instance_tab[] = {
	GPT_ENTRY( 2), GPT_ENTRY( 3), GPT_ENTRY( 4), GPT_ENTRY( 5),
	GPT_ENTRY( 6), GPT_ENTRY( 7), GPT_ENTRY( 8), GPT_ENTRY( 9),
	GPT_ENTRY(10), GPT_ENTRY(11), GPT_ENTRY(12),
};
#undef	GPT_ENTRY
#define GPTIMER_INSTANCE_CNT	__arraycount(gptimer_instance_tab)

static const gptimer_instance_t *
		gpt_lookup(struct obio_attach_args *);
static void	gpt_enable(struct mputmr_softc *,
			struct obio_attach_args *, const gptimer_instance_t *);

static int	obiomputmr_match(device_t, struct cfdata *, void *);
static void	obiomputmr_attach(device_t, device_t, void *);



CFATTACH_DECL(obiomputmr, sizeof(struct mputmr_softc),
    obiomputmr_match, obiomputmr_attach, NULL, NULL);

static int
obiomputmr_match(device_t parent, cfdata_t match, void *aux)
{
	struct obio_attach_args *obio = aux;

	if (obio->obio_addr == -1 || obio->obio_intr == -1)
	    panic("omapmputmr must have addr and intr specified in config.");

	if (obio->obio_size == 0)
		obio->obio_size = 256;	/* Per the OMAP TRM. */

	if (gpt_lookup(obio) == NULL)
		return 0;

	/* We implicitly trust the config file. */
	return 1;
}

void
obiomputmr_attach(device_t parent, device_t self, void *aux)
{
	struct mputmr_softc *sc = device_private(self);
	struct obio_attach_args *obio = aux;
	int ints_per_sec;

	sc->sc_iot = obio->obio_iot;
	sc->sc_intr = obio->obio_intr;

	if (bus_space_map(obio->obio_iot, obio->obio_addr, obio->obio_size, 0,
			 &sc->sc_ioh))
		panic("%s: Cannot map registers", self->dv_xname);

	switch (self->dv_unit) { /* XXX broken */
	case 0:
		clock_sc = sc;
		ints_per_sec = hz;
		break;
	case 1:
		stat_sc = sc;
		ints_per_sec = profhz = stathz = STATHZ;
		break;
	case 2:
		ref_sc = sc;
		ints_per_sec = hz;	/* Same rate as clock */
		break;
	default:
		ints_per_sec = hz;	/* Better value? */
		break;
	}

	aprint_normal(": OMAP MPU Timer");
	gpt_enable(sc, obio, gpt_lookup(obio));
	aprint_normal("\n");
	aprint_naive("\n");

	/* Stop the timer from counting, but keep the timer module working. */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MPU_CNTL_TIMER,
			  MPU_CLOCK_ENABLE);

	timer_factors tf;
	calc_timer_factors(ints_per_sec, &tf);

	switch (self->dv_unit) {	/* XXX broken */
	case 0:
		counts_per_hz = tf.reload + 1;
		counts_per_usec = tf.counts_per_usec;
		break;
	case 2:

		/*
		 * The microtime reference clock for all practical purposes
		 * just wraps around as an unsigned int.
		 */

		tf.reload = 0xffffffff;
		break;

	default:
		break;
	}

	/* Set the reload value. */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MPU_LOAD_TIMER, tf.reload);
	/* Set the PTV and the other required bits and pieces. */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MPU_CNTL_TIMER,
			  ( MPU_CLOCK_ENABLE
			    | (tf.ptv << MPU_PTV_SHIFT)
			    | MPU_AR
			    | MPU_ST));
	/* The clock is now running, but is not generating interrupts. */
}

static const gptimer_instance_t *
gpt_lookup(struct obio_attach_args *obio)
{
	const gptimer_instance_t *ip;
	uint i;

	for (i = 0, ip = gptimer_instance_tab;
	     i < GPTIMER_INSTANCE_CNT; i++, ip++) {
		if (ip->addr == obio->obio_addr && ip->intr == obio->obio_intr)
			return ip;
	}

	return NULL;
}

void
gpt_enable(
	struct mputmr_softc *sc,
	struct obio_attach_args *obio,
	const gptimer_instance_t *ip)
{
	bus_space_handle_t ioh;
	uint32_t r;
	int err;

	KASSERT(ip != NULL);

	aprint_normal(" #%d", ip->gptn);

	err = bus_space_map(obio->obio_iot, OMAP2_CM_BASE,
	    OMAP2_CM_SIZE, 0, &ioh);
	KASSERT(err == 0);

	r = bus_space_read_4(obio->obio_iot, ioh, OMAP2_CM_CLKSEL2_CORE);
	r |= ip->clksel2;
	bus_space_write_4(obio->obio_iot, ioh, OMAP2_CM_CLKSEL2_CORE, r);

	r = bus_space_read_4(obio->obio_iot, ioh, OMAP2_CM_FCLKEN1_CORE);
	r |= ip->fclken1;
	bus_space_write_4(obio->obio_iot, ioh, OMAP2_CM_FCLKEN1_CORE, r);

	r = bus_space_read_4(obio->obio_iot, ioh, OMAP2_CM_ICLKEN1_CORE);
	r |= ip->iclken1;
	bus_space_write_4(obio->obio_iot, ioh, OMAP2_CM_ICLKEN1_CORE, r);

	bus_space_unmap(obio->obio_iot, ioh, OMAP2_CM_SIZE);
}
