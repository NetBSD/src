/*	$NetBSD: setjmp.h,v 1.4 2002/09/05 09:53:17 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SH5_SETJMP_H
#define _SH5_SETJMP_H

/*
 * machine/setjmp.h: machine dependent setjmp-related information.
 */

/*
 * Define offsets for callee-saved registers plus some other bits.
 *
 * Note: the jmpbuf is the same size (and layout) as struct sigcontext
 * declared in <sh5/signal.h>
 *
 * XXX: Can't use sizeof(t) here; this is included by asm source...
 */

#define	_JB_SZINT		4
#define	_JB_SZREGISTER_T	8

#define	_JB_SIGONSTACK	(_JB_SZINT * 0)
#define	_JB_SIGMASK13	(_JB_SZINT * 1)
#define	_JB_SIGFPSTATE	(_JB_SZINT * 2)
#define	_JB_SIGPAD	(_JB_SZINT * 3)
#define	_JB_REGS	(_JB_SZINT * 4)
#define	_JB_SIGMASK	(_JB_DR(32))

#define	_JB_PC		(_JB_REGS + (_JB_SZREGISTER_T * 0))
#define	_JB_USR		(_JB_REGS + (_JB_SZREGISTER_T * 1))
#define	_JB_R(r)	(_JB_REGS + (_JB_SZREGISTER_T * 2) + ((r) * 8))
#define	_JB_TR(r)	(_JB_REGS + (_JB_SZREGISTER_T * 65) + ((r) * 8))
#define	_JB_FPSCR	(_JB_REGS + (_JB_SZREGISTER_T * 73))
#define	_JB_DR(r)	(_JB_REGS + (_JB_SZREGISTER_T * 74) + ((r) * 8))

#define	_JB_R2		_JB_R(2)
#define	_JB_R10		_JB_R(10)
#define	_JB_R11		_JB_R(11)
#define	_JB_R12		_JB_R(12)
#define	_JB_R13		_JB_R(13)
#define	_JB_R14		_JB_R(14)
#define	_JB_R15		_JB_R(15)
#define	_JB_R18		_JB_R(18)
#define	_JB_R24		_JB_R(24)
#define	_JB_R26		_JB_R(26)
#define	_JB_R27		_JB_R(27)
#define	_JB_R28		_JB_R(28)
#define	_JB_R29		_JB_R(29)
#define	_JB_R30		_JB_R(30)
#define	_JB_R31		_JB_R(31)
#define	_JB_R32		_JB_R(32)
#define	_JB_R33		_JB_R(33)
#define	_JB_R34		_JB_R(34)
#define	_JB_R35		_JB_R(35)
#define	_JB_R44		_JB_R(44)
#define	_JB_R45		_JB_R(45)
#define	_JB_R46		_JB_R(46)
#define	_JB_R47		_JB_R(47)
#define	_JB_R48		_JB_R(48)
#define	_JB_R49		_JB_R(49)
#define	_JB_R50		_JB_R(50)
#define	_JB_R51		_JB_R(51)
#define	_JB_R52		_JB_R(52)
#define	_JB_R53		_JB_R(53)
#define	_JB_R54		_JB_R(54)
#define	_JB_R55		_JB_R(55)
#define	_JB_R56		_JB_R(56)
#define	_JB_R57		_JB_R(57)
#define	_JB_R58		_JB_R(58)
#define	_JB_R59		_JB_R(59)
#define	_JB_TR0		_JB_TR(0)
#define	_JB_TR1		_JB_TR(1)
#define	_JB_TR2		_JB_TR(2)
#define	_JB_TR3		_JB_TR(3)
#define	_JB_TR4		_JB_TR(4)
#define	_JB_TR5		_JB_TR(5)
#define	_JB_TR6		_JB_TR(6)
#define	_JB_TR7		_JB_TR(7)

#define	_JB_DR12	_JB_DR(12)
#define	_JB_DR14	_JB_DR(14)
#define	_JB_DR36	_JB_DR(36)
#define	_JB_DR38	_JB_DR(38)
#define	_JB_DR40	_JB_DR(40)
#define	_JB_DR42	_JB_DR(42)
#define	_JB_DR44	_JB_DR(44)
#define	_JB_DR46	_JB_DR(46)
#define	_JB_DR48	_JB_DR(48)
#define	_JB_DR50	_JB_DR(50)
#define	_JB_DR52	_JB_DR(52)
#define	_JB_DR54	_JB_DR(54)
#define	_JB_DR56	_JB_DR(56)
#define	_JB_DR58	_JB_DR(58)
#define	_JB_DR60	_JB_DR(60)
#define	_JB_DR62	_JB_DR(62)

/*
 * XXX: It would be better if jmp_buf was an array of register_t ...
 */
#ifdef _LP64
#define	_JBLEN		110
#else
#define	_JBLEN		220
#define	_JB_ATTRIBUTES	__attribute__ ((__aligned__(8)))
#endif

#endif /* _SH5_SETJMP_H */
