/*	$NetBSD: linux_machdep.h,v 1.1 2021/09/23 06:56:27 ryo Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
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

#ifndef _AARCH64_LINUX_MACHDEP_H
#define _AARCH64_LINUX_MACHDEP_H

#include <aarch64/reg.h>
#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_siginfo.h>

/* from $LINUXSRC/arch/arm64/include/uapi/asm/sigcontext.h */
struct linux_aarch64_ctx {
	uint32_t magic;
	uint32_t size;
};

#define FPSIMD_MAGIC	0x46508001
struct fpsimd_context {
	struct linux_aarch64_ctx head;
	uint32_t fpsr;
	uint32_t fpcr;
	union fpelem vregs[32];
};

/* from $LINUXSRC/arch/arm64/include/uapi/asm/sigcontext.h */
struct linux_sigcontext {
	uint64_t fault_address;
	uint64_t regs[31];
	uint64_t sp;
	uint64_t pc;
	uint64_t pstate;
	uint8_t __reserved[4096] __attribute__((__aligned__(16)));
};

/* from $LINUXSRC/arch/arm64/include/uapi/asm/ucontext.h */
struct linux_ucontext {
	unsigned long luc_flags;
	struct linux_ucontext *luc_link;
	linux_stack_t luc_stack;
	linux_sigset_t luc_sigmask;
	uint8_t __reserved[1024 / 8 - sizeof(linux_sigset_t)];
	struct linux_sigcontext luc_mcontext;
};

/* from $LINUXSRC/arch/arm64/kernel/signal.c */
struct linux_rt_sigframe {
	struct linux_siginfo info;
	struct linux_ucontext uc;
};


#ifdef _KERNEL
__BEGIN_DECLS
void linux_syscall_intern(struct proc *);
__END_DECLS
#endif /* !_KERNEL */

#endif /* !_AARCH64_LINUX_MACHDEP_H */
