/*	$NetBSD: intr.h,v 1.9 2006/10/05 14:24:10 tsutsui Exp $	*/

/*
 * Copyright (c) 2001 Matt Fredette.
 * Copyright (c) 1998 Matt Thomas.
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
 * 3. The name of the company nor the names of the authors may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SUN68K_INTR_H_
#define _SUN68K_INTR_H_

#include <sys/queue.h>
#include <m68k/psl.h>

/*
 * Interrupt levels.  Right now these correspond to real
 * hardware levels, but I don't think anything counts on
 * that (yet?).
 */
#define _IPL_SOFT_LEVEL1	1
#define _IPL_SOFT_LEVEL2	2
#define _IPL_SOFT_LEVEL3	3
#define _IPL_SOFT_LEVEL_MIN	1
#define _IPL_SOFT_LEVEL_MAX	3

#define	IPL_NONE	0
#define	IPL_SOFT_LEVEL1	(PSL_S|PSL_IPL1)
#define	IPL_SOFT_LEVEL2	(PSL_S|PSL_IPL2)
#define	IPL_SOFT_LEVEL3	(PSL_S|PSL_IPL3)
#define	IPL_SOFTCLOCK	IPL_SOFT_LEVEL1
#define	IPL_SOFTNET	IPL_SOFT_LEVEL1
#define	IPL_BIO		(PSL_S|PSL_IPL2)
#define	IPL_NET		(PSL_S|PSL_IPL3)
#define	IPL_SOFTSERIAL	IPL_SOFT_LEVEL3
#define	IPL_TTY		(PSL_S|PSL_IPL4)
#define	IPL_VM		(PSL_S|PSL_IPL4)
/* Intersil or Am9513 clock hardware interrupts (hard-wired at 5) */
#define	IPL_CLOCK	(PSL_S|PSL_IPL5)
#define	IPL_STATCLOCK	IPL_CLOCK
#define	IPL_SCHED	(PSL_S|PSL_IPL7)
#define	IPL_HIGH	(PSL_S|PSL_IPL7)
#define	IPL_LOCK	(PSL_S|PSL_IPL7)
#define	IPL_SERIAL	(PSL_S|PSL_IPL6)

#ifdef _KERNEL
LIST_HEAD(sh_head, softintr_handler);

struct softintr_head {
	int shd_ipl;
	struct sh_head shd_intrs;
};

struct softintr_handler {
	struct softintr_head *sh_head;
	LIST_ENTRY(softintr_handler) sh_link;
	void (*sh_func)(void *);
	void *sh_arg;
	int sh_pending;
};

void softintr_init(void);
void *softintr_establish(int, void (*)(void *), void *);
void softintr_disestablish(void *);

/* These control the software interrupt register. */
void isr_soft_request(int);
void isr_soft_clear(int);

static __inline void
softintr_schedule(void *arg)
{
	struct softintr_handler * const sh = arg;
	if (sh->sh_pending == 0) {
		sh->sh_pending = 1;
		isr_soft_request(sh->sh_head->shd_ipl);
	}
}

extern void *softnet_cookie;
#define setsoftnet()	softintr_schedule(softnet_cookie)

/* These connect interrupt handlers. */
typedef int (*isr_func_t)(void *);
void isr_add_autovect(isr_func_t, void *, int);
void isr_add_vectored(isr_func_t, void *, int, int);
void isr_add_custom(int, void *);

/*
 * Define inline functions for PSL manipulation.
 * These are as close to macros as one can get.
 * When not optimizing gcc will call the locore.s
 * functions by the same names, so breakpoints on
 * these functions will work normally, etc.
 * (See the GCC extensions info document.)
 */

static __inline int _getsr(void);

/* Get current sr value. */
static __inline int
_getsr(void)
{
	int rv;

	__asm volatile ("clrl %0; movew %%sr,%0" : "=&d" (rv));
	return (rv);
}

/*
 * The rest of this is sun68k specific, because other ports may
 * need to do special things in spl0() (i.e. simulate SIR).
 * Suns have a REAL interrupt register, so spl0() and splx(s)
 * have no need to check for any simulated interrupts, etc.
 */

#define spl0()  _spl0()		/* we have real software interrupts */
#define splx(x)	_spl(x)

/* IPL used by soft interrupts: netintr(), softclock() */
#define	spllowersoftclock() spl1()

/* Zilog Serial hardware interrupts (hard-wired at 6) */
#define splzs()		spl6()

/* This returns true iff the spl given is spl0. */
#define	is_spl0(s)	(((s) & PSL_IPL7) == 0)

#define	splraiseipl(x)	_splraise(x)

#include <sys/spl.h>

#endif	/* _KERNEL */

#endif	/* _SUN68K_INTR_H */
