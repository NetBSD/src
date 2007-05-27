/*	$NetBSD: linux_machdep.h,v 1.29.30.1 2007/05/27 14:35:02 ad Exp $	*/

/*-
 * Copyright (c) 1995, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden.
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

#ifndef _I386_LINUX_MACHDEP_H
#define _I386_LINUX_MACHDEP_H

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_siginfo.h>

/*
 * The Linux sigcontext, pretty much a standard 386 trapframe.
 */
struct linux_fpreg {
	uint16_t	mant[4];
	uint16_t	expo;
};

struct linux_fpxreg {
	uint16_t	mant[4];
	uint16_t	expo;
	uint16_t	pad[3];
};

struct linux_xmmreg {
	uint32_t	reg[4];
};

struct linux_fpstate {
	uint32_t	cw;
	uint32_t	sw;
	uint32_t	tag;
	uint32_t	ipoff;
	uint32_t	cssel;
	uint32_t	dataoff;
	uint32_t	datasel;
	struct linux_fpreg	st[8];
	uint16_t	status;
	uint16_t	magic;
	uint32_t	fxsr_env[6];
	uint32_t	mxcsr;
	uint32_t	reserved;
	struct linux_fpxreg	fxsr_st[8];
	struct linux_xmmreg	xmm[8];
	uint32_t	padding[56];
};


struct linux_sigcontext {
	int	sc_gs;
	int	sc_fs;
	int	sc_es;
	int	sc_ds;
	int	sc_edi;
	int	sc_esi;
	int	sc_ebp;
	int	sc_esp;
	int	sc_ebx;
	int	sc_edx;
	int	sc_ecx;
	int	sc_eax;
	int	sc_trapno;
	int	sc_err;
	int	sc_eip;
	int	sc_cs;
	int	sc_eflags;
	int	sc_esp_at_signal;
	int	sc_ss;
	struct linux_fpstate *sc_387;
	/* XXX check this */
	linux_old_sigset_t sc_mask;
	int	sc_cr2;
};

struct linux_libc_fpreg {
	unsigned short  significand[4];
	unsigned short  exponent;
};

struct linux_libc_fpstate {
	unsigned long cw;
	unsigned long sw;
	unsigned long tag;
	unsigned long ipoff;
	unsigned long cssel;
	unsigned long dataoff;
	unsigned long datasel;
	struct linux_libc_fpreg _st[8];
	unsigned long status;
};

struct linux_ucontext {
	unsigned long	uc_flags;
	struct ucontext	*uc_link;
	struct linux_sigaltstack uc_stack;
	struct linux_sigcontext uc_mcontext;
	linux_sigset_t uc_sigmask;
	struct linux_libc_fpstate uc_fpregs_mem;
};

/*
 * We make the stack look like Linux expects it when calling a signal
 * handler, but use the BSD way of calling the handler and sigreturn().
 * This means that we need to pass the pointer to the handler too.
 * It is appended to the frame to not interfere with the rest of it.
 */
struct linux_rt_sigframe {
	int	sf_sig;
	struct	linux_siginfo  *sf_sip;
	struct	linux_ucontext *sf_ucp;
	struct	linux_siginfo  sf_si;
	struct	linux_ucontext sf_uc;
	sig_t	sf_handler;
};

struct linux_sigframe {
	int	sf_sig;
	struct	linux_sigcontext sf_sc;
	sig_t	sf_handler;
};

/*
 * Used in ugly patch to fake device numbers.
 */
/* Major device numbers for new style ptys. */
#define LINUX_PTC_MAJOR		2
#define LINUX_PTS_MAJOR		3
/* Major device numbers of VT device on both Linux and NetBSD. */
#define LINUX_CONS_MAJOR   	4

/*
 * Linux ioctl calls for the keyboard.
 */
#define LINUX_KDGKBMODE   0x4b44
#define LINUX_KDSKBMODE   0x4b45
#define LINUX_KDMKTONE    0x4b30
#define LINUX_KDSETMODE   0x4b3a
#define LINUX_KDGETMODE   0x4b3b
#define LINUX_KDENABIO    0x4b36
#define LINUX_KDDISABIO   0x4b37
#define LINUX_KDGETLED    0x4b31
#define LINUX_KDSETLED    0x4b32
#define LINUX_KDGKBTYPE   0x4b33
#define LINUX_KDGKBENT    0x4b46
#define LINUX_KIOCSOUND   0x4b2f

/*
 * Mode for KDSKBMODE which we don't have (we just use plain mode for this)
 */
#define LINUX_K_MEDIUMRAW 2

/*
 * VT ioctl calls in Linux (the ones that the pcvt emulation in wscons can handle)
 */
#define LINUX_VT_OPENQRY    0x5600
#define LINUX_VT_GETMODE    0x5601
#define LINUX_VT_SETMODE    0x5602
#define LINUX_VT_GETSTATE   0x5603
#define LINUX_VT_RELDISP    0x5605
#define LINUX_VT_ACTIVATE   0x5606
#define LINUX_VT_WAITACTIVE 0x5607
#define LINUX_VT_DISALLOCATE	0x5608

/*
 * This range used by VMWare (XXX)
 */
#define LINUX_VMWARE_NONE 200
#define LINUX_VMWARE_LAST 237

/*
 * Range of ioctls to just pass on, so that LKMs (like VMWare) can
 * handle them.
 */
#define LINUX_IOCTL_MIN_PASS	LINUX_VMWARE_NONE
#define LINUX_IOCTL_MAX_PASS	(LINUX_VMWARE_LAST+8)

#define LINUX_UNAME_ARCH	linux_get_uname_arch()

#ifdef _KERNEL
__BEGIN_DECLS
void linux_syscall_intern(struct proc *);
const char *linux_get_uname_arch(void);
__END_DECLS
#endif /* !_KERNEL */

#endif /* _I386_LINUX_MACHDEP_H */
