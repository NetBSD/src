/*	$NetBSD: bq4802_ebus.c,v 1.3 2026/07/07 12:33:39 jdc Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman.
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
__KERNEL_RCSID(0, "$NetBSD: bq4802_ebus.c,v 1.3 2026/07/07 12:33:39 jdc Exp $");

/* Clock driver for rtc/bq4802 (a Texas Instruments bq4802Y/bq4802LY). */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/types.h>
#include <sys/sysctl.h>

#include <sys/bus.h>
#include <machine/autoconf.h>

#include <dev/clock_subr.h>
#include <dev/ic/bq4802reg.h>

#include <dev/ebus/ebusreg.h>
#include <dev/ebus/ebusvar.h>


struct bq4802rtc_softc {
	device_t	sc_dev;

	struct todr_chip_handle	sc_tch;

	bus_space_tag_t		sc_bst;	
	bus_space_handle_t	sc_bsh;

	int			sc_osc_stp;	/* Oscillator stop (sysctl) */
};

static int	bq4802rtc_ebus_match(device_t, cfdata_t, void *);
static void	bq4802rtc_ebus_attach(device_t, device_t, void *);
static int	bq4802_gettime_ymdhms(struct todr_chip_handle *,
		    struct clock_ymdhms *);
static int	bq4802_settime_ymdhms(struct todr_chip_handle *,
		    struct clock_ymdhms *);
static int	sysctl_bq4802_osc(SYSCTLFN_ARGS);

CFATTACH_DECL_NEW(bq4802rtc_ebus, sizeof(struct bq4802rtc_softc),
    bq4802rtc_ebus_match, bq4802rtc_ebus_attach, NULL, NULL);

/* Register read and write. */
#define	bq4802_read(sc, reg) \
    bus_space_read_1(sc->sc_bst, sc->sc_bsh, reg)
#define bq4802_write(sc, reg, val) \
    bus_space_write_1(sc->sc_bst, sc->sc_bsh, reg, val)

static int
bq4802rtc_ebus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct ebus_attach_args *ea = aux;
	char *compat;

	if (strcmp("rtc", ea->ea_name) != 0)
		return 0;

	compat = prom_getpropstring(ea->ea_node, "compatible");
	if (compat != NULL && !strcmp(compat, "bq4802"))
		return 2;	/* We need a better match than 'rtc' */

	return 0;
}

static void
bq4802rtc_ebus_attach(device_t parent, device_t self, void *aux)
{
	struct bq4802rtc_softc *sc = device_private(self);
	struct ebus_attach_args *ea = aux;
	todr_chip_handle_t tch = &sc->sc_tch;
	const struct sysctlnode *me = NULL, *node = NULL;
	int sz;
	uint8_t ctrl, new;

	sc->sc_dev = self;
	sc->sc_bst = ea->ea_bustag;

	sz = ea->ea_reg[0].size;

	if (bus_space_map(sc->sc_bst,
			 EBUS_ADDR_FROM_REG(&ea->ea_reg[0]),
			 sz, 0,
			 &sc->sc_bsh) != 0) {
		aprint_error(": can't map register\n");
		return;
	}

	aprint_normal(": real time clock\n");

	tch->todr_dev = self;
	tch->todr_gettime_ymdhms = bq4802_gettime_ymdhms;
	tch->todr_settime_ymdhms = bq4802_settime_ymdhms;

	/* Setup: alarms off, disable DST, enable updates, 24-hour mode */
	ctrl = bq4802_read(sc, BQ4802_CTRL);
	if (ctrl & BQ4802_CTRL_STP)
		sc->sc_osc_stp = 0;
	else {
		aprint_error_dev(self,
		    "WARNING: oscillator is stopped (0x%02x)\n", ctrl);
		sc->sc_osc_stp = 1;
	}
	new = ctrl & ~(BQ4802_CTRL_DSE | BQ4802_CTRL_UTI);
	new = new | BQ4802_CTRL_24;
	if (new != ctrl)
		bq4802_write(sc, BQ4802_CTRL, ctrl);

	todr_attach(tch);

	/* Setup sysctl for the oscillator control */
	sysctl_createv(NULL, 0, NULL, &me,
	    CTLFLAG_READWRITE,
	    CTLTYPE_NODE, device_xname(self), NULL,
	    NULL, 0, NULL, 0,
	    CTL_HW, CTL_CREATE, CTL_EOL);
	if (me == NULL)
		aprint_error_dev(self, "unable to add sysctl root\n");
	else {
		sysctl_createv(NULL, 0, NULL, &node,
		    CTLFLAG_READWRITE | CTLFLAG_OWNDESC,
		    CTLTYPE_INT, "stop_oscillator", "Stop the chip oscillator",
		    sysctl_bq4802_osc, 1, (void *)sc, 0,
		    CTL_HW, me->sysctl_num, CTL_CREATE, CTL_EOL);
		if (node == NULL)
			aprint_error_dev(self, "unable to add sysctl node\n");
	}
}

static int
bq4802_gettime_ymdhms(struct todr_chip_handle *todrch, struct clock_ymdhms *dt)
{
	struct bq4802rtc_softc *sc = device_private(todrch->todr_dev);
	bq4802_regs regs;
	uint64_t	val;
	int s;

	s = splclock();
	BQ4802_GETTOD(sc, &regs);
	splx(s);

	dt->dt_sec = bcdtobin(regs[BQ4802_SEC]);
	dt->dt_min = bcdtobin(regs[BQ4802_MIN]);
	val = bcdtobin(regs[BQ4802_HOUR]);
	if (val == 24)
		val = 0;
	dt->dt_hour = val;
	dt->dt_wday = bcdtobin(regs[BQ4802_WDAY]);
	dt->dt_day = bcdtobin(regs[BQ4802_DAY]);
	dt->dt_mon = bcdtobin(regs[BQ4802_MONTH]);
	val = bcdtobin(regs[BQ4802_YEAR]);
	val += bcdtobin(regs[BQ4802_CENT]) * 100;
	dt->dt_year = val;

	return 0;
}

static int
bq4802_settime_ymdhms(struct todr_chip_handle *todrch, struct clock_ymdhms *dt)
{
	struct bq4802rtc_softc *sc = device_private(todrch->todr_dev);
	bq4802_regs regs;
	uint8_t val;
	int s;

	regs[BQ4802_SEC] = bintobcd(dt->dt_sec);
	regs[BQ4802_MIN] = bintobcd(dt->dt_min);
	if (dt->dt_hour == 0)
		val = 24;
	else
		val = dt->dt_hour;
	regs[BQ4802_HOUR] = bintobcd(val);
	regs[BQ4802_WDAY] = bintobcd(dt->dt_wday);
	regs[BQ4802_DAY] = bintobcd(dt->dt_day);
	regs[BQ4802_MONTH] = bintobcd(dt->dt_mon);
	regs[BQ4802_YEAR] = bintobcd(dt->dt_year % 100);
	regs[BQ4802_CENT] = bintobcd(dt->dt_year / 100);

	s = splclock();
	BQ4802_SETTOD(sc, &regs);
	splx(s);

	return 0;
}

static int
sysctl_bq4802_osc(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct bq4802rtc_softc *sc = node.sysctl_data;
	int stp;
	uint8_t ctrl;

	if (newp) {
		/* write */
		stp = sc->sc_osc_stp;
		node.sysctl_data = &stp;
		if (sysctl_lookup(SYSCTLFN_CALL(&node)) == 0) {
			if (stp != 0 && stp != 1)
				return EINVAL;

			if (stp != sc->sc_osc_stp) {
				sc->sc_osc_stp = stp;
				ctrl = bq4802_read(sc, BQ4802_CTRL);
				if (stp)
					ctrl &= ~BQ4802_CTRL_STP;
				else
					ctrl |= BQ4802_CTRL_STP;
				bq4802_write(sc, BQ4802_CTRL, ctrl);
			}

			return 0;
		}
		return EINVAL;
	} else {
		node.sysctl_data = &sc->sc_osc_stp;
		node.sysctl_size = 4;
		return (sysctl_lookup(SYSCTLFN_CALL(&node)));
	}

	return 0;
}

/*
SYSCTL_SETUP(sysctl_bq4802_setup, "sysctl bq4802 subtree setup")
{

	sysctl_createv(NULL, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "hw", NULL,
		       NULL, 0, NULL, 0,
		       CTL_HW, CTL_EOL);
}
*/
