/*	$NetBSD: promio.c,v 1.26 1999/01/27 03:12:24 simonb Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
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
 * from: Utah Hdr: cons.c 1.1 90/07/09
 *
 *	@(#)cons.c	8.2 (Berkeley) 1/11/94
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: promio.c,v 1.26 1999/01/27 03:12:24 simonb Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <dev/cons.h>

#include <pmax/stand/libsa/dec_prom.h>
#include <pmax/dev/promiovar.h>
#include <pmax/pmax/pmaxtype.h>


static int  romgetc	__P ((dev_t));
static void romputc	__P ((dev_t, int));
static void rompollc	__P((dev_t, int));

/*
 * Default consdev, for errors or warnings before
 * consinit runs: use the PROM.
 */
struct consdev promcd = {
	(void (*)(struct consdev *))0,		/* probe */
	(void (*)(struct consdev *))0,		/* init */
	(int  (*)(dev_t))     romgetc,		/* getc */
	(void (*)(dev_t, int))romputc,		/* putc */
	(void (*)(dev_t, int))rompollc,		/* pollc */
	makedev (0, 0),
	CN_DEAD,
};

/*
 * Get character from ROM console.
 */
static int
romgetc(dev)
	dev_t dev;
{
	int s = splhigh ();
	int chr;
	chr = (*callv->_getchar)();
	splx (s);
	return chr;
}


/*
 * Print a character on ROM console.
 */
static void
romputc (dev, c)
	dev_t dev;
	register int c;
{
	int s;
	s = splhigh();
	(*callv->_printf)("%c", c);
	splx(s);
}


/*
 * Toggle polling. Not implemented in NetBSD.
 */
static void
rompollc (dev, c)
	dev_t dev;
	register int c;
{
	return;
}



/*
 * Call back to the PROM to find out what devices it is using
 * as console.
 * Encoding is idiosyncratic; see DECstation Owners Guide.
 */
void
prom_findcons(kbdslot, crtslot, prom_using_screen)
	int *kbdslot;
	int *crtslot;
	int *prom_using_screen;
{
	register char *oscon = 0;	/* PROM osconsole string */

	/*
	 * Get and parse the "osconsole" environment variable.
	 */
	*crtslot = *kbdslot = -1;
	oscon = (*callv->_getenv)("osconsole");
	if (oscon && *oscon >= '0' && *oscon <= '9') {
		*kbdslot = *oscon - '0';
		*prom_using_screen = 0;
		while (*++oscon) {
			if (*oscon == ',')
				*prom_using_screen = 1;
			else if (*prom_using_screen &&
			    *oscon >= '0' && *oscon <= '9') {
				*crtslot = *kbdslot;
				*kbdslot = *oscon - '0';
				break;
			}
		}
	}

	/*
	 * compensate for discrepancies in PROM syntax.
	 * XXX use cons_init vector instead?
	*/
	if (systype == DS_PMAX && *kbdslot == 1)
		*prom_using_screen = 1;

	/*
	 * On a 5000/200, The boot program uses the old, pre-rex PROM
	 * entrypoints, so the ROM sets osconsole to '1' like the PMAX.
	 * our parser loses. fix it by hand.
	 */
	if (systype == DS_3MAX && *crtslot == -1 && *kbdslot == 1) {
		/* Try to use pmax onboard framebuffer */
		*prom_using_screen = 1;
		*crtslot = 0;
		*kbdslot = 7;
	}

}
