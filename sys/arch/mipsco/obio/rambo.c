/*	$NetBSD: rambo.c,v 1.6 2003/07/15 02:43:44 lukem Exp $	*/

/*
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rambo.c,v 1.6 2003/07/15 02:43:44 lukem Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/mainboard.h>
#include <machine/autoconf.h>
#include <machine/sysconf.h>
#include <machine/bus.h>

#include <mipsco/obio/rambo.h>

/*
 * Timer & Interrupt manipulation routines for the Rambo Custom ASIC 
 */

static int	rambo_match  __P((struct device *, struct cfdata *, void *));
static void	rambo_attach __P((struct device *, struct device *, void *));
static unsigned rambo_clkread __P((void));
void rambo_clkintr __P((struct clockframe *));

struct rambo_softc {
        struct device		dev; 
	struct evcnt		sc_intrcnt;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	u_int32_t		sc_tclast;
	u_int32_t		sc_hzticks;
};

static struct rambo_softc *rambo;

CFATTACH_DECL(rambo, sizeof(struct rambo_softc),
    rambo_match, rambo_attach, NULL, NULL);

static int
rambo_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return 1;
}

static void
rambo_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct confargs *ca = aux;
	struct rambo_softc *sc = (void *)self;

	sc->sc_bst = ca->ca_bustag;

	if (bus_space_map(ca->ca_bustag, ca->ca_addr, 256,
			  BUS_SPACE_MAP_LINEAR,
			  &sc->sc_bsh) != 0) {
		printf(": cannot map registers\n");
		return;
	}
	evcnt_attach_dynamic(&sc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
			     "timer", "intr");

	/* Setup RAMBO Timer to generate timer interrupts */
	sc->sc_hzticks = HZ_TO_TICKS(hz);

	sc->sc_tclast = 0;
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, RB_TCOUNT, 0);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, RB_TBREAK, sc->sc_hzticks);

	bus_space_write_4(sc->sc_bst, sc->sc_bsh, RB_CTLREG,
			  RB_PARITY_EN | RB_BUZZOFF | RB_CLR_IOERR);

	printf(": parity enabled\n");
	rambo = sc;
	platform.clkread = rambo_clkread;
}

void
rambo_clkintr(cf)
	struct clockframe *cf;
{
	register u_int32_t tbreak, tcount;
	register int delta;

	rambo->sc_intrcnt.ev_count++;
	tbreak = bus_space_read_4(rambo->sc_bst, rambo->sc_bsh, RB_TBREAK);
	tcount = bus_space_read_4(rambo->sc_bst, rambo->sc_bsh,	RB_TCOUNT);
	delta  = tcount - tbreak;

	if (delta > (rambo->sc_hzticks>>1)) {
		/*
		 * Either tcount may overtake the updated tbreak value
		 * or we have missed several interrupt's
		 */
		int cycles = 10 * hz;
		while (cycles && tbreak < tcount) {
			hardclock(cf);
			rambo->sc_tclast = tbreak;
			tbreak += rambo->sc_hzticks;
			cycles--;
		}
		if (cycles == 0) { /* catchup failed - assume we are in sync */
			tcount = bus_space_read_4(rambo->sc_bst,
						  rambo->sc_bsh, RB_TCOUNT);
			rambo->sc_tclast = tbreak = tcount;
		}
	} else {
		hardclock(cf);
		rambo->sc_tclast = tbreak;
	}

	tbreak += rambo->sc_hzticks;

	bus_space_write_4(rambo->sc_bst, rambo->sc_bsh, RB_TBREAK, tbreak);
}    

/*
 * Calculate the number of microseconds since the last clock tick
 */
static unsigned
rambo_clkread()
{
        register u_int32_t tcount;
	
	tcount = bus_space_read_4(rambo->sc_bst, rambo->sc_bsh, RB_TCOUNT);
	return TICKS_TO_USECS(tcount - rambo->sc_tclast);
}
