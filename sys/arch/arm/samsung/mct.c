/*	$NetBSD: mct.c,v 1.12.4.1 2018/07/28 04:37:29 pgoyette Exp $	*/

/*-
 * Copyright (c) 2014-2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Reinoud Zandijk and Jared McNeill.
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

__KERNEL_RCSID(1, "$NetBSD: mct.c,v 1.12.4.1 2018/07/28 04:37:29 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/timetc.h>
#include <sys/kmem.h>

#include <prop/proplib.h>

#include <arm/samsung/exynos_reg.h>
#include <arm/samsung/exynos_var.h>
#include <arm/samsung/mct_reg.h>
#include <arm/samsung/mct_var.h>

#include <dev/fdt/fdtvar.h>
#include <arm/fdt/arm_fdtvar.h>

static struct mct_softc mct_sc;

static int  mct_match(device_t, cfdata_t, void *);
static void mct_attach(device_t, device_t, void *);

static u_int mct_get_timecount(struct timecounter *);

static struct timecounter mct_timecounter = {
	.tc_get_timecount = mct_get_timecount,
	.tc_counter_mask = ~0u,
	.tc_frequency = EXYNOS_F_IN_FREQ,
	.tc_name = "MCT",
	.tc_quality = 500,
	.tc_priv = &mct_sc,
};

CFATTACH_DECL_NEW(exyo_mct, 0, mct_match, mct_attach, NULL, NULL);

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
		switch (o) {
		case MCT_G_COMP0_L:
			wreg = MCT_G_WSTAT;
			bit  = G_WSTAT_COMP0_L;
			break;
		case MCT_G_COMP0_U:
			wreg = MCT_G_WSTAT;
			bit  = G_WSTAT_COMP0_U;
			break;
		case MCT_G_COMP0_ADD_INCR:
			wreg = MCT_G_WSTAT;
			bit  = G_WSTAT_ADD_INCR;
			break;
		case MCT_G_TCON:
			wreg = MCT_G_WSTAT;
			bit  = G_WSTAT_TCON;
			break;
		case MCT_G_CNT_L:
			wreg = MCT_G_CNT_WSTAT;
			bit  = G_CNT_WSTAT_L;
			break;
		case MCT_G_CNT_U:
			wreg = MCT_G_CNT_WSTAT;
			bit  = G_CNT_WSTAT_U;
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

static void
mct_fdt_cpu_hatch(void *priv, struct cpu_info *ci)
{
	panic("%s: not implemented", __func__);
}

static int
mct_intr(void *arg)
{
	struct mct_softc * const sc = &mct_sc;
	struct clockframe *frame = arg;

	mct_write_global(sc, MCT_G_INT_CSTAT, G_INT_CSTAT_CLEAR);

	hardclock(frame);

	return 1;
}

static u_int
mct_get_timecount(struct timecounter *tc)
{
	struct mct_softc * const sc = tc->tc_priv;

	return mct_read_global(sc, MCT_G_CNT_L);
}

static uint64_t
mct_read_gcnt(struct mct_softc *sc)
{
	uint32_t gcntl, gcntu;

	do {
		gcntu = mct_read_global(sc, MCT_G_CNT_U);
		gcntl = mct_read_global(sc, MCT_G_CNT_L);
	} while (gcntu != mct_read_global(sc, MCT_G_CNT_U));

	return ((uint64_t)gcntu << 32) | gcntl;
}

static void
mct_cpu_initclocks(void)
{
	struct mct_softc * const sc = &mct_sc;
	char intrstr[128];

	if (!fdtbus_intr_str(sc->sc_phandle, 0, intrstr, sizeof(intrstr)))
		panic("%s: failed to decode interrupt", __func__);

	sc->sc_global_ih = fdtbus_intr_establish(sc->sc_phandle, 0, IPL_CLOCK,
	    FDT_INTR_MPSAFE, mct_intr, NULL);
	if (sc->sc_global_ih == NULL)
		panic("%s: failed to establish timer interrupt on %s", __func__, intrstr);

	aprint_normal_dev(sc->sc_dev, "interrupting on %s\n", intrstr);

	/* Start the timer */
	const u_int autoinc = sc->sc_freq / hz;
	const uint64_t comp0 = mct_read_gcnt(sc) + autoinc;

	mct_write_global(sc, MCT_G_TCON, G_TCON_START | G_TCON_COMP0_AUTOINC);
	mct_write_global(sc, MCT_G_COMP0_ADD_INCR, autoinc);
	mct_write_global(sc, MCT_G_COMP0_L, (uint32_t)comp0);
	mct_write_global(sc, MCT_G_COMP0_U, (uint32_t)(comp0 >> 32));
	mct_write_global(sc, MCT_G_INT_ENB, G_INT_ENB_ENABLE);
	mct_write_global(sc, MCT_G_TCON, G_TCON_START | G_TCON_COMP0_ENABLE | G_TCON_COMP0_AUTOINC);
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
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;
	sc->sc_freq = EXYNOS_F_IN_FREQ;
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d",
			     (uint64_t)addr, error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Exynos SoC multi core timer (64 bits)\n");

	tc_init(&mct_timecounter);

	arm_fdt_cpu_hatch_register(self, mct_fdt_cpu_hatch);
	arm_fdt_timer_register(mct_cpu_initclocks);
}

void
mct_delay(u_int n)
{
	struct mct_softc * const sc = &mct_sc;
	uint64_t cur, prev;

	if (sc->sc_bsh == 0)
		panic("%s: mct driver not attached", __func__);

	const long incs_per_us = sc->sc_freq / 1000000;
	long ticks = n * incs_per_us;

	prev = mct_read_gcnt(sc);
	while (ticks > 0) {
		cur = mct_read_gcnt(sc);
		if (cur > prev)
			ticks -= (cur - prev);
		else
			ticks -= (UINT64_MAX - cur + prev);
		prev = cur;
	}
}
