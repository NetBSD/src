/*	$NetBSD: consinit.c,v 1.2.38.2 2007/05/10 17:23:05 garbled Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
 * All rights reserved.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/cons.h>

#include "vga.h"
#if (NVGA > 0)
#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#include <dev/ic/vgareg.h>
#include <dev/ic/vgavar.h>
#endif

#include "pckbc.h"
#if (NPCKBC > 0)
#include <dev/isa/isareg.h>
#include <dev/ic/i8042reg.h>
#include <dev/ic/pckbcvar.h>
#endif

#include "com.h"
#if (NCOM > 0)
#include <sys/termios.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
void comsoft(void);
#endif

/*
 * consinit
 * Initialize system console.
 */
void
consinit(void)
{
	static int initted = 0;

	if (initted)
		return;
	initted = 1;

#if (NPFB > 0)
	if (!strcmp(CONSOLE, "fb")) {
		pfb_cnattach(CONSOLE_ADDR);
#if (NPCKBC > 0)
		pckbc_cnattach(&genppc_isa_io_space_tag, IO_KBD, KBCMDP,
		    PCKBC_KBD_SLOT);
#endif
		return;
	}
#endif

#if (NVGA > 0)
	if (!strcmp(CONSOLE, "vga")) {
#if (NVGA > 0)
		if (!vga_cnattach(&prep_io_space_tag, &prep_mem_space_tag, -1, 1))
			goto dokbd;
#endif
dokbd:
#if (NPCKBC > 0)
		pckbc_cnattach(&genppc_isa_io_space_tag, IO_KBD, KBCMDP,
		    PCKBC_KBD_SLOT);
#endif
		return;
	}
#endif /* VGA */

#if (NCOM > 0)
 	if (!strcmp(CONSOLE, "com")) {
		if (comcnattach(&genppc_isa_io_space_tag, CONSOLE_ADDR,
			    CONSOLE_SPEED, COM_FREQ, COM_TYPE_NORMAL,
			    (TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8))
			panic("can't init serial console");

		return;
	}
#endif
	panic("invalid console device " CONSOLE);
}
