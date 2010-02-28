/* $NetBSD: intr.h,v 1.3.96.6 2010/02/28 03:26:25 matt Exp $ */
/*-
 * Copyright (c) 2009, 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com>.
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

#ifndef _MIPS_INTR_H_
#define	_MIPS_INTR_H_

/*
 * This is a common <machine/intr.h> for all MIPS platforms.
 */

#define	IPL_NONE	0
#define	IPL_SOFTCLOCK	(IPL_NONE+1)
#define	IPL_SOFTBIO	(IPL_SOFTCLOCK)	/* shares SWINT with softclock */
#define	IPL_SOFTNET	(IPL_SOFTBIO+1)
#define	IPL_SOFTSERIAL	(IPL_SOFTNET)	/* shares SWINT with softnet */
#define	IPL_VM		(IPL_SOFTSERIAL+1)
#define	IPL_SCHED	(IPL_VM+1)
#define	IPL_HIGH	(IPL_SCHED)

#define	_IPL_N		(IPL_HIGH+1)

#define	IST_UNUSABLE	-1		/* interrupt cannot be used */
#define	IST_NONE	0		/* none (dummy) */
#define	IST_PULSE	1		/* pulsed */
#define	IST_EDGE	2		/* edge-triggered */
#define	IST_LEVEL	3		/* level-triggered */
#define	IST_LEVEL_HIGH	4		/* level triggered, active high */
#define	IST_LEVEL_LOW	5		/* level triggered, active low */

#define	IPI_NOP		__BIT(0)	/* do nothing, interrupt only */
#define	IPI_SHOOTDOWN	__BIT(1)	/* do a tlb shootdown */
#define	IPI_FPSYNC	__BIT(2)	/* save current fp registers */
#define	IPI_FPDISCARD	__BIT(3)	/* discard current fp registers */
#define	IPI_ISYNC	__BIT(4)	/* sync icache for pages */
#define	IPI_KPREEMPT	__BIT(5)	/* schedule a kernel preemption */

#ifdef __INTR_PRIVATE
struct splsw {
	int	(*splsw_splhigh)(void);
	int	(*splsw_splsched)(void);
	int	(*splsw_splvm)(void);
	int	(*splsw_splsoftserial)(void);
	int	(*splsw_splsoftnet)(void);
	int	(*splsw_splsoftbio)(void);
	int	(*splsw_splsoftclock)(void);
	int	(*splsw_splraise)(int);
	void	(*splsw_spl0)(void);
	void	(*splsw_splx)(int);
	int	(*splsw_splhigh_noprof)(void);
	void	(*splsw_splx_noprof)(int);
	void	(*splsw__setsoftintr)(uint32_t);
	void	(*splsw__clrsoftintr)(uint32_t);
	int	(*splsw_splintr)(uint32_t *);
	void	(*splsw_splcheck)(void);
};

struct ipl_sr_map {
	uint32_t sr_bits[_IPL_N];
};
#endif /* __INTR_PRIVATE */

typedef int ipl_t;
typedef struct {
        ipl_t _spl;
} ipl_cookie_t;

#ifdef _KERNEL
#ifdef MULTIPROCESSOR
#define __HAVE_PREEMPTION
#define SOFTINT_KPREEMPT	(SOFTINT_COUNT+0)
#endif

#ifdef __INTR_PRIVATE
extern	struct splsw	mips_splsw;
extern	struct ipl_sr_map ipl_sr_map;
#endif /* __INTR_PRIVATE */

int	splhigh(void);
int	splhigh_noprof(void);
int	splsched(void);
int	splvm(void);
int	splsoftserial(void);
int	splsoftnet(void);
int	splsoftbio(void);
int	splsoftclock(void);
int	splraise(int);
void	splx(int);
void	splx_noprof(int);
void	spl0(void);
int	splintr(uint32_t *);
void	_setsoftintr(uint32_t);
void	_clrsoftintr(uint32_t);

/*
 * These make no sense *NOT* to be inlined.
 */
static inline ipl_cookie_t
makeiplcookie(ipl_t s)
{
	return (ipl_cookie_t){._spl = s};
}

static inline int
splraiseipl(ipl_cookie_t icookie)
{
	return splraise(icookie._spl);
}

#endif /* _KERNEL */
#endif /* _MIPS_INTR_H_ */
