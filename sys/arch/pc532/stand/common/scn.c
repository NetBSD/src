/*	$NetBSD: scn.c,v 1.1 1997/05/17 13:56:09 matthias Exp $	*/

/*-
 * Copyright (c) 1994 Philip L. Budne.
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
 *	This product includes software developed by Philip L. Budne.
 * 4. The name of Philip L. Budne may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PHILIP BUDNE ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL PHILIP BUDNE BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * scn.c -- scn2681/2692/68881 standalone console driver
 * Phil Budne, May 10, 1994
 *
 */

#ifdef SCNCONSOLE
#include <sys/types.h>
#include <dev/cons.h>

#define DUART	0x28000000

/* registers */
#define SCN_STAT 1
#define SCN_DATA 3

/* status bits */
#define STAT_RXRDY 0x01
#define STAT_TXRDY 0x04

#ifndef SCNCNUNIT
#define SCNCNUNIT 0
#endif

u_char * volatile scncnaddr = (u_char *) DUART + 8 * SCNCNUNIT;

scnprobe(cp)
	struct consdev *cp;
{
	/* the only game in town */
	cp->cn_pri = CN_NORMAL;		/* XXX remote? */
}

scninit(cp)
	struct consdev *cp;
{
	/* leave things the way the PROM set them */
}

scngetchar(cp)
	struct consdev *cp;
{
	if ((scncnaddr[SCN_STAT] & STAT_RXRDY) == 0)
		return(0);
	return(scncnaddr[SCN_DATA]);
}

scnputchar(cp, c)
	struct consdev *cp;
	register int c;
{
	register int timo;
	short stat;

	/* wait a reasonable time for the transmitter to come ready */
	timo = 50000;
	while (((stat = scncnaddr[SCN_STAT]) & STAT_TXRDY) == 0 && --timo)
		;
	scncnaddr[SCN_DATA] = c;
}
#endif
