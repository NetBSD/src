/*	$NetBSD: setjmp.h,v 1.1 2002/07/05 13:32:01 scw Exp $	*/

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
 * Reserve enough space for callee-saved registers plus some other
 * bits.
 *
 *  r10 - r15	(6)	6
 *  r18		(1)	7
 *  r26 - r35	(10)	17
 *  r44 - r59	(16)	33
 *  tr5 - tr7	(3)	36
 *  signals     (2)	38
 */

#define	_JB_R10		0
#define	_JB_R11		1
#define	_JB_R12		2
#define	_JB_R13		3
#define	_JB_R14		4
#define	_JB_R15		5
#define	_JB_R18		6
#define	_JB_R26		7
#define	_JB_R27		8
#define	_JB_R28		9
#define	_JB_R29		10
#define	_JB_R30		11
#define	_JB_R31		12
#define	_JB_R32		13
#define	_JB_R33		14
#define	_JB_R34		15
#define	_JB_R35		16
#define	_JB_R44		17
#define	_JB_R45		18
#define	_JB_R46		19
#define	_JB_R47		20
#define	_JB_R48		21
#define	_JB_R49		22
#define	_JB_R50		23
#define	_JB_R51		24
#define	_JB_R52		25
#define	_JB_R53		26
#define	_JB_R54		27
#define	_JB_R55		28
#define	_JB_R56		29
#define	_JB_R57		30
#define	_JB_R58		31
#define	_JB_R59		32
#define	_JB_TR5		33
#define	_JB_TR6		34
#define	_JB_TR7		35

#define	_JB_SP		_JB_R14
#define	_JB_PC		_JB_R18

#define	_JB_SIGFLAG	36
#define	_JB_SIGMASK	37

#define	_JB_DR0		38
#define	_JB_DR0		39
#define	_JB_DR0		40
#define	_JB_DR0		41
#define	_JB_DR0		42
#define	_JB_DR0		43
#define	_JB_DR0		44
#define	_JB_DR0		45

#define	_JBLEN		46

#endif /* _SH5_SETJMP_H */
