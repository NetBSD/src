/*	$NetBSD: intr.h,v 1.1 1997/04/13 05:12:40 scottr Exp $	*/

#ifndef _MAC68K_INTR_H_
#define _MAC68K_INTR_H_

#ifdef _KERNEL
/*
 * spl functions; all but spl0 are done in-line
 */

#define _spl(s)								\
({									\
        register int _spl_r;						\
									\
        __asm __volatile ("clrl %0; movew sr,%0; movew %1,sr" :		\
                "&=d" (_spl_r) : "di" (s));				\
        _spl_r;								\
})

#define _splraise(s)							\
({									\
	register int _spl_r;						\
									\
	__asm __volatile ("clrl %0; movew sr,%0;" : "&=d" (_spl_r) : );	\
	if ((_spl_r & PSL_IPL) < ((s) & PSL_IPL))			\
		__asm __volatile ("movew %0,sr;" : : "di" (s));		\
	_spl_r;								\
})

/* spl0 requires checking for software interrupts */
#define spl1()  _spl(PSL_S|PSL_IPL1)
#define spl2()  _spl(PSL_S|PSL_IPL2)
#define spl3()  _spl(PSL_S|PSL_IPL3)
#define spl4()  _spl(PSL_S|PSL_IPL4)
#define spl5()  _spl(PSL_S|PSL_IPL5)
#define spl6()  _spl(PSL_S|PSL_IPL6)
#define spl7()  _spl(PSL_S|PSL_IPL7)

/*
 * These should be used for:
 * 1) ensuring mutual exclusion (why use processor level?)
 * 2) allowing faster devices to take priority
 *
 * Note that on the Mac, most things are masked at spl1, almost
 * everything at spl2, and everything but the panic switch and
 * power at spl4.
 */
#define	splsoftclock()	spl1()	/* disallow softclock */
#define	splsoftnet()	spl1()	/* disallow network */
#define	spltty()	spl1()	/* disallow tty (softserial & ADB) */
#define	splbio()	spl2()	/* disallow block I/O */
#define	splnet()	spl2()	/* disallow network */
#define	splimp()	spl2()	/* mutual exclusion for memory allocation */
#define	splclock()	spl2()	/* disallow clock (and other) interrupts */
#define	splstatclock()	spl2()	/* ditto */
#define	splzs()		spl4()	/* disallow serial hw interrupts */
#define	spladb()	spl7()	/* disallow adb interrupts */
#define	splhigh()	spl7()	/* disallow everything */
#define	splsched()	spl7()	/* disallow scheduling */

/* watch out for side effects */
#define splx(s)         ((s) & PSL_IPL ? _spl(s) : spl0())

/*
 * simulated software interrupt register
 */
extern volatile u_int8_t ssir;

#define	SIR_NET		0x01
#define	SIR_CLOCK	0x02
#define	SIR_SERIAL	0x04

#define	siron(mask)	\
	__asm __volatile ( "orb %0,_ssir" : : "i" (mask))
#define	siroff(mask)	\
	__asm __volatile ( "andb %0,_ssir" : : "ir" (~(mask)));

#define	setsoftnet()	siron(SIR_NET)
#define	setsoftclock()	siron(SIR_CLOCK)
#define	setsoftserial()	siron(SIR_SERIAL)

/* locore.s */
int	spl0 __P((void));
#endif /* _KERNEL */

#endif /* _MAC68K_INTR_H_ */
