/*	$NetBSD: param.h,v 1.1 2001/05/14 18:23:13 drochner Exp $	*/

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

/*
 * Machine dependent constants for CES FIC8234.
 */
#define	_MACHINE	cesfic
#define	MACHINE		"cesfic"

/*
 * Round p (pointer or byte index) up to a correctly-aligned value for all
 * data types (int, long, ...).   The result is u_int and must be cast to
 * any desired pointer type.
 *
 * ALIGNED_POINTER is a boolean macro that checks whether an address
 * is valid to fetch data elements of type t from on this architecture.
 * This does not reflect the optimal alignment, just the possibility
 * (within reasonable limits). 
 *
 */

#define	PGSHIFT		12		/* LOG2(NBPG) */
#define	NPTEPG		(NBPG/(sizeof (pt_entry_t)))

#define	SEGSHIFT	22		/* LOG2(NBSEG) */
#define NBSEG		(1 << SEGSHIFT)	/* bytes/segment */
#define	SEGOFSET	(NBSEG-1)	/* byte offset into segment */

#define	KERNBASE	0x00002000	/* start of kernel virtual */

/* NOTE: SSIZE, SINCR and UPAGES must be multiples of CLSIZE */
#define	UPAGES		2		/* pages of u-area */

#include <m68k/param.h>

/*
 * Minimum and maximum sizes of the kernel malloc arena in PAGE_SIZE-sized
 * logical pages.
 */
#define	NKMEMPAGES_MIN_DEFAULT	((3 * 1024 * 1024) >> PAGE_SHIFT)
#define	NKMEMPAGES_MAX_DEFAULT	((3 * 1024 * 1024) >> PAGE_SHIFT)

/*
 * spl functions; all but spl0 are done in-line
 */
#include <machine/psl.h>

/* spl0 requires checking for software interrupts */
#define spl1()  _spl(PSL_S|PSL_IPL1)
#define spl2()  _spl(PSL_S|PSL_IPL2)
#define spl3()  _spl(PSL_S|PSL_IPL3)
#define spl4()  _spl(PSL_S|PSL_IPL4)
#define spl5()  _spl(PSL_S|PSL_IPL5)
#define spl6()  _spl(PSL_S|PSL_IPL6)
#define spl7()  _spl(PSL_S|PSL_IPL7)

#if defined(_KERNEL) && !defined(_LOCORE)
/*
 * These four globals contain the appropriate PSL_S|PSL_IPL? values
 * to raise interrupt priority to the requested level.
 */
extern	unsigned short cesfic_bioipl;
extern	unsigned short cesfic_netipl;
extern	unsigned short cesfic_ttyipl;
extern	unsigned short cesfic_impipl;
#endif /* _KERNEL && !_LOCORE */

/* These spl calls are _not_ to be used by machine-independent code. */
#define splhil()	_splraise(PSL_S|PSL_IPL1)
#define splkbd()	splhil()

/* These spl calls are used by machine-independent code. */
#define	spllowersoftclock() spl1()
#define	splsoft()	splraise1()
#define splsoftclock()	splsoft()
#define splsoftnet()	splsoft()
#define splbio()	_splraise(cesfic_bioipl)
#define splnet()	_splraise(cesfic_netipl)
#define spltty()	_splraise(cesfic_ttyipl)
#define splimp()	_splraise(cesfic_impipl)
#define splclock()	spl6()
#define splstatclock()	spl6()
#define splvm()		spl6()
#define splhigh()	spl7()
#define splsched()	spl7()
#define spllock()	spl7()

/* watch out for side effects */
#define splx(s)         (s & PSL_IPL ? _spl(s) : spl0())

#if defined(_KERNEL) && !defined(_LOCORE)
#define	delay(us)	_delay((us) << 8)
#define DELAY(us)	delay(us)

int	spl0 __P((void));
void	_delay __P((u_int));
#endif /* _KERNEL && !_LOCORE */

