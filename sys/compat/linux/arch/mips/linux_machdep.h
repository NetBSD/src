/* $NetBSD: linux_machdep.h,v 1.10.10.1 2011/06/06 09:07:25 jruoho Exp $ */

/*-
 * Copyright (c) 1995, 2000, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden and Emmanuel Dreyfus.
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

#ifndef _MIPS_LINUX_MACHDEP_H
#define _MIPS_LINUX_MACHDEP_H

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_siginfo.h>

/*
 * From Linux's include/asm-mips64/sigcontext.h
 */
#ifndef __mips_o32
struct linux_sigcontext {		/* N32 too */
	uint64_t lsc_regs[32];
	uint64_t lsc_fpregs[32];
	uint64_t lsc_mdhi;
	uint64_t lsc_hi1;
	uint64_t lsc_hi2;
	uint64_t lsc_hi3;
	uint64_t lsc_mdlo;
	uint64_t lsc_lo1;
	uint64_t lsc_lo2;
	uint64_t lsc_lo3;
	uint64_t lsc_pc;
	uint32_t lsc_fpc_csr;
	uint32_t lsc_ownedfp;
	uint32_t lsc_dsp;
	uint32_t lsc_reserved;
};
#endif

/*
 * From Linux's include/asm-mips/sigcontext.h
 */
struct
#ifdef __mips_o32
	linux_sigcontext
#else
	linux_sigcontext32
#endif
{
	uint32_t lsc_regmask;		/* Unused */
	uint32_t lsc_status;
	uint64_t lsc_pc;
	uint64_t lsc_regs[32];
	uint64_t lsc_fpregs[32];	/* Unused */
	uint32_t lsc_acx;		/* Was owned_fp */
	uint32_t lsc_fpc_csr;		/* Unused */
	uint32_t lsc_fpc_eir;		/* Unused */
	uint32_t lsc_used_math;		/* Unused */
	uint32_t lsc_dsp;		/* dsp status; was ssflags */
	uint64_t lsc_mdhi;
	uint64_t lsc_mdlo;
	uint32_t lsc_hi1;  		/* Unused; was cause */
	uint32_t lsc_lo1;	  	/* Unused; was badvddr */
	uint32_t lsc_sigset[4]; 	/* kernel's sigset_t */
};

/*
 * From Linux's include/asm-mips/elf.h
 */
#define LINUX_ELF_NGREG 45
#define LINUX_ELF_NFPREG 33
typedef unsigned long linux_elf_greg_t;
typedef linux_elf_greg_t linux_elf_gregset_t[LINUX_ELF_NGREG];

#ifndef __mips_o32
typedef struct linux_sigaltstack32 {
	int32_t ss_sp;
	uint32_t ss_size;
	int ss_flags;
} linux_stack32_t;
#endif /* !mips_o32 */

/*
 * From Linux's arch/mips/kernel/signal.c
 */
struct linux_sigframe {
	uint32_t lsf_ass[4];
	uint32_t lsf_code[2];
	struct linux_sigcontext lsf_sc;
	linux_sigset_t lsf_mask;
};

#ifndef __mips_o32
struct linux_sigframe32 {
	uint32_t lsf_ass[4];
	uint32_t lsf_code[2];
	struct linux_sigcontext32 lsf_sc;
	linux_sigset_t lsf_mask;
};
#endif /* !mips_o32 */

/*
 * From Linux's include/asm-mips/ucontext.h
 */
struct linux_ucontext {
	unsigned long luc_flags;
	struct linux_ucontext *luc_link;
	linux_stack_t luc_stack;
	struct linux_sigcontext luc_mcontext;
	linux_sigset_t luc_sigmask;
};

#ifndef __mips_o32
struct linux_ucontext32 {
	uint32_t luc_flags;
	int32_t luc_link;
	linux_stack32_t luc_stack;
	struct linux_sigcontext32 luc_mcontext;
	linux_sigset_t luc_sigmask;
};

struct linux_ucontextn32 {
	uint32_t luc_flags;
	int32_t luc_link;
	linux_stack32_t luc_stack;
	struct linux_sigcontext luc_mcontext;
	linux_sigset_t luc_sigmask;
};

#endif /* !__mips_o32 */

/*
 * From Linux's arch/mips/kernel/signal.c
 */
struct linux_rt_sigframe {
	uint32_t lrs_ass[4];
	uint32_t lrs_code[2];
	struct linux_siginfo lrs_info;
	struct linux_ucontext lrs_uc;
};

#ifndef __mips_o32
/*
 * From Linux's arch/mips/kernel/signal.c
 */
struct linux_rt_sigframe32 {
	uint32_t lrs_ass[4];
	uint32_t lrs_code[2];
	struct linux_siginfo lrs_info;
	struct linux_ucontext32 lrs_uc;
};
#endif /* !__mips_o32 */

/*
 * From Linux's include/asm-mips/sysmips.h
 */
#define LINUX_SETNAME		1	/* set hostname                  */
#define LINUX_FLUSH_CACHE	3	/* writeback and invalidate caches */
#define LINUX_MIPS_FIXADE	7	/* control address error fixing  */
#define LINUX_MIPS_RDNVRAM	10	/* read NVRAM */
#define LINUX_MIPS_ATOMIC_SET	2001	/* atomically set variable       */

/*
 * From Linux's include/linux/utsname.h
 */
#define LINUX___NEW_UTS_LEN	64

/*
 * Major device numbers of VT device on both Linux and NetBSD. Used in
 * ugly patch to fake device numbers.
 *
 * LINUX_CONS_MAJOR is from Linux's include/linux/major.h
 */
#define LINUX_CONS_MAJOR 4
#define NETBSD_WSCONS_MAJOR 47 /* XXX */

/*
 * Linux ioctl calls for the keyboard.
 *
 * From Linux's include/linux/kd.h
 */
#define LINUX_KDGKBMODE	0x4b44
#define LINUX_KDSKBMODE	0x4b45
#define LINUX_KDMKTONE	0x4b30
#define LINUX_KDSETMODE	0x4b3a
#define LINUX_KDENABIO	0x4b36
#define LINUX_KDDISABIO	0x4b37
#define LINUX_KDGETLED	0x4b31
#define LINUX_KDSETLED	0x4b32
#define LINUX_KDGKBTYPE	0x4B33
#define LINUX_KDGKBENT	0x4B46

/*
 * Mode for KDSKBMODE which we don't have (we just use plain mode for this)
 *
 * From Linux's include/linux/kd.h
 */
#define LINUX_K_MEDIUMRAW 2

/*
 * VT ioctl calls in Linux (the ones that the pcvt emulation in
 * wscons can handle)
 *
 * From Linux's include/linux/vt.h
 */
#define LINUX_VT_OPENQRY	0x5600
#define LINUX_VT_GETMODE	0x5601
#define LINUX_VT_SETMODE	0x5602
#define LINUX_VT_GETSTATE	0x5603
#define LINUX_VT_RELDISP	0x5605
#define LINUX_VT_ACTIVATE	0x5606
#define LINUX_VT_WAITACTIVE 	0x5607
#define LINUX_VT_DISALLOCATE	0x5608

/*
 * This range used by VMWare (XXX)
 *
 * From Linux's include/linux/vt.h
 * XXX not needed for mips
 */
#define LINUX_VMWARE_NONE 200
#define LINUX_VMWARE_LAST 237

/*
 * Range of ioctls to just pass on, so that modules (like VMWare) can
 * handle them.
 *
 * From Linux's include/linux/vt.h
 */
#define LINUX_IOCTL_MIN_PASS LINUX_VMWARE_NONE
#define LINUX_IOCTL_MAX_PASS (LINUX_VMWARE_LAST+8)

#ifdef _KERNEL
__BEGIN_DECLS /* XXX from NetBSD/i386. Not arch dependent? */
void linux_syscall_intern(struct proc *);
__END_DECLS
#endif /* !_KERNEL */

#endif /* _MIPS_LINUX_MACHDEP_H */
