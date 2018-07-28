/*	$NetBSD: trap.S,v 1.2.2.2 2018/07/28 04:37:27 pgoyette Exp $	*/

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

IDTVEC(trap10)
	TRAPENTRY(T_TSSFLT)
IDTVEC_END(trap10)

IDTVEC(trap11)
	ZTRAPENTRY(T_SEGNPFLT)
IDTVEC_END(trap11)

IDTVEC(trap12)
	ZTRAPENTRY(T_STKFLT)
IDTVEC_END(trap12)

IDTVEC(trap13)
	ZTRAPENTRY(T_PROTFLT)
IDTVEC_END(trap13)

IDTVEC(trap14)
	TRAPENTRY(T_PAGEFLT)
IDTVEC_END(trap14)

IDTVEC(trap15)
	ZTRAPENTRY(T_ASTFLT)
IDTVEC_END(trap15)

IDTVEC(trap16)
	ZTRAPENTRY(T_ARITHTRAP)
IDTVEC_END(trap16)

IDTVEC(trap17)
	TRAPENTRY(T_ALIGNFLT)
IDTVEC_END(trap17)

IDTVEC(trap18)
	ZTRAPENTRY(T_MCA)
IDTVEC_END(trap18)

IDTVEC(trap19)
	ZTRAPENTRY(T_XMM)
IDTVEC_END(trap19)

IDTVEC(trap20)
IDTVEC(trap21)
IDTVEC(trap22)
IDTVEC(trap23)
IDTVEC(trap24)
IDTVEC(trap25)
IDTVEC(trap26)
IDTVEC(trap27)
IDTVEC(trap28)
IDTVEC(trap29)
IDTVEC(trap30)
IDTVEC(trap31)
	/* 20 - 31 reserved for future exp */
	ZTRAPENTRY(T_RESERVED)
IDTVEC_END(trap31)
IDTVEC_END(trap30)
IDTVEC_END(trap29)
IDTVEC_END(trap28)
IDTVEC_END(trap27)
IDTVEC_END(trap26)
IDTVEC_END(trap25)
IDTVEC_END(trap24)
IDTVEC_END(trap23)
IDTVEC_END(trap22)
IDTVEC_END(trap21)
IDTVEC_END(trap20)

/*
 * Arguments pushed on the stack:
 *  sf_trapno
 *  sf_err  (dummy inserted if not defined)
 *  sf_rip
 *  sf_cs
 *  sf_rflags
 *  sf_rsp
 *  sf_ss
 */
ENTRY(alltraps)
	movq	%rsp,%rdi
	call	_C_LABEL(trap)
	/* NOTREACHED */
END(alltraps)

	.section .rodata

LABEL(x86_exceptions)
	.quad	_C_LABEL(Xtrap00), _C_LABEL(Xtrap01)
	.quad	_C_LABEL(Xtrap02), _C_LABEL(Xtrap03)
	.quad	_C_LABEL(Xtrap04), _C_LABEL(Xtrap05)
	.quad	_C_LABEL(Xtrap06), _C_LABEL(Xtrap07)
	.quad	_C_LABEL(Xtrap08), _C_LABEL(Xtrap09)
	.quad	_C_LABEL(Xtrap10), _C_LABEL(Xtrap11)
	.quad	_C_LABEL(Xtrap12), _C_LABEL(Xtrap13)
	.quad	_C_LABEL(Xtrap14), _C_LABEL(Xtrap15)
	.quad	_C_LABEL(Xtrap16), _C_LABEL(Xtrap17)
	.quad	_C_LABEL(Xtrap18), _C_LABEL(Xtrap19)
	.quad	_C_LABEL(Xtrap20), _C_LABEL(Xtrap21)
	.quad	_C_LABEL(Xtrap22), _C_LABEL(Xtrap23)
	.quad	_C_LABEL(Xtrap24), _C_LABEL(Xtrap25)
	.quad	_C_LABEL(Xtrap26), _C_LABEL(Xtrap27)
	.quad	_C_LABEL(Xtrap28), _C_LABEL(Xtrap29)
	.quad	_C_LABEL(Xtrap30), _C_LABEL(Xtrap31)
END(x86_exceptions)
