/*	$NetBSD: psl.h,v 1.17 2001/04/13 23:29:57 thorpej Exp $	*/

#ifndef _MACHINE_PSL_H_
#define _MACHINE_PSL_H_

#include <m68k/psl.h>

#if defined(_KERNEL) && !defined(_LOCORE)

#define	spl0()			_spl0()	/* we have real software interrupts */

#define splnone()		spl0()
#define	spllowersoftclock()	spl1()

#define splsoftclock()		splraise1()
#define splsoftnet()		splraise1()
#define splbio()		splraise3()
#define splnet()		splraise3()

/*
 * splserial hack, idea by Jason Thorpe.
 * drivers which need it (at the present only the coms) raise the variable to
 * their serial interrupt level.
 *
 * serialspl is statically initialized in machdep.c at the moment; should 
 * be some driver independent file.
 *
 * XXX should serialspl be volatile? I think not; it is intended to be set only
 * during xxx_attach() time, and will be used only later.
 *	-is
 */

extern u_int16_t	amiga_serialspl;
#define splserial()	_splraise(amiga_serialspl)
#define spltty()	splraise4()
#define	splvm()		splraise4()

#ifndef LEV6_DEFER
#define splclock()	splraise6()
#define splstatclock()	splraise6()
#define splhigh()	spl7()
#define splsched()	spl7()
#define spllock()	spl7()
#else
#define splclock()	splraise4()
#define splstatclock()	splraise4()
#define splhigh()	splraise4()
#define splsched()	splraise4()
#define spllock()	splraise4()
#endif

#define splx(s)		_spl(s)

#endif	/* KERNEL && !_LOCORE */
#endif	/* _MACHINE_PSL_H_ */
