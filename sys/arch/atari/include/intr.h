/*	$NetBSD: intr.h,v 1.3 1997/06/05 19:38:16 leo Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ATARI_INTR_H_
#define _ATARI_INTR_H_

#define	IPL_NONE	0	/* disable no interrupts	*/
#define	IPL_BIO		3	/* disable block I/O interrupts */
#define	IPL_NET		3	/* disable network interrupts	*/
#define	IPL_TTY		4	/* disable terminal interrupts	*/
#define	IPL_CLOCK	6	/* disable clock interrupts	*/
#define	IPL_HIGH	7	/* disable all interrupts	*/

#define	IST_UNUSABLE	-1	/* interrupt cannot be used	*/
#define	IST_NONE	0	/* none (dummy)			*/
#define	IST_PULSE	1	/* pulsed			*/
#define	IST_EDGE	2	/* edge-triggered		*/
#define	IST_LEVEL	3	/* level-triggered		*/

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

#define	_splraise(s) \
({ \
	register int _spl_r; \
\
	__asm __volatile ("clrl %0; movew sr,%0;" : "&=d" (_spl_r) : ); \
	if ((_spl_r & PSL_IPL) < ((s) & PSL_IPL)) \
		__asm __volatile ("movew %0,sr;" : : "di" (s)); \
	_spl_r; \
})

/* spl0 requires checking for software interrupts */
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

#define splbio()	_splraise(PSL_S|PSL_IPL3)
#define splnet()	_splraise(PSL_S|PSL_IPL3)
#define spltty()	_splraise(PSL_S|PSL_IPL4)
#define splimp()	_splraise(PSL_S|PSL_IPL4)

#define splclock()	spl6()
#define splstatclock()	spl6()
#define splvm()		spl6()
#define splhigh()	spl7()
#define splsched()	spl7()

#define splx(s)         (s & PSL_IPL ? _spl(s) : spl0())

#ifdef _KERNEL
int spl0 __P((void));
#endif /* _KERNEL */

#endif /* _ATARI_INTR_H_ */

