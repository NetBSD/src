/*	$NetBSD: linux_trap.c,v 1.8 2003/09/25 22:01:31 christos Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

/*
 * 386 Trap and System call handling
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_trap.c,v 1.8 2003/09/25 22:01:31 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/savar.h>
#include <sys/user.h>
#include <sys/acct.h>
#include <sys/kernel.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/syscall.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/trap.h>
#include <machine/userret.h>

#include <compat/linux/common/linux_exec.h>

#define LINUX_T_DIVIDE			0
#define LINUX_T_DEBUG			1
#define LINUX_T_NMI			2
#define LINUX_T_INT3			3
#define LINUX_T_OVERFLOW		4
#define LINUX_T_BOUNDS			5
#define LINUX_T_INVALID_OP		6
#define LINUX_T_DEVICE_NOT_AVAIL	7
#define LINUX_T_DOUBLE_FAULT		8
#define LINUX_T_COPROC_SEG_OVERRUN	9
#define LINUX_T_INVALID_TSS		10
#define LINUX_T_SEG_NOT_PRESENT		11
#define LINUX_T_STACK_SEG_FAULT		12
#define LINUX_T_GENERAL_PROT_FAULT	13
#define LINUX_T_PAGE_FAULT		14
#define LINUX_T_SPURIOUS_INTERRUPT	15
#define LINUX_T_COPROC_ERROR		16
#define LINUX_T_ALIGN_CHECK		17
#define LINUX_T_MACHINE_CHECK		18	/* XXX */
#define LINUX_T_SIMD_COPROC_ERROR	19	/* XXX */

/* Note 255 is bogus */
static int trapno_to_x86_vec[] = {
 	LINUX_T_INVALID_OP,		/*  0 T_PRIVINFLT */
 	LINUX_T_INT3,			/*  1 T_BPTFLT */
 	LINUX_T_COPROC_ERROR,		/*  2 T_ARITHTRAP */
 	LINUX_T_SPURIOUS_INTERRUPT,	/*  3 T_ASTFLT XXX: ??? */
 	LINUX_T_GENERAL_PROT_FAULT,	/*  4 T_PROTFLT */
 	LINUX_T_DEBUG,			/*  5 T_TRCTRAP */
 	LINUX_T_PAGE_FAULT,		/*  6 T_PAGEFLT */
 	LINUX_T_ALIGN_CHECK,		/*  7 T_ALIGNFLT */
 	LINUX_T_DIVIDE,			/*  8 T_DIVIDE */
 	LINUX_T_NMI,			/*  9 T_NMI */
 	LINUX_T_OVERFLOW,		/* 10 T_OFLOW */
 	LINUX_T_BOUNDS,			/* 11 T_BOUND */
 	LINUX_T_DEVICE_NOT_AVAIL,	/* 12 T_DNA */
 	LINUX_T_DOUBLE_FAULT,		/* 13 T_DOUBLEFLT */
 	LINUX_T_COPROC_SEG_OVERRUN,	/* 14 T_FPOPFLT */
 	LINUX_T_INVALID_TSS,		/* 15 T_TSSFLT */
 	LINUX_T_SEG_NOT_PRESENT,	/* 16 T_SEGNPFLT */
 	LINUX_T_STACK_SEG_FAULT,	/* 17 T_STKFLT */
 	LINUX_T_MACHINE_CHECK		/* 18 T_RESERVED XXX: ??? */
};

/* For the nmi and reserved below linux does not post a signal. */
static int linux_x86_vec_to_sig[] = {
	SIGFPE,				/*  0 LINUX_T_DIVIDE */
	SIGTRAP,			/*  1 LINUX_T_DEBUG */
/*nmi*/	SIGSEGV,			/*  2 LINUX_T_NMI */
	SIGTRAP,			/*  3 LINUX_T_INT3 */
 	SIGSEGV,			/*  4 LINUX_T_OVERFLOW */
	SIGSEGV,			/*  5 LINUX_T_BOUNDS */
	SIGILL,				/*  6 LINUX_T_INVALIDOP */
	SIGSEGV,			/*  7 LINUX_T_DEVICE_NOT_AVAIL */
 	SIGSEGV,			/*  8 LINUX_T_DOUBLE_FAULT */
	SIGFPE,				/*  9 LINUX_T_COPROC_SEG_OVERRUN */
	SIGSEGV,			/* 10 LINUX_T_INVALID_TSS */
	SIGBUS,				/* 11 LINUX_T_SEG_NOT_PRESENT */
 	SIGBUS,				/* 12 LINUX_T_STACK_SEG_FAULT */
	SIGSEGV,			/* 13 LINUX_T_GENERAL_PROT_FAULT */
	SIGSEGV,			/* 14 LINUX_T_PAGE_FAULT */
/*resv*/SIGSEGV,			/* 15 LINUX_T_SPURIOUS_INTERRUPT */
 	SIGFPE,				/* 16 LINUX_T_COPROC_ERROR */
	SIGSEGV,			/* 17 LINUX_T_ALIGN_CHECK */
	SIGSEGV				/* 18 LINUX_T_MACHINE_CHECK */
};

#define ASIZE(a) (sizeof(a) / sizeof(a[0]))

void
linux_trapsignal(struct lwp *l, const ksiginfo_t *ksi)
{

	switch (ksi->ksi_signo) {
	case SIGILL:
	case SIGTRAP:
	case SIGIOT:
	case SIGBUS:
	case SIGFPE:
	case SIGSEGV:
		if (ksi->ksi_trap <= ASIZE(trapno_to_x86_vec)) {
			ksiginfo_t nksi = *ksi;
			nksi.ksi_trap = trapno_to_x86_vec[ksi->ksi_trap];
			if (nksi.ksi_trap <= ASIZE(linux_x86_vec_to_sig)) {
				nksi.ksi_signo 
				    = linux_x86_vec_to_sig[nksi.ksi_trap];
			} else {
				uprintf("Unhandled sig type %d\n",
				    ksi->ksi_trap);
			}
			ksi = &nksi;
		} else {
			uprintf("Unhandled trap type %d\n", ksi->ksi_trap);
		}
		/*FALLTHROUGH*/
	default:
		trapsignal(l, ksi);
		return;
	}
}
