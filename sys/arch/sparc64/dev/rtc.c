/*	$NetBSD: rtc.c,v 1.2.46.1 2008/01/10 23:44:05 bouyer Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1994 Gordon W. Ross
 * Copyright (c) 1993 Adam Glass
 * Copyright (c) 1996 Paul Kranenburg
 * Copyright (c) 1996
 * 	The President and Fellows of Harvard College. All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by Harvard University.
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 *	This product includes software developed by Paul Kranenburg.
 *	This product includes software developed by Harvard University.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)clock.c	8.1 (Berkeley) 6/11/93
 * from: NetBSD: clock.c,v 1.80 2006/09/03 22:27:45 gdamore Exp
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rtc.c,v 1.2.46.1 2008/01/10 23:44:05 bouyer Exp $");

/*
 * Clock driver for 'rtc' - mc146818 driver.
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <machine/bus.h>
#include <machine/autoconf.h>

#include <dev/clock_subr.h>
#include <dev/ic/mc146818reg.h>
#include <dev/ic/mc146818var.h>

#include <dev/ebus/ebusreg.h>
#include <dev/ebus/ebusvar.h>

static int	rtc_ebus_match(struct device *, struct cfdata *, void *);
static void	rtc_ebus_attach(struct device *, struct device *, void *);

CFATTACH_DECL(rtc_ebus, sizeof(struct mc146818_softc),
    rtc_ebus_match, rtc_ebus_attach, NULL, NULL);

u_int rtc_read_reg(struct mc146818_softc *, u_int);
void rtc_write_reg(struct mc146818_softc *, u_int, u_int);
u_int rtc_getcent(struct mc146818_softc *);
void rtc_setcent(struct mc146818_softc *, u_int);

static int
rtc_ebus_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct ebus_attach_args *ea = aux;

	return (strcmp("rtc", ea->ea_name) == 0);
}

/*
 * `rtc' is a ds1287 on an ebus (actually an isa bus, but we use the
 * ebus driver for isa.)  So we can use ebus_wenable() but need to do
 * different attach work and use different todr routines.  It does not
 * incorporate an IDPROM.
 */

/*
 * XXX the stupid ds1287 is not mapped directly but uses an address
 * and a data reg so we cannot access the stuuupid thing w/o having
 * write access to the registers.
 *
 * XXXX We really need to mutex register access!
 */
#define	RTC_ADDR	0
#define	RTC_DATA	1
u_int
rtc_read_reg(struct mc146818_softc *sc, u_int reg)
{

	bus_space_write_1(sc->sc_bst, sc->sc_bsh, RTC_ADDR, reg);
	return (bus_space_read_1(sc->sc_bst, sc->sc_bsh, RTC_DATA));
}
void 
rtc_write_reg(struct mc146818_softc *sc, u_int reg, u_int val)
{

	bus_space_write_1(sc->sc_bst, sc->sc_bsh, RTC_ADDR, reg);
	bus_space_write_1(sc->sc_bst, sc->sc_bsh, RTC_DATA, val);
}

/* ARGSUSED */
static void
rtc_ebus_attach(struct device *parent, struct device *self, void *aux)
{
	struct mc146818_softc *sc = (void *)self;
	struct ebus_attach_args *ea = aux;
	char *model;
	int sz;

	sc->sc_bst = ea->ea_bustag;

	/* hard code to 8K? */
	sz = ea->ea_reg[0].size;

	if (bus_space_map(sc->sc_bst,
			 EBUS_ADDR_FROM_REG(&ea->ea_reg[0]),
			 sz,
			 BUS_SPACE_MAP_LINEAR,
			 &sc->sc_bsh) != 0) {
		printf("%s: can't map register\n", self->dv_xname);
		return;
	}

	model = prom_getpropstring(ea->ea_node, "model");
#ifdef DIAGNOSTIC
	if (model == NULL)
		panic("clockattach_rtc: no model property");
#endif

	/* Our TOD clock year 0 is 0 */
	sc->sc_year0 = 0;
	sc->sc_flag = MC146818_NO_CENT_ADJUST;
	sc->sc_mcread = rtc_read_reg;
	sc->sc_mcwrite = rtc_write_reg;
	sc->sc_getcent = rtc_getcent;
	sc->sc_setcent = rtc_setcent;
	mc146818_attach(sc);

	printf(": %s\n", model);

	/*
	 * Turn interrupts off, just in case. (Although they shouldn't
	 * be wired to an interrupt controller on sparcs).
	 */
	rtc_write_reg(sc, MC_REGB, MC_REGB_BINARY | MC_REGB_24HR);

	/*
	 * Apparently on some machines the TOD registers are on the same
	 * physical page as the COM registers.  So we won't protect them.
	 */
	/*sc->sc_handle.todr_setwen = NULL;*/
}

/*
 * MD mc146818 RTC todr routines.
 */

/* Loooks like Sun stores the century info somewhere in CMOS RAM */
#define MC_CENT 0x32

u_int
rtc_getcent(sc)
	struct mc146818_softc *sc;
{

	return rtc_read_reg(sc, MC_CENT);
}
void 
rtc_setcent(sc, cent)
	struct mc146818_softc *sc;
	u_int cent;
{

	rtc_write_reg(sc, MC_CENT, cent);
}
