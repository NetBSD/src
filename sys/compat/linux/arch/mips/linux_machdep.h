/* $NetBSD: linux_machdep.h,v 1.2.6.2 2001/09/21 22:35:16 nathanw Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <compat/linux/common/linux_signal.h>

#if defined(ELFSIZE) && (ELFSIZE == 64)
struct linux_sigcontext {
	unsigned long long sc_regs[32];
	unsigned long long sc_fpregs[32];
	unsigned long long sc_mdhi;
	unsigned long long sc_mdlo;
	unsigned long long sc_pc;
	unsigned int sc_status;
	unsigned int sc_ownedfp;
	unsigned int sc_fpc_csr;
	unsigned int sc_fpc_eir;
	unsigned int sc_cause;
	unsigned int sc_badvaddr;
}
#else
/* 
 * From Linux's include/asm-mips64/sigcontext.h 
 */
struct linux_sigcontext { 
	unsigned int lsc_regmask;		/* Unused */
	unsigned int lsc_status;
	unsigned long long lsc_pc;
	unsigned long long lsc_regs[32];
	unsigned long long lsc_fpregs[32];	/* Unused */
	unsigned int lsc_ownedfp;
	unsigned int lsc_fpc_csr;		/* Unused */
	unsigned int lsc_fpc_eir;		/* Unused */
	unsigned int lsc_ssflags;		/* Unused */
	unsigned long long lsc_mdhi;
	unsigned long long lsc_mdlo;
	unsigned int lsc_cause;	  		/* Unused */
	unsigned int lsc_badvaddr;	  	/* Unused */
	unsigned long lsc_sigset[4]; 		/* kernel's sigset_t */	
};
#endif

/*
 * From Linux's include/asm-mips/elf.h
 */
#define LINUX_ELF_NGREG 45
#define LINUX_ELF_NFPREG 33
typedef unsigned long linux_elf_greg_t;
typedef linux_elf_greg_t linux_elf_gregset_t[LINUX_ELF_NGREG];

/*
 * From Linux's arch/mips/kernel/signal.c
 */
struct linux_sigframe {
	unsigned int lsf_ass[4];
	unsigned int lsf_code[2];
	struct linux_sigcontext lsf_sc;
	sigset_t lsf_mask;
};

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

/*
 * From Linux's arch/mips/kernel/signal.c, the real rt_sigframe
 */
struct linux_rt_sigframe
{
	unsigned int lrs_ass[4];
	unsigned int lrs_code[2];
	struct linux_siginfo lrs_info;
	struct linux_ucontext lrs_uc;
};

#ifdef _KERNEL
__BEGIN_DECLS				
void linux_sendsig __P((sig_t, int, sigset_t *, u_long));
dev_t linux_fakedev __P((dev_t));
__END_DECLS
#endif /* _KERNEL */

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
 * Range of ioctls to just pass on, so that LKMs (like VMWare) can
 * handle them.
 * 
 * From Linux's include/linux/vt.h
 */
#define LINUX_IOCTL_MIN_PASS LINUX_VMWARE_NONE
#define LINUX_IOCTL_MAX_PASS (LINUX_VMWARE_LAST+8)

#ifdef _KERNEL
__BEGIN_DECLS /* XXX from NetBSD/i386. Not arch dependent? */
void linux_syscall_intern __P((struct proc *));
__END_DECLS
#endif /* !_KERNEL */

#endif /* _MIPS_LINUX_MACHDEP_H */
