/* $Id: intr.h,v 1.5.2.2 1999/01/22 04:13:50 nisimura Exp $ */
/*	$NetBSD: intr.h,v 1.5.2.2 1999/01/22 04:13:50 nisimura Exp $	*/

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

#ifndef _PMAX_INTR_H_
#define _PMAX_INTR_H_

#define	IPL_NONE	1	/* disable only this interrupt */
#define	IPL_BIO		2	/* disable block I/O interrupts */
#define	IPL_NET		3	/* disable network interrupts */
#define	IPL_TTY		4	/* disable terminal interrupts */
#define	IPL_CLOCK	5	/* disable clock interrupts */
#define	IPL_IMP		6	/* disable memory allocation */
#define	IPL_SERIAL	7	/* disable serial hardware interrupts */
#define	IPL_DMA		8	/* disable DMA reload interrupts */
#define	IPL_HIGH	9	/* disable all interrupts */

#ifdef _KERNEL
#ifndef _LOCORE

#include <mips/cpuregs.h>

extern int _splraise __P((int));
extern int _spllower __P((int)); 
extern int _splset __P((int));
extern int _splget __P((void));
extern void _setsoftintr __P((int));
extern void _clrsoftintr __P((int));

#define setsoftclock()  _setsoftintr(MIPS_SOFT_INT_MASK_0)
#define setsoftnet()    _setsoftintr(MIPS_SOFT_INT_MASK_1)
#define clearsoftclock() _clrsoftintr(MIPS_SOFT_INT_MASK_0)
#define clearsoftnet()   _clrsoftintr(MIPS_SOFT_INT_MASK_1) 

struct splclosure {
	int (*func) __P((int));
	int arg;
};

struct splsw {
	struct splclosure lower;
	struct splclosure bio;
	struct splclosure net;
	struct splclosure tty;
	struct splclosure imp;
	struct splclosure clock;
	struct splclosure splx;
};
extern struct splsw *__spl;

struct splvec {
	int	splbio;
	int	splnet;
	int	spltty;
	int	splimp;
	int	splclock;
	int	splstatclock;
};
extern struct splvec splvec;

#ifdef NEWSPL

#define	splhigh()	_splraise(MIPS_INT_MASK)
#define	spl0()		(void)(*__spl->lower.func)(0)
#define	splx(lvl)	(void)(*__spl->splx.func)(lvl)
#define	splbio()	(*__spl->bio.func)(__spl->bio.arg)
#define	splnet()	(*__spl->net.func)(__spl->net.arg)
#define	spltty()	(*__spl->tty.func)(__spl->tty.arg)
#define	splimp()	(*__spl->imp.func)(__spl->imp.arg)
#define	splclock()	(*__spl->clock.func)(__spl->clock.arg)
#define	splstatclock()	(*__spl->clock.func)(__spl->clock.arg)
#define	splsoftclock()	(*__spl->lower.func)(MIPS_SOFT_INT_MASK_0)
#define	splsoftnet()	_splraise(MIPS_SOFT_INT_MASK_1)

#else

#define	splhigh()       _splraise(MIPS_INT_MASK)
#define	spl0()          (void)_spllower(0)
#define	splx(s)         (void)_splset(s)
#define	splbio()	(_splraise(splvec.splbio))
#define	splnet()	(_splraise(splvec.splnet))
#define	spltty()	(_splraise(splvec.spltty))
#define	splimp()	(_splraise(splvec.splimp))
#define	splpmap()	(_splraise(splvec.splimp))
#define	splclock()	(_splraise(splvec.splclock))
#define	splstatclock()	(_splraise(splvec.splstatclock))
#define	splsoftclock()  _spllower(MIPS_SOFT_INT_MASK_0)
#define	splsoftnet()    _splraise(MIPS_SOFT_INT_MASK_1) 

#endif

#define	MIPS_SPLHIGH (MIPS_INT_MASK)
#define	MIPS_SPL0 (MIPS_INT_MASK_0|MIPS_SOFT_INT_MASK_0|MIPS_SOFT_INT_MASK_1)
#define	MIPS_SPL1 (MIPS_INT_MASK_1|MIPS_SOFT_INT_MASK_0|MIPS_SOFT_INT_MASK_1)
#define	MIPS_SPL3 (MIPS_INT_MASK_3|MIPS_SOFT_INT_MASK_0|MIPS_SOFT_INT_MASK_1)
#define	MIPS_SPL_0_1	 (MIPS_INT_MASK_1|MIPS_SPL0)
#define	MIPS_SPL_0_1_2   (MIPS_INT_MASK_2|MIPS_SPL_0_1)
#define	MIPS_SPL_0_1_3   (MIPS_INT_MASK_3|MIPS_SPL_0_1)
#define	MIPS_SPL_0_1_2_3 (MIPS_INT_MASK_3|MIPS_SPL_0_1_2)

extern unsigned intrcnt[];

#define	SOFTCLOCK_INTR	0
#define	SOFTNET_INTR	1
#define	SERIAL0_INTR	2
#define	SERIAL1_INTR	3
#define	SERIAL2_INTR	4
#define	LANCE_INTR	5
#define	SCSI_INTR	6
#define	ERROR_INTR	7
#define	HARDCLOCK	8
#define	FPU_INTR	9
#define	SLOT0_INTR	10
#define	SLOT1_INTR	11
#define	SLOT2_INTR	12
#define	DTOP_INTR	13
#define	ISDN_INTR	14
#define	FLOPPY_INTR	15
#define	STRAY_INTR	16

struct intrhand {
	int (*ih_func) __P((void *));
	void *ih_arg;
};
extern struct intrhand intrtab[];

#define	SYS_DEV_SCSI	SCSI_INTR
#define	SYS_DEV_LANCE	LANCE_INTR
#define	SYS_DEV_SCC0	SERIAL0_INTR
#define	SYS_DEV_SCC1	SERIAL1_INTR
#define	SYS_DEV_SCC2	SERIAL2_INTR
#define	SYS_DEV_DTOP	DTOP_INTR
#define	SYS_DEV_FDC	FLOPPY_INTR
#define	SYS_DEV_ISDN	ISDN_INTR
#define	SYS_DEV_OPT0	SLOT0_INTR
#define	SYS_DEV_OPT1	SLOT1_INTR
#define	SYS_DEV_OPT2	SLOT2_INTR
#define	SYS_DEV_BOGUS	-1
#define	MAX_DEV_NCOOKIES 16

/* handle i/o device interrupts */
extern int (*mips_hardware_intr) __P((unsigned, unsigned, unsigned, unsigned));

#endif /* !_LOCORE */
#endif /* _KERNEL */

#endif /* !_PMAX_INTR_H_ */
