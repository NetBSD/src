/*	$NetBSD: j720pwr.c,v 1.3 2006/10/07 14:02:09 peter Exp $	*/

/*-
 * Copyright (c) 2002, 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus and Peter Postma.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/* Jornada 720 power management. */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: j720pwr.c,v 1.3 2006/10/07 14:02:09 peter Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/device.h>

#include <dev/apm/apmbios.h>

#include <machine/config_hook.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <arm/sa11x0/sa11x0_var.h>
#include <arm/sa11x0/sa11x0_gpioreg.h>
#include <arm/sa11x0/sa11x0_ppcreg.h>
#include <arm/sa11x0/sa11x0_sspreg.h>

#include <hpcarm/dev/j720sspvar.h>

#ifdef DEBUG
#define DPRINTF(arg)	printf arg
#else
#define DPRINTF(arg)	/* nothing */
#endif

#define arraysize(ary)	(sizeof(ary) / sizeof(ary[0]))

struct j720pwr_softc {
	struct device		sc_dev;

	struct j720ssp_softc	*sc_ssp;

	volatile int		sc_state;
#define J720PWR_POWEROFF	0x01
#define J720PWR_SLEEPING	0x02
};

static int	j720pwr_match(struct device *, struct cfdata *, void *);
static void	j720pwr_attach(struct device *, struct device *, void *);

static void	j720pwr_sleep(void *);
static int	j720pwr_suspend_hook(void *, int, long, void *);
static int	j720pwr_event_hook(void *, int, long, void *);
static int	j720pwr_apm_getpower_hook(void *, int, long, void *);
static int	j720pwr_get_battery(struct j720pwr_softc *);
static int	j720pwr_get_ac_status(struct j720pwr_softc *);
static int	j720pwr_get_charge_status(struct j720pwr_softc *);

static const struct {
	int	percent;
	int	value;
	int	state;
} battery_table[] = {
	{ 100,	670,	APM_BATT_FLAG_HIGH	},
	{  90,	660,	APM_BATT_FLAG_HIGH	},
	{  80,	650,	APM_BATT_FLAG_HIGH	},
	{  70,	640,	APM_BATT_FLAG_HIGH	},
	{  60,	630,	APM_BATT_FLAG_HIGH	},
	{  50,	620,	APM_BATT_FLAG_HIGH	},
	{  40,	605,	APM_BATT_FLAG_LOW	},
	{  30,	580,	APM_BATT_FLAG_LOW	},
	{  20,	545,	APM_BATT_FLAG_LOW	},
	{  10,	500,	APM_BATT_FLAG_CRITICAL	},
	{   0,  430,	APM_BATT_FLAG_CRITICAL	},
};

CFATTACH_DECL(j720pwr, sizeof(struct j720pwr_softc),
    j720pwr_match, j720pwr_attach, NULL, NULL);


static int
j720pwr_match(struct device *parent, struct cfdata *cf, void *aux)
{

	if (!platid_match(&platid, &platid_mask_MACH_HP_JORNADA_7XX))
		return 0;
	if (strcmp(cf->cf_name, "j720pwr") != 0)
		return 0;

	return 1;
}

static void
j720pwr_attach(struct device *parent, struct device *self, void *aux)
{
	struct j720pwr_softc *sc = (struct j720pwr_softc *)self;
	extern void (*__sleep_func)(void *);
	extern void *__sleep_ctx;

	printf("\n");

	sc->sc_ssp = (struct j720ssp_softc *)parent;
	sc->sc_state = 0;

	/* Register apm sleep function. */
	__sleep_func = j720pwr_sleep;
	__sleep_ctx = sc;

	/* Battery status query hook. */
	config_hook(CONFIG_HOOK_GET, CONFIG_HOOK_BATTERYVAL,
		    CONFIG_HOOK_EXCLUSIVE, j720pwr_apm_getpower_hook, sc);

	/* Battery charge status query hook. */
	config_hook(CONFIG_HOOK_GET, CONFIG_HOOK_CHARGE,
		    CONFIG_HOOK_EXCLUSIVE, j720pwr_apm_getpower_hook, sc);

	/* AC status query hook. */
	config_hook(CONFIG_HOOK_GET, CONFIG_HOOK_ACADAPTER,
		    CONFIG_HOOK_EXCLUSIVE, j720pwr_apm_getpower_hook, sc);

	/* Suspend/resume button hook. */
	config_hook(CONFIG_HOOK_BUTTONEVENT,
		    CONFIG_HOOK_BUTTONEVENT_POWER,
		    CONFIG_HOOK_SHARE, j720pwr_suspend_hook, sc);

	/* Receive suspend/resume events. */
	config_hook(CONFIG_HOOK_PMEVENT,
		    CONFIG_HOOK_PMEVENT_HARDPOWER,
		    CONFIG_HOOK_SHARE, j720pwr_event_hook, sc);

	/* Attach hpcapm. */
	config_found_ia(self, "hpcapmif", NULL, NULL);
}

static void
j720pwr_sleep(void *ctx)
{
	struct j720pwr_softc *sc = ctx;
	struct j720ssp_softc *ssp = sc->sc_ssp;
	uint32_t oldfer;

	/* Disable falling-edge detect on all GPIO ports, except keyboard. */
	oldfer = bus_space_read_4(ssp->sc_iot, ssp->sc_gpioh, SAGPIO_FER);
	bus_space_write_4(ssp->sc_iot, ssp->sc_gpioh, SAGPIO_FER, 1 << 0);

	while (sc->sc_state & J720PWR_POWEROFF) {
		/*
		 * Just sleep here until the poweroff bit gets unset.
		 * We need to wait here because when machine_sleep() returns
		 * hpcapm(4) assumes that we are "resuming".
		 */
		(void)tsleep(&sc->sc_state, PWAIT, "j720slp", 0);
	}

	/* Restore previous FER value. */
	bus_space_write_4(ssp->sc_iot, ssp->sc_gpioh, SAGPIO_FER, oldfer);
}

static int
j720pwr_suspend_hook(void *ctx, int type, long id, void *msg)
{
	struct j720pwr_softc *sc = ctx;

	if (type != CONFIG_HOOK_BUTTONEVENT ||
	    id != CONFIG_HOOK_BUTTONEVENT_POWER)
		return EINVAL;

	if ((sc->sc_state & (J720PWR_POWEROFF | J720PWR_SLEEPING)) == 0) {
		sc->sc_state |= J720PWR_POWEROFF;
	} else if ((sc->sc_state & (J720PWR_POWEROFF | J720PWR_SLEEPING)) ==
	    (J720PWR_POWEROFF | J720PWR_SLEEPING)) {
		sc->sc_state &= ~J720PWR_POWEROFF;
		wakeup(&sc->sc_state);
	} else {
		DPRINTF(("j720pwr_suspend_hook: busy\n"));
		return EBUSY;
	}

	config_hook_call(CONFIG_HOOK_PMEVENT,
		         CONFIG_HOOK_PMEVENT_SUSPENDREQ, NULL);

	return 0;
}

static int
j720pwr_event_hook(void *ctx, int type, long id, void *msg)
{
	struct j720pwr_softc *sc = ctx;
	int event = (int)msg;

	if (type != CONFIG_HOOK_PMEVENT ||
	    id != CONFIG_HOOK_PMEVENT_HARDPOWER)
		return EINVAL;

	switch (event) {
	case PWR_SUSPEND:
		sc->sc_state |= (J720PWR_SLEEPING | J720PWR_POWEROFF);
		break;
	case PWR_RESUME:
		sc->sc_state &= ~(J720PWR_SLEEPING | J720PWR_POWEROFF);
		break;
	default:
		return EINVAL;
	}

	return 0;
}

static int
j720pwr_apm_getpower_hook(void *ctx, int type, long id, void *msg)
{
	int * const pval = msg;
	int val, tmp, i, state = 0;

	if (type != CONFIG_HOOK_GET)
		return EINVAL;

	switch (id) {
	case CONFIG_HOOK_BATTERYVAL:
		val = j720pwr_get_battery(ctx);

		if (val != -1) {
			for (i = 0; i < arraysize(battery_table); i++)
				if (val > battery_table[i].value)
					break;
			if (i == 0) {
				/* Battery charge status is at maximum. */
				*pval = 100;
			} else {
				/*
				 * Use linear interpolation to calculate
				 * the estimated charge status.
				 */
				tmp = ((val - battery_table[i].value) * 100) /
				    (battery_table[i - 1].value -
				     battery_table[i].value);
				*pval = battery_table[i].percent +
				    ((battery_table[i - 1].percent -
				      battery_table[i].percent) * tmp) / 100;
			}
		} else {
			/* Battery is absent. */
			*pval = 0;
		}

		return 0;

	case CONFIG_HOOK_CHARGE:
		val = j720pwr_get_battery(ctx);

		if (val != -1) {
			for (i = 1; i < arraysize(battery_table); i++) {
				if (val > battery_table[i].value) {
					state = battery_table[i - 1].state;
					break;
				}
			}

			if (j720pwr_get_charge_status(ctx) == 0)
				state |= APM_BATT_FLAG_CHARGING;
		} else {
			state = APM_BATT_FLAG_NO_SYSTEM_BATTERY;
		}

		*pval = state;
		return 0;

	case CONFIG_HOOK_ACADAPTER:
		*pval = j720pwr_get_ac_status(ctx) ? APM_AC_OFF : APM_AC_ON;

		return 0;
	}

	return EINVAL;	
}

static int
j720pwr_get_battery(struct j720pwr_softc *sc)
{
	struct j720ssp_softc *ssp = sc->sc_ssp;
	int data, i, pmdata[3];

	bus_space_write_4(ssp->sc_iot, ssp->sc_gpioh, SAGPIO_PCR, 0x2000000);

	if (j720ssp_readwrite(ssp, 1, 0xc0, &data, 500) < 0 || data != 0x11) {
		DPRINTF(("j720pwr_get_battery: no dummy received\n"));
		goto out;
	}

	for (i = 0; i < 3; i++) {
		if (j720ssp_readwrite(ssp, 0, 0x11, &pmdata[i], 100) < 0)
			goto out;
	}

	bus_space_write_4(ssp->sc_iot, ssp->sc_gpioh, SAGPIO_PSR, 0x2000000);

	pmdata[0] |= (pmdata[2] & 0x3) << 8;	/* Main battery. */
	pmdata[1] |= (pmdata[2] & 0xc) << 6;	/* Backup battery (unused). */

	DPRINTF(("j720pwr_get_battery: data[0]=%d data[1]=%d data[2]=%d\n",
	    pmdata[0], pmdata[1], pmdata[2]));

	/* If bit 0 and 1 are both set, the main battery is absent. */
	if ((pmdata[2] & 3) == 3)
		return -1;

	return pmdata[0];

out:
	bus_space_write_4(ssp->sc_iot, ssp->sc_gpioh, SAGPIO_PSR, 0x2000000);

	/* reset SSP */
	bus_space_write_4(ssp->sc_iot, ssp->sc_ssph, SASSP_CR0, 0x307);
	delay(100);
	bus_space_write_4(ssp->sc_iot, ssp->sc_ssph, SASSP_CR0, 0x387);

	DPRINTF(("j720pwr_get_battery: error %x\n", data));
	return 0;
}

static int
j720pwr_get_ac_status(struct j720pwr_softc *sc)
{
	struct j720ssp_softc *ssp = sc->sc_ssp;
	uint32_t status;

	status = bus_space_read_4(ssp->sc_iot, ssp->sc_gpioh, SAGPIO_PLR);

	return status & (1 << 4);
}

static int
j720pwr_get_charge_status(struct j720pwr_softc *sc)
{
	struct j720ssp_softc *ssp = sc->sc_ssp;
	uint32_t status;

	status = bus_space_read_4(ssp->sc_iot, ssp->sc_gpioh, SAGPIO_PLR);

	return status & (1 << 26);
}
