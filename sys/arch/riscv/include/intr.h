/* $NetBSD: intr.h,v 1.1 2014/09/19 17:36:26 matt Exp $ */

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

#ifndef _RISCV_INTR_H_
#define	_RISCV_INTR_H_

#ifdef _KERNEL_OPT
#include "opt_multiprocessor.h"
#endif

/*
 * This is a common <machine/intr.h> for all RISCV platforms.
 */

#define	IPL_NONE	0
#define	IPL_SOFTCLOCK	(IPL_NONE+1)
#define	IPL_SOFTBIO	(IPL_SOFTCLOCK)	/* shares SWINT with softclock */
#define	IPL_SOFTNET	(IPL_SOFTBIO+1)
#define	IPL_SOFTSERIAL	(IPL_SOFTNET)	/* shares SWINT with softnet */
#define	IPL_VM		(IPL_SOFTSERIAL+1)
#define	IPL_SCHED	(IPL_VM+1)
#define	IPL_DDB		(IPL_SCHED+1)
#define	IPL_HIGH	(IPL_DDB+1)

#define	IPL_SAFEPRI	IPL_SOFTSERIAL

#define	_IPL_N		(IPL_HIGH+1)
#define	_IPL_NAMES(pfx)	{ pfx"none", pfx"softclock/bio", pfx"softnet/serial", \
			  pfx"vm", pfx"sched", pfx"ddb", pfx"high" }

#define	IST_UNUSABLE	-1		/* interrupt cannot be used */
#define	IST_NONE	0		/* none (dummy) */
#define	IST_PULSE	1		/* pulsed */
#define	IST_EDGE	2		/* edge-triggered */
#define	IST_LEVEL	3		/* level-triggered */
#define	IST_LEVEL_HIGH	4		/* level triggered, active high */
#define	IST_LEVEL_LOW	5		/* level triggered, active low */

#define	IPI_NOP		0		/* do nothing, interrupt only */
#define	IPI_AST		1		/* force ast */
#define	IPI_SHOOTDOWN	2		/* do a tlb shootdown */
#define	IPI_SYNCICACHE	3		/* sync icache for pages */
#define	IPI_KPREEMPT	4		/* schedule a kernel preemption */
#define	IPI_SUSPEND	5		/* DDB suspend signaling */
#define	IPI_HALT	6		/* halt cpu */
#define	IPI_XCALL	7		/* xcall */
#define	IPI_GENERIC	8		/* generic IPI */
#define	NIPIS		9

#ifdef __INTR_PRIVATE
#if 0
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
	int	(*splsw_splintr)(uint32_t *);
	void	(*splsw_splcheck)(void);
};

struct ipl_sr_map {
	uint32_t sr_bits[_IPL_N];
};
#endif
#else
struct splsw;
#endif /* __INTR_PRIVATE */

typedef int ipl_t;
typedef struct {
        ipl_t _spl;
} ipl_cookie_t;

#ifdef _KERNEL

#if defined(MULTIPROCESSOR) && defined(__HAVE_FAST_SOFTINTS)
#define __HAVE_PREEMPTION	1
#define SOFTINT_KPREEMPT	(SOFTINT_COUNT+0)
#endif

#ifdef __INTR_PRIVATE
#if 0
extern	struct splsw	cpu_splsw;
#endif
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

struct cpu_info;

void	ipi_init(struct cpu_info *);
void	ipi_process(struct cpu_info *, uint64_t);

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
#endif /* _RISCV_INTR_H_ */
