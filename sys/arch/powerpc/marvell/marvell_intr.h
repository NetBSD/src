/*	$NetBSD: marvell_intr.h,v 1.15.14.1 2009/05/13 17:18:15 jym Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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

#ifndef _MVPPPC_INTR_H_
#define _MVPPPC_INTR_H_

#include <powerpc/psl.h>
#include <powerpc/frame.h>

/*
 * Interrupt Priority Levels
 */
#define	IPL_NONE	0	/* nothing */
#define	IPL_SOFTCLOCK	1	/* timeouts */
#define	IPL_SOFTBIO	2	/* block I/O */
#define	IPL_SOFTNET	3	/* protocol stacks */
#define	IPL_SOFTSERIAL	4	/* serial */
#define	IPL_VM		12	/* memory allocation */
#define	IPL_SCHED	14	/* clock */
#define	IPL_HIGH	15	/* everything */
#define	NIPL		16
#define IPL_PRIMASK	0xf
#define IPL_EE		0x10	/* enable external interrupts on splx */

/* Interrupt sharing types. */
#define	IST_NONE	0	/* none */
#define	IST_PULSE	1	/* pulsed */
#define	IST_EDGE	2	/* edge-triggered */
#define	IST_LEVEL	3	/* level-triggered */
#define	IST_SOFT	4	/* software-triggered */
#define	IST_CLOCK	5	/* exclusive for clock */
#define	NIST		6

#if !defined(_LOCORE) && defined(_KERNEL)

#define	CLKF_BASEPRI(frame)	((frame)->pri == IPL_NONE)

/*
 * we support 128 IRQs:
 *	96 (ICU_LEN) hard interrupt IRQs:
 *		- 64 Main Cause IRQs,
 *		- 32 GPP IRQs,
 *	and 32 softint IRQs
 */
#define ICU_LEN		96	/* number of  HW IRQs */
#define IRQ_GPP_BASE	64	/* base of GPP IRQs */
#define IRQ_GPP_SUM	(32+24) /* GPP[7..0] interrupt */	/* XXX */
#define NIRQ		128	/* total # of HW IRQs */

#define IMASK_ICU_LO	0
#define IMASK_ICU_HI	1
#define IMASK_ICU_GPP	2
#define IMASK_SOFTINT	3
#define IMASK_WORDSHIFT 5	/* log2(32) */
#define IMASK_BITMASK	~((~0) << IMASK_WORDSHIFT)

#define IRQ_IS_GPP(irq) ((irq >= IRQ_GPP_BASE) && (irq < ICU_LEN))

/*
 * interrupt mask bit vector
 */
typedef struct {
	u_int32_t bits[4];
} imask_t __attribute__ ((aligned(16)));

static inline void imask_zero(imask_t *);
static inline void imask_zero_v(volatile imask_t *);
static inline void imask_dup_v(imask_t *, const volatile imask_t *);
static inline void imask_and(imask_t *, const imask_t *);
static inline void imask_andnot_v(volatile imask_t *, const imask_t *);
static inline void imask_andnot_icu_vv(volatile imask_t *, const volatile imask_t *);
static inline int imask_empty(const imask_t *);
static inline void imask_orbit(imask_t *, int);
static inline void imask_orbit_v(volatile imask_t *, int);
static inline void imask_clrbit(imask_t *, int);
static inline void imask_clrbit_v(volatile imask_t *, int);
static inline u_int32_t imask_andbit_v(const volatile imask_t *, int);
static inline int imask_test_v(const volatile imask_t *, const imask_t *);

static inline void
imask_zero(imask_t *idp)
{
	idp->bits[IMASK_ICU_LO]  = 0;
	idp->bits[IMASK_ICU_HI]  = 0;
	idp->bits[IMASK_ICU_GPP] = 0;
	idp->bits[IMASK_SOFTINT] = 0;
}

static inline void
imask_zero_v(volatile imask_t *idp)
{
	idp->bits[IMASK_ICU_LO]  = 0;
	idp->bits[IMASK_ICU_HI]  = 0;
	idp->bits[IMASK_ICU_GPP] = 0;
	idp->bits[IMASK_SOFTINT] = 0;
}

static inline void
imask_dup_v(imask_t *idp, const volatile imask_t *isp)
{
	*idp = *isp;
}

static inline void
imask_and(imask_t *idp, const imask_t *isp)
{
	idp->bits[IMASK_ICU_LO]  &= isp->bits[IMASK_ICU_LO]; 
	idp->bits[IMASK_ICU_HI]  &= isp->bits[IMASK_ICU_HI]; 
	idp->bits[IMASK_ICU_GPP] &= isp->bits[IMASK_ICU_GPP]; 
	idp->bits[IMASK_SOFTINT] &= isp->bits[IMASK_SOFTINT];
}

static inline void
imask_andnot_v(volatile imask_t *idp, const imask_t *isp)
{
	idp->bits[IMASK_ICU_LO]  &= ~isp->bits[IMASK_ICU_LO]; 
	idp->bits[IMASK_ICU_HI]  &= ~isp->bits[IMASK_ICU_HI]; 
	idp->bits[IMASK_ICU_GPP] &= ~isp->bits[IMASK_ICU_GPP]; 
	idp->bits[IMASK_SOFTINT] &= ~isp->bits[IMASK_SOFTINT];
}

static inline void
imask_andnot_icu_vv(volatile imask_t *idp, const volatile imask_t *isp)
{
	idp->bits[IMASK_ICU_LO]  &= ~isp->bits[IMASK_ICU_LO]; 
	idp->bits[IMASK_ICU_HI]  &= ~isp->bits[IMASK_ICU_HI]; 
	idp->bits[IMASK_ICU_GPP] &= ~isp->bits[IMASK_ICU_GPP]; 
}

static inline int
imask_empty(const imask_t *isp)
{
	return (! (isp->bits[IMASK_ICU_LO] | isp->bits[IMASK_ICU_HI] | 
		   isp->bits[IMASK_ICU_GPP]| isp->bits[IMASK_SOFTINT]));
}

static inline void
imask_orbit(imask_t *idp, int bitno)
{
	idp->bits[bitno>>IMASK_WORDSHIFT] |= (1 << (bitno&IMASK_BITMASK));
}

static inline void
imask_orbit_v(volatile imask_t *idp, int bitno)
{
	idp->bits[bitno>>IMASK_WORDSHIFT] |= (1 << (bitno&IMASK_BITMASK));
}

static inline void
imask_clrbit(imask_t *idp, int bitno)
{
	idp->bits[bitno>>IMASK_WORDSHIFT] &= ~(1 << (bitno&IMASK_BITMASK));
}

static inline void
imask_clrbit_v(volatile imask_t *idp, int bitno)
{
	idp->bits[bitno>>IMASK_WORDSHIFT] &= ~(1 << (bitno&IMASK_BITMASK));
}

static inline u_int32_t
imask_andbit_v(const volatile imask_t *idp, int bitno)
{
	return idp->bits[bitno>>IMASK_WORDSHIFT] & (1 << (bitno&IMASK_BITMASK));
}

static inline int
imask_test_v(const volatile imask_t *idp, const imask_t *isp)
{
	return ((idp->bits[IMASK_ICU_LO]  & isp->bits[IMASK_ICU_LO]) || 
		(idp->bits[IMASK_ICU_HI]  & isp->bits[IMASK_ICU_HI]) || 
		(idp->bits[IMASK_ICU_GPP] & isp->bits[IMASK_ICU_GPP])|| 
		(idp->bits[IMASK_SOFTINT] & isp->bits[IMASK_SOFTINT]));
}

#ifdef EXT_INTR_STATS
/*
 * ISR timing stats
 */

typedef struct ext_intr_hist {
	u_int64_t tcause;
	u_int64_t tcommit;
	u_int64_t tstart;
	u_int64_t tfin;
} ext_intr_hist_t __attribute__ ((aligned(32)));

typedef struct ext_intr_stat {
        struct ext_intr_hist *histp;
        unsigned int histix;
        u_int64_t cnt;
        u_int64_t sum;
        u_int64_t min;
        u_int64_t max;
        u_int64_t pnd;
        u_int64_t borrowed;
        struct ext_intr_stat *save;
	unsigned long preempted[NIRQ];	/* XXX */
} ext_intr_stat_t  __attribute__ ((aligned(32)));

extern int intr_depth_max;
extern int ext_intr_stats_enb;
extern ext_intr_stat_t ext_intr_stats[];
extern ext_intr_stat_t *ext_intr_statp;

extern void ext_intr_stats_init(void);
extern void ext_intr_stats_cause
(u_int32_t, u_int32_t, u_int32_t, u_int32_t);
extern void ext_intr_stats_pend
(u_int32_t, u_int32_t, u_int32_t, u_int32_t);
extern void ext_intr_stats_commit(imask_t *);
extern void ext_intr_stats_commit_m(imask_t *);
extern void ext_intr_stats_commit_irq(u_int);
extern u_int64_t ext_intr_stats_pre(int);
extern void ext_intr_stats_post(int, u_int64_t);

#define EXT_INTR_STATS_INIT() ext_intr_stats_init()
#define EXT_INTR_STATS_CAUSE(l, h, g, s)  ext_intr_stats_cause(l, h, g, s)
#define EXT_INTR_STATS_COMMIT_M(m) ext_intr_stats_commit_m(m)
#define EXT_INTR_STATS_COMMIT_IRQ(i) ext_intr_stats_commit_irq(i)
#define EXT_INTR_STATS_DECL(t) u_int64_t t
#define EXT_INTR_STATS_PRE(i, t) t = ext_intr_stats_pre(i)
#define EXT_INTR_STATS_POST(i, t) ext_intr_stats_post(i, t)
#define EXT_INTR_STATS_PEND(l, h, g, s) ext_intr_stats_pend(l, h, g, s)
#define EXT_INTR_STATS_PEND_IRQ(i) ext_intr_stats[i].pnd++
#define EXT_INTR_STATS_DEPTH() \
		 intr_depth_max = (intr_depth > intr_depth_max) ? \
			 intr_depth : intr_depth_max

#else /* EXT_INTR_STATS */

#define EXT_INTR_STATS_INIT()
#define EXT_INTR_STATS_CAUSE(l, h, g, s)
#define EXT_INTR_STATS_COMMIT_M(m)
#define EXT_INTR_STATS_COMMIT_IRQ(i)
#define EXT_INTR_STATS_DECL(t)
#define EXT_INTR_STATS_PRE(irq, t)
#define EXT_INTR_STATS_POST(i, t)
#define EXT_INTR_STATS_PEND(l, h, g, s)
#define EXT_INTR_STATS_PEND_IRQ(i)
#define EXT_INTR_STATS_DEPTH()

#endif	/* EXT_INTR_STATS */


#ifdef SPL_STATS
typedef struct spl_hist {
	int level;
	void *addr;
	u_int64_t time;
} spl_hist_t;

extern  void spl_stats_init();
extern  void spl_stats_log();
extern unsigned int spl_stats_enb;

#define SPL_STATS_INIT()	spl_stats_init()
#define SPL_STATS_LOG(ipl, cc)	spl_stats_log((ipl), (cc))

#else

#define SPL_STATS_INIT()
#define SPL_STATS_LOG(ipl, cc)

#endif	/* SPL_STATS */


void intr_dispatch(void);
#ifdef SPL_INLINE
static inline int splraise(int);
static inline int spllower(int);
static inline void splx(int);
#else
extern int splraise(int);
extern int spllower(int);
extern void splx(int);
#endif

extern volatile int tickspending;

extern volatile imask_t ipending;
extern imask_t imask[];

/*
 * inlines for manipulating PSL_EE
 */
static inline void
extintr_restore(register_t omsr)
{
	__asm volatile ("sync; mtmsr %0;" :: "r"(omsr));
}

static inline register_t
extintr_enable(void)
{
	register_t omsr;

	__asm volatile("sync;");
	__asm volatile("mfmsr %0;" : "=r"(omsr));
	__asm volatile("mtmsr %0;" :: "r"(omsr | PSL_EE)); 

	return omsr;
}

static inline register_t
extintr_disable(void)
{
	register_t omsr;

	__asm volatile("mfmsr %0;" : "=r"(omsr));
	__asm volatile("mtmsr %0;" :: "r"(omsr & ~PSL_EE)); 
	__asm volatile("isync;");

	return omsr;
}

#ifdef SPL_INLINE
static inline int
splraise(int ncpl)
{
	int ocpl;
	register_t omsr;

	omsr = extintr_disable();
	ocpl = cpl; 
        if (ncpl > cpl) {
		SPL_STATS_LOG(ncpl, 0);
                cpl = ncpl;
		if ((ncpl == IPL_HIGH) && ((omsr & PSL_EE) != 0)) {
			/* leave external interrupts disabled */
			return (ocpl | IPL_EE);
		}
	}
        extintr_restore(omsr);
        return (ocpl);
}

static inline void
splx(int xcpl)
{
	imask_t *ncplp;
	register_t omsr;
	int ncpl = xcpl & IPL_PRIMASK;

	ncplp = &imask[ncpl];

	omsr = extintr_disable();
	if (ncpl < cpl) {
		cpl = ncpl;
		SPL_STATS_LOG(ncpl, 0);
		if (imask_test_v(&ipending, ncplp))
			intr_dispatch();
	}
	if (xcpl & IPL_EE)
		omsr |= PSL_EE;
	extintr_restore(omsr); 
}

static inline int
spllower(int ncpl)
{
	int ocpl;
	imask_t *ncplp;
	register_t omsr;

	ncpl &= IPL_PRIMASK;
	ncplp = &imask[ncpl];

	omsr = extintr_disable();
	ocpl = cpl;
	cpl = ncpl;
	SPL_STATS_LOG(ncpl, 0);
#ifdef EXT_INTR_STATS 
        ext_intr_statp = 0;
#endif
	if (imask_test_v(&ipending, ncplp))
		intr_dispatch();

	if (ncpl < IPL_HIGH)
		omsr |= PSL_EE;
	extintr_restore(omsr);

	return (ocpl);
}
#endif	/* SPL_INLINE */


/*
 * Soft interrupt IRQs
 * see also intrnames[] in locore.S
 */
#define SIR_BASE	(NIRQ-32)
#define SIXBIT(ipl)	((ipl) - SIR_BASE) /* XXX rennovate later */
#define SIR_SOFTCLOCK	(NIRQ-5)
#define SIR_CLOCK	SIXBIT(SIR_SOFTCLOCK) /* XXX rennovate later */
#define SIR_SOFTNET	(NIRQ-4)
#define SIR_SOFTBIO	(NIRQ-3)
#define SIR_SOFTSERIAL	(NIRQ-2)
#define SIR_HWCLOCK	(NIRQ-1)
#define SPL_CLOCK	SIXBIT(SIR_HWCLOCK) /* XXX rennovate later */
#define SIR_RES		~(SIBIT(SIR_SOFTCLOCK)|\
			  SIBIT(SIR_SOFTNET)|\
			  SIBIT(SIR_SOFTBIO)|\
			  SIBIT(SIR_SOFTSERIAL)|\
			  SIBIT(SIR_HWCLOCK))

struct intrhand;

/*
 * Miscellaneous
 */
#define	spl0()		spllower(IPL_NONE)

typedef int ipl_t;
typedef struct {
	ipl_t _ipl;
} ipl_cookie_t;

static inline ipl_cookie_t
makeiplcookie(ipl_t ipl)
{

	return (ipl_cookie_t){._ipl = ipl};
}

static inline int
splraiseipl(ipl_cookie_t icookie)
{

	return splraise(icookie._ipl);
}

#include <sys/spl.h>

#define SIBIT(ipl)	(1 << ((ipl) - SIR_BASE))

void	*intr_establish(int, int, int, int (*)(void *), void *);
void	intr_disestablish(void *);
void	init_interrupt(void);
const char * intr_typename(int);
const char * intr_string(int);
const struct evcnt * intr_evcnt(int);
void	ext_intr(struct intrframe *);

/* the following are needed to compile until this port is properly
 * converted to ppcoea-rennovation.
 */
void genppc_cpu_configure(void);

void	strayintr(int);

/*
 * defines for indexing intrcnt
 */
#define CNT_IRQ0	0
#define CNT_CLOCK	SIR_HWCLOCK
#define CNT_SOFTCLOCK	SIR_SOFTCLOCK
#define CNT_SOFTNET	SIR_NET
#define CNT_SOFTSERIAL	SIR_SOFTSERIAL
#define CNT_SOFTBIO	SIR_BIO

#endif /* !_LOCORE */

#endif /* _MVPPPC_INTR_H_ */
