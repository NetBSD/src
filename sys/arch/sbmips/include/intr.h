/* $NetBSD: intr.h,v 1.4 2005/11/11 23:45:56 simonb Exp $ */

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
 *    Broadcom Corporation.  The "Broadcom Corporation" name may not be
 *    used to endorse or promote products derived from this software
 *    without the prior written permission of Broadcom Corporation.
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
#define	IPL_SOFT	1	/* generic software interrupts */
#define	IPL_SOFTCLOCK	2	/* clock software interrupts */
#define	IPL_SOFTNET	3	/* network software interrupts */
#define	IPL_SOFTSERIAL	4	/* serial software interrupts */
#define	_IPL_NSOFT	4	/* max soft IPL - IPL_SOFT + 1 */

#define	IPL_SERIAL	5
#define	IPL_STATCLOCK	6
#define	IPL_CLOCK	7
#define	IPL_BIO		8
#define	IPL_NET		9
#define	IPL_TTY		10
#define	_NIPL		11

#define	IPL_SOFTNAMES {							\
	"misc",								\
	"clock",							\
	"net",								\
	"serial",							\
}

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

int	_splraise(int);
int	_spllower(int);
int	_splset(int);
int	_splget(void);
int	_splnone(void);
int	_setsoftintr(int);
int	_clrsoftintr(int);

#include <mips/softintr.h>

#endif /* _SBMIPS_INTR_H_ */
