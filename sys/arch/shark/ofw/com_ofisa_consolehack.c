/*	$NetBSD: com_ofisa_consolehack.c,v 1.4 2003/07/15 03:36:02 lukem Exp $	*/

/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

/*
 *  OFW Attachment for 'com' serial driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: com_ofisa_consolehack.c,v 1.4 2003/07/15 03:36:02 lukem Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/tty.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/ofw/openfirm.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <machine/isa_machdep.h>	/* XXX for space tags */

#include <dev/cons.h>

void comcnprobe(struct consdev *);
void comcninit(struct consdev *);

void
comcnprobe(cp)
        struct consdev *cp;
{

#ifdef  COMCONSOLE
	cp->cn_pri = CN_REMOTE;		/* Force a serial port console */
#else
	cp->cn_pri = CN_NORMAL;
#endif
}

void
comcninit(cp)
        struct consdev *cp;
{

#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif

	if (comcnattach(&isa_io_bs_tag, 0x3f8, 9600, COM_FREQ, COM_TYPE_NORMAL,
	    CONMODE))
		panic("can't init serial console @%x", 0x3f8);
}
