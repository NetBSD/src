/* $NetBSD: imx23_timrot.c,v 1.8 2026/02/02 06:23:37 skrll Exp $ */

/*
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Petri Laakso.
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

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/fdt/arm_fdtvar.h>
#include <arm/imx/imx23_timrotreg.h>
#include <arm/imx/imx23var.h>
#include <arm/pic/picvar.h>

#include "opt_arm_timer.h"
#ifdef __HAVE_GENERIC_CPU_INITCLOCKS
void	imx23_timrot_cpu_initclocks(void);
#else
#define imx23_timrot_cpu_initclocks	cpu_initclocks
#endif


struct imx23_timrot_softc {
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_hdl;
};

extern int hz;
extern int stathz;

static int	imx23_timrot_match(device_t, cfdata_t, void *);
static void	imx23_timrot_attach(device_t, device_t, void *);

static void	imx23_timrot_reset(struct imx23_timrot_softc *);
int 		imx23_timrot_systimer_irq(void *frame);
int 		imx23_timrot_stattimer_irq(void *);


void	cpu_initclocks(void);
void 	setstatclockrate(int);

CFATTACH_DECL_NEW(imx23timrot, sizeof(struct imx23_timrot_softc),
		  imx23_timrot_match, imx23_timrot_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "fsl,imx23-timrot" },
	{ .compat = "fsl,timrot" },
	DEVICE_COMPAT_EOL
};

static struct imx23_timrot_softc *timer_sc;

#define TIMROT_SOFT_RST_LOOP 455 /* At least 1 us ... */
#define TIMROT_READ(sc, reg)						\
	bus_space_read_4(sc->sc_iot, sc->sc_hdl, (reg))
#define TIMROT_WRITE(sc, reg, val)					\
	bus_space_write_4(sc->sc_iot, sc->sc_hdl, (reg), (val))

#define TIMER_WRITE(sc, reg, val)					\
	bus_space_write_4(sc->sc_iot, sc->sc_hdl, (reg), (val))
#define TIMER_WRITE_2(sc, reg, val)					\
	bus_space_write_2(sc->sc_iot, sc->sc_hdl, (reg), (val))

#define SELECT_32KHZ	0x8	/* Use 32kHz clock source. */
#define SOURCE_32KHZ_HZ	32000	/* Above source in Hz. */

#define IRQ_EN HW_TIMROT_TIMCTRL0_IRQ_EN
#define UPDATE HW_TIMROT_TIMCTRL0_UPDATE
#define RELOAD HW_TIMROT_TIMCTRL0_RELOAD

static int
imx23_timrot_match(device_t parent, cfdata_t match, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
imx23_timrot_attach(device_t parent, device_t self, void *aux)
{
	struct imx23_timrot_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	char intrstr[128];

	timer_sc = sc;
	sc->sc_iot = faa->faa_bst;
	stathz = (hz>>1);

	/* ma timer registers */
	bus_addr_t addr;
	bus_size_t size;
	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get register address\n");
		return;
	}
	if (bus_space_map(faa->faa_bst, addr, size, 0, &sc->sc_hdl)) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	/* reset timer */
	imx23_timrot_reset(sc);

	/* establish system timer */
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}
	void *ih = fdtbus_intr_establish_xname(phandle, 0, IPL_CLOCK, 0,
					       imx23_timrot_systimer_irq, NULL,
					       device_xname(self));
	if (ih == NULL) {
		aprint_error_dev(self,
				 "couldn't install systimer interrupt handler\n");
		return;
	}
	aprint_normal(": systimer on %s", intrstr);

	/* establish stat timer */
	if (!fdtbus_intr_str(phandle, 1, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}
	ih = fdtbus_intr_establish_xname(phandle, 1, IPL_CLOCK, 0,
					 imx23_timrot_stattimer_irq, NULL,
					 device_xname(self));
	if (ih == NULL) {
		aprint_error_dev(self,
			"couldn't install stattimer interrupt handler\n");
		return;
	}
	aprint_normal(": stattimer on %s\n", intrstr);

	arm_fdt_timer_register(imx23_timrot_cpu_initclocks);
}

/*
 * imx23_timrot_cpu_initclocks is called once at the boot time. It actually
 * starts the timers.
 */
void
imx23_timrot_cpu_initclocks(void)
{
	struct imx23_timrot_softc *sc = timer_sc;
	uint32_t ctrl = IRQ_EN | UPDATE | RELOAD | SELECT_32KHZ;

	// systimer
	TIMER_WRITE_2(sc, HW_TIMROT_TIMCOUNT0,
		      __SHIFTIN(SOURCE_32KHZ_HZ / hz - 1,
				HW_TIMROT_TIMCOUNT0_FIXED_COUNT));
	TIMER_WRITE(sc, HW_TIMROT_TIMCTRL0, ctrl);

	// stattimer
	TIMER_WRITE_2(sc, HW_TIMROT_TIMCOUNT1,
		      __SHIFTIN(SOURCE_32KHZ_HZ / stathz - 1,
				HW_TIMROT_TIMCOUNT1_FIXED_COUNT));
	TIMER_WRITE(sc, HW_TIMROT_TIMCTRL1, ctrl);

	return;
}

/*
 * Change statclock rate when profiling takes place.
 */
void
setstatclockrate(int newhz)
{
	struct imx23_timrot_softc *sc = timer_sc;

	TIMER_WRITE_2(sc, HW_TIMROT_TIMCOUNT1,
		      __SHIFTIN(SOURCE_32KHZ_HZ / newhz - 1,
				HW_TIMROT_TIMCOUNT1_FIXED_COUNT));

	return;
}

/*
 * Timer IRQ handlers.
 */
int
imx23_timrot_systimer_irq(void *frame)
{
	struct imx23_timrot_softc *sc = timer_sc;

	hardclock(frame);

	TIMER_WRITE(sc, HW_TIMROT_TIMCTRL0_CLR, HW_TIMROT_TIMCTRL0_IRQ);

	return 1;
}

int
imx23_timrot_stattimer_irq(void *frame)
{
	struct imx23_timrot_softc *sc = timer_sc;

	statclock(frame);

	TIMER_WRITE(sc, HW_TIMROT_TIMCTRL1_CLR, HW_TIMROT_TIMCTRL1_IRQ);

	return 1;
}

/*
 * Reset the TIMROT block.
 *
 * Inspired by i.MX23 RM "39.3.10 Correct Way to Soft Reset a Block"
 */
static void
imx23_timrot_reset(struct imx23_timrot_softc *sc)
{
	unsigned int loop;

	/* Prepare for soft-reset by making sure that SFTRST is not currently
	* asserted. Also clear CLKGATE so we can wait for its assertion below.
	*/
	TIMROT_WRITE(sc, HW_TIMROT_ROTCTRL_CLR, HW_TIMROT_ROTCTRL_SFTRST);

	/* Wait at least a microsecond for SFTRST to deassert. */
	loop = 0;
	while ((TIMROT_READ(sc, HW_TIMROT_ROTCTRL) & HW_TIMROT_ROTCTRL_SFTRST) ||
	    (loop < TIMROT_SOFT_RST_LOOP))
		loop++;

	/* Clear CLKGATE so we can wait for its assertion below. */
	TIMROT_WRITE(sc, HW_TIMROT_ROTCTRL_CLR, HW_TIMROT_ROTCTRL_CLKGATE);

	/* Soft-reset the block. */
	TIMROT_WRITE(sc, HW_TIMROT_ROTCTRL_SET, HW_TIMROT_ROTCTRL_SFTRST);

	/* Wait until clock is in the gated state. */
	while (!(TIMROT_READ(sc, HW_TIMROT_ROTCTRL) & HW_TIMROT_ROTCTRL_CLKGATE));

	/* Bring block out of reset. */
	TIMROT_WRITE(sc, HW_TIMROT_ROTCTRL_CLR, HW_TIMROT_ROTCTRL_SFTRST);

	loop = 0;
	while ((TIMROT_READ(sc, HW_TIMROT_ROTCTRL) & HW_TIMROT_ROTCTRL_SFTRST) ||
	    (loop < TIMROT_SOFT_RST_LOOP))
		loop++;

	TIMROT_WRITE(sc, HW_TIMROT_ROTCTRL_CLR, HW_TIMROT_ROTCTRL_CLKGATE);
	/* Wait until clock is in the NON-gated state. */
	while (TIMROT_READ(sc, HW_TIMROT_ROTCTRL) & HW_TIMROT_ROTCTRL_CLKGATE);

	return;
}
