/*	$NetBSD: p_nec_j96a.c,v 1.2 2002/12/07 13:09:46 tsutsui Exp $	*/

/*-
 * Copyright (C) 2002 Izumi Tsutsui.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/pio.h>
#include <machine/platform.h>

#include <arc/jazz/rd94.h>
#include <arc/jazz/jazziovar.h>
#include <arc/dev/mcclockvar.h>
#include <arc/jazz/mcclock_jazziovar.h>

u_int mc_nec_j96a_read __P((struct mcclock_softc *, u_int));
void mc_nec_j96a_write __P((struct mcclock_softc *, u_int, u_int));

void p_nec_j96a_init __P((void));

struct platform platform_nec_j96a = {
	"NEC-J96A",
	"NEC W&S",
	" R4400 EISA",
	"Express5800/240",
	"NEC",
	200, /* MHz */
	c_jazz_eisa_mainbusdevs,
	platform_generic_match,
	p_nec_j96a_init,
	c_jazz_eisa_cons_init,
	jazzio_reset,
	c_nec_jazz_set_intr,
};

static struct mcclock_jazzio_config mcclock_nec_j96a_conf = {
	0x80004000, 1,
	{ mc_nec_j96a_read, mc_nec_j96a_write }
};

u_int
mc_nec_j96a_read(sc, reg)
	struct mcclock_softc *sc;
	u_int reg;
{
	int i, as;

	as = in32(RD94_SYS_EISA_AS) & 0x80;
	out32(RD94_SYS_EISA_AS, as | reg);
	i = bus_space_read_1(sc->sc_iot, sc->sc_ioh, 0);
	return i;
}

void
mc_nec_j96a_write(sc, reg, datum)
	struct mcclock_softc *sc;
	u_int reg, datum;
{
	int as;

	as = in32(RD94_SYS_EISA_AS) & 0x80;
	out32(RD94_SYS_EISA_AS, as | reg);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, 0, datum);
}

void
p_nec_j96a_init()
{

	c_nec_eisa_init();

	mcclock_jazzio_conf = &mcclock_nec_j96a_conf;
}
