/* $NetBSD: setjmp.h,v 1.1 2014/09/03 19:34:26 matt Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#define	_JB_MAGIC_OR1K__SETJMP	0x6F71316B // or1k
#define	_JB_MAGIC_OR1K_SETJMP	0x4F51314B // OR1K

			/* magic + 13 reg + 8 simd + 4 sigmask + 6 slop */
#define _JBLEN		(32 * sizeof(_BSD_JBSLOT_T_)/sizeof(long))
#define _JB_MAGIC	0
#define	_JB_SP		1
#define _JB_FP		2
#define _JB_LR		3
#define _JB_R10		4
#define _JB_R14		5
#define _JB_R16		6
#define _JB_R18		7
#define _JB_R20		8
#define _JB_R22		9
#define _JB_R24		10
#define _JB_R26		11
#define _JB_R28		12
#define _JB_R30		13
#define _JB_PC		14
#define _JB_RV		15
#define _JB_FPCSR	16

#define _JB_SIGMASK	18

#ifndef _BSD_JBSLOT_T_
#define	_BSD_JBSLOT_T_	long long
#endif
