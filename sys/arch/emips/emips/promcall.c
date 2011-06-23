/*	$NetBSD: promcall.c,v 1.2.4.1 2011/06/23 14:19:05 cherry Exp $	*/

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
 * 3. Neither the name of the University nor the names of its contributors
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: promcall.c,v 1.2.4.1 2011/06/23 14:19:05 cherry Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/reboot.h>
#include <sys/systm.h>

#include <dev/cons.h>

#include <mips/cpuregs.h>
#include <emips/stand/common/prom_iface.h>
#include <emips/emips/machdep.h>

static int  romgetc(dev_t);
static void romputc(dev_t, int);
static void *nope(void);
static int  nogetchar(void);
static void noprintf(const char *, ...);
static int  nogetsysid(void);
static void nohalt(int *, int);

/*
 * Callback vector. We keep this fall-back copy jic the bootloader is broken.
 */
static void *nope(void)
{

	return NULL;
}

static int  nogetchar(void)
{

	return -1;
}

static void noprintf(const char *fmt, ...)
{
}

static int  nogetsysid(void)
{

	/* say its an eMIPS, ML board */
	return MAKESYSID(1, 1, XS_ML40x, MIPS_eMIPS);
}

static void nohalt(int *unused, int howto)
{

	while (1);	/* fool gcc */
	/*NOTREACHED*/
}

const struct callback callvec = {
	nope,
	nope,
	nope,
	nope,
	nope,
	nope,
	nope,
	nope,
	nope,
	nogetchar,
	nope,
	nope,
	noprintf,
	nope,
	nope,
	nope,
	nope,
	nope,
	nope,
	nope,
	nope,
	nope,
	nope,
	nope,
	nope,
	nope,
	nope,
	nope,
	nope,
	nope,
	nope,
	nope,
	nogetsysid,
	nope,
	nope,
	nope,
	nope,
	nope,
	nope,
	nohalt
};

const   struct callback *callv = &callvec;

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
	NULL,		/* bell */
	makedev(0, 0),
	CN_DEAD,
};

/*
 * Get character from PROM console.
 */
static int
romgetc(dev_t dev)
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
romputc(dev_t dev, int c)
{
	int s;

	s = splhigh();
	(*callv->_printf)("%c", c);
	splx(s);
}

/*
 * Get 32bit system type:
 *	cputype,		u_int8_t [3]
 *	systype,		u_int8_t [2]
 *	firmware revision,	u_int8_t [1]
 *	hardware revision.	u_int8_t [0]
 */
int
prom_systype(void)
{

	return (*callv->_getsysid)();
}

/*
 * Halt/reboot machine.
 */
void __attribute__((__noreturn__))
prom_halt(int howto)
{

	(*callv->_halt)((int *)0, howto);
	while(1) ;	/* fool gcc */
	/*NOTREACHED*/
}
