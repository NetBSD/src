/* $NetBSD: proc.h,v 1.21.20.1 2014/08/10 06:53:49 tls Exp $ */

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

#ifndef _ALPHA_PROC_H
#define _ALPHA_PROC_H

#include <machine/frame.h>
#include <machine/pcb.h>

/*
 * Machine-dependent part of the lwp struct for the Alpha.
 */
struct mdlwp {
	u_long md_flags;		/* see below */
	struct trapframe *md_tf;	/* trap/syscall registers */
	struct pcb *md_pcbpaddr;	/* phys addr of the pcb */
	volatile int md_astpending;	/* AST pending for this thread */
};

/*
 * md_flags usage
 * --------------
 * MDLWP_FPUSED
 * 	A largely unused bit indicating the presence of FPU history.
 * 	Cleared on exec. Set but not used by the fpu context switcher
 * 	itself.
 *
 * MDLWP_FP_C
 * 	The architected FP Control word. It should forever begin at bit 1,
 * 	as the bits are AARM specified and this way it doesn't need to be
 * 	shifted.
 *
 * 	Until C99 there was never an IEEE 754 API, making most of the
 * 	standard useless.  Because of overlapping AARM, OSF/1, NetBSD, and
 * 	C99 API's, the use of the MDLWP_FP_C bits is defined variously in
 * 	ieeefp.h and fpu.h.
 */
#define	MDLWP_FP_C	0x007ffffe	/* Extended FP_C Quadword bits */
#define	MDLWP_FPACTIVE	__BIT(63)	/* FPU is active on LWP's PCU CPU */

/*
 * Machine-dependent part of the proc struct for the Alpha.
 */
struct lwp;
struct mdproc {
					/* this process's syscall vector */
	void	(*md_syscall)(struct lwp *, uint64_t, struct trapframe *);
};

#endif /* !_ALPHA_PROC_H_ */
