/*	$NetBSD: c_isa.c,v 1.1 2001/06/13 15:27:17 soda Exp $	*/
/*	$OpenBSD: isabus.c,v 1.15 1998/03/16 09:38:46 pefo Exp $	*/

/*-
 * Copyright (c) 1995 Per Fogelstrom
 * Copyright (c) 1993, 1994 Charles M. Hannum.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz and Don Ahn.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
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
 *	@(#)isa.c	7.2 (Berkeley) 5/12/91
 */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
  Copyright 1988, 1989 by Intel Corporation, Santa Clara, California.

		All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appears in all
copies and that both the copyright notice and this permission notice
appear in supporting documentation, and that the name of Intel
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.

INTEL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL INTEL BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/*
 * for "DESKTECH-ARCStation I" and DESKTECH-TYNE
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/pio.h>
#include <machine/platform.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <arc/isa/mcclock_isavar.h>
#include <arc/isa/timer_isavar.h>
#include <arc/isa/isabrvar.h>

#include "pc.h"
#if NPC_ISA > 0
#include <arc/isa/pccons_isavar.h>
#endif

#include "vga_isa.h"
#if NVGA_ISA > 0
#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#include <dev/isa/vga_isavar.h>
#endif

#include "pckbc.h"
#if NPCKBC > 0
#include <dev/ic/pckbcvar.h>
#endif

#include "com.h"
#if NCOM > 0
#include <sys/termios.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

/*
 * chipset-dependent isa bus configuration
 */

int isabr_dti_intr_status __P((void));

struct isabr_config isabr_dti_conf = {
	isabr_dti_intr_status,
};

int
isabr_dti_intr_status()
{
	int isa_vector;
	char vector;

	isa_outb(IO_ICU1, 0x0f);	/* Poll */
	vector = isa_inb(IO_ICU1);
	if (vector < 0) /* XXX: OpenBSD source had a bug, re-look this */
		return (-1);
	isa_vector = vector & 7;
	if (isa_vector == 2) { 
		isa_outb(IO_ICU2, 0x0f);
		vector = isa_inb(IO_ICU2);
		if (vector > 0) {
			printf("isa: spurious interrupt.\n");
			return (-1);
		}
		isa_vector = (vector & 7) | 8;
	}
	return (isa_vector);
}

/*
 * common configuration for DTI platforms
 */
void
c_isa_init()
{
	/* chipset-dependent mcclock configuration */
	mcclock_isa_conf = 1;

	/* chipset-dependent timer configuration */
	timer_isa_conf = 1;

	/* chipset-dependent isa bus configuration */
	isabr_conf = &isabr_dti_conf;
}

/*
 * console initialization
 */
void
c_isa_cons_init()
{
	if (!com_console) {
#if NVGA_ISA > 0
		if (vga_isa_cnattach(&arc_bus_io, &arc_bus_mem) == 0) {
#if NPCKBC > 0
			pckbc_cnattach(&arc_bus_io, IO_KBD, KBCMDP,
			    PCKBC_KBD_SLOT);
			return;
#endif
		}
#endif
#if NPC_ISA > 0
		if (pccons_isa_cnattach(&arc_bus_io, &arc_bus_mem) == 0)
			return;
#endif
	}

#if NCOM > 0
	if (com_console_address == 0)
		com_console_address = IO_COM1;
	comcnattach(&arc_bus_io, com_console_address,
	    com_console_speed, com_freq, com_console_mode);
#endif
}
