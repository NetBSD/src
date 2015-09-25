/*	$NetBSD: ptrace.h,v 1.6 2015/09/25 16:05:17 christos Exp $	*/

/*
 * Copyright (c) 1993 Christopher G. Demetriou
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christopher G. Demetriou.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
#ifndef _AMD64_PTRACE_H_
#define _AMD64_PTRACE_H_

#ifdef __x86_64__
/*
 * i386-dependent ptrace definitions
 */
#define	PT_STEP		(PT_FIRSTMACH + 0)
#define	PT_GETREGS	(PT_FIRSTMACH + 1)
#define	PT_SETREGS	(PT_FIRSTMACH + 2)
#define	PT_GETFPREGS	(PT_FIRSTMACH + 3)
#define	PT_SETFPREGS	(PT_FIRSTMACH + 4)

#define PT_MACHDEP_STRINGS \
	"PT_STEP", \
	"PT_GETREGS", \
	"PT_SETREGS", \
	"PT_GETFPREGS", \
	"PT_SETFPREGS",

#include <machine/reg.h>
#define PTRACE_REG_PC(r)	(r)->regs[_REG_RIP]
#define PTRACE_REG_SET_PC(r, v)	(r)->regs[_REG_RIP] = (v)
#define PTRACE_REG_SP(r)	(r)->regs[_REG_RSP]
#define PTRACE_REG_INTRV(r)	(r)->regs[_REG_RAX]

#define PTRACE_BREAKPOINT	((const uint8_t[]) { 0xcc })
#define PTRACE_BREAKPOINT_SIZE	1
#define PTRACE_BREAKPOINT_ADJ	1

#ifdef _KERNEL_OPT
#include "opt_compat_netbsd32.h"

#ifdef COMPAT_NETBSD32
#include <machine/netbsd32_machdep.h>

#define process_read_regs32	netbsd32_process_read_regs
#define process_read_fpregs32	netbsd32_process_read_fpregs

#define process_reg32		struct reg32
#define process_fpreg32		struct fpreg32
#endif	/* COMPAT_NETBSD32 */
#endif	/* _KERNEL_OPT */

#else	/* !__x86_64__ */

#include <i386/ptrace.h>

#endif	/* __x86_64__ */

#endif	/* _AMD64_PTRACE_H_ */
