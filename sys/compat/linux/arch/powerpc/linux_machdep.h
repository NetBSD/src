/*	$NetBSD: linux_machdep.h,v 1.1.2.3 2001/04/21 17:46:18 bouyer Exp $ */

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

#ifndef _POWERPC_LINUX_MACHDEP_H
#define _POWERPC_LINUX_MACHDEP_H

#include <compat/linux/common/linux_signal.h>
/* 
 * From Linux's include/asm-ppc/ptrace.h 
 * Needed for sigcontext 
 */
struct linux_pt_regs {
	unsigned long lgpr[32];  
	unsigned long lnip;
	unsigned long lmsr;
	unsigned long lorig_gpr3;	/* Used for restarting system calls */
	unsigned long lctr;
	unsigned long llink;
	unsigned long lxer;
	unsigned long lccr;
	unsigned long lmq;	 	/* 601 only (not used at present) */
	/* Used on APUS to hold IPL value. */
	unsigned long ltrap;	 	/* Reason for being here */
	unsigned long ldar;	 	/* Fault registers */
	unsigned long ldsisr;
	unsigned long lresult;	/* Result of a system call */
};

/* 
 * From Linux's include/asm-ppc/sigcontext.h 
 * Linux/ppc calls that struct sigcontect_struct 
 */
struct linux_sigcontext { 
	unsigned long _unused[4];
	int lsignal;  
	unsigned long lhandler;
	unsigned long lmask;
	struct linux_pt_regs	*lregs;
};

/*
 * From Linux's include/asm-ppc/elf.h
 */
#define LINUX_ELF_NGREG 48		 	/* includes nip, msr, lr, etc. */
#define LINUX_ELF_NFPREG 33		/* includes fpscr */
typedef unsigned long linux_elf_greg_t;
typedef linux_elf_greg_t linux_elf_gregset_t[LINUX_ELF_NGREG];

/*
 * From Linux's include/asm-ppc/ptrace.h
 */
#define LINUX__SIGNAL_FRAMESIZE 64

/*
 * We make the stack look like Linux expects it when calling a signal
 * handler, but use the BSD way of calling the handler and sigreturn().
 * This means that we need to pass the pointer to the handler too.
 * It is appended to the frame to not interfere with the rest of it.
 * 
 * The following is from Linux's arch/ppc/kern/signal.c:
 * 
 * > When we have signals to deliver, we set up on the
 * > user stack, going down from the original stack pointer:
 * > a sigregs struct
 * > one or more sigcontext structs with
 * > a gap of LINUX__SIGNAL_FRAMESIZE bytes
 * > 
 * > Each of these things must be a multiple of 16 bytes in size.
 *  
 * linux_sigregs is a linux_sigframe on other Linux ports. Linux/PowerPC
 * defines a rt_sigframe struct, but it is only used for RT signals. for
 * non RT signals, struct sigregs is used instead.
 *
 * About the ltramp filed: that trampoline code is not used. Instead, the
 * sigcode (7 bytes long) trampoline code, copied by exec() on program startup 
 * is used. However, Linux binaries might expect it to be here.  
 */
struct linux_sigregs {
	linux_elf_gregset_t lgp_regs;
	double lfp_regs[LINUX_ELF_NFPREG];
	unsigned long ltramp[2]; 
	/* 
	 * Programs using the rs6000/xcoff abi can save up to 19 gp regs
	 * and 18 fp regs below sp before decrementing it. 
	 */
	int labigap[56];
};

/* 
 * The following is not from Linux, we define it for convenience. It is the
 * size of the abigap field of linux_sigregs.
 */
#define LINUX_ABIGAP	(56*sizeof(int))

/* 
 * linux sigframe in a nutshell (however we don't use it):
 *
 * struct linux_sigframe {
 *		char _gap[LINUX__SIGNAL_FRAMESIZE];
 *		struct sigcontext lsc;
 *		struct linux_sigregs	lsg;
 * };
 */

/*
 * From Linux's include/asm-ppc/ucontext.h
 */
struct linux_ucontext {
	unsigned long luc_flags;
	struct linux_ucontext *luc_link;
	linux_stack_t	luc_stack;
	struct linux_sigcontext luc_context;
	linux_sigset_t	luc_sigmask;  /* mask last for extensibility */
};

/*
 * From Linux's arch/ppc/kernel/signal.c, the real rt_sigframe
 */
struct linux_rt_sigframe
{
	unsigned long  _unused[2];
	struct linux_siginfo *lpinfo;
	void *lpuc;
	struct linux_siginfo linfo; 
	struct linux_ucontext luc;
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
#define NETBSD_WSCONS_MAJOR 47

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
 * XXX It's not sure this s needed for powerpc 
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

#endif /* _POWERPC_LINUX_MACHDEP_H */
