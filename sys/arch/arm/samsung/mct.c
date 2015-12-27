/*	$NetBSD: mct.c,v 1.5.2.1 2015/12/27 12:09:32 skrll Exp $	*/

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

__KERNEL_RCSID(1, "$NetBSD: mct.c,v 1.5.2.1 2015/12/27 12:09:32 skrll Exp $");

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

#include <dev/fdt/fdtvar.h>

static int  mct_match(device_t, cfdata_t, void *);
static void mct_attach(device_t, device_t, void *);

static int clockhandler(void *);

CFATTACH_DECL_NEW(exyo_mct, 0, mct_match, mct_attach, NULL, NULL);


#if 0
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
#endif


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
	const char * const compatible[] = { "samsung,exynos4210-mct",
					    NULL };

	struct fdt_attach_args * const faa = aux;
	return of_match_compatible(faa->faa_phandle, compatible);
}


static void
mct_attach(device_t parent, device_t self, void *aux)
{
	struct mct_softc * const sc = &mct_sc;
//	prop_dictionary_t dict = device_properties(self);
//	char freqbuf[sizeof("XXX SHz")];
	struct fdt_attach_args * const faa = aux;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	self->dv_private = sc;
	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	/* MJF: Need to get irq from the dtd */
//	sc->sc_irq = exyo->exyo_loc.loc_intr;

	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d",
			     (uint64_t)addr, error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Exynos SoC multi core timer (64 bits)\n");

	evcnt_attach_dynamic(&sc->sc_ev_missing_ticks, EVCNT_TYPE_MISC, NULL,
		device_xname(self), "missing interrupts");

	sc->sc_global_ih = intr_establish(sc->sc_irq, IPL_CLOCK, IST_EDGE,
		clockhandler, NULL);
	if (sc->sc_global_ih == NULL)
		panic("%s: unable to register timer interrupt", __func__);
	aprint_normal_dev(sc->sc_dev, "interrupting on irq %d\n", sc->sc_irq);
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


#if 0
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

#endif
