/*	$NetBSD: marvell_intr.h,v 1.6 2003/09/03 21:33:36 matt Exp $	*/

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

#ifndef _MVPPPC_INTR_H_
#define _MVPPPC_INTR_H_

/*
 * Interrupt Priority Levels
 */
#define	IPL_NONE	0	/* nothing */
#define	IPL_SOFTCLOCK	1	/* timeouts */
#define	IPL_SOFTNET	2	/* protocol stacks */
#define	IPL_BIO		3	/* block I/O */
#define	IPL_NET		4	/* network */
#define IPL_NCP		5	/* network processors */
#define IPL_SOFTI2C	6	/* i2c */
#define	IPL_SOFTSERIAL	7	/* serial */
#define	IPL_TTY		8	/* terminal */
#define IPL_AUDIO       9       /* boom box */
#define IPL_EJECT	10	/* card eject */
#define IPL_GTERR	10	/* GT-64260 errors */
#define	IPL_I2C		11	/* i2c */
#define	IPL_VM		12	/* memory allocation */
#define	IPL_SERIAL	13	/* serial */
#define	IPL_CLOCK	14	/* clock */
#define	IPL_SCHED	14	/* schedular */
#define	IPL_LOCK	14	/* same as high for now */
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

static __inline void imask_zero(imask_t *);
static __inline void imask_zero_v(volatile imask_t *);
static __inline void imask_dup_v(imask_t *, const volatile imask_t *);
static __inline void imask_and(imask_t *, const imask_t *);
static __inline void imask_andnot_v(volatile imask_t *, const imask_t *);
static __inline void imask_andnot_icu_vv(volatile imask_t *, const volatile imask_t *);
static __inline int imask_empty(const imask_t *);
static __inline void imask_orbit(imask_t *, int);
static __inline void imask_orbit_v(volatile imask_t *, int);
static __inline void imask_clrbit(imask_t *, int);
static __inline void imask_clrbit_v(volatile imask_t *, int);
static __inline u_int32_t imask_andbit_v(const volatile imask_t *, int);
static __inline int imask_test_v(const volatile imask_t *, const imask_t *);

static __inline void
imask_zero(imask_t *idp)
{
	idp->bits[IMASK_ICU_LO]  = 0;
	idp->bits[IMASK_ICU_HI]  = 0;
	idp->bits[IMASK_ICU_GPP] = 0;
	idp->bits[IMASK_SOFTINT] = 0;
}

static __inline void
imask_zero_v(volatile imask_t *idp)
{
	idp->bits[IMASK_ICU_LO]  = 0;
	idp->bits[IMASK_ICU_HI]  = 0;
	idp->bits[IMASK_ICU_GPP] = 0;
	idp->bits[IMASK_SOFTINT] = 0;
}

static __inline void
imask_dup_v(imask_t *idp, const volatile imask_t *isp)
{
	*idp = *isp;
}

static __inline void
imask_and(imask_t *idp, const imask_t *isp)
{
	idp->bits[IMASK_ICU_LO]  &= isp->bits[IMASK_ICU_LO]; 
	idp->bits[IMASK_ICU_HI]  &= isp->bits[IMASK_ICU_HI]; 
	idp->bits[IMASK_ICU_GPP] &= isp->bits[IMASK_ICU_GPP]; 
	idp->bits[IMASK_SOFTINT] &= isp->bits[IMASK_SOFTINT];
}

static __inline void
imask_andnot_v(volatile imask_t *idp, const imask_t *isp)
{
	idp->bits[IMASK_ICU_LO]  &= ~isp->bits[IMASK_ICU_LO]; 
	idp->bits[IMASK_ICU_HI]  &= ~isp->bits[IMASK_ICU_HI]; 
	idp->bits[IMASK_ICU_GPP] &= ~isp->bits[IMASK_ICU_GPP]; 
	idp->bits[IMASK_SOFTINT] &= ~isp->bits[IMASK_SOFTINT];
}

static __inline void
imask_andnot_icu_vv(volatile imask_t *idp, const volatile imask_t *isp)
{
	idp->bits[IMASK_ICU_LO]  &= ~isp->bits[IMASK_ICU_LO]; 
	idp->bits[IMASK_ICU_HI]  &= ~isp->bits[IMASK_ICU_HI]; 
	idp->bits[IMASK_ICU_GPP] &= ~isp->bits[IMASK_ICU_GPP]; 
}

static __inline int
imask_empty(const imask_t *isp)
{
	return (! (isp->bits[IMASK_ICU_LO] | isp->bits[IMASK_ICU_HI] | 
		   isp->bits[IMASK_ICU_GPP]| isp->bits[IMASK_SOFTINT]));
}

static __inline void
imask_orbit(imask_t *idp, int bitno)
{
	idp->bits[bitno>>IMASK_WORDSHIFT] |= (1 << (bitno&IMASK_BITMASK));
}

static __inline void
imask_orbit_v(volatile imask_t *idp, int bitno)
{
	idp->bits[bitno>>IMASK_WORDSHIFT] |= (1 << (bitno&IMASK_BITMASK));
}

static __inline void
imask_clrbit(imask_t *idp, int bitno)
{
	idp->bits[bitno>>IMASK_WORDSHIFT] &= ~(1 << (bitno&IMASK_BITMASK));
}

static __inline void
imask_clrbit_v(volatile imask_t *idp, int bitno)
{
	idp->bits[bitno>>IMASK_WORDSHIFT] &= ~(1 << (bitno&IMASK_BITMASK));
}

static __inline u_int32_t
imask_andbit_v(const volatile imask_t *idp, int bitno)
{
	return idp->bits[bitno>>IMASK_WORDSHIFT] & (1 << (bitno&IMASK_BITMASK));
}

static __inline int
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

extern void ext_intr_stats_init __P((void));
extern void ext_intr_stats_cause
	__P((u_int32_t, u_int32_t, u_int32_t, u_int32_t));
extern void ext_intr_stats_pend
	__P((u_int32_t, u_int32_t, u_int32_t, u_int32_t));
extern void ext_intr_stats_commit __P((imask_t *));
extern void ext_intr_stats_commit_m __P((imask_t *));
extern void ext_intr_stats_commit_irq __P((u_int));
extern u_int64_t ext_intr_stats_pre  __P((int));
extern void ext_intr_stats_post __P((int, u_int64_t));

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


void setsoftclock __P((void));
void clearsoftclock __P((void));
int  splsoftclock __P((void));
void setsoftnet   __P((void));
void clearsoftnet __P((void));
int  splsoftnet   __P((void));

void intr_dispatch __P((void));
#ifdef SPL_INLINE
static __inline int splraise __P((int));
static __inline int spllower __P((int));
static __inline void splx __P((int));
#else
extern int splraise __P((int));
extern int spllower __P((int));
extern void splx __P((int));
#endif

extern volatile int tickspending;

extern volatile imask_t ipending;
extern imask_t imask[];

/*
 * inlines for manipulating PSL_EE
 */
static __inline void
extintr_restore(register_t omsr)
{
	__asm __volatile ("sync; mtmsr %0;" :: "r"(omsr));
}

static __inline register_t
extintr_enable(void)
{
	register_t omsr;

	__asm __volatile("sync;");
	__asm __volatile("mfmsr %0;" : "=r"(omsr));
	__asm __volatile("mtmsr %0;" :: "r"(omsr | PSL_EE)); 

	return omsr;
}

static __inline register_t
extintr_disable(void)
{
	register_t omsr;

	__asm __volatile("mfmsr %0;" : "=r"(omsr));
	__asm __volatile("mtmsr %0;" :: "r"(omsr & ~PSL_EE)); 
	__asm __volatile("isync;");

	return omsr;
}

#ifdef SPL_INLINE
static __inline int
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

static __inline void
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

static __inline int
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
#define SIR_SOFTCLOCK	(NIRQ-5)
#define SIR_SOFTNET	(NIRQ-4)
#define SIR_SOFTI2C	(NIRQ-3)
#define SIR_SOFTSERIAL	(NIRQ-2)
#define SIR_HWCLOCK	(NIRQ-1)
#define SIR_RES		~(SIBIT(SIR_SOFTCLOCK)|\
			  SIBIT(SIR_SOFTNET)|\
			  SIBIT(SIR_SOFTI2C)|\
			  SIBIT(SIR_SOFTSERIAL)|\
			  SIBIT(SIR_HWCLOCK))

/*
 * standard hardware interrupt spl's
 */
#define splbio()	splraise(IPL_BIO)
#define splnet()	splraise(IPL_NET)
#define spltty()	splraise(IPL_TTY)
#define	splaudio()	splraise(IPL_AUDIO)
#define splsched()	splraise(IPL_SCHED)
#define splclock()	splraise(IPL_CLOCK)
#define splstatclock()	splclock()
#define	splserial()	splraise(IPL_SERIAL)

#define spllpt()	spltty()

/*
 * Software interrupt spl's
 *
 * NOTE: splsoftclock() is used by hardclock() to lower the priority from
 * clock to softclock before it calls softclock().
 */
#define	spllowersoftclock()	spllower(IPL_SOFTCLOCK)
#define	splsoftclock()		splraise(IPL_SOFTCLOCK)
#define	splsoftnet()		splraise(IPL_SOFTNET)
#define	splsoftserial()		splraise(IPL_SOFTSERIAL)

#define __HAVE_GENERIC_SOFT_INTERRUPTS	/* should be in <machine/types.h> */
void *softintr_establish(int level, void (*fun)(void *), void *arg);
void softintr_disestablish(void *cookie);
void softintr_schedule(void *cookie);


/*
 * Miscellaneous
 */
#define splvm()		splraise(IPL_VM)
#define spllock()	splraise(IPL_LOCK)
#define	splhigh()	splraise(IPL_HIGH)
#define	spl0()		spllower(IPL_NONE)

#define SIBIT(ipl)	(1 << ((ipl) - SIR_BASE))
#if 0
#define	setsoftclock()	softintr(SIBIT(SIR_SOFTCLOCK))
#define	setsoftnet()	softintr(SIBIT(SIR_SOFTNET))
#define	setsoftserial()	softintr(SIBIT(SIR_SOFTSERIAL))
#define	setsofti2c()	softintr(SIBIT(SIR_SOFTI2C))
#endif

extern void *softnet_si;
void	*intr_establish(int, int, int, int (*)(void *), void *);
void	intr_disestablish(void *);
void	init_interrupt(void);
const char * intr_typename(int);
const char * intr_string(int);
const struct evcnt * intr_evcnt(int);
void	ext_intr(struct intrframe *);

#if 0
void	softserial(void);
#endif
void	strayintr(int);

#define	schednetisr(isr)  do {			\
	__asm __volatile(			\
		"1:	lwarx	0,0,%1\n"	\
		"	or	0,0,%0\n"	\
		"	stwcx.	0,0,%1\n"	\
		"	bne-	1b"		\
	   :					\
	   : "r"(1 << (isr)), "b"(&netisr)	\
	   : "cr0", "r0");			\
	softintr_schedule(softnet_si);		\
} while (/*CONSTCOND*/ 0)

/*
 * defines for indexing intrcnt
 */
#define CNT_IRQ0	0
#define CNT_CLOCK	SIR_HWCLOCK
#define CNT_SOFTCLOCK	SIR_SOFTCLOCK
#define CNT_SOFTNET	SIR_NET
#define CNT_SOFTSERIAL	SIR_SOFTSERIAL
#define CNT_SOFTI2C	SIR_I2C

#endif /* !_LOCORE */

#endif /* _MVPPPC_INTR_H_ */
