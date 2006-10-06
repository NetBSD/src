/*	$NetBSD: intr.h,v 1.15 2006/10/06 18:16:46 tsutsui Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ignatios Souvatzis.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * machine/intr.h for the Amiga port.
 * Currently, only a wrapper, for most of the stuff, around the old
 * include files.
 */

#ifndef _AMIGA_INTR_H_
#define _AMIGA_INTR_H_

#include <amiga/amiga/isr.h>
#include <amiga/include/mtpr.h>
#include <m68k/psl.h>

#define IPL_SOFTCLOCK 1
#define IPL_SOFTSERIAL 1
#define IPL_SOFTNET 1

/* not used yet, should reflect psl.h */
#define IPL_BIO		3
#define IPL_NET		3
#define IPL_SERIAL	4
#define IPL_TTY		4


#ifdef splaudio
#undef splaudio
#define splaudio spl6
#endif

#define spllpt()	spl6()

#if !defined(_LKM)
#include "opt_lev6_defer.h"
#endif

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
 */

extern uint16_t		amiga_serialspl;

#define splserial()	_splraise(amiga_serialspl)
#define spltty()	splraise4()
#define	splvm()		splraise4()

#ifndef _LKM

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

#else	/* _LKM */

extern int _spllkm6(void);
extern int _spllkm7(void);

#define splclock()	_spllkm6()
#define splstatclock()	_spllkm6()
#define spllock()	_spllkm7()
#define splhigh()	_spllkm7()
#define splsched()	_spllkm7()

#endif /* _LKM */

#define splx(s)		_spl(s)

#endif	/* !_AMIGA_INTR_H_ */
