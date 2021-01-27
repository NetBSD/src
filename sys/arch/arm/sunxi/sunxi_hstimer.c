/* $NetBSD: sunxi_hstimer.c,v 1.4 2021/01/27 03:10:20 thorpej Exp $ */

/*-
 * Copyright (c) 2019 Tobias Nygren <tnn@NetBSD.org>
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sunxi_hstimer.c,v 1.4 2021/01/27 03:10:20 thorpej Exp $");

#include <sys/bus.h>
#include <sys/device.h>
#include <sys/timetc.h>
#include <dev/fdt/fdtvar.h>

/* High Speed Timer registers */
#define	HS_TMR_IRQ_EN_REG	0x0
#define	HS_TMR_IRQ_EN(n)	__BIT(n)
#define	HS_TMR_IRQ_STAS_REG	0x4
#define	HS_TMR_STAS_PEND(n)	__BIT(n)
#define	HS_TMR0_CTRL_REG	0x10
#define	 HS_TMR0_CTRL_MODE	__BIT(7)
#define	 HS_TMR0_CTRL_CLK_PRESCALE	__BITS(6,4)
#define	 HS_TMR0_CTRL_RELOAD	__BIT(1)
#define	 HS_TMR0_CTRL_EN	__BIT(0)
#define	HS_TMR0_INTV_LO_REG	0x14
#define	HS_TMR0_INTV_HI_REG	0x18
#define	HS_TMR0_CURNT_LO_REG	0x1c
#define	HS_TMR0_CURNT_HI_REG	0x20
#define	HS_TMR1_CTRL_REG	0x30
#define	 HS_TMR1_CTRL_MODE	__BIT(7)
#define	 HS_TMR1_CTRL_CLK_PRESCALE	__BITS(6,4)
#define	 HS_TMR1_CTRL_RELOAD	__BIT(1)
#define	 HS_TMR1_CTRL_EN	__BIT(0)
#define	HS_TMR1_INTV_LO_REG	0x34
#define	HS_TMR1_INTV_HI_REG	0x38
#define	HS_TMR1_CURNT_LO_REG	0x3c
#define	HS_TMR1_CURNT_HI_REG	0x40
#define	HS_TMR2_CTRL_REG	0x50
#define	 HS_TMR2_CTRL_MODE	__BIT(7)
#define	 HS_TMR2_CTRL_CLK_PRESCALE	__BITS(6,4)
#define	 HS_TMR2_CTRL_RELOAD	__BIT(1)
#define	 HS_TMR2_CTRL_EN	__BIT(0)
#define	HS_TMR2_INTV_LO_REG	0x54
#define	HS_TMR2_INTV_HI_REG	0x58
#define	HS_TMR2_CURNT_LO_REG	0x5c
#define	HS_TMR2_CURNT_HI_REG	0x60
#define	HS_TMR3_CTRL_REG	0x70
#define	 HS_TMR3_CTRL_MODE	__BIT(7)
#define	 HS_TMR3_CTRL_CLK_PRESCALE	__BITS(6,4)
#define	 HS_TMR3_CTRL_RELOAD	__BIT(1)
#define	 HS_TMR3_CTRL_EN	__BIT(0)
#define	HS_TMR3_INTV_LO_REG	0x74
#define	HS_TMR3_INTV_HI_REG	0x78
#define	HS_TMR3_CURNT_LO_REG	0x7c
#define	HS_TMR3_CURNT_HI_REG	0x80

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "allwinner,sun5i-a13-hstimer" },
	{ .compat = "allwinner,sun6i-a31-hstimer" },
	{ .compat = "allwinner,sun7i-a20-hstimer" },
	DEVICE_COMPAT_EOL
};

struct sunxi_hstimer_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;
	void			*sc_ih;
	struct timecounter	sc_tc;
};

#define TIMER_READ(sc, reg) \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define TIMER_WRITE(sc, reg, val) \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int
sunxi_hstimer_intr(void *arg)
{
	struct sunxi_hstimer_softc *sc = arg;
	uint32_t stas;

	stas = TIMER_READ(sc, HS_TMR_IRQ_STAS_REG);
	if (stas == 0)
		return 0;
	TIMER_WRITE(sc, HS_TMR_IRQ_STAS_REG, stas);

	return 1;
}

static u_int
sunxi_hstimer_get_timecount(struct timecounter *tc)
{
	struct sunxi_hstimer_softc *sc = tc->tc_priv;

	/*
	 * Timer current value is a 56-bit down counter.
	 * But we only need the lower 32 bits for timecounter.
	 */
	return ~TIMER_READ(sc, HS_TMR0_CURNT_LO_REG);
}

static int
sunxi_hstimer_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
sunxi_hstimer_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_hstimer_softc *sc = device_private(self);
	struct fdt_attach_args *faa = aux;
	struct timecounter *tc = &sc->sc_tc;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	char intrstr[128];
	struct clk *clk;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;

	if ((clk = fdtbus_clock_get_index(phandle, 0)) == NULL
	    || clk_enable(clk) != 0) {
		aprint_error(": couldn't enable clock\n");
		return;
	}

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0
	    || bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": High Speed Timer\n");

	/* Disable IRQs and all timers */
	TIMER_WRITE(sc, HS_TMR_IRQ_EN_REG, 0);
	TIMER_WRITE(sc, HS_TMR_IRQ_STAS_REG, TIMER_READ(sc, HS_TMR_IRQ_STAS_REG));
	/* Enable Timer 0 (timecounter) */
	TIMER_WRITE(sc, HS_TMR0_CTRL_REG, 0);
	TIMER_WRITE(sc, HS_TMR0_INTV_LO_REG, ~0u);
	TIMER_WRITE(sc, HS_TMR0_INTV_HI_REG, ~0u);
	TIMER_WRITE(sc, HS_TMR0_CTRL_REG,
	    HS_TMR0_CTRL_RELOAD | HS_TMR0_CTRL_EN);

	/* Timecounter setup */
	tc->tc_get_timecount = sunxi_hstimer_get_timecount;
	tc->tc_counter_mask = ~0u;
	tc->tc_frequency = clk_get_rate(clk);
	tc->tc_name = "hstimer";
	tc->tc_quality = 300;
	tc->tc_priv = sc;
	tc_init(tc);

	if (!fdtbus_intr_str(sc->sc_phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}
	sc->sc_ih = fdtbus_intr_establish_xname(sc->sc_phandle, 0, IPL_CLOCK,
	    FDT_INTR_MPSAFE, sunxi_hstimer_intr, sc, device_xname(sc->sc_dev));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
				 intrstr);
		return;
	}
	aprint_normal_dev(sc->sc_dev, "interrupting on %s\n", intrstr);
}

CFATTACH_DECL_NEW(sunxi_hstimer, sizeof(struct sunxi_hstimer_softc),
	sunxi_hstimer_match, sunxi_hstimer_attach, NULL, NULL);

