/* $NetBSD $ */

/*-
* Copyright (c) 2025 The NetBSD Foundation, Inc.
* All rights reserved.
*
* This code is derived from software contributed to The NetBSD Foundation
* by Yuri Honegger.
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

/*
 * Timer Interrupts for the TI AM18XX SOC
 */

#include <sys/param.h>
#include <sys/cdefs.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>

#include <arm/fdt/arm_fdtvar.h>

struct am18xx_timer_softc {
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
};

static int 	am18xx_timer_match(device_t, cfdata_t, void *);
static void 	am18xx_timer_attach(device_t, device_t, void *);
static void	am18xx_timer_cpu_initclocks(void);
static int	am18xx_timer_irq(void *);

static struct am18xx_timer_softc *timer_sc;

CFATTACH_DECL_NEW(am18xxtimer, sizeof(struct am18xx_timer_softc),
		  am18xx_timer_match, am18xx_timer_attach, NULL, NULL);

#define AM18XX_TIMER_TIM12 0x10
#define AM18XX_TIMER_TIM34 0x14
#define AM18XX_TIMER_PRD12 0x18
#define AM18XX_TIMER_PRD34 0x1C
#define AM18XX_TIMER_TCR 0x20
#define AM18XX_TIMER_TGCR 0x24

#define AM18XX_TIMER_TCR_ENAMODE12_CONTINUOUS 0x80
#define AM18XX_TIMER_TGCR_TIMMODE32_UNCHAINED 0x4
#define AM18XX_TIMER_TGCR_TIM12EN 0x1

#define	TIMER_READ(sc, reg)					\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, reg)
#define	TIMER_WRITE(sc, reg, val)				\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, reg, val)

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ti,da830-timer" },
	DEVICE_COMPAT_EOL
};

static void
am18xx_timer_cpu_initclocks(void)
{
	struct am18xx_timer_softc *sc = timer_sc;
	uint32_t timer_interval = 24000000/hz;

	/* disable counter to allow changing mode */
	TIMER_WRITE(sc, AM18XX_TIMER_TCR, 0);
	/* set mode to 32-bit unchained */
	TIMER_WRITE(sc, AM18XX_TIMER_TGCR, AM18XX_TIMER_TGCR_TIMMODE32_UNCHAINED
					   | AM18XX_TIMER_TGCR_TIM12EN);
	/* start counting from zero */
	TIMER_WRITE(sc, AM18XX_TIMER_TIM12, 0);
	TIMER_WRITE(sc, AM18XX_TIMER_TIM34, 0);
	/* load period registers with maximum period */
	TIMER_WRITE(sc, AM18XX_TIMER_PRD12, timer_interval);
	TIMER_WRITE(sc, AM18XX_TIMER_PRD34, 0);
	/* enable timer */
	TIMER_WRITE(sc, AM18XX_TIMER_TCR, AM18XX_TIMER_TCR_ENAMODE12_CONTINUOUS);
}

int
am18xx_timer_irq(void *frame)
{
	hardclock(frame);

	return 1;
}

int
am18xx_timer_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

void
am18xx_timer_attach(device_t parent, device_t self, void *aux)
{
	struct am18xx_timer_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	char intrstr[128];

	sc->sc_bst = faa->faa_bst;
	timer_sc = sc;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	/* establish interrupt */
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}
	void *ih = fdtbus_intr_establish_xname(phandle, 0, IPL_CLOCK, 0,
					       am18xx_timer_irq, NULL,
					       device_xname(self));
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't install timer interrupt\n");
		return;
	}

	arm_fdt_timer_register(am18xx_timer_cpu_initclocks);

	aprint_normal(": timer on %s\n", intrstr);
}

