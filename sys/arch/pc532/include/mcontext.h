/*	$NetBSD: mcontext.h,v 1.1 2003/01/21 20:50:43 kleink Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus Klein.
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

#ifndef _NS32K_MCONTEXT_H_
#define _NS32K_MCONTEXT_H_

/*
 * Layout of mcontext_t for the ns32k architecture.  Lacking an SVR4
 * ABI definition, reuse the `struct reg' layout for simplicity.
 */

#define	_NGREG		13
typedef int		__greg_t;
typedef __greg_t	__gregset_t[_NGREG];

#define	_REG_R7		0
#define	_REG_R6		1
#define	_REG_R5		2
#define	_REG_R4		3
#define	_REG_R3		4
#define	_REG_R2		5
#define	_REG_R1		6
#define	_REG_R0		7
#define	_REG_SP		8
#define	_REG_SB		9
#define	_REG_FP		10
#define	_REG_PC		11
#define	_REG_PS		12

typedef struct {
	int		__fpr_psr;
	double		__fpr_regs[8];
} __fpregset_t;

typedef struct {
	__gregset_t	__gregs;
	__fpregset_t	__fpregs;
} mcontext_t;

#define	_UC_MACHINE_SP(uc)	((uc)->uc_mcontext.__gregs[_REG_SP])

#endif /* !_NS32K_MCONTEXT_H_ */
