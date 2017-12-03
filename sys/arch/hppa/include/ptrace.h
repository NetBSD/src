/*	$NetBSD: ptrace.h,v 1.3.22.1 2017/12/03 11:36:16 jdolecek Exp $	*/

/*	$OpenBSD: ptrace.h,v 1.2 1998/12/01 03:05:44 mickey Exp $	*/

/*
 * Copyright (c) 1998 Michael Shalayeff
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * MD ptrace definitions
 */
#define	PT_STEP		(PT_FIRSTMACH + 0)
#define	PT_GETREGS	(PT_FIRSTMACH + 1)
#define	PT_SETREGS	(PT_FIRSTMACH + 2)
#define	PT_GETFPREGS	(PT_FIRSTMACH + 3)
#define	PT_SETFPREGS	(PT_FIRSTMACH + 4)
#define	PT_SETSTEP	(PT_FIRSTMACH + 5)
#define	PT_CLEARSTEP	(PT_FIRSTMACH + 6)

#define PT_MACHDEP_STRINGS \
	"PT_STEP", \
	"PT_GETREGS", \
	"PT_SETREGS", \
	"PT_GETFPREGS", \
	"PT_SETFPREGS", \
	"PT_SETSTEP", \
	"PT_CLEARSTEP",

#include <machine/reg.h>
#define PTRACE_REG_PC(r)	(r)->r_pcoqh
#define PTRACE_REG_SET_PC(r, v)	do {	\
	(r)->r_pcoqh = (v);		\
	(r)->r_pcoqt = (v) + 4;		\
    } while (/*CONSTCOND*/0)
#define PTRACE_REG_SP(r)	(r)->r_regs[30]
#define PTRACE_REG_INTRV(r)	(r)->r_regs[28]

#define PTRACE_BREAKPOINT	((const uint8_t[]) { 0x00, 0x01, 0x00, 0x04 })
#define PTRACE_BREAKPOINT_ASM	__asm __volatile("break	%0, %1" :: "i" (HPPA_BREAK_KERNEL), "i" (HPPA_BREAK_SS) : "memory")
#define PTRACE_BREAKPOINT_SIZE	4
