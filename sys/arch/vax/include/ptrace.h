/*	$NetBSD: ptrace.h,v 1.4.54.1 2017/12/03 11:36:48 jdolecek Exp $	*/

/*
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden.
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
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

 /* All bugs are subject to removal without further notice */


#define PT_STEP         (PT_FIRSTMACH + 0)
#define PT_GETREGS      (PT_FIRSTMACH + 1)
#define PT_SETREGS      (PT_FIRSTMACH + 2)
#define PT_SETSTEP      (PT_FIRSTMACH + 3)
#define PT_CLEARSTEP    (PT_FIRSTMACH + 4)

#define PT_MACHDEP_STRINGS \
	"PT_STEP", \
	"PT_GETREGS", \
	"PT_SETREGS", \
	"PT_SETSTEP", \
	"PT_CLEARSTEP",

#include <machine/reg.h>

#define PTRACE_REG_PC(r)	(r)->pc
#define PTRACE_REG_SET_PC(r, v)	(r)->pc = (v)
#define PTRACE_REG_SP(r)	(r)->sp
#define PTRACE_REG_INTRV(r)	(r)->r0

#define PTRACE_BREAKPOINT	((const uint8_t[]) { 0x03 })
#define PTRACE_BREAKPOINT_ASM	__asm __volatile("bpt")
#define PTRACE_BREAKPOINT_SIZE	1
