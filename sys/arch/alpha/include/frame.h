/* $NetBSD: frame.h,v 1.8.36.1 2012/04/17 00:05:55 yamt Exp $ */

/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#ifndef _ALPHA_FRAME_H_
#define	_ALPHA_FRAME_H_

#include <machine/alpha_cpu.h>
#include <sys/signal.h>

/*
 * Software trap, exception, and syscall frame.
 *
 * Includes "hardware" (PALcode) frame.
 *
 * PALcode puts ALPHA_HWFRAME_* fields on stack.  We have to add
 * all of the general-purpose registers except for zero, for sp
 * (which is automatically saved in the PCB's USP field for entries
 * from user mode, and which is implicitly saved and restored by the
 * calling conventions for entries from kernel mode), and (on traps
 * and exceptions) for a0, a1, and a2 (which are saved by PALcode).
 */

/* Quadword offsets of the registers to be saved. */
#define	FRAME_V0	0
#define	FRAME_T0	1
#define	FRAME_T1	2
#define	FRAME_T2	3
#define	FRAME_T3	4
#define	FRAME_T4	5
#define	FRAME_T5	6
#define	FRAME_T6	7
#define	FRAME_T7	8
#define	FRAME_S0	9
#define	FRAME_S1	10
#define	FRAME_S2	11
#define	FRAME_S3	12
#define	FRAME_S4	13
#define	FRAME_S5	14
#define	FRAME_S6	15
#define	FRAME_A3	16
#define	FRAME_A4	17
#define	FRAME_A5	18
#define	FRAME_T8	19
#define	FRAME_T9	20
#define	FRAME_T10	21
#define	FRAME_T11	22
#define	FRAME_RA	23
#define	FRAME_T12	24
#define	FRAME_AT	25
#define	FRAME_SP	26

#define	FRAME_SW_SIZE	(FRAME_SP + 1)
#define	FRAME_HW_OFFSET	FRAME_SW_SIZE

#define	FRAME_PS	(FRAME_HW_OFFSET + ALPHA_HWFRAME_PS)
#define	FRAME_PC	(FRAME_HW_OFFSET + ALPHA_HWFRAME_PC)
#define	FRAME_GP	(FRAME_HW_OFFSET + ALPHA_HWFRAME_GP)
#define	FRAME_A0	(FRAME_HW_OFFSET + ALPHA_HWFRAME_A0)
#define	FRAME_A1	(FRAME_HW_OFFSET + ALPHA_HWFRAME_A1)
#define	FRAME_A2	(FRAME_HW_OFFSET + ALPHA_HWFRAME_A2)

#define	FRAME_HW_SIZE	ALPHA_HWFRAME_SIZE
#define	FRAME_SIZE	(FRAME_HW_OFFSET + FRAME_HW_SIZE)

struct trapframe {
	unsigned long	tf_regs[FRAME_SIZE];	/* See above */
};

#if (defined(COMPAT_16) || defined(COMPAT_OSF1)) && defined(_KERNEL)
struct sigframe_sigcontext {
	/*  ra address of trampoline */
	/*  a0 signum for handler */
	/*  a1 code for handler */
	/*  a2 struct	sigcontext for handler */
	struct sigcontext sf_sc; /* actual saved context */
};
#endif

struct sigframe_siginfo {
	/*  ra address of trampoline */
        /*  a0 signal number arg for handler */
	/*  a1 siginfo_t * arg for handler */
	/*  a2 ucontext_t * arg for handler */
	siginfo_t sf_si; /* actual saved siginfo */
	ucontext_t sf_uc; /* actual saved ucontext */
};

#ifdef _KERNEL
void *getframe(const struct lwp *, int, int *);
void buildcontext(struct lwp *, const void *, const void *, const void *);
void sendsig_siginfo(const ksiginfo_t *, const sigset_t *);
#if defined(COMPAT_16) || defined(COMPAT_OSF1)
void sendsig_sigcontext(const ksiginfo_t *, const sigset_t *);
#endif
#endif

#endif /* _ALPHA_FRAME_H_ */
