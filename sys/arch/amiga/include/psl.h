/*	$NetBSD: psl.h,v 1.9 1996/10/10 23:56:47 christos Exp $	*/

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
		kprintf ("%s:%d:spl(%d) ==> spl(%d)!!\n",__FILE__,__LINE__, \
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
#define spltty()	spl4()
#define splimp()	spl4()
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
