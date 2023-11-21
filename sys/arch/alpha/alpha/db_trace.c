/* $NetBSD: db_trace.c,v 1.38 2023/11/21 21:23:56 thorpej Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: db_trace.c,v 1.38 2023/11/21 21:23:56 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>

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
	uint32_t pi_regmask;	   /* which registers are in frame */
	int	pi_frame_size;	   /* frame size */
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
	db_addr_t pc;

	pi->pi_regmask = 0;
	pi->pi_frame_size = 0;

#define	CHECK_FRAMESIZE							\
do {									\
	if (pi->pi_frame_size != 0) {					\
		(*pr)("frame size botch: adjust register offsets?\n"); \
	}								\
} while (0)

	for (pc = func; pc < callpc; pc += sizeof(alpha_instruction)) {
		db_read_bytes(pc, sizeof(ins.bits), (char *)&ins.bits);

		if (ins.mem_format.opcode == op_lda &&
		    ins.mem_format.ra == 30 &&
		    ins.mem_format.rb == 30) {
			/*
			 * GCC 2.7-style stack adjust:
			 *
			 *	lda	sp, -64(sp)
			 */
			signed_immediate = (long)ins.mem_format.displacement;
			/*
			 * The assumption here is that a positive
			 * stack offset is the function epilogue,
			 * which may come before callpc when an
			 * aggressive optimizer (like GCC 3.3 or later)
			 * has moved part of the function "out of
			 * line", past the epilogue. Therefore, ignore
			 * the positive offset so that
			 * pi->pi_frame_size has the correct value
			 * when we reach callpc.
			 */
			if (signed_immediate <= 0) {
				CHECK_FRAMESIZE;
				pi->pi_frame_size += -signed_immediate;
			}
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

static void
decode_syscall(int number, void (*pr)(const char *, ...))
{
	(*pr)(" (%d)", number);
}

void
db_stack_trace_print(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif, void (*pr)(const char *, ...))
{

	db_stack_trace_print_ra(/*ra*/0, /*have_ra*/false, addr, have_addr,
	    count, modif, pr);
}

void
db_stack_trace_print_ra(db_expr_t ra, bool have_ra,
    db_expr_t addr, bool have_addr,
    db_expr_t count,
    const char *modif, void (*pr)(const char *, ...))
{
	db_addr_t callpc, frame, symval;
	struct prologue_info pi;
	db_expr_t diff;
	db_sym_t sym;
	u_long tfps;
	const char *symname;
	struct pcb *pcbp;
	const char *cp = modif;
	struct trapframe *tf;
	bool ra_from_tf;
	u_long last_ipl = ~0L;
	char c;
	bool trace_thread = false;
	bool lwpaddr = false;

	while ((c = *cp++) != 0) {
		trace_thread |= c == 't';
		trace_thread |= c == 'a';
		lwpaddr |= c == 'a';
	}

	if (!have_addr) {
		addr = DDB_REGS->tf_regs[FRAME_SP] - FRAME_SIZE * 8;
		tf = (struct trapframe *)addr;
		callpc = db_alpha_tf_reg(tf, FRAME_PC);
		frame = (db_addr_t)tf + FRAME_SIZE * 8;
		ra_from_tf = true;
	} else {
#ifdef _KERNEL
		struct proc *p = NULL;
		struct lwp *l = NULL;
#else
		struct proc pstore, *p = &pstore;
		struct lwp lstore, *l = &lstore;
#endif /* _KERNEL */

		if (trace_thread) {
			if (lwpaddr) {
#ifdef _KERNEL
				l = (struct lwp *)addr;
				p = l->l_proc;
#else
				db_read_bytes(addr, sizeof(*l), (char *)l);
				db_read_bytes((db_addr_t)l->l_proc,
				    sizeof(*p), (char *)p);
#endif /* _KERNEL */
				(*pr)("trace: pid %d ", p->p_pid);
			} else {
#ifdef _KERNEL
				(*pr)("trace: pid %d ", (int)addr);
				p = proc_find_raw(addr);
				if (p == NULL) {
					(*pr)("not found\n");
					return;
				}
				l = LIST_FIRST(&p->p_lwps);
				KASSERT(l != NULL);
#else
				(*pr)("no proc_find_raw() in crash\n");
				return;
#endif /* _KERNEL */
			}
			(*pr)("lid %d ", l->l_lid);
			pcbp = lwp_getpcb(l);
			addr = db_alpha_read_saved_reg(&pcbp->pcb_hw.apcb_ksp);
			callpc = db_alpha_read_saved_reg(&pcbp->pcb_context[7]);
			(*pr)("at 0x%lx\n", addr);
		} else if (have_ra) {
			callpc = ra;
			(*pr)("at 0x%lx pc 0x%lx\n", addr, callpc);
		} else {
			(*pr)("alpha trace requires known PC =eject=\n");
			return;
		}
		frame = addr;
		tf = NULL;
		ra_from_tf = false;
	}

	while (count--) {
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
		 * If the previous RA pointed at the kernel thread
		 * backstop, then we are at the root of the call
		 * graph.
		 */
		if (db_alpha_sym_is_backstop(symval)) {
			(*pr)("--- kernel thread backstop ---\n");
			break;
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
		if (db_alpha_sym_is_trap(symval)) {
			tf = (struct trapframe *)frame;

			(*pr)("--- %s", db_alpha_trapsym_description(symval));

			tfps = db_alpha_tf_reg(tf, FRAME_PS);
			if (db_alpha_sym_is_syscall(symval)) {
				decode_syscall(db_alpha_tf_reg(tf, FRAME_V0),
				    pr);
			}
			if ((tfps & ALPHA_PSL_IPL_MASK) != last_ipl) {
				last_ipl = tfps & ALPHA_PSL_IPL_MASK;
				if (! db_alpha_sym_is_syscall(symval)) {
					(*pr)(" (from ipl %ld)", last_ipl);
				}
			}
			(*pr)(" ---\n");
			if (tfps & ALPHA_PSL_USERMODE) {
				(*pr)("--- user mode ---\n");
				break;	/* Terminate search.  */
			}
			callpc = db_alpha_tf_reg(tf, FRAME_PC);
			frame = (db_addr_t)tf + FRAME_SIZE * 8;
			ra_from_tf = true;
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
			if (ra_from_tf) {
				callpc = db_alpha_tf_reg(tf, FRAME_RA);
			} else {
				(*pr)("--- root of call graph ---\n");
				break;
			}
		} else {
			unsigned long reg;

			db_read_bytes(frame + pi.pi_reg_offset[26],
			    sizeof(reg), (char *)&reg);
			callpc = reg;
		}
		frame += pi.pi_frame_size;
		ra_from_tf = false;
	}
}
