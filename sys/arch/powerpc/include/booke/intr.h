/*	$NetBSD: intr.h,v 1.1.2.2 2010/04/30 14:39:43 uebayasi Exp $	*/

/*-
 * Copyright (c) 1998, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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

#ifndef _BOOKE_INTR_H_
#define _BOOKE_INTR_H_

/* Interrupt priority `levels'. */
#define	IPL_NONE	0	/* nothing */
#define	IPL_SOFTCLOCK	1	/* software clock interrupt */
#define	IPL_SOFTBIO	2	/* software block i/o interrupt */
#define	IPL_SOFTNET	3	/* software network interrupt */
#define	IPL_SOFTSERIAL	4	/* software serial interrupt */
#define	IPL_VM		5	/* memory allocation */
#define	IPL_SCHED	6	/* clock */
#define	IPL_HIGH	7	/* everything */
#define	NIPL		8

/* Interrupt sharing types. */
#define	IST_NONE	0	/* none */
#define	IST_EDGE	1	/* edge-triggered */
#define	IST_LEVEL	2	/* level-triggered active-low */
#define	IST_LEVEL_LOW	IST_LEVEL
#define	IST_LEVEL_HIGH	3	/* level-triggered active-high */
#define	IST_MSI		4	/* message signaling interrupt (PCI) */
#define	IST_ONCHIP	5	/* on-chip device */
#ifdef __INTR_PRIVATE
#define	IST_MSIGROUP	6	/* openpic msi groups */
#define	IST_TIMER	7	/* openpic timers */
#define	IST_IPI		8	/* openpic ipi */
#define	IST_MI		9	/* openpic message */
#endif

#ifndef _LOCORE

void 	*intr_establish(int, int, int, int (*)(void *), void *);
void 	intr_disestablish(void *);
int	spl0(void);
int 	splraise(int);
void 	splx(int);

typedef int ipl_t;
typedef struct {
	ipl_t _ipl;
} ipl_cookie_t;

#ifdef __INTR_PRIVATE
#include <sys/lwp.h>

struct intrsw {
	void *(*intrsw_establish)(int, int, int, int (*)(void *), void *);
	void (*intrsw_disestablish)(void *);
	void (*intrsw_cpu_init)(struct cpu_info *);
	void (*intrsw_init)(void);
	void (*intrsw_critintr)(struct trapframe *);
	void (*intrsw_decrintr)(struct trapframe *);
	void (*intrsw_extintr)(struct trapframe *);
	void (*intrsw_fitintr)(struct trapframe *);
	void (*intrsw_wdogintr)(struct trapframe *);
	int (*intrsw_splraise)(int);
	int (*intrsw_spl0)(void);
	void (*intrsw_splx)(int);
#ifdef __HAVE_FAST_SOFTINTS
	void (*intrsw_softint_init_md)(lwp_t *, u_int, uintptr_t *);
	void (*intrsw_softint_trigger)(uintptr_t);
#endif
};

extern struct intrsw powerpc_intrsw;
#endif /* __INTR_PRIVATE */

static inline int 
splhigh(void)
{

	return splraise(IPL_HIGH);
}

static inline int 
splsched(void)
{

	return splraise(IPL_SCHED);
}

static inline int 
splvm(void)
{

	return splraise(IPL_VM);
}

static inline int 
splsoftserial(void)
{

	return splraise(IPL_SOFTSERIAL);
}

static inline int 
splsoftnet(void)
{

	return splraise(IPL_SOFTNET);
}

static inline int 
splsoftbio(void)
{

	return splraise(IPL_SOFTBIO);
}

static inline int 
splsoftclock(void)
{

	return splraise(IPL_SOFTCLOCK);
}

static inline int
splraiseipl(ipl_cookie_t icookie)
{

	return splraise(icookie._ipl);
}

static inline ipl_cookie_t
makeiplcookie(ipl_t ipl)
{

	return (ipl_cookie_t){._ipl = ipl};
}

#endif /* !_LOCORE */
#endif /* !_BOOKE_INTR_H_ */
