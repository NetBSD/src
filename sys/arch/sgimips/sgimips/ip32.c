/*	$NetBSD: ip32.c,v 1.1 2000/06/14 16:02:46 soren Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.netbsd.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 */

/*
 * IP32: O2
 */

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/autoconf.h> 
#include <machine/intr.h>

#define CRIME_NINTR 42

static struct {
        int     (*func)(void *);
        void    *arg;
} icu[CRIME_NINTR];

void *	crime_intr_establish(int, int, int, int (*)(void *), void *);
int	crime_intr(void *);

void *   
crime_intr_establish(irq, type, level, func, arg)
        int irq;
        int type;
        int level;
        int (*func)(void *);
        void *arg;
{
	int i;

	for (i = 0; i <= CRIME_NINTR; i++) {
		if (i == CRIME_NINTR)
			panic("too many IRQs");

		if (icu[i].func != NULL)
			continue;

		icu[i].func = func;
		icu[i].arg = arg;
		break;
	}

	return (void *)-1;
}

int
crime_intr(arg)
	void *arg;
{
	int i;

	for (i = 0; i < CRIME_NINTR; i++) {
		if (icu[i].func == NULL)
			return 0;

#if 1
if (
		(*icu[i].func)(icu[i].arg)
== 1)
return 1;
#else
		(*icu[i].func)(icu[i].arg);
#endif
	}

	return 0;
}
