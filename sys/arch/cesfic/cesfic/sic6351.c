/* $NetBSD: sic6351.c,v 1.2 2003/07/15 01:29:20 lukem Exp $ */

/*
 * Copyright (c) 1997, 1999
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
__KERNEL_RCSID(0, "$NetBSD: sic6351.c,v 1.2 2003/07/15 01:29:20 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/pte.h>

void sicinit __P((void*));
int act2icr __P((int));
void sic_enable_int __P((int, int, int, int, int));

static char *sicbase;

void
sicinit(base)
	void *base;
{
	int i;

	sicbase = base;

	for(i = 0x100; i <= 0x1a4; i += 4)
		*(volatile int *)(sicbase + i) = 0;
}

int
act2icr(act)
	int act;
{
	if (act == 17)
		return (0x90); /* ILACC */
	if (act == 19)
		return (0x94); /* ZS */
	if (act == 25)
		return (0xa4); /* RTC */
	if (act == 39)
		return (0x6c); /* ABORT A */
	panic("sic: unknown act %d", act);
}

void
sic_enable_int(nr, type, icod, level, vector)
	int nr, type, icod, level, vector;
{

	if (icod == 0)
		*(volatile int *)(sicbase + 0x300 + nr * 4) = vector;
	*(volatile int *)(sicbase + 0x200 + nr * 4) = icod;
	*(volatile int *)(sicbase + 0x100 + act2icr(nr))
		= 0x80 | (type << 4) | level;
}
