/*	$NetBSD: mkclock.c,v 1.1.2.3 2000/12/08 09:28:26 bouyer Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wayne Knowles
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

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/clock_subr.h>

#include <machine/cpu.h>
#include <machine/mainboard.h>
#include <machine/autoconf.h>
#include <machine/sysconf.h>
#include <machine/bus.h>

#include <mipsco/obio/clockreg.h>

struct	mkclock_softc {
        struct  device dev; 
	bus_space_tag_t	sc_bst;
	bus_space_handle_t sc_bsh;
};

static int mkclock_match (struct device *, struct cfdata *, void *);
static void mkclock_attach (struct device *, struct device *, void *);

struct cfattach mkclock_ca = {
	sizeof(struct mkclock_softc), mkclock_match, mkclock_attach
};

static struct mkclock_softc *mk0; 

void mkclock_read (struct clock_ymdhms *);
void mkclock_write (struct clock_ymdhms *);

static int mk_read (struct mkclock_softc *, int);
static void mk_write (struct mkclock_softc *, int, int);

int
mkclock_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return 1;
}

void
mkclock_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
        struct mkclock_softc *sc = (void *)self;
	struct confargs *ca = aux;

	sc->sc_bst = ca->ca_bustag;
	if (bus_space_map(ca->ca_bustag, ca->ca_addr,
			  0x10000,	
			  BUS_SPACE_MAP_LINEAR,
			  &sc->sc_bsh) != 0) {
		printf(": cannot map registers\n");
		return;
	}

	platform.write_todr = mkclock_write;
	platform.read_todr  = mkclock_read;
	mk0 = sc;
	printf("\n");
}

static int
mk_read(sc, reg)
	struct mkclock_softc *sc;
	int reg;
{
	u_int8_t val;

	val = bus_space_read_1(sc->sc_bst, sc->sc_bsh, DATA_PORT + reg*4);
	return FROMBCD(val);
}

static void
mk_write(sc, reg, val)
	struct mkclock_softc *sc;
	int reg, val;
{
	bus_space_write_1(sc->sc_bst, sc->sc_bsh,
			  DATA_PORT + reg*4, TOBCD(val));
}

void
mkclock_read(dt)
	struct clock_ymdhms *dt;
{
	struct mkclock_softc *sc = mk0;
	int s = splclock();

	bus_space_write_1(sc->sc_bst, sc->sc_bsh, RTC_PORT, READ_CLOCK); 
	dt->dt_sec  = mk_read(sc, 0);
	dt->dt_min  = mk_read(sc, 1);
	dt->dt_hour = mk_read(sc, 2);
	dt->dt_wday = mk_read(sc, 3);
	dt->dt_day  = mk_read(sc, 4);
	dt->dt_mon  = mk_read(sc, 5);
	dt->dt_year = mk_read(sc, 6);
	bus_space_write_1(sc->sc_bst, sc->sc_bsh, RTC_PORT, 0); 
	splx(s);

	dt->dt_year = dt->dt_year + (dt->dt_year >= 70 ? 1900 : 2000);
}

void
mkclock_write(dt)
	struct clock_ymdhms *dt;
{
	struct mkclock_softc *sc = mk0;
	int year, s;

	year = dt->dt_year % 100;

	s = splclock();
	bus_space_write_1(sc->sc_bst, sc->sc_bsh, RTC_PORT, SET_CLOCK); 
	mk_write(sc, 0, dt->dt_sec);
	mk_write(sc, 1, dt->dt_min);
	mk_write(sc, 2, dt->dt_hour);
	/* week day not set */
	mk_write(sc, 4, dt->dt_day);
	mk_write(sc, 5, dt->dt_mon);
	mk_write(sc, 6, year);
	bus_space_write_1(sc->sc_bst, sc->sc_bsh, RTC_PORT, 0); 
	splx(s);
}
