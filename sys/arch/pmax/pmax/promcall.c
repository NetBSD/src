/*	$NetBSD: promcall.c,v 1.3 1999/06/08 23:40:19 simonb Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: promcall.c,v 1.3 1999/06/08 23:40:19 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>
#include <dev/cons.h>

#include <pmax/pmax/pmaxtype.h>
#include <pmax/pmax/machdep.h>
#include <machine/dec_prom.h>

static int  romgetc __P((dev_t));
static void romputc __P((dev_t, int));
static int  atoi __P((const char *));

#define DEFAULT_SCSIID	7    /* XXX - this should really live somewhere else */

/*
 * Default consdev, for errors or warnings before
 * consinit runs: use the PROM.
 */
struct consdev promcd = {
	NULL,		/* probe */
	NULL,		/* init */
	romgetc,	/* getc */
	romputc,	/* putc */
	nullcnpollc,	/* pollc */
	makedev(0, 0),
	CN_DEAD,
};

/*
 * Get character from PROM console.
 */
static int
romgetc(dev)
	dev_t dev;
{
	int chr, s;

	s  = splhigh();
	chr = (*callv->_getchar)();
	splx(s);
	return chr;
}

/*
 * Print a character on PROM console.
 */
static void
romputc(dev, c)
	dev_t dev;
	int c;
{
	int s;

	s = splhigh();
	(*callv->_printf)("%c", c);
	splx(s);
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
	oscon = prom_getenv("osconsole");
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

/*
 * Get a prom environment variable.
 */
char *
prom_getenv(name)
	char *name;
{
	return (*callv->_getenv)(name);
}

/*
 * Get 32bit system type of Digital hardware.
 *	cputype,		u_int8_t [3]
 *	systype,		u_int8_t [2]
 *	firmware revision,	u_int8_t [1]
 *	hardware revision.	u_int8_t [0]
 */
int
prom_systype()
{
	char *cp;

	if (callv != &callvec)
		return (*callv->_getsysid)();
	cp = prom_getenv("systype");
	return (cp != NULL) ? atoi(cp) : 0;
}

/*
 * Reset machine by haltbutton.
 */
void
prom_haltbutton()
{
	(*callv->_halt)((int *)0, 0);
}

/*
 * Halt/reboot machine.
 */
volatile void
prom_halt(howto, bootstr)
	int howto;
	char *bootstr;
{
	if (callv != &callvec)
		(*callv->_rex)((howto & RB_HALT) ? 'h' : 'b');
	else {
		volatile void (*f) __P((void));

		f = (howto & RB_HALT)
			? (void *)DEC_PROM_REINIT
			: (void *)DEC_PROM_AUTOBOOT;
		(*f)();
	}

	while(1) ;	/* fool gcc */
	/*NOTREACHED*/
}

/*
 * Get the host SCSI ID from the PROM.
 */
int
prom_scsiid(cnum)
	int cnum;
{
	char scsiid_var[8];	/* strlen("scsiidX") + NULL */
	char *cp;

	snprintf(scsiid_var, 8, "scsiid%d", cnum);
	cp = prom_getenv(scsiid_var);
	return (cp != NULL) ? atoi(cp) : DEFAULT_SCSIID;
}

/*
 * over-engineered atoi(3)
 */
static int
atoi(s)
	const char *s;
{
	int c;
	unsigned base = 10, d;
	int neg = 0, val = 0;

	if (s == 0 || (c = *s++) == 0)
		goto out;

	/* skip spaces if any */
	while (c == ' ' || c == '\t')
		c = *s++;

	/* parse sign, allow more than one (compat) */
	while (c == '-') {
		neg = !neg;
		c = *s++;
	}

	/* parse base specification, if any */
	if (c == '0') {
		c = *s++;
		switch (c) {
		case 'X':
		case 'x':
			base = 16;
			break;
		case 'B':
		case 'b':
			base = 2;
			break;
		default:
			base = 8;
		}
	}

	/* parse number proper */
	for (;;) {
		if (c >= '0' && c <= '9')
			d = c - '0';
		else if (c >= 'a' && c <= 'z')
			d = c - 'a' + 10;
		else if (c >= 'A' && c <= 'Z')
			d = c - 'A' + 10;
		else
			break;
		val *= base;
		val += d;
		c = *s++;
	}
	if (neg)
		val = -val;
out:
	return val;
}
