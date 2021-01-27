/*	$NetBSD: meson6_timer.c,v 1.2 2021/01/27 03:10:18 thorpej Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
__KERNEL_RCSID(0, "$NetBSD: meson6_timer.c,v 1.2 2021/01/27 03:10:18 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/timetc.h>

#include <dev/fdt/fdtvar.h>

static int	meson6_timer_match(device_t, cfdata_t, void *);
static void	meson6_timer_attach(device_t, device_t, void *);

struct meson6_timer_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct timecounter sc_tc;
};

CFATTACH_DECL_NEW(meson6_timer, sizeof(struct meson6_timer_softc),
    meson6_timer_match, meson6_timer_attach, NULL, NULL);

#define TIMER_READ(sc, reg)			\
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define TIMER_WRITE(sc, reg, val)		\
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

#define MESON_TIMER_MUX					0x00
#define  MESON_TIMER_MUX_TIMERD_EN			__BIT(19)
#define  MESON_TIMER_MUX_TIMERC_EN			__BIT(18)
#define  MESON_TIMER_MUX_TIMERB_EN			__BIT(17)
#define  MESON_TIMER_MUX_TIMERA_EN			__BIT(16)
#define  MESON_TIMER_MUX_TIMERD_MODE			__BIT(15)
#define  MESON_TIMER_MUX_TIMERC_MODE			__BIT(14)
#define  MESON_TIMER_MUX_TIMERB_MODE			__BIT(13)
#define  MESON_TIMER_MUX_TIMERA_MODE			__BIT(12)
#define  MESON_TIMER_MUX_TIMERE_INPUT_CLOCK_MASK	__BITS(10, 8)
#define   MESON_TIMER_MUX_TIMERE_INPUT_CLOCK_SYSTEM_CLOCK	0x0
#define   MESON_TIMER_MUX_TIMERE_INPUT_CLOCK_1US		0x1
#define   MESON_TIMER_MUX_TIMERE_INPUT_CLOCK_10US		0x2
#define   MESON_TIMER_MUX_TIMERE_INPUT_CLOCK_100US		0x3
#define   MESON_TIMER_MUX_TIMERE_INPUT_CLOCK_1MS		0x4
#define  MESON_TIMER_MUX_TIMERD_INPUT_CLOCK_MASK	__BITS(7, 6)
#define  MESON_TIMER_MUX_TIMERC_INPUT_CLOCK_MASK	__BITS(5, 4)
#define  MESON_TIMER_MUX_TIMERB_INPUT_CLOCK_MASK	__BITS(3, 2)
#define  MESON_TIMER_MUX_TIMERA_INPUT_CLOCK_MASK	__BITS(1, 0)
#define   MESON_TIMER_MUX_TIMERABCD_INPUT_CLOCK_1US		0x0
#define   MESON_TIMER_MUX_TIMERABCD_INPUT_CLOCK_10US		0x1
#define   MESON_TIMER_MUX_TIMERABCD_INPUT_CLOCK_100US		0x2
#define   MESON_TIMER_MUX_TIMERABCD_INPUT_CLOCK_1MS		0x3

#define MESON_TIMERA					0x04
#define MESON_TIMERB					0x08
#define MESON_TIMERC					0x0c
#define MESON_TIMERD					0x10
#define MESON_TIMERE					0x14

static u_int
meson6_timer_get_timecount(struct timecounter *tc)
{
	struct meson6_timer_softc * const sc = tc->tc_priv;

	return TIMER_READ(sc, MESON_TIMERE);
}

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "amlogic,meson6-timer" },
	DEVICE_COMPAT_EOL
};

static int
meson6_timer_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
meson6_timer_attach(device_t parent, device_t self, void *aux)
{
	struct meson6_timer_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	/* Enable clocks */
	struct clk *clk;
	for (int i = 0; (clk = fdtbus_clock_get_index(phandle, i)); i++) {
		if (clk_enable(clk) != 0) {
			aprint_error(": failed to enable clock #%d\n", i);
			return;
		}
	}

	aprint_naive("\n");
	aprint_normal(": Timers\n");

	uint32_t mask = TIMER_READ(sc, MESON_TIMER_MUX);
	mask &= ~MESON_TIMER_MUX_TIMERE_INPUT_CLOCK_MASK;
	mask |= __SHIFTIN(MESON_TIMER_MUX_TIMERE_INPUT_CLOCK_1US, MESON_TIMER_MUX_TIMERE_INPUT_CLOCK_MASK);

	TIMER_WRITE(sc, MESON_TIMER_MUX, mask);

	/* Timecounter setup */
	struct timecounter *tc = &sc->sc_tc;
	tc->tc_get_timecount = meson6_timer_get_timecount;
	tc->tc_counter_mask = ~0u;
	tc->tc_frequency = 1000000;
	tc->tc_name = "Timer E";
	tc->tc_quality = 500;
	tc->tc_priv = sc;
	tc_init(tc);
}

