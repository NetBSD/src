/*	$NetBSD: intr.h,v 1.9.2.2 2006/05/24 10:57:20 yamt Exp $	*/
/*	NetBSD intr.h,v 1.15 2004/10/31 10:39:34 yamt Exp	*/

/*-
 * Copyright (c) 1998, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum, and by Jason R. Thorpe.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#ifndef _XEN_INTR_H_
#define	_XEN_INTR_H_

#include <machine/intrdefs.h>

#ifndef _LOCORE
#include <machine/cpu.h>
#include <machine/pic.h>

#include "opt_xen.h"

/*
 * Struct describing an event channel. 
 */

struct evtsource {
	int ev_maxlevel;		/* max. IPL for this source */
	u_int32_t ev_imask;		/* interrupt mask */
	struct intrhand *ev_handlers;	/* handler chain */
	struct evcnt ev_evcnt;		/* interrupt counter */
	char ev_evname[32];		/* event counter name */
};

/*
 * Structure describing an interrupt level. struct cpu_info has an array of
 * IPL_MAX of theses. The index in the array is equal to the stub number of
 * the stubcode as present in vector.s
 */

struct intrstub {
#if 0
	void *ist_entry;
#endif
	void *ist_recurse; 
	void *ist_resume;
};

#ifdef XEN3
/* for x86 compatibility */
extern struct intrstub i8259_stubs[];
#endif

struct iplsource {
	struct intrhand *ipl_handlers;   /* handler chain */
	void *ipl_recurse;               /* entry for spllower */
	void *ipl_resume;                /* entry for doreti */
	u_int32_t ipl_evt_mask1;	/* pending events for this IPL */
	u_int32_t ipl_evt_mask2[NR_EVENT_CHANNELS];
};



/*
 * Interrupt handler chains. These are linked in both the evtsource and
 * the iplsource.
 * The handler is called with its (single) argument.
 */

struct intrhand {
	int	(*ih_fun)(void *);
	void	*ih_arg;
	int	ih_level;
	struct	intrhand *ih_ipl_next;
	struct	intrhand *ih_evt_next;
	struct cpu_info *ih_cpu;
};


extern struct intrstub xenev_stubs[];

#define IUNMASK(ci,level) (ci)->ci_iunmask[(level)]

extern void Xspllower(int);

static __inline int splraise(int);
static __inline void spllower(int);
static __inline void softintr(int);

/*
 * Add a mask to cpl, and return the old value of cpl.
 */
static __inline int
splraise(int nlevel)
{
	int olevel;
	struct cpu_info *ci = curcpu();

	olevel = ci->ci_ilevel;
	if (nlevel > olevel)
		ci->ci_ilevel = nlevel;
	__insn_barrier();
	return (olevel);
}

/*
 * Restore a value to cpl (unmasking interrupts).  If any unmasked
 * interrupts are pending, call Xspllower() to process them.
 */
static __inline void
spllower(int nlevel)
{
	struct cpu_info *ci = curcpu();
	u_int32_t imask;
	u_long psl;

	__insn_barrier();

	imask = IUNMASK(ci, nlevel);
	psl = read_psl();
	disable_intr();
	if (ci->ci_ipending & imask) {
		Xspllower(nlevel);
		/* Xspllower does enable_intr() */
	} else {
		ci->ci_ilevel = nlevel;
		write_psl(psl);
	}
}

#define SPL_ASSERT_BELOW(x) KDASSERT(curcpu()->ci_ilevel < (x))

/*
 * Software interrupt masks
 *
 * NOTE: spllowersoftclock() is used by hardclock() to lower the priority from
 * clock to softclock before it calls softclock().
 */
#define	spllowersoftclock() spllower(IPL_SOFTCLOCK)

#define splsoftxenevt()	splraise(IPL_SOFTXENEVT)

/*
 * Miscellaneous
 */
#define	spl0()		spllower(IPL_NONE)
#define splraiseipl(x) 	splraise(x)
#define	splx(x)		spllower(x)

#include <sys/spl.h>

/*
 * Software interrupt registration
 *
 * We hand-code this to ensure that it's atomic.
 *
 * XXX always scheduled on the current CPU.
 */
static __inline void
softintr(int sir)
{
	struct cpu_info *ci = curcpu();

	__asm volatile("lock ; orl %1, %0" :
	    "=m"(ci->ci_ipending) : "ir" (1 << sir));
}

/*
 * XXX
 */
#define	setsoftnet()	softintr(SIR_NET)

/*
 * Stub declarations.
 */

extern void Xsoftclock(void);
extern void Xsoftnet(void);
extern void Xsoftserial(void);
extern void Xsoftxenevt(void);

struct cpu_info;

extern char idt_allocmap[];

struct pcibus_attach_args;

void intr_default_setup(void);
int x86_nmi(void);
void intr_calculatemasks(struct evtsource *);

void *intr_establish(int, struct pic *, int, int, int, int (*)(void *), void *);
void intr_disestablish(struct intrhand *);
const char *intr_string(int);
void cpu_intr_init(struct cpu_info *);
#ifdef INTRDEBUG
void intr_printconfig(void);
#endif


#endif /* !_LOCORE */

/*
 * Generic software interrupt support.
 */

#define	X86_SOFTINTR_SOFTCLOCK		0
#define	X86_SOFTINTR_SOFTNET		1
#define	X86_SOFTINTR_SOFTSERIAL		2
#define	X86_NSOFTINTR			3

#ifndef _LOCORE
#include <sys/queue.h>

struct x86_soft_intrhand {
	TAILQ_ENTRY(x86_soft_intrhand)
		sih_q;
	struct x86_soft_intr *sih_intrhead;
	void	(*sih_fn)(void *);
	void	*sih_arg;
	int	sih_pending;
};

struct x86_soft_intr {
	TAILQ_HEAD(, x86_soft_intrhand)
		softintr_q;
	int softintr_ssir;
	struct simplelock softintr_slock;
};

#define	x86_softintr_lock(si, s)					\
do {									\
	(s) = splhigh();						\
	simple_lock(&si->softintr_slock);				\
} while (/*CONSTCOND*/ 0)

#define	x86_softintr_unlock(si, s)					\
do {									\
	simple_unlock(&si->softintr_slock);				\
	splx((s));							\
} while (/*CONSTCOND*/ 0)

void	*softintr_establish(int, void (*)(void *), void *);
void	softintr_disestablish(void *);
void	softintr_init(void);
void	softintr_dispatch(int);

#define	softintr_schedule(arg)						\
do {									\
	struct x86_soft_intrhand *__sih = (arg);			\
	struct x86_soft_intr *__si = __sih->sih_intrhead;		\
	int __s;							\
									\
	x86_softintr_lock(__si, __s);					\
	if (__sih->sih_pending == 0) {					\
		TAILQ_INSERT_TAIL(&__si->softintr_q, __sih, sih_q);	\
		__sih->sih_pending = 1;					\
		softintr(__si->softintr_ssir);				\
	}								\
	x86_softintr_unlock(__si, __s);					\
} while (/*CONSTCOND*/ 0)
#endif /* _LOCORE */

#endif /* _XEN_INTR_H_ */
