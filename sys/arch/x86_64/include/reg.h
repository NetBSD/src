/*	$NetBSD: reg.h,v 1.2 2002/05/28 23:11:38 fvdl Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	@(#)reg.h	5.5 (Berkeley) 1/18/91
 */

#ifndef _X86_64_REG_H_
#define _X86_64_REG_H_

/*
 * XXX
 * The #defines aren't used in the kernel, but some user-level code still
 * expects them.
 */

/* When referenced during a trap/exception, registers are at these offsets */

#define tR15	0
#define tR14	1
#define tR13	2
#define tR12	3
#define tR11	4
#define tR10	5
#define tR9	6
#define tR8	7
#define	tRDI	8
#define	tRSI	9
#define	tRBP	10
#define	tRBX	11
#define	tRDX	12
#define	tRCX	13
#define	tRAX	14

#define	tRIP	17
#define	tCS	18
#define	tRFLAGS	19
#define	tRSP	20
#define	tSS	21

/*
 * Registers accessible to ptrace(2) syscall for debugger
 * The machine-dependent code for PT_{SET,GET}REGS needs to
 * use whichver order, defined above, is correct, so that it
 * is all invisible to the user.
 */
struct reg {
	u_int64_t	r_r15;
	u_int64_t	r_r14;
	u_int64_t	r_r13;
	u_int64_t	r_r12;
	u_int64_t	r_r11;
	u_int64_t	r_r10;
	u_int64_t	r_r9;
	u_int64_t	r_r8;
	u_int64_t	r_rdi;
	u_int64_t	r_rsi;
	u_int64_t	r_rbp;
	u_int64_t	r_rbx;
	u_int64_t	r_rdx;
	u_int64_t	r_rcx;
	u_int64_t	r_rax;
	u_int64_t	r_rsp;
	u_int64_t	r_rip;
	u_int64_t	r_rflags;
	u_int64_t	r_cs;
	u_int64_t	r_ss;
	u_int64_t	r_ds;
	u_int64_t	r_es;
	u_int64_t	r_fs;
	u_int64_t	r_gs;
};

struct fpreg {
	struct fxsave64 fxstate;
};

#endif /* !_X86_64_REG_H_ */
