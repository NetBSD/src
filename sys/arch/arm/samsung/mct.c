/*	$NetBSD: mct.c,v 1.5 2014/08/28 20:29:05 snj Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Reinoud Zandijk.
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

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: mct.c,v 1.5 2014/08/28 20:29:05 snj Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/timetc.h>

#include <prop/proplib.h>

#include <arm/samsung/exynos_reg.h>
#include <arm/samsung/exynos_var.h>
#include <arm/samsung/mct_reg.h>
#include <arm/samsung/mct_var.h>


static int  mct_match(device_t, cfdata_t, void *);
static void mct_attach(device_t, device_t, void *);

static int clockhandler(void *);
static u_int mct_get_timecount(struct timecounter *);


CFATTACH_DECL_NEW(exyo_mct, 0, mct_match, mct_attach, NULL, NULL);


static struct timecounter mct_timecounter = {
	.tc_get_timecount = mct_get_timecount,
	.tc_poll_pps = 0,
	.tc_counter_mask = ~0u,
	.tc_frequency = 0,		/* set by cpu_initclocks() */
	.tc_name = NULL,		/* set by cpu_initclocks() */
	.tc_quality = 500,		/* why 500? */
	.tc_priv = &mct_sc,
	.tc_next = NULL,
};


static inline uint32_t
mct_read_global(struct mct_softc *sc, bus_size_t o)
{
	return bus_space_read_4(sc->sc_bst, sc->sc_bsh, o);
}


static inline void
mct_write_global(struct mct_softc *sc, bus_size_t o, uint32_t v)
{
	bus_size_t wreg;
	uint32_t bit;
	int i;

	/* do the write */
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, o, v);
//	printf("%s: write %#x at %#x\n",
//		__func__, ((uint32_t) sc->sc_bsh + (uint32_t) o), v);

	/* dependent on the write address, do the ack dance */
	if (o == MCT_G_CNT_L || o == MCT_G_CNT_U) {
		wreg = MCT_G_CNT_WSTAT;
		bit  = (o == MCT_G_CNT_L) ? G_CNT_WSTAT_L : G_CNT_WSTAT_U;
	} else {
		wreg = MCT_G_WSTAT;
		switch (o) {
		case MCT_G_COMP0_L:
			bit  = G_WSTAT_COMP0_L;
			break;
		case MCT_G_COMP0_U:
			bit  = G_WSTAT_COMP0_U;
			break;
		case MCT_G_COMP0_ADD_INCR:
			bit  = G_WSTAT_ADD_INCR;
			break;
		case MCT_G_TCON:
			bit  = G_WSTAT_TCON;
			break;
		default:
			/* all other registers */
			return;
		}
	}

	/* wait for ack */
	for (i = 0; i < 10000000; i++) {
		/* value accepted by the hardware/hal ? */
		if (mct_read_global(sc, wreg) & bit) {
			/* ack */
			bus_space_write_4(sc->sc_bst, sc->sc_bsh, wreg, bit);
			return;
		}
	}
	panic("MCT hangs after writing %#x at %#x", v, (uint32_t) o);
}


static int
mct_match(device_t parent, cfdata_t cf, void *aux)
{
	/* not used if Generic Timer is Available */
	if (armreg_pfr1_read() & ARM_PFR1_GTIMER_MASK)
		return 0;

	/* sanity check, something is mixed up! */
	if (!device_is_a(parent, "exyo"))
		return 1;

	/* there can only be one */
	if (mct_sc.sc_dev != NULL)
		return 0;

	return 1;
}


static void
mct_attach(device_t parent, device_t self, void *aux)
{
	struct exyo_attach_args *exyo = (struct exyo_attach_args *) aux;
	struct mct_softc * const sc = &mct_sc;
	prop_dictionary_t dict = device_properties(self);
	char freqbuf[sizeof("XXX SHz")];
	const char *pin_name;

	self->dv_private = sc;
	sc->sc_dev = self;
	sc->sc_bst = exyo->exyo_core_bst;
	sc->sc_irq = exyo->exyo_loc.loc_intr;

	bus_space_subregion(sc->sc_bst, exyo->exyo_core_bsh,
		exyo->exyo_loc.loc_offset, exyo->exyo_loc.loc_size, &sc->sc_bsh);

	KASSERTMSG(sc->sc_bsh,
		"%s: can't map in registers for %#x + %#x for device %s\n",
		__func__,
		(uint32_t) exyo->exyo_loc.loc_offset,
		(uint32_t) exyo->exyo_loc.loc_size,
		device_xname(sc->sc_dev));

	prop_dictionary_get_uint32(dict, "frequency", &sc->sc_freq);

	humanize_number(freqbuf, sizeof(freqbuf), sc->sc_freq, "Hz", 1000);

	aprint_naive("\n");
	aprint_normal(": Exynos SoC multi core timer (64 bits) (%s)\n", freqbuf);

	evcnt_attach_dynamic(&sc->sc_ev_missing_ticks, EVCNT_TYPE_MISC, NULL,
		device_xname(self), "missing interrupts");

	sc->sc_global_ih = intr_establish(sc->sc_irq, IPL_CLOCK, IST_EDGE,
		clockhandler, NULL);
	if (sc->sc_global_ih == NULL)
		panic("%s: unable to register timer interrupt", __func__);
	aprint_normal_dev(sc->sc_dev, "interrupting on irq %d\n", sc->sc_irq);

	/* blink led */
	if (prop_dictionary_get_cstring_nocopy(dict, "heartbeat", &pin_name)) {
		if (!exynos_gpio_pin_reserve(pin_name, &sc->sc_gpio_led)) {
			aprint_error_dev(self,
				"failed to reserve GPIO \"%s\" "
				"for heartbeat led\n", pin_name);
		} else {
			sc->sc_has_blink_led = true;
			sc->sc_led_state = false;
			sc->sc_led_timer = hz;
		}
	}
}


static inline uint64_t
mct_gettime(struct mct_softc *sc)
{
	uint32_t lo, hi;
	do {
		hi = mct_read_global(sc, MCT_G_CNT_U);
		lo = mct_read_global(sc, MCT_G_CNT_L);
	} while (hi != mct_read_global(sc, MCT_G_CNT_U));
	return ((uint64_t) hi << 32) | lo;
}


static u_int
mct_get_timecount(struct timecounter *tc)
{
	struct mct_softc * const sc = tc->tc_priv;
	return (u_int) (mct_gettime(sc));
}


/* interrupt handler */
static int
clockhandler(void *arg)
{
	struct clockframe * const cf = arg;
	struct mct_softc * const sc = &mct_sc;
	const uint64_t now = mct_gettime(sc);
	int64_t delta = now - sc->sc_lastintr;
	int64_t periods = delta / sc->sc_autoinc;

	KASSERT(delta >= 0);
	KASSERT(periods >= 0);

	/* ack the interrupt */
	mct_write_global(sc, MCT_G_INT_CSTAT, G_INT_CSTAT_CLEAR);

	/* check if we missed clock interrupts */
	if (periods > 1)
		sc->sc_ev_missing_ticks.ev_count += periods - 1;

	sc->sc_lastintr = now;
	hardclock(cf);

	if (sc->sc_has_blink_led) {
		/* we could subtract `periods' here */
		sc->sc_led_timer = sc->sc_led_timer - 1;
		if (sc->sc_led_timer <= 0) {
			sc->sc_led_state = !sc->sc_led_state;
			exynos_gpio_pindata_write(&sc->sc_gpio_led,
				sc->sc_led_state);
			while (sc->sc_led_timer <= 0)
				sc->sc_led_timer += hz;
		}
	}

	/* handled */
	return 1;
}


void
mct_init_cpu_clock(struct cpu_info *ci)
{
	struct mct_softc * const sc = &mct_sc;
	uint64_t now = mct_gettime(sc);
	uint64_t then;
	uint32_t tcon;

	KASSERT(ci == curcpu());

	sc->sc_lastintr = now;

	/* get current config */
	tcon = mct_read_global(sc, MCT_G_TCON);

	/* setup auto increment */
	mct_write_global(sc, MCT_G_COMP0_ADD_INCR, sc->sc_autoinc);

	/* (re)setup comparator */
	then = now + sc->sc_autoinc;
	mct_write_global(sc, MCT_G_COMP0_L, (uint32_t) then);
	mct_write_global(sc, MCT_G_COMP0_U, (uint32_t) (then >> 32));
	tcon |= G_TCON_COMP0_AUTOINC;
	tcon |= G_TCON_COMP0_ENABLE;

	/* start timer */
	tcon |= G_TCON_START;

	/* enable interrupt */
	mct_write_global(sc, MCT_G_INT_ENB, G_INT_ENB_ENABLE);

	/* update config, starting the thing */
	mct_write_global(sc, MCT_G_TCON, tcon);
}


void
cpu_initclocks(void)
{
	struct mct_softc * const sc = &mct_sc;

	sc->sc_autoinc = sc->sc_freq / hz;
	mct_init_cpu_clock(curcpu());

	mct_timecounter.tc_name = device_xname(sc->sc_dev);
	mct_timecounter.tc_frequency = sc->sc_freq;

	tc_init(&mct_timecounter);

#if 0
	{
		uint64_t then, now;

		printf("testing timer\n");
		for (int i = 0; i < 200; i++) {
			printf("cstat %d\n", mct_read_global(sc, MCT_G_INT_CSTAT));
			then = mct_get_timecount(&mct_timecounter);
			do {
				now = mct_get_timecount(&mct_timecounter);
			} while (now == then);
			printf("\tgot %"PRIu64"\n", now);
			for (int j = 0; j < 90000; j++);
		}
		printf("passed\n");
	}
#endif
}


void
setstatclockrate(int newhz)
{
}

