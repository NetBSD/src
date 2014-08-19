/*	$NetBSD: consinit.c,v 1.11.12.2 2014/08/20 00:03:24 tls Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: consinit.c,v 1.11.12.2 2014/08/20 00:03:24 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <dev/cons.h>
#include <sys/bus.h>
#include <dev/isa/isavar.h>

#include "vga.h"
#if (NVGA > 0)
#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#include <dev/ic/vgareg.h>
#include <dev/ic/vgavar.h>
#endif
#include "vga_ofbus.h"
#if (NVGA_OFBUS > 0)
#include <shark/ofw/vga_ofbusvar.h>
#endif

#include "pckbc.h"
#if (NPCKBC > 0)
#include <dev/isa/isareg.h>
#include <dev/ic/i8042reg.h>
#include <dev/ic/pckbcvar.h>
#include <dev/pckbport/pckbportvar.h>
#endif
#include "pckbd.h" /* for pckbc_machdep_cnattach */

#include "com.h"
#if (NCOM > 0)
#include <sys/termios.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

#include "ofcons.h"
#if (NOFCONS > 0)
cons_decl(ofcons_)
static struct consdev ofcons = cons_init(ofcons_);
#endif

#include "igsfb_ofbus.h"
#include "chipsfb_ofbus.h"
#include <shark/ofw/igsfb_ofbusvar.h>

#if (NCHIPSFB_OFBUS > 0)
extern int chipsfb_ofbus_cnattach(bus_space_tag_t, bus_space_tag_t);
#endif

#if (NCOM > 0)
#ifndef CONADDR
#define CONADDR 0x3f8
#endif
#ifndef CONSPEED
#define CONSPEED TTYDEF_SPEED
#endif
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif
int comcnmode = CONMODE;
#endif /* NCOM */
#ifdef COMCONSOLE
int comconsole = 1;
#else
int comconsole = 0;
#endif

/*
 * consinit:
 * initialize the system console.
 */
void
consinit(void)
{
	static int initted;

	if (initted)
		return;
	initted = 1;

#if (NVGA > 0)
	/* The font built into the VGA ROM is broken: all the characters
	 * above the 127th do not match the standard set expected by the
	 * console.  E.g. boxes drawn using the ACS are incorrect. */
	vga_no_builtinfont = 1;
#endif

	if (!comconsole) {
#if (NVGA > 0) || (NCHIPSFB_OFBUS > 0) || (NIGSFB_OFBUS > 0)
#if (NIGSFB_OFBUS > 0)
		if (!igsfb_ofbus_cnattach(&isa_io_bs_tag, &isa_mem_bs_tag)) {
#if (NPCKBC > 0)
			pckbc_cnattach(&isa_io_bs_tag, IO_KBD, KBCMDP,
			    PCKBC_KBD_SLOT, 0);
#endif /* NPCKBC */
			return;
		}
#endif /* NIGSFB_OFBUS */
#if (NCHIPSFB_OFBUS > 0)
		if (!chipsfb_ofbus_cnattach(&isa_io_bs_tag, &isa_mem_bs_tag)) {
#if (NPCKBC > 0)
			pckbc_cnattach(&isa_io_bs_tag, IO_KBD, KBCMDP,
			    PCKBC_KBD_SLOT, 0);
#endif /* NPCKBC */
			return;
		}
#endif /* NCHIPSFB_OFBUS */
#if (NVGA_OFBUS > 0)
		if (!vga_ofbus_cnattach(&isa_io_bs_tag, &isa_mem_bs_tag)) {
#if (NPCKBC > 0)
			pckbc_cnattach(&isa_io_bs_tag, IO_KBD, KBCMDP,
			    PCKBC_KBD_SLOT, 0);
#endif /* NPCKBC */
			return;
		}
#endif /* NVGA_OFBUS */

#else /* NVGA */
#if (NOFCONS > 0)
		struct consdev *cp = &ofcons;
		ofcons_cnprobe(cp);
		if (cp->cn_pri == CN_INTERNAL) {
			ofcons_cninit(cp);
			cn_tab = cp;
			return;
		}
#endif /* NOFCONS */
#endif /* NVGA */
	}
#if (NCOM > 0)
	if (comcnattach(&isa_io_bs_tag, CONADDR, CONSPEED, COM_FREQ,
	    COM_TYPE_NORMAL, comcnmode))
		panic("can't init serial console");
#endif
}
