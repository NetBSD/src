/*	$NetBSD: psl.h,v 1.12 1998/04/11 18:28:33 is Exp $	*/

#ifndef _MACHINE_PSL_H_
#define _MACHINE_PSL_H_

#include <m68k/psl.h>

#if defined(_KERNEL) && !defined(_LOCORE)

#if 0
#define _debug_spl(s) \
({ \
        register int _spl_r; \
\
        __asm __volatile ("clrl %0; movew sr,%0; movew %1,sr" : \
                "&=d" (_spl_r) : "di" (s)); \
	if ((_spl_r&PSL_IPL) > ((s)&PSL_IPL)&&((s)&PSL_IPL)!=PSL_IPL1) \
		printf ("%s:%d:spl(%d) ==> spl(%d)!!\n",__FILE__,__LINE__, \
		    ((PSL_IPL&_spl_r)>>8), ((PSL_IPL&(s))>>8)); \
        _spl_r; \
})
#else
/*
 * Don't lower IPL below current IPL (unless new IPL is 6)
 */
#define _debug_spl(s) \
({ \
        register int _spl_r; \
\
        __asm __volatile ("clrl %0; movew sr,%0" : \
                "&=d" (_spl_r)); \
	if ((((s)&PSL_IPL) >= PSL_IPL6) || (_spl_r&PSL_IPL) < ((s)&PSL_IPL) || ((s)&PSL_IPL) <= PSL_IPL1) \
		__asm __volatile ("movew %0,sr" : : "di" (s)); \
        _spl_r; \
})
#endif

#define _spl_no_check(s) \
({ \
        register int _spl_r; \
\
        __asm __volatile ("clrl %0; movew sr,%0; movew %1,sr" : \
                "&=d" (_spl_r) : "di" (s)); \
        _spl_r; \
})
#if defined (DEBUGXX)		/* No workee */
#define _spl _debug_spl
#else
#define _spl _spl_no_check
#endif

#define spl0()	_spl(PSL_S|PSL_IPL0)
#define spl1()	_spl(PSL_S|PSL_IPL1)
#define spl2()	_spl(PSL_S|PSL_IPL2)
#define spl3()	_spl(PSL_S|PSL_IPL3)
#define spl4()	_spl(PSL_S|PSL_IPL4)
#define spl5()	_spl(PSL_S|PSL_IPL5)
#define spl6()	_spl(PSL_S|PSL_IPL6)
#define spl7()	_spl(PSL_S|PSL_IPL7)

#define splnone()	spl0()
#define splsoftclock()	spl1()
#define splsoftnet()	spl1()
#define splbio()	spl3()
#define splnet()	spl3()

/*
 * splserial hack, idea by Jason Thorpe.
 * drivers which need it (at the present only the coms) raise the variable to
 * their serial interupt level.
 *
 * serialspl is statically initialized in machdep.c at the moment; should 
 * be some driver independent file.
 *
 * XXX should serialspl be volatile? I think not; it is intended to be set only
 * during xxx_attach() time, and will be used only later.
 *	-is
 */

extern u_int16_t	amiga_serialspl;
#define splserial()	_spl(amiga_serialspl)
#define spltty()	spl4()
#define splimp()	spltty()	/* XXX for the full story, see i386 */

#ifndef LEV6_DEFER
#define splclock()	spl6()
#define splstatclock()	spl6()
#define splvm()		spl6()
#define splhigh()	spl7()
#define splsched()	spl7()
#else
#define splclock()	spl4()
#define splstatclock()	spl4()
#define splvm()		spl4()
#define splhigh()	spl4()
#define splsched()	spl4()
#endif

#define splx(s)		_spl_no_check(s)

#endif	/* KERNEL && !_LOCORE */
#endif	/* _MACHINE_PSL_H_ */
