/*	$NetBSD: c_jazz_eisa.c,v 1.13.2.1 2012/10/30 17:18:52 yamt Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: c_jazz_eisa.c,v 1.13.2.1 2012/10/30 17:18:52 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kcore.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <sys/bus.h>
#include <machine/pio.h>
#include <machine/platform.h>

#include <dev/clock_subr.h>
#include <dev/ic/mc146818var.h>

#include <arc/arc/arcbios.h>
#include <arc/jazz/pica.h>
#include <arc/jazz/jazziovar.h>
#include <arc/jazz/mcclock_jazziovar.h>

#include "pc.h"
#if NPC_JAZZIO > 0
#include <arc/jazz/pccons_jazziovar.h>
#endif

#include "vga_isa.h"
#if NVGA_ISA > 0
#include <dev/isa/vga_isavar.h>
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

const char *c_jazz_eisa_mainbusdevs[] = {
	"jazzio",
	"jazzisabr",
	NULL,
};

/*
 * chipset-dependent mcclock routines.
 */
static u_int mc_jazz_eisa_read(struct mc146818_softc *, u_int);
static void mc_jazz_eisa_write(struct mc146818_softc *, u_int, u_int);

struct mcclock_jazzio_config mcclock_jazz_eisa_conf = {
	0x80004000,		/* I/O base */
	1,			/* I/O size */
	mc_jazz_eisa_read,	/* read function */
	mc_jazz_eisa_write	/* write function */
};

static u_int
mc_jazz_eisa_read(struct mc146818_softc *sc, u_int reg)
{
	u_int i, as;

	as = in32(arc_bus_io.bs_vbase + C_JAZZ_EISA_TODCLOCK_AS) & 0x80;
	out32(arc_bus_io.bs_vbase + C_JAZZ_EISA_TODCLOCK_AS, as | reg);
	i = bus_space_read_1(sc->sc_bst, sc->sc_bsh, 0);
	return i;
}

static void
mc_jazz_eisa_write(struct mc146818_softc *sc, u_int reg, u_int datum)
{
	u_int as;

	as = in32(arc_bus_io.bs_vbase + C_JAZZ_EISA_TODCLOCK_AS) & 0x80;
	out32(arc_bus_io.bs_vbase + C_JAZZ_EISA_TODCLOCK_AS, as | reg);
	bus_space_write_1(sc->sc_bst, sc->sc_bsh, 0, datum);
}

/*
 * common configuration for Magnum derived and NEC EISA generation machines.
 */
void
c_jazz_eisa_init(void)
{

	/* chipset-dependent mcclock configuration */
        mcclock_jazzio_conf = &mcclock_jazz_eisa_conf;
}

/*
 * console initialization
 */
void
c_jazz_eisa_cons_init(void)
{

	if (!com_console) {

#if NRASDISPLAY_JAZZIO > 0
		if (rasdisplay_jazzio_cnattach(arc_displayc_id) == 0) {
#if NPCKBC_JAZZIO > 0
			pckbc_cnattach(&jazzio_bus, PICA_SYS_KBD,
			    JAZZIO_KBCMDP, PCKBC_KBD_SLOT, 0);
#endif
			return;
		}
#endif

#if NVGA_JAZZIO > 0
		if (vga_jazzio_cnattach(arc_displayc_id) == 0) {
#if NPCKBC_JAZZIO > 0
			pckbc_cnattach(&jazzio_bus, PICA_SYS_KBD,
			    JAZZIO_KBCMDP, PCKBC_KBD_SLOT, 0);
#endif
			return;
		}
#endif

#if NVGA_ISA > 0
		if (strcmp(arc_displayc_id, "necvdfrb") == 0
			/* NEC RISCserver 2200 R4400 EISA [NEC-R96] */
			/* NEC Express5800/240 R4400 EISA [NEC-J96A] */
		    ) {
			if (vga_isa_cnattach(&arc_bus_io, &arc_bus_mem) == 0) {
#if NPCKBC_JAZZIO > 0
				pckbc_cnattach(&jazzio_bus, PICA_SYS_KBD,
				    JAZZIO_KBCMDP, PCKBC_KBD_SLOT, 0);
#endif
				return;
			}
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
	    com_console_speed, com_freq, COM_TYPE_NORMAL, com_console_mode);
#endif
}
