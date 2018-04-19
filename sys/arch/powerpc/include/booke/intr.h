/*	$NetBSD: intr.h,v 1.11 2018/04/19 21:50:07 christos Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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
#define	IST_NONE	(NIPL+0) /* none */
#define	IST_EDGE	(NIPL+1) /* edge-triggered */
#define	IST_LEVEL	(NIPL+2) /* level-triggered active-low */
#define	IST_LEVEL_LOW	IST_LEVEL
#define	IST_LEVEL_HIGH	(NIPL+3) /* level-triggered active-high */
#define	IST_PULSE	(NIPL+4) /* pulsed */
#define	IST_MSI		(NIPL+5) /* message signaling interrupt (PCI) */
#define	IST_ONCHIP	(NIPL+6) /* on-chip device */
#ifdef __INTR_PRIVATE
#define	IST_MSIGROUP	(NIPL+7) /* openpic msi groups */
#define	IST_TIMER	(NIPL+8) /* openpic timers */
#define	IST_IPI		(NIPL+9) /* openpic ipi */
#define	IST_MI		(NIPL+10) /* openpic message */
#define	IST_MAX		(NIPL+11)
#endif

#define	IPI_DST_ALL	((cpuid_t)-2)
#define	IPI_DST_NOTME	((cpuid_t)-1)

#define IPI_NOMESG	0x0000
#define IPI_HALT	0x0001
#define IPI_XCALL	0x0002
#define	IPI_KPREEMPT	0x0004
#define IPI_TLB1SYNC	0x0008
#define IPI_GENERIC	0x0010
#define IPI_SUSPEND	0x0020

#define	__HAVE_FAST_SOFTINTS	1
#define	SOFTINT_KPREEMPT	SOFTINT_COUNT

#ifndef _LOCORE

struct cpu_info;

void 	*intr_establish(int, int, int, int (*)(void *), void *);
void 	*intr_establish_xname(int, int, int, int (*)(void *), void *,
	    const char *);
void 	intr_disestablish(void *);
void	intr_cpu_attach(struct cpu_info *);
void	intr_cpu_hatch(struct cpu_info *);
void	intr_init(void);
const char *
	intr_string(int, int, char *, size_t);
const char *
	intr_typename(int);

void	cpu_send_ipi(cpuid_t, uint32_t);

void	spl0(void);
int 	splraise(int);
void 	splx(int);
#ifdef __INTR_NOINLINE
int	splhigh(void);
int	splsched(void);
int	splvm(void);
int	splsoftserial(void);
int	splsoftnet(void);
int	splsoftbio(void);
int	splsoftclock(void);
#endif

typedef int ipl_t;
typedef struct {
	ipl_t _ipl;
} ipl_cookie_t;

#ifdef __INTR_PRIVATE

struct trapframe;

struct intrsw {
	void *(*intrsw_establish)(int, int, int, int (*)(void *), void *,
	    const char *);
	void (*intrsw_disestablish)(void *);
	void (*intrsw_cpu_attach)(struct cpu_info *);
	void (*intrsw_cpu_hatch)(struct cpu_info *);
	void (*intrsw_cpu_send_ipi)(cpuid_t, uint32_t);
	void (*intrsw_init)(void);
	void (*intrsw_critintr)(struct trapframe *);
	void (*intrsw_decrintr)(struct trapframe *);
	void (*intrsw_extintr)(struct trapframe *);
	void (*intrsw_fitintr)(struct trapframe *);
	void (*intrsw_wdogintr)(struct trapframe *);
	int (*intrsw_splraise)(int);
	void (*intrsw_spl0)(void);
	void (*intrsw_splx)(int);
	const char *(*intrsw_string)(int, int, char *, size_t);
	const char *(*intrsw_typename)(int);
#ifdef __HAVE_FAST_SOFTINTS
	void (*intrsw_softint_init_md)(struct lwp *, u_int, uintptr_t *);
	void (*intrsw_softint_trigger)(uintptr_t);
#endif
};

extern const struct intrsw *powerpc_intrsw;
void	softint_fast_dispatch(struct lwp *, int);
#endif /* __INTR_PRIVATE */

#ifndef __INTR_NOINLINE
static __inline int 
splhigh(void)
{

	return splraise(IPL_HIGH);
}

static __inline int 
splsched(void)
{

	return splraise(IPL_SCHED);
}

static __inline int 
splvm(void)
{

	return splraise(IPL_VM);
}

static __inline int 
splsoftserial(void)
{

	return splraise(IPL_SOFTSERIAL);
}

static __inline int 
splsoftnet(void)
{

	return splraise(IPL_SOFTNET);
}

static __inline int 
splsoftbio(void)
{

	return splraise(IPL_SOFTBIO);
}

static __inline int 
splsoftclock(void)
{

	return splraise(IPL_SOFTCLOCK);
}

static __inline int
splraiseipl(ipl_cookie_t icookie)
{

	return splraise(icookie._ipl);
}

static __inline ipl_cookie_t
makeiplcookie(ipl_t ipl)
{

	return (ipl_cookie_t){._ipl = ipl};
}
#endif /* !__INTR_NOINLINE */

#endif /* !_LOCORE */
#endif /* !_BOOKE_INTR_H_ */
