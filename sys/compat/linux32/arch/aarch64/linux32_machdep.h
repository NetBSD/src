/*	$NetBSD: linux32_machdep.h,v 1.1 2021/11/25 03:08:04 ryo Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ryo Shimizu.
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

#ifndef _AARCH64_LINUX32_MACHDEP_H_
#define _AARCH64_LINUX32_MACHDEP_H_

#include <compat/linux32/arch/aarch64/linux32_missing.h>
#include <compat/linux32/arch/aarch64/linux32_siginfo.h>
#include <compat/linux32/arch/aarch64/linux32_signal.h>

struct linux32_sigcontext {
	uint32_t trap_no;
	uint32_t error_code;
	uint32_t oldmask;
	uint32_t arm_r0;
	uint32_t arm_r1;
	uint32_t arm_r2;
	uint32_t arm_r3;
	uint32_t arm_r4;
	uint32_t arm_r5;
	uint32_t arm_r6;
	uint32_t arm_r7;
	uint32_t arm_r8;
	uint32_t arm_r9;
	uint32_t arm_r10;
	uint32_t arm_fp;
	uint32_t arm_ip;
	uint32_t arm_sp;
	uint32_t arm_lr;
	uint32_t arm_pc;
	uint32_t arm_cpsr;
	uint32_t fault_address;
};

struct linux32_user_vfp {
	uint64_t fpregs[32];
	uint32_t fpscr;
};

struct linux32_user_vfp_exc {
	uint32_t fpexc;
	uint32_t fpinst;
	uint32_t fpinst2;
};

struct linux32_vfp_sigframe {
	uint32_t magic;
	uint32_t size;
	struct linux32_user_vfp ufp;
	struct linux32_user_vfp_exc ufp_exc;
} __aligned(8);

struct linux32_aux_sigframe {
	/* struct iwmmxt_sigframe iwmmxt; */	/* XXX: not supported */
	struct linux32_vfp_sigframe vfp;
	uint32_t end_magic;
} __aligned(8);

struct linux32_ucontext {
	uint32_t luc_flags;
	struct linux32_ucontext *luc_link;
	linux32_stack_t luc_stack;
	struct linux32_sigcontext luc_mcontext;

	linux32_sigset_t luc_sigmask;
	int luc__unused[32 - (sizeof(linux32_sigset_t) / sizeof(int))];

	uint32_t luc_regspace[128] __aligned(8);
};

struct linux32_sigframe {
	struct linux32_ucontext uc;
	uint32_t retcode[4];
};

struct linux32_rt_sigframe {
	struct linux32_siginfo info;
	struct linux32_sigframe sig;
};

#undef LINUX_UNAME_ARCH
#define LINUX_UNAME_ARCH	"armv7l"

#ifdef _KERNEL
__BEGIN_DECLS
void linux32_syscall_intern(struct proc *);
__END_DECLS
#endif /* !_KERNEL */

#endif /* _AARCH64_LINUX32_MACHDEP_H_ */
