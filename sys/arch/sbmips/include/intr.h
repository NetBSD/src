/* $NetBSD: intr.h,v 1.1.10.2 2002/06/23 17:40:11 jdolecek Exp $ */

/*
 * Copyright 2000, 2001
 * Broadcom Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and copied only
 * in accordance with the following terms and conditions.  Subject to these
 * conditions, you may download, copy, install, use, modify and distribute
 * modified or unmodified copies of this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce and
 *    retain this copyright notice and list of conditions as they appear in
 *    the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Broadcom Corporation. Neither the "Broadcom Corporation" name nor any
 *    trademark or logo of Broadcom Corporation may be used to endorse or
 *    promote products derived from this software without the prior written
 *    permission of Broadcom Corporation.
 *
 * 3) THIS SOFTWARE IS PROVIDED "AS-IS" AND ANY EXPRESS OR IMPLIED
 *    WARRANTIES, INCLUDING BUT NOT LIMITED TO, ANY IMPLIED WARRANTIES OF
 *    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
 *    NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT SHALL BROADCOM BE LIABLE
 *    FOR ANY DAMAGES WHATSOEVER, AND IN PARTICULAR, BROADCOM SHALL NOT BE
 *    LIABLE FOR DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *    BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *    OR OTHERWISE), EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SBMIPS_INTR_H_
#define	_SBMIPS_INTR_H_

#include <machine/systemsw.h>

/* Interrupt levels */
#define	IPL_SERIAL	0
#define	IPL_STATCLOCK	1
#define	IPL_CLOCK	2
#define	IPL_BIO		3
#define	IPL_NET		4
#define	IPL_TTY		5
#define	_NIPL		6

#define	IPL_SOFTSERIAL	1000
#define	IPL_SOFTNET	1001
#define	IPL_SOFTCLOCK	1002

#define	_IMR_SOFT	(MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1)
#define	_IMR_VM		(_IMR_SOFT | MIPS_INT_MASK_0)
#define	_IMR_SCHED	(_IMR_VM | MIPS_INT_MASK_1 | MIPS_INT_MASK_5)
#define	_IMR_SERIAL	(_IMR_SCHED | MIPS_INT_MASK_2)
#define	_IMR_HIGH	(MIPS_INT_MASK)

#define	splbio()		_splraise(_IMR_VM)
#define	splclock()		_splraise(_IMR_SCHED)
#define	splhigh()		_splraise(_IMR_HIGH)
#define	spllock()		splhigh()
#define	splvm()			_splraise(_IMR_VM)
#define	splnet()		_splraise(_IMR_VM)
#define	splsched()		_splraise(_IMR_SCHED)
#define	splserial()		_splraise(_IMR_SERIAL)
#define	splsoftclock()		_splraise(_IMR_SOFT)
#define	splsoftnet()		_splraise(_IMR_SOFT)
#define	splsoftserial()		_splraise(_IMR_SOFT)
#define	splstatclock()		_splraise(_IMR_SCHED)
#define	spltty()		_splraise(_IMR_VM)

#define	spl0()			_spllower(0)
#define	spllowersoftclock()	_spllower(_IMR_SOFT)
#define	splx(s)			_splset(s)

#define	__GENERIC_SOFT_INTERRUPTS

void		*softintr_establish(int level, void (*fun)(void *), void *arg);
void		softintr_disestablish(void *cookie);
void		softintr_schedule(void *cookie);

/* for interrupt dispatch code */
void		dosoftints(void);

/* XXX backward-compat */
void		setsoftclock(void);
void		setsoftnet(void);

extern int		_splraise(int);
extern int		_spllower(int);
extern int		_splset(int);
extern int		_splget(void);
extern void		_splnone(void);
extern void		_setsoftintr(int);
extern void		_clrsoftintr(int);

/* XXX */
extern u_long	intrcnt[];
#define	SOFTCLOCK_INTR	0
#define	SOFTNET_INTR	1

#endif /* _SBMIPS_INTR_H_ */
