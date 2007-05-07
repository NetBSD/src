/*	$NetBSD: linux_machdep.h,v 1.5.26.2 2007/05/07 10:55:12 yamt Exp $ */

/*-
 * Copyright (c) 2005 Emmanuel Dreyfus, all rights reserved.
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
 *	This product includes software developed by Emmanuel Dreyfus
 * 4. The name of the author may not be used to endorse or promote 
 *    products derived from this software without specific prior written 
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE THE AUTHOR AND CONTRIBUTORS ``AS IS'' 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS 
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _AMD64_LINUX_MACHDEP_H
#define _AMD64_LINUX_MACHDEP_H

#define LINUX_STATFS_64BIT	/* Needed for full 64bit struct statfs */

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_siginfo.h>

/* From <asm/sigcontext.h> */
struct linux__fpstate {
	u_int16_t cwd;
	u_int16_t swd;
	u_int16_t twd;
	u_int16_t fop;
	u_int64_t rip;
	u_int64_t rdp;
	u_int32_t mxcsr;
	u_int32_t mxcsr_mask;
	u_int32_t st_space[32];
	u_int32_t xmm_space[64];
	u_int32_t reserved2[24];
};

/* From <asm/sigcontext.h> */
struct linux_sigcontext {
	u_int64_t r8;
	u_int64_t r9;
	u_int64_t r10;
	u_int64_t r11;
	u_int64_t r12;
	u_int64_t r13;
	u_int64_t r14;
	u_int64_t r15;
	u_int64_t rdi;
	u_int64_t rsi;
	u_int64_t rbp;
	u_int64_t rbx;
	u_int64_t rdx;
	u_int64_t rcx;
	u_int64_t rsp;
	u_int64_t rip;
	u_int64_t eflags;
	u_int16_t cs;
	u_int16_t gs;
	u_int16_t fs;
	u_int16_t pad0;
	u_int64_t err;
	u_int64_t trapno;
	u_int64_t oldmask;
	u_int64_t cr2;
	struct linux__fpstate *fpstate;
	u_int64_t reserved1[8];
};

/* From <asm/ucontext.h> */
struct linux_ucontext {
	u_int64_t luc_flags;
	struct linux_ucontext *luc_link;
	linux_stack_t luc_stack;
	struct linux_sigcontext luc_mcontext;
	linux_sigset_t luc_sigmask;
};

/* From linux/arch/x86_64/kernel/signal.c */
struct linux_rt_sigframe {
	char *pretcode;
	struct linux_ucontext uc;
	struct linux_siginfo info;
};

#ifdef _KERNEL
__BEGIN_DECLS
void linux_syscall_intern __P((struct proc *));
__END_DECLS
#endif /* !_KERNEL */

#define LINUX_VSYSCALL_START	0xffffffffff600000
#define LINUX_VSYSCALL_SIZE	1024
#define LINUX_VSYSCALL_MAXNR	3

#define LINUX_UNAME_ARCH MACHINE_ARCH
#define LINUX_NPTL
#define LINUX_LARGEFILE64

/*
 * Used in ugly patch to fake device numbers.
 */
/* Major device numbers for new style ptys. */
#define LINUX_PTC_MAJOR                2
#define LINUX_PTS_MAJOR                3
/* Major device numbers of VT device on both Linux and NetBSD. */
#define LINUX_CONS_MAJOR       4


#endif /* _AMD64_LINUX_MACHDEP_H */
