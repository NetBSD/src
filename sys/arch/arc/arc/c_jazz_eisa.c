/*	$NetBSD: c_jazz_eisa.c,v 1.3 2003/01/27 15:33:36 tsutsui Exp $	*/

/*
 * Copyright (c) 1998
 *	Matthias Drochner.  All rights reserved.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * for Magnum derived machines like Microsoft-Jazz and PICA-61,
 * and NEC EISA generation machines.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kcore.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/pio.h>
#include <machine/platform.h>

#include <arc/arc/arcbios.h>
#include <arc/jazz/pica.h>
#include <arc/jazz/jazziovar.h>
#include <arc/dev/mcclockvar.h>
#include <arc/jazz/mcclock_jazziovar.h>

#include "pc.h"
#if NPC_JAZZIO > 0
#include <arc/jazz/pccons_jazziovar.h>
#endif

#include "vga_jazzio.h"
#if NVGA_JAZZIO > 0
#include <arc/jazz/vga_jazziovar.h>
#endif

#include "rasdisplay_jazzio.h"
#if NRASDISPLAY_JAZZIO > 0
#include <arc/jazz/rasdisplay_jazziovar.h>
#endif

#include "pckbc_jazzio.h"
#if NPCKBC_JAZZIO > 0
#include <arc/jazz/pckbc_jazzioreg.h>
#include <dev/ic/pckbcvar.h>
#endif

#include "com.h"
#if NCOM > 0
#include <sys/termios.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

char *c_jazz_eisa_mainbusdevs[] = {
	"jazzio",
	"jazzisabr",
	NULL,
};

/*
 * chipset-dependent mcclock routines.
 */
u_int mc_jazz_eisa_read __P((struct mcclock_softc *, u_int));
void mc_jazz_eisa_write __P((struct mcclock_softc *, u_int, u_int));

struct mcclock_jazzio_config mcclock_jazz_eisa_conf = {
	0x80004000, 1,
	{ mc_jazz_eisa_read, mc_jazz_eisa_write }
};

u_int
mc_jazz_eisa_read(sc, reg)
	struct mcclock_softc *sc;
	u_int reg;
{
	int i, as;

	as = in32(arc_bus_io.bs_vbase + C_JAZZ_EISA_TODCLOCK_AS) & 0x80;
	out32(arc_bus_io.bs_vbase + C_JAZZ_EISA_TODCLOCK_AS, as | reg);
	i = bus_space_read_1(sc->sc_iot, sc->sc_ioh, 0);
	return i;
}

void
mc_jazz_eisa_write(sc, reg, datum)
	struct mcclock_softc *sc;
	u_int reg, datum;
{
	int as;

	as = in32(arc_bus_io.bs_vbase + C_JAZZ_EISA_TODCLOCK_AS) & 0x80;
	out32(arc_bus_io.bs_vbase + C_JAZZ_EISA_TODCLOCK_AS, as | reg);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, 0, datum);
}

/*
 * common configuration for Magnum derived and NEC EISA generation machines.
 */
void
c_jazz_eisa_init()
{

	/* chipset-dependent mcclock configuration */
        mcclock_jazzio_conf = &mcclock_jazz_eisa_conf;
}

/*
 * console initialization
 */
void
c_jazz_eisa_cons_init()
{
	if (!com_console) {

#if NRASDISPLAY_JAZZIO > 0
		if (rasdisplay_jazzio_cnattach(arc_displayc_id) == 0) {
#if NPCKBC_JAZZIO > 0
			pckbc_cnattach(&jazzio_bus, PICA_SYS_KBD,
			    JAZZIO_KBCMDP, PCKBC_KBD_SLOT);
#endif
			return;
		}
#endif

#if NVGA_JAZZIO > 0
		if (vga_jazzio_cnattach(arc_displayc_id) == 0) {
#if NPCKBC_JAZZIO > 0
			pckbc_cnattach(&jazzio_bus, PICA_SYS_KBD,
			    JAZZIO_KBCMDP, PCKBC_KBD_SLOT);
#endif
			return;
		}
#endif
#if NPC_JAZZIO > 0
		if (pccons_jazzio_cnattach(arc_displayc_id, &jazzio_bus) == 0)
			return;
#endif
		printf("jazz: display controller [%s] is not configured\n",
		    arc_displayc_id);
	}

#if NCOM > 0
	if (com_console_address == 0) {
		/*
		 * XXX
		 * the following is either PICA_SYS_COM1 or RD94_SYS_COM1.
		 */
		com_console_address = jazzio_bus.bs_start + 0x6000;
	}
	comcnattach(&jazzio_bus, com_console_address,
	    com_console_speed, com_freq, com_console_mode);
#endif
}
