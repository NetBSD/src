/*	$NetBSD: softintr.c,v 1.1 2001/05/22 17:25:16 toshii Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by IWAMOTO Toshihiro.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/malloc.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/irqhandler.h>

extern int ipl_to_spl(int);
int softintr_free(void *);

struct softintr_handler {
	struct softintr_handler *sh_vlink;	/* vertical link */
	struct softintr_handler *sh_hlink;	/* horizontal link */

	int (*sh_fun)(void *);
	void *sh_arg;
	int sh_level;

	int sh_pending;
};

struct softintr_handler *softintrs;
struct softintr_handler *softintr_pending;

void *
softintr_establish(int level, int (*fun)(void *), void *arg)
{
	struct softintr_handler *sh;

	sh = malloc(sizeof(*sh), M_DEVBUF, M_NOWAIT);
	if (sh == NULL)
		return(NULL);

	sh->sh_fun = fun;
	sh->sh_level = ipl_to_spl(level);
	sh->sh_arg = arg;
	sh->sh_pending = 0;

	return sh;
}

void
softintr_disestablish(void *cookie)
{
	struct softintr_handler *sh = cookie;
	u_int saved_cpsr;

	saved_cpsr = SetCPSR(I32_bit, I32_bit);
	if (sh->sh_pending) {
		sh->sh_fun = softintr_free;
		sh->sh_arg = sh;
		SetCPSR(I32_bit, I32_bit & saved_cpsr);
	} else {
		SetCPSR(I32_bit, I32_bit & saved_cpsr);
		free(sh, M_DEVBUF);
	}
}

int
softintr_free(void *arg)
{

	free(arg, M_DEVBUF);
	return 0;
}

void
softintr_schedule(void *cookie)
{
	struct softintr_handler **p, *sh = cookie;
	register int pending;
	u_int saved_cpsr;

	pending = 1;
	asm("swp %0, %0, [%1]" : "+r" (pending) : "r" (&sh->sh_pending));

	if (pending)
		return;

	sh->sh_vlink = NULL;
	sh->sh_hlink = NULL;

	saved_cpsr = SetCPSR(I32_bit, I32_bit);
	p = &softintr_pending;

	for (;; p = &(*p)->sh_vlink) {
		if (*p == NULL) {
			*p = sh;
			SetCPSR(I32_bit, I32_bit & saved_cpsr);
			return;
		}
		if ((*p)->sh_level <= sh->sh_level)
			break;
	}

	if ((*p)->sh_level == sh->sh_level) {
		sh->sh_hlink = *p;
		sh->sh_vlink = (*p)->sh_vlink;
		(*p)->sh_vlink = NULL;
		*p = sh;
		SetCPSR(I32_bit, I32_bit & saved_cpsr);
		return;
	}

	sh->sh_vlink = *p;
	*p = sh;
	SetCPSR(I32_bit, I32_bit & saved_cpsr);
	return;
}

void
softintr_dispatch(int s)
{
	struct softintr_handler *sh, *sh1;
	u_int saved_cpsr;

	while (1) {
		/* Protect list operation from interrupts */
		saved_cpsr = SetCPSR(I32_bit, I32_bit);

		if (softintr_pending == NULL ||
		    softintr_pending->sh_level <= s) {
			SetCPSR(I32_bit, I32_bit & saved_cpsr);
			return;
		}
		sh = softintr_pending;
		softintr_pending = softintr_pending->sh_vlink;
		SetCPSR(I32_bit, I32_bit & saved_cpsr);

		s = raisespl(sh->sh_level);
		while (1) {
			/* The order is important */
			sh1 = sh->sh_hlink;
			sh->sh_pending = 0;

			(sh->sh_fun)(sh->sh_arg);
			if (sh1 == NULL)
				break;
			sh = sh1;
		}
		splx(s);
	}
}
