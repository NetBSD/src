/*	$NetBSD: param.h,v 1.9 1997/06/10 18:47:53 veego Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: machparam.h 1.16 92/12/20$
 *
 *	@(#)param.h	8.1 (Berkeley) 6/10/93
 */

#ifndef	_MACHINE_PARAM_H_
#define	_MACHINE_PARAM_H_

/*
 * Machine dependent constants for mvme68k, based on HP9000 series 300.
 */
#define	_MACHINE 	mvme68k
#define	MACHINE		"mvme68k"

#define	PGSHIFT		12		/* LOG2(NBPG) */
#define	KERNBASE	0x00000000	/* start of kernel virtual */

#define	SEGSHIFT	22		/* LOG2(NBSEG) */
#define NBSEG		(1 << SEGSHIFT)	/* bytes/segment */
#define	SEGOFSET	(NBSEG-1)	/* byte offset into segment */

#define	UPAGES		3		/* pages of u-area */

#include <m68k/param.h>

/*
 * Size of kernel malloc arena in CLBYTES-sized logical pages
 */
#ifndef NKMEMCLUSTERS
# define	NKMEMCLUSTERS	(2048 * 1024 / CLBYTES)
#endif

/*
 * spl functions; all but spl0 are done in-line
 */
#include <machine/psl.h>

#define _spl(s) \
({ \
        register int _spl_r; \
\
        __asm __volatile ("clrl %0; movew sr,%0; movew %1,sr" : \
                "&=d" (_spl_r) : "di" (s)); \
        _spl_r; \
})

/* spl0 requires checking for software interrupts */
#define spl1()  _spl(PSL_S|PSL_IPL1)
#define spl2()  _spl(PSL_S|PSL_IPL2)
#define spl3()  _spl(PSL_S|PSL_IPL3)
#define spl4()  _spl(PSL_S|PSL_IPL4)
#define spl5()  _spl(PSL_S|PSL_IPL5)
#define spl6()  _spl(PSL_S|PSL_IPL6)
#define spl7()  _spl(PSL_S|PSL_IPL7)

#define splsoftclock()  spl1()
#define splsoftnet()    spl1()
#define splbio()        spl2()
#define splnet()        spl3()
#define spltty()        spl3()
#define splimp()        spl3()
#define splclock()      spl5()
#define splstatclock()	spl5()
#define splvm()         spl5()
#define splhigh()       spl7()
#define splsched()      spl7()

/* watch out for side effects */
#define splx(s)         (s & PSL_IPL ? _spl(s) : spl0())

#if defined(_KERNEL) && !defined(_LOCORE)
extern void _delay __P((unsigned));
#define delay(us)	_delay((us)<<8)
#define DELAY(n)	delay(n)
#endif	/* _KERNEL && !_LOCORE */

#endif	/* !_MACHINE_PARAM_H_ */
