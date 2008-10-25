/*	$NetBSD: linux_machdep.h,v 1.34 2008/10/25 23:38:28 christos Exp $	*/

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

struct linux_user_desc {
	unsigned int		entry_number;
	unsigned int		base_addr;
	unsigned int		limit;
	unsigned int		seg_32bit:1;
	unsigned int		contents:2;
	unsigned int		read_exec_only:1;
	unsigned int		limit_in_pages:1;
	unsigned int		seg_not_present:1;
	unsigned int		useable:1;
};

struct linux_desc_struct {
	unsigned long	a, b;
};


#define	LINUX_LOWERWORD	0x0000ffff

/*
 * Macros which does the same thing as those in Linux include/asm-um/ldt-i386.h.
 * These convert Linux user space descriptor to machine one.
 */
#define	LINUX_LDT_entry_a(info)					\
	((((info)->base_addr & LINUX_LOWERWORD) << 16) |	\
	((info)->limit & LINUX_LOWERWORD))

#define	LINUX_ENTRY_B_READ_EXEC_ONLY	9
#define	LINUX_ENTRY_B_CONTENTS		10
#define	LINUX_ENTRY_B_SEG_NOT_PRESENT	15
#define	LINUX_ENTRY_B_BASE_ADDR		16
#define	LINUX_ENTRY_B_USEABLE		20
#define	LINUX_ENTRY_B_SEG32BIT		22
#define	LINUX_ENTRY_B_LIMIT		23

#define	LINUX_LDT_entry_b(info)						\
	(((info)->base_addr & 0xff000000) |				\
	((info)->limit & 0xf0000) |					\
	((info)->contents << LINUX_ENTRY_B_CONTENTS) |			\
	(((info)->seg_not_present == 0) << LINUX_ENTRY_B_SEG_NOT_PRESENT) |\
	(((info)->base_addr & 0x00ff0000) >> LINUX_ENTRY_B_BASE_ADDR) |	\
	(((info)->read_exec_only == 0) << LINUX_ENTRY_B_READ_EXEC_ONLY) |\
	((info)->seg_32bit << LINUX_ENTRY_B_SEG32BIT) |			\
	((info)->useable << LINUX_ENTRY_B_USEABLE) |			\
	((info)->limit_in_pages << LINUX_ENTRY_B_LIMIT) | 0x7000)

#define	LINUX_LDT_empty(info)		\
	((info)->base_addr == 0 &&	\
	(info)->limit == 0 &&		\
	(info)->contents == 0 &&	\
	(info)->seg_not_present == 1 &&	\
	(info)->read_exec_only == 1 &&	\
	(info)->seg_32bit == 0 &&	\
	(info)->limit_in_pages == 0 &&	\
	(info)->useable == 0)

/*
 * Macros for converting segments.
 * They do the same as those in arch/i386/kernel/process.c in Linux.
 */
#define	LINUX_GET_BASE(desc)				\
	((((desc)->a >> 16) & LINUX_LOWERWORD) |	\
	(((desc)->b << 16) & 0x00ff0000) |		\
	((desc)->b & 0xff000000))

#define	LINUX_GET_LIMIT(desc)			\
	(((desc)->a & LINUX_LOWERWORD) |	\
	((desc)->b & 0xf0000))

#define	LINUX_GET_32BIT(desc)		\
	(((desc)->b >> LINUX_ENTRY_B_SEG32BIT) & 1)
#define	LINUX_GET_CONTENTS(desc)	\
	(((desc)->b >> LINUX_ENTRY_B_CONTENTS) & 3)
#define	LINUX_GET_WRITABLE(desc)	\
	(((desc)->b >> LINUX_ENTRY_B_READ_EXEC_ONLY) & 1)
#define	LINUX_GET_LIMIT_PAGES(desc)	\
	(((desc)->b >> LINUX_ENTRY_B_LIMIT) & 1)
#define	LINUX_GET_PRESENT(desc)		\
	(((desc)->b >> LINUX_ENTRY_B_SEG_NOT_PRESENT) & 1)
#define	LINUX_GET_USEABLE(desc)		\
	(((desc)->b >> LINUX_ENTRY_B_USEABLE) & 1)

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
#define LINUX_NPTL

#ifdef _KERNEL
__BEGIN_DECLS
void linux_syscall_intern(struct proc *);
const char *linux_get_uname_arch(void);
__END_DECLS
#endif /* !_KERNEL */

#endif /* _I386_LINUX_MACHDEP_H */
