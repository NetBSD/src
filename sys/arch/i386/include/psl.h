/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)psl.h	5.2 (Berkeley) 1/18/91
 *	$Id: psl.h,v 1.10.2.2 1994/10/06 03:53:47 mycroft Exp $
 */

#ifndef _I386_PSL_H_
#define _I386_PSL_H_

/*
 * 386 processor status longword.
 */
#define	PSL_C		0x00000001	/* carry bit */
#define	PSL_PF		0x00000004	/* parity bit */
#define	PSL_AF		0x00000010	/* bcd carry bit */
#define	PSL_Z		0x00000040	/* zero bit */
#define	PSL_N		0x00000080	/* negative bit */
#define	PSL_T		0x00000100	/* trace enable bit */
#define	PSL_I		0x00000200	/* interrupt enable bit */
#define	PSL_D		0x00000400	/* string instruction direction bit */
#define	PSL_V		0x00000800	/* overflow bit */
#define	PSL_IOPL	0x00003000	/* i/o priviledge level enable */
#define	PSL_NT		0x00004000	/* nested task bit */
#define	PSL_RF		0x00010000	/* restart flag bit */
#define	PSL_VM		0x00020000	/* virtual 8086 mode bit */
#define	PSL_AC		0x00040000	/* alignment checking */
#define	PSL_VIF		0x00080000	/* virtual interrupt enable */
#define	PSL_VIP		0x00100000	/* virtual interrupt pending */
#define	PSL_ID		0x00200000	/* identification bit */

#define	PSL_MBO		0x00000002	/* must be one bits */
#define	PSL_MBZ		0xffc08028	/* must be zero bits */

#define	PSL_USERSET	(PSL_MBO | PSL_I)
#define	PSL_USERCLR	(PSL_MBZ | PSL_VIP | PSL_VIF | PSL_NT | PSL_VM)

#ifdef KERNEL
#define	IPL_NONE	-1
#define	IPL_BIO		0
#define	IPL_NET		1
#define	IPL_TTY		2
#define	IPL_CLOCK	3

#define	SIR_CLOCK	31
#define	SIR_CLOCKMASK	(1 << SIR_CLOCK)
#define	SIR_NET		30
#define	SIR_NETMASK	((1 << SIR_NET) | SIR_CLOCKMASK)
#define	SIR_TTY		29
#define	SIR_TTYMASK	((1 << SIR_TTY) | SIR_CLOCKMASK)

#ifndef LOCORE
int cpl, ipending, astpending, imask[4];

#define	SPL(name, adjust) \
static __inline int			\
__CONCAT(spl,name)()				\
{ 					\
	register int ocpl = cpl;	\
	adjust;				\
	return ocpl;			\
}

SPL(bio, cpl |= imask[IPL_BIO])
SPL(imp, cpl |= imask[IPL_NET])
SPL(tty, cpl |= imask[IPL_TTY])
SPL(clock, cpl |= imask[IPL_CLOCK])
#define	splstatclock()	splclock()
SPL(high, cpl = -1)
/*
 * splsoftclock() is used by hardclock() to lower the priority from clock to
 * softclock before it calls softclock().
 */
SPL(softclock, cpl = SIR_CLOCKMASK)
SPL(softnet, cpl |= SIR_NETMASK)
#define	splnet()	splsoftnet()
SPL(softtty, cpl |= SIR_TTYMASK)

extern void spllower __P((void));

static __inline void
spl0()
{
	cpl = 0;
	if (ipending)
		spllower();
}

static __inline void
splx(ncpl)
	register int ncpl;
{
	cpl = ncpl;
	if (ipending & ~ncpl)
		spllower();
}

#define	softintr(n)	(ipending |= (1 << (n)))
#define	setsoftast()	(astpending = 1)
#define	setsoftclock()	softintr(SIR_CLOCK)
#define	setsoftnet()	softintr(SIR_NET)
#define	setsofttty()	softintr(SIR_TTY)
#endif /* !LOCORE */
#endif /* KERNEL */

#endif /* !_I386_PSL_H_ */
