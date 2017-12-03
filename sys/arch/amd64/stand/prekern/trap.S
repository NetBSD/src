/*	$NetBSD: trap.S,v 1.1.2.2 2017/12/03 11:35:48 jdolecek Exp $	*/

/*
 * Copyright (c) 2017 The NetBSD Foundation, Inc. All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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

#define _KERNEL

/* Override user-land alignment before including asm.h */
#define	ALIGN_DATA	.align	8
#define ALIGN_TEXT	.align 16,0x90
#define _ALIGN_TEXT	ALIGN_TEXT

#include <machine/asm.h>

#include <machine/trap.h>

#define	TRAPENTRY(a) \
	pushq $(a); \
	jmp _C_LABEL(alltraps);

#define	ZTRAPENTRY(a) \
	pushq $0; \
	pushq $(a); \
	jmp _C_LABEL(alltraps);

	.text

IDTVEC(trap00)
	ZTRAPENTRY(T_DIVIDE)
IDTVEC_END(trap00)

IDTVEC(trap01)
	ZTRAPENTRY(T_TRCTRAP)
IDTVEC_END(trap01)

IDTVEC(trap02)
	ZTRAPENTRY(T_NMI)
IDTVEC_END(trap02)

IDTVEC(trap03)
	ZTRAPENTRY(T_BPTFLT)
IDTVEC_END(trap03)

IDTVEC(trap04)
	ZTRAPENTRY(T_OFLOW)
IDTVEC_END(trap04)

IDTVEC(trap05)
	ZTRAPENTRY(T_BOUND)
IDTVEC_END(trap05)

IDTVEC(trap06)
	ZTRAPENTRY(T_PRIVINFLT)
IDTVEC_END(trap06)

IDTVEC(trap07)
	ZTRAPENTRY(T_DNA)
IDTVEC_END(trap07)

IDTVEC(trap08)
	TRAPENTRY(T_DOUBLEFLT)
IDTVEC_END(trap08)

IDTVEC(trap09)
	ZTRAPENTRY(T_FPOPFLT)
IDTVEC_END(trap09)

IDTVEC(trap0a)
	TRAPENTRY(T_TSSFLT)
IDTVEC_END(trap0a)

IDTVEC(trap0b)		/* #NP() Segment not present */
	ZTRAPENTRY(T_SEGNPFLT)
IDTVEC_END(trap0b)

IDTVEC(trap0c)		/* #SS() Stack exception */
	ZTRAPENTRY(T_STKFLT)
IDTVEC_END(trap0c)

IDTVEC(trap0d)		/* #GP() General protection */
	ZTRAPENTRY(T_PROTFLT)
IDTVEC_END(trap0d)

IDTVEC(trap0e)
	TRAPENTRY(T_PAGEFLT)
IDTVEC_END(trap0e)

IDTVEC(trap0f)
	ZTRAPENTRY(T_ASTFLT)
IDTVEC_END(trap0f)

IDTVEC(trap10)
	ZTRAPENTRY(T_ARITHTRAP)
IDTVEC_END(trap10)

IDTVEC(trap11)
	TRAPENTRY(T_ALIGNFLT)
IDTVEC_END(trap11)

IDTVEC(trap12)
	ZTRAPENTRY(T_MCA)
IDTVEC_END(trap12)

IDTVEC(trap13)
	ZTRAPENTRY(T_XMM)
IDTVEC_END(trap13)

IDTVEC(trap14)
IDTVEC(trap15)
IDTVEC(trap16)
IDTVEC(trap17)
IDTVEC(trap18)
IDTVEC(trap19)
IDTVEC(trap1a)
IDTVEC(trap1b)
IDTVEC(trap1c)
IDTVEC(trap1d)
IDTVEC(trap1e)
IDTVEC(trap1f)
	/* 20 - 31 reserved for future exp */
	ZTRAPENTRY(T_RESERVED)
IDTVEC_END(trap1f)
IDTVEC_END(trap1e)
IDTVEC_END(trap1d)
IDTVEC_END(trap1c)
IDTVEC_END(trap1b)
IDTVEC_END(trap1a)
IDTVEC_END(trap19)
IDTVEC_END(trap18)
IDTVEC_END(trap17)
IDTVEC_END(trap16)
IDTVEC_END(trap15)
IDTVEC_END(trap14)

IDTVEC(exceptions)
	.quad	_C_LABEL(Xtrap00), _C_LABEL(Xtrap01)
	.quad	_C_LABEL(Xtrap02), _C_LABEL(Xtrap03)
	.quad	_C_LABEL(Xtrap04), _C_LABEL(Xtrap05)
	.quad	_C_LABEL(Xtrap06), _C_LABEL(Xtrap07)
	.quad	_C_LABEL(Xtrap08), _C_LABEL(Xtrap09)
	.quad	_C_LABEL(Xtrap0a), _C_LABEL(Xtrap0b)
	.quad	_C_LABEL(Xtrap0c), _C_LABEL(Xtrap0d)
	.quad	_C_LABEL(Xtrap0e), _C_LABEL(Xtrap0f)
	.quad	_C_LABEL(Xtrap10), _C_LABEL(Xtrap11)
	.quad	_C_LABEL(Xtrap12), _C_LABEL(Xtrap13)
	.quad	_C_LABEL(Xtrap14), _C_LABEL(Xtrap15)
	.quad	_C_LABEL(Xtrap16), _C_LABEL(Xtrap17)
	.quad	_C_LABEL(Xtrap18), _C_LABEL(Xtrap19)
	.quad	_C_LABEL(Xtrap1a), _C_LABEL(Xtrap1b)
	.quad	_C_LABEL(Xtrap1c), _C_LABEL(Xtrap1d)
	.quad	_C_LABEL(Xtrap1e), _C_LABEL(Xtrap1f)
IDTVEC_END(exceptions)

/*
 * Arguments pushed on the stack:
 *  tf_trapno
 *  tf_err: Dummy inserted if not defined
 *  tf_rip
 *  tf_cs
 *  tf_rflags
 *  tf_rsp
 *  tf_ss
 */

NENTRY(alltraps)
	movq	%rsp,%rdi
	call	_C_LABEL(trap)
	/* NOTREACHED */
END(alltraps)
