/*	$NetBSD: intr.h,v 1.6.2.4 2001/04/21 17:53:09 bouyer Exp $	*/

/*
 * Copyright (c) 1998 Jonathan Stone.  All rights reserved.
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
 *	This product includes software developed by Jonathan Stone for
 *      the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARC_INTR_H_
#define _ARC_INTR_H_

#define IPL_NONE	0	/* disable only this interrupt */
#define IPL_BIO		1	/* disable block I/O interrupts */
#define IPL_NET		2	/* disable network interrupts */
#define IPL_TTY		3	/* disable terminal interrupts */
#define IPL_IMP		4	/* memory allocation */
#define IPL_CLOCK	5	/* disable clock interrupts */
#define IPL_STATCLOCK	6	/* disable profiling interrupts */
#if 0 /* XXX */
#define IPL_SERIAL	7	/* disable serial hardware interrupts */
#endif
#define IPL_HIGH	8	/* disable all interrupts */
#define NIPL		9

/* Interrupt sharing types. */
#define IST_NONE	0	/* none */
#define IST_PULSE	1	/* pulsed */
#define IST_EDGE	2	/* edge-triggered */
#define IST_LEVEL	3	/* level-triggered */

/* Soft interrupt masks. */
/* XXX - revisit here */
#define SIR_CLOCK	31
#define SIR_NET		30
#define SIR_CLOCKMASK	((1 << SIR_CLOCK))
#define SIR_NETMASK	((1 << SIR_NET) | SIR_CLOCKMASK)
#define SIR_ALLMASK	(SIR_CLOCKMASK | SIR_NETMASK)

#ifdef _KERNEL
#ifndef _LOCORE

#include <mips/cpuregs.h>

extern int _splraise __P((int));
extern int _spllower __P((int));
extern int _splset __P((int));
extern int _splget __P((void));
extern void _splnone __P((void));
extern void _setsoftintr __P((int));
extern void _clrsoftintr __P((int));

#define setsoftclock()	_setsoftintr(MIPS_SOFT_INT_MASK_0)
#define setsoftnet()	_setsoftintr(MIPS_SOFT_INT_MASK_1)
#define clearsoftclock() _clrsoftintr(MIPS_SOFT_INT_MASK_0)
#define clearsoftnet()	_clrsoftintr(MIPS_SOFT_INT_MASK_1)

/*
 * nesting interrupt masks.
 */
#define MIPS_INT_MASK_SPL_SOFT0	MIPS_SOFT_INT_MASK_0
#define MIPS_INT_MASK_SPL_SOFT1	(MIPS_SOFT_INT_MASK_1|MIPS_INT_MASK_SPL_SOFT0)
#define MIPS_INT_MASK_SPL0	(MIPS_INT_MASK_0|MIPS_INT_MASK_SPL_SOFT1)
#define MIPS_INT_MASK_SPL1	(MIPS_INT_MASK_1|MIPS_INT_MASK_SPL0)
#define MIPS_INT_MASK_SPL2	(MIPS_INT_MASK_2|MIPS_INT_MASK_SPL1)
#define MIPS_INT_MASK_SPL3	(MIPS_INT_MASK_3|MIPS_INT_MASK_SPL2)
#define MIPS_INT_MASK_SPL4	(MIPS_INT_MASK_4|MIPS_INT_MASK_SPL3)
#define MIPS_INT_MASK_SPL5	(MIPS_INT_MASK_5|MIPS_INT_MASK_SPL4)
#define MIPS_INT_MASK_SPLHIGH	MIPS_INT_MASK_SPL5

#define spl0()		(void)_spllower(0)
#define splx(s)		(void)_splset(s)
#define splbio()	(_splraise(splvec.splbio))
#define splnet()	(_splraise(splvec.splnet))
#define spltty()	(_splraise(splvec.spltty))
#define splvm()		(_splraise(splvec.splvm))
#define splclock()	(_splraise(splvec.splclock))
#define splstatclock()	(_splraise(splvec.splstatclock))
#define splhigh()	_splraise(MIPS_INT_MASK_SPLHIGH)

#define splsoftclock()	_splraise(MIPS_INT_MASK_SPL_SOFT0)
#define splsoftnet()	_splraise(MIPS_INT_MASK_SPL_SOFT1)
#define spllowersoftclock() _spllower(MIPS_INT_MASK_SPL_SOFT0)

#define	splsched()	splhigh()
#define	spllock()	splhigh()
#define spllpt()	spltty()		/* lpt driver */

struct splvec {
	int	splbio;
	int	splnet;
	int	spltty;
	int	splvm;
	int	splclock;
	int	splstatclock;
};
extern struct splvec splvec;

/*
 * Index into intrcnt[], which is defined in locore
 */
#define SOFTCLOCK_INTR	0
#define SOFTNET_INTR	1
#define FPU_INTR	2
extern u_long intrcnt[];

struct clockframe;
void set_intr __P((int, int(*)(u_int, struct clockframe *), int));

/* XXX - revisit here */
int imask[NIPL];

#endif /* !_LOCORE */
#endif /* _KERNEL */

#endif /* _ARC_INTR_H_ */
