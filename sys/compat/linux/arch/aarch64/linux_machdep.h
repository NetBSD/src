/*	$NetBSD: linux_machdep.h,v 1.3 2021/11/30 01:52:06 ryo Exp $	*/

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

/*
 * Not required for COMPAT_LINUX,
 * but required for COMPAT_LINUX32.
 */
struct linux_sys_open_args {
	syscallarg(const char *) path;
	syscallarg(int) flags;
	syscallarg(linux_umode_t) mode;
};
int linux_sys_open(struct lwp *, const struct linux_sys_open_args *, register_t *);

struct linux_sys_eventfd_args {
	syscallarg(unsigned int) initval;
};
int linux_sys_eventfd(struct lwp *, const struct linux_sys_eventfd_args *, register_t *);

struct linux_sys_llseek_args {
	syscallarg(int) fd;
	syscallarg(u_int32_t) ohigh;
	syscallarg(u_int32_t) olow;
	syscallarg(void *) res;
	syscallarg(int) whence;
};
int linux_sys_llseek(struct lwp *, const struct linux_sys_llseek_args *, register_t *);

struct linux_sys_unlink_args {
	syscallarg(const char *) path;
};
int linux_sys_unlink(struct lwp *, const struct linux_sys_unlink_args *, register_t *);

struct linux_sys_mknod_args {
	syscallarg(const char *) path;
	syscallarg(linux_umode_t) mode;
	syscallarg(unsigned) dev;
};
int linux_sys_mknod(struct lwp *, const struct linux_sys_mknod_args *, register_t *);

struct linux_sys_alarm_args {
	syscallarg(unsigned int) secs;
};
int linux_sys_alarm(struct lwp *, const struct linux_sys_alarm_args *, register_t *);

int linux_sys_pause(struct lwp *, const void *, register_t *);

#ifdef _KERNEL
__BEGIN_DECLS
void linux_syscall_intern(struct proc *);
__END_DECLS
#endif /* !_KERNEL */

#if BYTE_ORDER != BIG_ENDIAN
#define LINUX_UNAME_ARCH	"aarch64"
#else
#define LINUX_UNAME_ARCH	"aarch64_be"
#endif
#define LINUX_LARGEFILE64

#endif /* !_AARCH64_LINUX_MACHDEP_H */
