/*	$NetBSD: intr.h,v 1.8 2001/05/14 17:55:03 matt Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang
 * All rights reserved.
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.netbsd.org/ for
 *          information about NetBSD.
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

#ifndef	_SGIMIPS_INTR_H_
#define	_SGIMIPS_INTR_H_

#define	IPL_NONE	0	/* Disable only this interrupt. */
#define	IPL_BIO		1	/* Disable block I/O interrupts. */
#define	IPL_NET		2	/* Disable network interrupts. */
#define	IPL_TTY		3	/* Disable terminal interrupts. */
#define	IPL_CLOCK	4	/* Disable clock interrupts. */
#define	IPL_STATCLOCK	5	/* Disable profiling interrupts. */
#ifndef __NO_SOFT_SERIAL_INTERRUPT
#define	IPL_SERIAL	6	/* Disable serial hardware interrupts. */
#endif
#define	IPL_HIGH	7	/* Disable all interrupts. */
#define NIPL		8

/* Interrupt sharing types. */
#define IST_NONE	0	/* none */
#define IST_PULSE	1	/* pulsed */
#define IST_EDGE	2	/* edge-triggered */
#define IST_LEVEL	3	/* level-triggered */

/* Soft interrupt numbers */
#ifndef __NO_SOFT_SERIAL_INTERRUPT
#define	IPL_SOFTSERIAL	0	/* serial software interrupts */
#endif
#define	IPL_SOFTNET	1	/* network software interrupts */
#define	IPL_SOFTCLOCK	2	/* clock software interrupts */
#define	IPL_NSOFT	3

#define	IPL_SOFTNAMES {							\
	"serial",							\
	"net",								\
	"clock",							\
}

#ifdef _KERNEL
#ifndef _LOCORE

#include <sys/queue.h>
#include <sys/types.h>
#include <sys/device.h>
#include <mips/cpuregs.h>

/*
 * software simulated interrupt
 */
#define setsoft(x)	do {			\
	extern u_int ssir;			\
	int s;					\
						\
	s = splhigh();				\
	ssir |= 1 << (x);			\
	_setsoftintr(MIPS_SOFT_INT_MASK_1);	\
	splx(s);				\
} while (0)

#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS

#define softintr_schedule(arg)						\
do {									\
	struct sgi_intrhand *__ih = (arg);				\
	__ih->ih_pending = 1;						\
	setsoft(__ih->ih_intrhead->intr_ipl);				\
} while (0)

extern struct sgi_intrhand *softnet_intrhand;

#define	setsoftnet()	softintr_schedule(softnet_intrhand)

#else /* ! __HAVE_GENERIC_SOFT_INTERRUPTS */

#define SIR_NET		0x01
#define SIR_SERIAL	0x02

#define setsoftclock()	_setsoftintr(MIPS_SOFT_INT_MASK_0)
#define setsoftnet()	setsoft(SIR_NET)
#define setsoftserial()	setsoft(SIR_SERIAL)
#endif /* __HAVE_GENERIC_SOFT_INTERRUPTS */

#define NINTR	32

struct sgi_intrhand {
	LIST_ENTRY(sgi_intrhand)
		ih_q;
	int	(*ih_fun) __P((void *));
	void	 *ih_arg;
	struct	sgi_intr *ih_intrhead;
	int	ih_pending;
};

struct sgi_intr {
	LIST_HEAD(,sgi_intrhand)
		intr_q;
	struct	evcnt ih_evcnt;
	unsigned long intr_ipl;
};

extern struct sgi_intrhand intrtab[];

extern int		_splraise(int);
extern int		_spllower(int); 
extern int		_splset(int);
extern int		_splget(void);
extern void		_splnone(void); 
extern void		_setsoftintr(int);
extern void		_clrsoftintr(int);

extern u_int32_t 	biomask;
extern u_int32_t 	netmask;
extern u_int32_t 	ttymask;
extern u_int32_t 	clockmask;

#define splhigh()       _splraise(MIPS_INT_MASK)
#define spl0()          (void)_spllower(0)
#define splx(s)         (void)_splset(s)
#define splbio()        _splraise(biomask)
#define splnet()        _splraise(netmask)
#define spltty()        _splraise(ttymask)
#define	splserial()	spltty()
#define splvm()         spltty()
#define splclock()      _splraise(clockmask)
#define splstatclock()  splclock()

#define	splsched()	splhigh()
#define	spllock()	splhigh()
#define spllpt()	spltty()

#define splsoftclock()	_splraise(MIPS_SOFT_INT_MASK_0)
#define splsoft()	_splraise(MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1)
#define spllowersoftclock() _spllower(MIPS_SOFT_INT_MASK_0)
#define splsoftnet()	splsoft()

extern void *		cpu_intr_establish(int, int, int (*)(void *), void *);
void *			softintr_establish(int, void (*)(void *), void *);
void			softintr_disestablish(void *);
void			softintr_init(void);
void			softintr_dispatch(void);


#endif /* _LOCORE */
#endif /* _KERNEL */

#endif	/* !_SGIMIPS_INTR_H_ */
