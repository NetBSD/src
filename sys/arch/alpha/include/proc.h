/* $NetBSD: proc.h,v 1.11 2001/07/14 17:55:42 thorpej Exp $ */

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

#include <machine/frame.h>

/*
 * Machine-dependent part of the proc struct for the Alpha.
 */
struct proc;
struct mdproc {
	u_long	md_flags;
	struct	trapframe *md_tf;	/* trap/syscall registers */
	struct pcb *md_pcbpaddr;	/* phys addr of the pcb */
					/* this process's syscall vector */
	void	(*md_syscall)(struct proc *, u_int64_t, struct trapframe *);
	__volatile int md_astpending;	/* AST pending for this process */
};
/*
 * md_flags usage
 * --------------
 * MDP_FPUSED
 * 	A largely unused bit indicating the presence of FPU history.
 * 	Cleared on exec. Set but not used by the fpu context switcher
 * 	itself.
 * 
 * MDP_FP_C
 * 	The architected FP Control word. It should forever begin at bit 1,
 * 	as the bits are AARM specified and this way it doesn't need to be
 * 	shifted.
 * 
 * 	Until C99 there was never an IEEE 754 API, making most of the
 * 	standard useless.  Because of overlapping AARM, OSF/1, NetBSD, and
 * 	C99 API's, the use of the MDP_FP_C bits is defined variously in
 * 	ieeefp.h and fpu.h.
 */
#define	MDP_FPUSED	0x00000001	/* Process used the FPU */
#define	MDP_FP_C	0x007ffffe	/* Extended FP_C Quadword bits */
