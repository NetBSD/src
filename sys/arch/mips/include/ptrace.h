/*	$NetBSD: ptrace.h,v 1.13 2015/09/15 15:49:02 christos Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)ptrace.h	8.1 (Berkeley) 6/10/93
 */

/*
 * Mips-dependent ptrace definitions.
 *
 */

#ifndef _MIPS_PTRACE_H_
#define _MIPS_PTRACE_H_

/*#define	PT_STEP		(PT_FIRSTMACH + 0)*/
#define	PT_GETREGS	(PT_FIRSTMACH + 1)
#define	PT_SETREGS	(PT_FIRSTMACH + 2)

#define	PT_GETFPREGS	(PT_FIRSTMACH + 3)
#define	PT_SETFPREGS	(PT_FIRSTMACH + 4)

#define PT_MACHDEP_STRINGS \
	"(unused)", \
	"PT_GETREGS", \
	"PT_SETREGS", \
	"PT_GETFPREGS", \
	"PT_SETFPREGS",

#include <machine/reg.h>
#define PTRACE_REG_PC(r)	(r)->r_regs[35]
#define PTRACE_REG_SET_PC(r, v)	(r)->r_regs[35] = (v)
#define PTRACE_REG_SP(r)	(r)->r_regs[29]
#define PTRACE_REG_INTRV(r)	(r)->r_regs[2]
/*
 * Glue for gdb: map NetBSD register names to legacy ptrace register names
 */
#define GPR_BASE 0

#ifndef JB_PC
#define JB_PC	2	/* pc is at ((long *)jmp_buf)[2] */
#endif

#include <machine/reg.h>	/* Historically in sys/ptrace.h */
#include <machine/regnum.h>	/* real register names */

#endif	 /* _MIPS_PTRACE_H_ */
