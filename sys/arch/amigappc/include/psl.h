/*	$NetBSD: psl.h,v 1.2 2009/07/21 09:49:16 phx Exp $ */

#ifndef	_AMIGAPPC_PSL_H_
#define	_AMIGAPPC_PSL_H_

#include <powerpc/psl.h>

/*
 * Compatibility with m68k/include/psl.h for amiga/68k devices
 */
/*XXX #define spl0()		spllower(0)*/
#define spl1()		spllower(1)
#define spl2()		spllower(2)
#define spl3()		spllower(3)
#define spl4()		spllower(4)
#define spl5()		spllower(5)
#define spl6()		spllower(6)
#define spl7()		spllower(7)
#define splraise0()	splraise(0)
#define splraise1()	splraise(1)
#define splraise2()	splraise(2)
#define splraise3()	splraise(3)
#define splraise4()	splraise(4)
#define splraise5()	splraise(5)
#define splraise6()	splraise(6)
#define splraise7()	splraise(7)

/* hack to make amiga device drivers compatible with P5_IPL_EMU */
#define PSL_S		0
#define PSL_IPL0	0
#define PSL_IPL1	1
#define PSL_IPL2	2
#define PSL_IPL3	3
#define PSL_IPL4	4
#define PSL_IPL5	5
#define PSL_IPL6	6
#define PSL_IPL7	7

#endif /* _AMIGAPPC_PSL_H_ */
