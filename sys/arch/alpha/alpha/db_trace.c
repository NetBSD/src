/* $NetBSD: db_trace.c,v 1.9.2.1 2001/08/30 23:43:39 nathanw Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ross Harvey.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: db_trace.c,v 1.9.2.1 2001/08/30 23:43:39 nathanw Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>

#include <machine/alpha.h>
#include <machine/db_machdep.h>

#include <alpha/alpha/db_instruction.h>

#include <ddb/db_sym.h> 
#include <ddb/db_access.h>
#include <ddb/db_variables.h>
#include <ddb/db_output.h>
#include <ddb/db_interface.h>

/*
 * Information about the `standard' Alpha function prologue.
 */
struct prologue_info {
	int	pi_reg_offset[32]; /* offset of registers in stack frame */
	u_int32_t pi_regmask;	   /* which registers are in frame */
	int	pi_frame_size;	   /* frame size */
};

/*
 * We use several symbols to take special action:
 *
 *	Trap vectors, which use a different (fixed-size) stack frame:
 *
 *		XentArith
 *		XentIF
 *		XentInt
 *		XentMM
 *		XentSys
 *		XentUna
 */

static struct special_symbol {
	vaddr_t ss_val;
	const char *ss_note;
} special_symbols[] = {
	{ (vaddr_t)&XentArith,		"arithmetic trap" },
	{ (vaddr_t)&XentIF,		"instruction fault" },
	{ (vaddr_t)&XentInt,		"interrupt" },
	{ (vaddr_t)&XentMM,		"memory management fault" },
	{ (vaddr_t)&XentSys,		"syscall" },
	{ (vaddr_t)&XentUna,		"unaligned access fault" },
	{ (vaddr_t)&XentRestart,	"console restart" },
	{ NULL }
};

/*
 * Decode the function prologue for the function we're in, and note
 * which registers are stored where, and how large the stack frame is.
 */
static void
decode_prologue(db_addr_t callpc, db_addr_t func,
    struct prologue_info *pi, void (*pr)(const char *, ...))
{
	long signed_immediate;
	alpha_instruction ins;
	db_expr_t pc;

	pi->pi_regmask = 0;
	pi->pi_frame_size = 0;

#define	CHECK_FRAMESIZE							\
do {									\
	if (pi->pi_frame_size != 0) {					\
		(*pr)("frame size botch: adjust register offsets?\n"); \
	}								\
} while (0)

	for (pc = func; pc < callpc; pc += sizeof(alpha_instruction)) {
		ins.bits = *(unsigned int *)pc;

		if (ins.mem_format.opcode == op_lda &&
		    ins.mem_format.ra == 30 &&
		    ins.mem_format.rb == 30) {
			/*
			 * GCC 2.7-style stack adjust:
			 *
			 *	lda	sp, -64(sp)
			 */
			signed_immediate = (long)ins.mem_format.displacement;
#if 1
			if (signed_immediate > 0)
				(*pr)("prologue botch: displacement %ld\n",
				    signed_immediate);
#endif
			CHECK_FRAMESIZE;
			pi->pi_frame_size += -signed_immediate;
		} else if (ins.operate_lit_format.opcode == op_arit &&
			   ins.operate_lit_format.function == op_subq &&
			   ins.operate_lit_format.ra == 30 &&
			   ins.operate_lit_format.rc == 30) {
			/*
			 * EGCS-style stack adjust:
			 *
			 *	subq	sp, 64, sp
			 */
			CHECK_FRAMESIZE;
			pi->pi_frame_size += ins.operate_lit_format.literal;
		} else if (ins.mem_format.opcode == op_stq &&
			   ins.mem_format.rb == 30 &&
			   ins.mem_format.ra != 31) {
			/* Store of (non-zero) register onto the stack. */
			signed_immediate = (long)ins.mem_format.displacement;
			pi->pi_regmask |= 1 << ins.mem_format.ra;
			pi->pi_reg_offset[ins.mem_format.ra] = signed_immediate;
		}
	}
}

static int
sym_is_trapsymbol(vaddr_t v)
{
	int i;

	for (i = 0; special_symbols[i].ss_val != NULL; ++i)
		if (v == special_symbols[i].ss_val)
			return 1;
	return 0;
}

static void
decode_syscall(int number, struct proc *p, void (*pr)(const char *, ...))
{

	(*pr)(" (%d)", number);
}

void
db_stack_trace_print(db_expr_t addr, boolean_t have_addr, db_expr_t count,
    char *modif, void (*pr)(const char *, ...))
{
	db_addr_t callpc, frame, symval;
	struct prologue_info pi;
	db_expr_t diff;
	db_sym_t sym;
	int i;
	u_long tfps;
	char *symname;
	struct pcb *pcbp;
	char c, *cp = modif;
	struct trapframe *tf;
	boolean_t ra_from_tf;
	boolean_t ra_from_pcb;
	u_long last_ipl = ~0L;
	struct proc *p = NULL;
	struct lwp *l = NULL;
	boolean_t trace_thread = FALSE;
	boolean_t have_trapframe = FALSE;

	while ((c = *cp++) != 0)
		trace_thread |= c == 't';

	if (!have_addr) {
		p = curproc->l_proc;
		addr = DDB_REGS->tf_regs[FRAME_SP] - FRAME_SIZE * 8;
		tf = (struct trapframe *)addr;
		have_trapframe = 1;
	} else {
		if (trace_thread) {
			(*pr)("trace: pid %d ", (int)addr);
			p = pfind(addr);
			if (p == NULL) {
				(*pr)("not found\n");
				return;
			}	
			l = LIST_FIRST(&p->p_lwps); /* XXX NJWLWP */
			if ((l->l_flag & L_INMEM) == 0) {
				(*pr)("swapped out\n");
				return;
			}
			pcbp = &l->l_addr->u_pcb;
			addr = (db_expr_t)pcbp->pcb_hw.apcb_ksp;
			callpc = pcbp->pcb_context[7];
			(*pr)("at 0x%lx\n", addr);
		} else {
			(*pr)("alpha trace requires known PC =eject=\n");
			return;
		}
		frame = addr;
	}

	while (count--) {
		if (have_trapframe) {
			frame = (db_addr_t)tf + FRAME_SIZE * 8;
			callpc = tf->tf_regs[FRAME_PC];
			ra_from_tf = TRUE;
			have_trapframe = 0;
		}
		sym = db_search_symbol(callpc, DB_STGY_ANY, &diff);
		if (sym == DB_SYM_NULL)
			break;

		db_symbol_values(sym, &symname, (db_expr_t *)&symval);

		if (callpc < symval) {
			(*pr)("symbol botch: callpc 0x%lx < "
			    "func 0x%lx (%s)\n", callpc, symval, symname);
			return;
		}

		/*
		 * XXX Printing out arguments is Hard.  We'd have to
		 * keep lots of state as we traverse the frame, figuring
		 * out where the arguments to the function are stored
		 * on the stack.
		 *
		 * Even worse, they may be stored to the stack _after_
		 * being modified in place; arguments are passed in
		 * registers.
		 *
		 * So, in order for this to work reliably, we pretty much
		 * have to have a kernel built with `cc -g':
		 *
		 *	- The debugging symbols would tell us where the
		 *	  arguments are, how many there are, if there were
		 *	  any passed on the stack, etc.
		 *
		 *	- Presumably, the compiler would be careful to
		 *	  store the argument registers on the stack before
		 *	  modifying the registers, so that a debugger could
		 *	  know what those values were upon procedure entry.
		 *
		 * Because of this, we don't bother.  We've got most of the
		 * benefit of back tracking without the arguments, and we
		 * could get the arguments if we use a remote source-level
		 * debugger (for serious debugging).
		 */
		(*pr)("%s() at ", symname);
		db_printsym(callpc, DB_STGY_PROC, pr);
		(*pr)("\n");

		/*
		 * If we are in a trap vector, frame points to a
		 * trapframe.
		 */
		if (sym_is_trapsymbol(symval)) {
			tf = (struct trapframe *)frame;

			for (i = 0; special_symbols[i].ss_val != NULL; ++i)
				if (symval == special_symbols[i].ss_val)
					(*pr)("--- %s",
					    special_symbols[i].ss_note);

			tfps = tf->tf_regs[FRAME_PS];
			if (symval == (vaddr_t)&XentSys)
				decode_syscall(tf->tf_regs[FRAME_V0], p, pr);
			if ((tfps & ALPHA_PSL_IPL_MASK) != last_ipl) {
				last_ipl = tfps & ALPHA_PSL_IPL_MASK;
				if (symval != (vaddr_t)&XentSys)
					(*pr)(" (from ipl %ld)", last_ipl);
			}
			(*pr)(" ---\n");
			if (tfps & ALPHA_PSL_USERMODE) {
				(*pr)("--- user mode ---\n");
				break;	/* Terminate search.  */
			}
			have_trapframe = 1;
			continue;
		}

		/*
		 * This is a bit trickier; we must decode the function
		 * prologue to find the saved RA.
		 *
		 * XXX How does this interact w/ alloca()?!
		 */
		decode_prologue(callpc, symval, &pi, pr);
		if ((pi.pi_regmask & (1 << 26)) == 0) {
			/*
			 * No saved RA found.  We might have RA from
			 * the trap frame, however (e.g trap occurred
			 * in a leaf call).  If not, we've found the
			 * root of the call graph.
			 */
			if (ra_from_tf)
				callpc = tf->tf_regs[FRAME_RA];
			else {
				(*pr)("--- root of call graph ---\n");
				break;
			}
		} else
			callpc = *(u_long *)(frame + pi.pi_reg_offset[26]);
		ra_from_tf = ra_from_pcb = FALSE;
#if 0
		/*
		 * The call was actually made at RA - 4; the PC is
		 * updated before being stored in RA.
		 */
		callpc -= 4;
#endif
		frame += pi.pi_frame_size;
	}
}
