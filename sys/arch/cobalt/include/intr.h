/*	$NetBSD: intr.h,v 1.14 2003/09/12 15:03:24 tsutsui Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef	_COBALT_INTR_H_
#define	_COBALT_INTR_H_

#define	IPL_NONE	0	/* Disable only this interrupt. */
#define	IPL_BIO		1	/* Disable block I/O interrupts. */
#define	IPL_NET		2	/* Disable network interrupts. */
#define	IPL_TTY		3	/* Disable terminal interrupts. */
#define	IPL_VM		4	/* Memory allocation */
#define	IPL_CLOCK	5	/* Disable clock interrupts. */
#define	IPL_STATCLOCK	6	/* Disable profiling interrupts. */
#define	IPL_HIGH	7	/* Disable all interrupts. */
#define NIPL		8

/* Interrupt sharing types. */
#define IST_NONE	0	/* none */
#define IST_PULSE	1	/* pulsed */
#define IST_EDGE	2	/* edge-triggered */
#define IST_LEVEL	3	/* level-triggered */

/* Soft interrupt masks. */
#define SIR_CLOCK	31
#define SIR_NET		30
#define SIR_CLOCKMASK	((1 << SIR_CLOCK))
#define SIR_NETMASK	((1 << SIR_NET) | SIR_CLOCKMASK)
#define SIR_ALLMASK	(SIR_CLOCKMASK | SIR_NETMASK)

#ifdef _KERNEL
#ifndef _LOCORE

#include <mips/cpuregs.h>

extern int		_splraise(int);
extern int		_spllower(int);
extern int		_splset(int);
extern int		_splget(void);
extern void		_splnone(void);
extern void		_setsoftintr(int);
extern void		_clrsoftintr(int);

#define setsoftclock()	_setsoftintr(MIPS_SOFT_INT_MASK_0)
#define setsoftnet()	_setsoftintr(MIPS_SOFT_INT_MASK_1)
#define clearsoftclock() _clrsoftintr(MIPS_SOFT_INT_MASK_0)
#define clearsoftnet()	_clrsoftintr(MIPS_SOFT_INT_MASK_1)

#define splhigh()       _splraise(MIPS_INT_MASK)
#define spl0()          (void)_spllower(0)
#define splx(s)         (void)_splset(s)
#define SPLSOFT		MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1
#define SPLBIO		SPLSOFT | MIPS_INT_MASK_4
#define SPLNET		SPLBIO | MIPS_INT_MASK_1 | MIPS_INT_MASK_2
#define SPLTTY		SPLNET | MIPS_INT_MASK_3
#define SPLCLOCK	SPLTTY | MIPS_INT_MASK_0 | MIPS_INT_MASK_5
#define splbio()        _splraise(SPLBIO)
#define splnet()        _splraise(SPLNET)
#define spltty()        _splraise(SPLTTY)
#define splclock()      _splraise(SPLCLOCK)
#define splvm()		splclock()
#define splstatclock()  splclock()
#define splsoftclock()	_splraise(MIPS_SOFT_INT_MASK_0)
#define splsoftnet()	_splraise(MIPS_SOFT_INT_MASK_0|MIPS_SOFT_INT_MASK_1)
#define spllowersoftclock() _spllower(MIPS_SOFT_INT_MASK_0)

#define	splsched()	splhigh()
#define	spllock()	splhigh()

extern unsigned int	intrcnt[];
#define SOFTCLOCK_INTR	0
#define SOFTNET_INTR	1

#endif /* !_LOCORE */
#endif /* _LOCORE */

#endif	/* !_COBALT_INTR_H_ */
