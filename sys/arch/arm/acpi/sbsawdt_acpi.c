/* $NetBSD: sbsawdt_acpi.c,v 1.1 2018/10/24 11:01:47 jmcneill Exp $ */

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jared McNeill <jmcneill@invisible.ca>.
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
 * ARM Server Base System Architecture (SBSA)-compliant generic watchdog
 * driver, as specified in SBSA v3.1:
 *
 * https://static.docs.arm.com/den0029/a/Server_Base_System_Architecture_v3_1_ARM_DEN_0029A.pdf
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sbsawdt_acpi.c,v 1.1 2018/10/24 11:01:47 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/wdog.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

extern struct bus_space arm_generic_bs_tag;

#define	SBSAWDT_REFRESH_SIZE	0x1000
#define	SBSAWDT_CONTROL_SIZE	0x1000

/* Refresh frame registers */
#define	R_WRR_REG		0x000
#define	R_W_IIDR_REG		0xfcc

/* Control frame registers */
#define	C_WCS_REG		0x000
#define	 C_WCS_WS1		__BIT(2)
#define	 C_WCS_WS0		__BIT(1)
#define	 C_WCS_EN		__BIT(0)
#define	C_WOR_REG		0x008
#define	C_WCV_LO_REG		0x010
#define	C_WCV_HI_REG		0x014
#define	C_W_IIDR_REG		0xfcc

struct sbsawdt_acpi_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh_c;	/* control frame */
	bus_space_handle_t	sc_bsh_r;	/* refresh frame */

	uint32_t		sc_cntfreq;
	uint32_t		sc_max_period;
	struct sysmon_wdog	sc_smw;
};

#define	REFRESH_RD4(sc, reg)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh_r, (reg))
#define	REFRESH_WR4(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh_r, (reg), (val))

#define	CONTROL_RD4(sc, reg)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh_c, (reg), (val))
#define	CONTROL_WR4(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh_c, (reg), (val))

static int	sbsawdt_acpi_match(device_t, cfdata_t, void *);
static void	sbsawdt_acpi_attach(device_t, device_t, void *);

static int	sbsawdt_acpi_tickle(struct sysmon_wdog *);
static int	sbsawdt_acpi_setmode(struct sysmon_wdog *);

CFATTACH_DECL_NEW(sbsawdt_acpi, sizeof(struct sbsawdt_acpi_softc), sbsawdt_acpi_match, sbsawdt_acpi_attach, NULL, NULL);

static int
sbsawdt_acpi_match(device_t parent, cfdata_t cf, void *aux)
{
	ACPI_GTDT_HEADER * const hdrp = aux;

	if (hdrp->Type != ACPI_GTDT_TYPE_WATCHDOG)
		return 0;

	ACPI_GTDT_WATCHDOG * const wdog = aux;

	if ((wdog->TimerFlags & ACPI_GTDT_WATCHDOG_SECURE) != 0)
		return 0;		/* We need a non-secure timer */

	return 1;
}

static void
sbsawdt_acpi_attach(device_t parent, device_t self, void *aux)
{
	struct sbsawdt_acpi_softc * const sc = device_private(self);
	ACPI_GTDT_WATCHDOG * const wdog = aux;

	aprint_naive("\n");
	aprint_normal(": mem %#" PRIx64 "-%#" PRIx64 ",%#" PRIx64 "-%#" PRIx64 "\n",
	    wdog->RefreshFrameAddress, wdog->RefreshFrameAddress + SBSAWDT_REFRESH_SIZE - 1,
	    wdog->ControlFrameAddress, wdog->ControlFrameAddress + SBSAWDT_CONTROL_SIZE - 1);

	sc->sc_dev = self;
	sc->sc_bst = &arm_generic_bs_tag;
	sc->sc_cntfreq = gtmr_cntfrq_read();
	sc->sc_max_period = howmany((uint64_t)UINT32_MAX, sc->sc_cntfreq);
	if (bus_space_map(sc->sc_bst, wdog->RefreshFrameAddress, SBSAWDT_REFRESH_SIZE, 0, &sc->sc_bsh_r) != 0) {
		aprint_error_dev(self, "failed to map refresh frame\n");
		return;
	}
	if (bus_space_map(sc->sc_bst, wdog->ControlFrameAddress, SBSAWDT_CONTROL_SIZE, 0, &sc->sc_bsh_c) != 0) {
		aprint_error_dev(self, "failed to map control frame\n");
		return;
	}

	sc->sc_smw.smw_name = device_xname(self);
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_period = sc->sc_max_period;
	sc->sc_smw.smw_tickle = sbsawdt_acpi_tickle;
	sc->sc_smw.smw_setmode = sbsawdt_acpi_setmode;

	aprint_normal_dev(self, "default watchdog period is %u seconds\n",
	    sc->sc_smw.smw_period);

	if (sysmon_wdog_register(&sc->sc_smw) != 0)
		aprint_error_dev(self, "couldn't register with sysmon\n");
}

static int
sbsawdt_acpi_tickle(struct sysmon_wdog *smw)
{
	struct sbsawdt_acpi_softc * const sc = smw->smw_cookie;

	REFRESH_WR4(sc, R_WRR_REG, 0);

	return 0;
}

static int
sbsawdt_acpi_setmode(struct sysmon_wdog *smw)
{
	struct sbsawdt_acpi_softc * const sc = smw->smw_cookie;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		CONTROL_WR4(sc, C_WCS_REG, 0);
		return 0;
	}

	if (smw->smw_period == WDOG_PERIOD_DEFAULT)
		smw->smw_period = sc->sc_max_period;
	else if (smw->smw_period > sc->sc_max_period)
		return EINVAL;

	/*
	 * Divide the watchdog offset value by two. The first time that the
	 * offset is reached, the WD0 signal is raised with an interrupt. The
	 * second time that the offset is reached, the WD1 signal is raised
	 * which will either interrupt privileged software or cause a PE reset.
	 */
	const uint32_t wor = (smw->smw_period * sc->sc_cntfreq) / 2;

	CONTROL_WR4(sc, C_WCS_REG, 0);
	CONTROL_WR4(sc, C_WOR_REG, wor);
	CONTROL_WR4(sc, C_WCS_REG, C_WCS_EN);
	REFRESH_WR4(sc, R_WRR_REG, 0);

	return 0;
}
