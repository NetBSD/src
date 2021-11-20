/*	$NetBSD: __sigtramp2.S,v 1.8 2021/11/20 19:26:20 thorpej Exp $	*/

/*
 * Copyright (c) 1994, 1995, 1996 Carnegie-Mellon University.
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

#include "SYS.h"
#include "assym.h"

/*
 * The Alpha signal trampoline is invoked only to return from
 * the signal; the kernel calls the signal handler directly.
 *
 * On entry, the stack looks like:
 *
 *		ucontext structure	[128] == sp + sizeof(siginfo_t)]
 *	sp->	siginfo structure	[0]
 *
 * The DWARF register numbers for the general purpose registers are the
 * same as the architected register numbers.  For Alpha, there is a DWARF
 * pseudo-register for signal handler return addresses.
 */

#if defined(__LIBGCC_DWARF_ALT_FRAME_RETURN_COLUMN__)
#define	DWARF_SIGRETURN_REG		__LIBGCC_DWARF_ALT_FRAME_RETURN_COLUMN__
#else
#define	DWARF_SIGRETURN_REG		64
#endif

#define	CFI_OFFSET_DWARF_REG(d, r)	.cfi_offset d, r*8
#define	CFI_OFFSET(r)			CFI_OFFSET_DWARF_REG(r, r)

	.cfi_startproc simple
	.cfi_signal_frame
	.cfi_def_cfa _REG_SP, SIZEOF_SIGINFO + UC_GREGS
	CFI_OFFSET(_REG_V0)
	CFI_OFFSET(_REG_T0)
	CFI_OFFSET(_REG_T1)
	CFI_OFFSET(_REG_T2)
	CFI_OFFSET(_REG_T3)
	CFI_OFFSET(_REG_T4)
	CFI_OFFSET(_REG_T5)
	CFI_OFFSET(_REG_T6)
	CFI_OFFSET(_REG_T7)
	CFI_OFFSET(_REG_S0)
	CFI_OFFSET(_REG_S1)
	CFI_OFFSET(_REG_S2)
	CFI_OFFSET(_REG_S3)
	CFI_OFFSET(_REG_S4)
	CFI_OFFSET(_REG_S5)
	CFI_OFFSET(_REG_S6)
	CFI_OFFSET(_REG_A0)
	CFI_OFFSET(_REG_A1)
	CFI_OFFSET(_REG_A2)
	CFI_OFFSET(_REG_A3)
	CFI_OFFSET(_REG_A4)
	CFI_OFFSET(_REG_A5)
	CFI_OFFSET(_REG_T8)
	CFI_OFFSET(_REG_T9)
	CFI_OFFSET(_REG_T10)
	CFI_OFFSET(_REG_T11)
	CFI_OFFSET(_REG_RA)
	CFI_OFFSET(_REG_T12)	/* a.k.a. _REG_PV */
	CFI_OFFSET(_REG_AT)
	CFI_OFFSET(_REG_GP)
	CFI_OFFSET(_REG_SP)
	.cfi_return_column DWARF_SIGRETURN_REG
	CFI_OFFSET_DWARF_REG(DWARF_SIGRETURN_REG, _REG_PC)

/*
 * The unwind entry includes one instruction slot prior to the trampoline
 * because the unwinder will look up to (return PC - 1 insn) while unwinding.
 * Normally this would be the jump / branch, but since there isn't one in
 * this case, we place an explicit nop there instead.
 */
	nop

NESTED_NOPROFILE(__sigtramp_siginfo_2,0,0,ra,0,0)
	ldgp	gp,0(ra)
	lda	a0,(SIZEOF_SIGINFO)(sp)	/* get pointer to ucontext */
	CALLSYS_NOERROR(setcontext)	/* and call setcontext() with it */
	ldiq	a0,-1			/* if that failed, set an exit code */
	CALLSYS_NOERROR(exit)		/* and call exit() */
	.cfi_endproc
END(__sigtramp_siginfo_2)
