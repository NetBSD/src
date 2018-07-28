/* $NetBSD: dwcwdt_fdt.c,v 1.1.2.2 2018/07/28 04:37:44 pgoyette Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: dwcwdt_fdt.c,v 1.1.2.2 2018/07/28 04:37:44 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/wdog.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/fdt/fdtvar.h>

#define	WDT_CR				0x00
#define	 WDT_CR_RST_PULSE_LENGTH	__BITS(4,2)
#define	 WDT_CR_RESP_MODE		__BIT(1)
#define	 WDT_CR_WDT_EN			__BIT(0)

#define	WDT_TORR			0x04
#define	 WDT_TORR_TIMEOUT_PERIOD	__BITS(3,0)

#define	WDT_CCVR			0x08

#define	WDT_CRR				0x0c
#define	 WDT_CRR_CNT_RESTART		__BITS(7,0)
#define	  WDT_CRR_CNT_RESTART_MAGIC	0x76

#define	WDT_STAT			0x10
#define	 WDT_STAT_WDT_STATUS		__BIT(0)

#define	WDT_EOI				0x14
#define	 WDT_EOI_WDT_INT_CLR		__BIT(0)

static const uint8_t wdt_torr[] = {
	0x0000ffff,
	0x0001ffff,
	0x0003ffff,
	0x0007ffff,
	0x000fffff,
	0x001fffff,
	0x003fffff,
	0x007fffff,
	0x00ffffff,
	0x01ffffff,
	0x03ffffff,
	0x07ffffff,
	0x0fffffff,
	0x1fffffff,
	0x3fffffff,
	0x7fffffff,
};

#define	DWCWDT_PERIOD_DEFAULT		15

static const char * const compatible[] = {
	"snps,dw-wdt",
	NULL
};

struct dwcwdt_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	struct sysmon_wdog sc_smw;
	u_int sc_clkrate;
};

#define RD4(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define WR4(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int
dwcwdt_map_period(struct dwcwdt_softc *sc, u_int period,
    u_int *aperiod)
{
	int i;

	if (period == 0)
		return -1;

	for (i = 0; i < __arraycount(wdt_torr); i++) {
		const u_int ms = (u_int)((((uint64_t)wdt_torr[i] + 1) * 1000) / sc->sc_clkrate);
		if (ms >= period) {
			*aperiod = ms;
			return i;
		}
	}

	return -1;
}

static int
dwcwdt_setmode(struct sysmon_wdog *smw)
{
	struct dwcwdt_softc * const sc = smw->smw_cookie;
	uint32_t cr, torr;
	int intv;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED)
		return EIO;

	if (smw->smw_period == WDOG_PERIOD_DEFAULT)
		smw->smw_period = DWCWDT_PERIOD_DEFAULT;

	intv = dwcwdt_map_period(sc, smw->smw_period,
	    &sc->sc_smw.smw_period);
	if (intv == -1)
		return EINVAL;

	torr = __SHIFTIN(intv, WDT_TORR_TIMEOUT_PERIOD);
	WR4(sc, WDT_TORR, torr);

	cr = RD4(sc, WDT_CR);
	cr &= ~WDT_CR_RESP_MODE;
	cr |= WDT_CR_WDT_EN;
	WR4(sc, WDT_CR, cr);

	return 0;
}

static int
dwcwdt_tickle(struct sysmon_wdog *smw)
{
	struct dwcwdt_softc * const sc = smw->smw_cookie;
	const uint32_t crr =
	    __SHIFTIN(WDT_CRR_CNT_RESTART_MAGIC, WDT_CRR_CNT_RESTART);

	WR4(sc, WDT_CRR, crr);

	return 0;
}

static int
dwcwdt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
dwcwdt_attach(device_t parent, device_t self, void *aux)
{
	struct dwcwdt_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct fdtbus_reset *rst;
	struct clk *clk;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	clk = fdtbus_clock_get_index(phandle, 0);
	if (clk == NULL || clk_enable(clk) != 0) {
		aprint_error(": couldn't enable clock\n");
		return;
	}
	rst = fdtbus_reset_get_index(phandle, 0);
	if (rst && fdtbus_reset_deassert(rst) != 0) {
		aprint_error(": couldn't de-assert reset\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_clkrate = clk_get_rate(clk);

	aprint_naive("\n");
	aprint_normal(": DesignWare Watchdog Timer\n");

	sc->sc_smw.smw_name = device_xname(self);
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_period = DWCWDT_PERIOD_DEFAULT;
	sc->sc_smw.smw_setmode = dwcwdt_setmode;
	sc->sc_smw.smw_tickle = dwcwdt_tickle;

	aprint_normal_dev(self,
	    "default watchdog period is %u seconds\n",
	    sc->sc_smw.smw_period);

	if (sysmon_wdog_register(&sc->sc_smw) != 0)
		aprint_error_dev(self, "couldn't register with sysmon\n");
}

CFATTACH_DECL_NEW(dwcwdt_fdt, sizeof(struct dwcwdt_softc),
	dwcwdt_match, dwcwdt_attach, NULL, NULL);
