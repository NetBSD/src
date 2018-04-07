/* $NetBSD: tegra_timer.c,v 1.7.10.1 2018/04/07 04:12:12 pgoyette Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: tegra_timer.c,v 1.7.10.1 2018/04/07 04:12:12 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/wdog.h>

#include <dev/sysmon/sysmonvar.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_timerreg.h>
#include <arm/nvidia/tegra_var.h>

#include <dev/fdt/fdtvar.h>

#define TEGRA_TIMER_WDOG_PERIOD_DEFAULT	10

static int	tegra_timer_match(device_t, cfdata_t, void *);
static void	tegra_timer_attach(device_t, device_t, void *);

struct tegra_timer_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct sysmon_wdog	sc_smw;
};

static int	tegra_timer_wdt_setmode(struct sysmon_wdog *);
static int	tegra_timer_wdt_tickle(struct sysmon_wdog *);

CFATTACH_DECL_NEW(tegra_timer, sizeof(struct tegra_timer_softc),
	tegra_timer_match, tegra_timer_attach, NULL, NULL);

#define TIMER_READ(sc, reg)			\
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define TIMER_WRITE(sc, reg, val)		\
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define TIMER_SET_CLEAR(sc, reg, set, clr)	\
    tegra_reg_set_clear((sc)->sc_bst, (sc)->sc_bsh, (reg), (set), (clr))

static int
tegra_timer_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = {
		"nvidia,tegra210-timer",
		"nvidia,tegra124-timer",
		"nvidia,tegra20-timer",
		NULL
	};
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
tegra_timer_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_timer_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d", (uint64_t)addr, error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Timers\n");

	sc->sc_smw.smw_name = device_xname(self);
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_setmode = tegra_timer_wdt_setmode;
	sc->sc_smw.smw_tickle = tegra_timer_wdt_tickle;
	sc->sc_smw.smw_period = TEGRA_TIMER_WDOG_PERIOD_DEFAULT;

	aprint_normal_dev(self,
	    "default watchdog period is %u seconds\n",
	    sc->sc_smw.smw_period);

	if (sysmon_wdog_register(&sc->sc_smw) != 0) {
		aprint_error_dev(self,
		    "couldn't register with sysmon\n");
	}
}

static int
tegra_timer_wdt_setmode(struct sysmon_wdog *smw)
{
	struct tegra_timer_softc * const sc = smw->smw_cookie;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		TIMER_SET_CLEAR(sc, TMR1_PTV_REG, 0, TMR_PTV_EN);
	} else {
		if (smw->smw_period == WDOG_PERIOD_DEFAULT) {
			sc->sc_smw.smw_period = TEGRA_TIMER_WDOG_PERIOD_DEFAULT;
		} else if (smw->smw_period == 0 || smw->smw_period > 1000) {
			return EINVAL;
		} else {
			sc->sc_smw.smw_period = smw->smw_period;
		}
		u_int tval = (sc->sc_smw.smw_period * 1000000) / 2;
		TIMER_WRITE(sc, TMR1_PTV_REG,
		    TMR_PTV_EN | TMR_PTV_PER | __SHIFTIN(tval, TMR_PTV_VAL));
		TIMER_WRITE(sc, TMR1_PCR_REG, TMR_PCR_INTR_CLR);
	}

	return 0;
}

static int
tegra_timer_wdt_tickle(struct sysmon_wdog *smw)
{
	struct tegra_timer_softc * const sc = smw->smw_cookie;

	TIMER_WRITE(sc, TMR1_PCR_REG, TMR_PCR_INTR_CLR);

	return 0;
}

void
tegra_timer_delay(u_int us)
{
	static bool timerus_configured = false;
	extern struct bus_space arm_generic_bs_tag;
	bus_space_tag_t bst = &arm_generic_bs_tag;
	bus_space_handle_t bsh;

	bus_space_subregion(bst, tegra_ppsb_bsh, TEGRA_TIMER_OFFSET,
	    TEGRA_TIMER_SIZE, &bsh);

	if (__predict_false(timerus_configured == false)) {
		/* clk_m frequency 12 MHz */
		bus_space_write_4(bst, bsh, TMRUS_USEC_CFG_REG, 0xb);
		timerus_configured = true;
	}

	u_int nus = 0;
	u_int us_prev = bus_space_read_4(bst, bsh, TMRUS_CNTR_1US_REG);

	while (nus < us) {
		const u_int us_cur = bus_space_read_4(bst, bsh,
		    TMRUS_CNTR_1US_REG);
		if (us_cur < us_prev) {
			nus += (0xffffffff - us_prev) + us_cur;
		} else {
			nus += (us_cur - us_prev);
		}
		us_prev = us_cur;
	}
}
