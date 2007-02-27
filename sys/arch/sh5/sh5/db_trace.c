/*	$NetBSD: db_trace.c,v 1.18.8.1 2007/02/27 16:53:03 yamt Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_trace.c,v 1.18.8.1 2007/02/27 16:53:03 yamt Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/systm.h>

#include <machine/db_machdep.h>
#include <machine/frame.h>

#include <ddb/db_sym.h>
#include <ddb/db_command.h>
#include <ddb/db_extern.h>
#include <ddb/db_access.h>
#include <ddb/db_interface.h>
#include <ddb/db_output.h>
#include <ddb/ddbvar.h>


/*
 * Given a signed immediate value 'imm', of length 'bits', return a
 * sign-extended version of 'imm'.
 */
#define	IMM_EXT(imm, bits)	\
    ((((imm) & (1 << ((bits) - 1))) != 0) ? ((imm) | (-1 << (bits))) : (imm))

/*
 * A stack frame is created using "addi{,.l} r15, -n, r15"
 * or "sub r15, rN, r15" if the frame is bigger than 511 bytes.
 * In the latter case, there will have been a preceding "movi ###, rN".
 */
#define	OP_ADDI_R15_n_R15_M	0xfff003ff
#define	OP_ADDI_n(op)		IMM_EXT(((int)((op) >> 10) & 0x3ff), 10)
#ifndef _LP64
#define	OP_ADDI_R15_n_R15	0xd4f000f0
#else
#define	OP_ADDI_R15_n_R15	0xd0f000f0
#endif

/*
 * Detect frame creation using movi/sub.
 * Note that this will not work if the frame is > 32767 bytes in size.
 * Having said that, anyone who writes a kernel function which uses
 * that much automatic variable space on the stack should be hung, drawn
 * and quartered.
 */
#define	OP_SUB_R15_Rn_R15_M	0xffff03ff
#define	OP_SUB_n(op)		(((op) >> 10) & 0x3f)
#define	OP_MOVI_imm_Rn_M	0xfc00000f
#define	OP_MOVI_imm(op)		IMM_EXT(((int)((op) >> 10) & 0xffff), 16)
#define	OP_MOVI_reg(op)		(((op) >> 4) & 0x3f)
#ifndef _LP64
#define	OP_SUB_R15_Rn_R15	0x00fa00f0
#else
#define	OP_SUB_R15_Rn_R15	0x00fb00f0
#endif
#define	OP_MOVI_imm_Rn		0xcc000000

/*
 * The prologue is generally terminated by "add{,.l} r15, r63, r14"
 */
#define	OP_ADD_R15_R63_R14_M	0xffffffff
#ifndef _LP64
#define	OP_ADD_R15_R63_R14	0x00f8fce0
#else
#define	OP_ADD_R15_R63_R14	0x00f9fce0
#endif

/*
 * Identify the opcodes which preserve r18 and r14
 */
#define	OP_ST_R15_n_R14_M	0xfff003ff
#define	OP_ST_R15_n_R18_M	0xfff003ff
#ifndef _LP64
#define	OP_ST_R15_n_R14		0xa8f000e0
#define	OP_ST_R15_n_R18		0xa8f00120
#define	OP_ST_n(op)		(IMM_EXT((int)(((op) >> 10) & 0x3ff), 10) * 4)
#else
#define	OP_ST_R15_n_R14		0xacf000e0
#define	OP_ST_R15_n_R18		0xacf00120
#define	OP_ST_n(op)		(IMM_EXT((int)(((op) >> 10) & 0x3ff), 10) * 8)
#endif

/*
 * Leaf functions will generally just stuff r18 into a branch-target register
 * using the "ptabs" instruction.
 */
#define	OP_PTABS_R18_TRn_M	0xfffffd8f
#define	OP_PTABS_R18_TRn	0x6bf14800
#define	OP_PTABS_n(op)		(((op) >> 4) & 0x7)

/*
 * Depending on how many callee-saved registers the prologue pushes on the
 * stack, we could have to search a fair few instructions to find what we're
 * looking for. The following figure seems to be sufficient for the kernel.
 */
#define	PROLOGUE_LEN		56

static db_addr_t find_prologue(db_addr_t);
static int prev_frame(db_addr_t, db_addr_t, db_addr_t *, db_addr_t *);

static struct intrframe *cur_intrframe;

/*
 * When grovelling a function's prologue, we track the values assigned
 * to r0-r62 using "movi" instructions. This is in case the prologue is
 * setting up a >511 byte frame by subtracting a register from r15.
 */
static	int	prologue_movi[64];

void
db_stack_trace_print(db_expr_t addr, int have_addr, db_expr_t count,
    const char *modif, void (*pr)(const char *, ...))
{
	db_addr_t pc, fp;
	db_addr_t nextpc = 0, nextfp;
	db_addr_t lastpc = 0, lastfp = 0;
	db_sym_t sym;
	db_expr_t diff, pc_adj;
	const char *symp;
	int trace_thread, dump_eframe, lwpaddr;

	/* trace_thread is non-zero if tracing a specific process */
	lwpaddr = (strchr(modif, 'a') != NULL);
	trace_thread = ((strchr(modif, 't') != NULL) | lwpaddr);
	dump_eframe = (strchr(modif, 'e') != NULL);

	if (have_addr == 0) {
		/*
		 * The usual case. Trace back from the current trapframe
		 */
		pc = (db_addr_t) ddb_regs.tf_state.sf_spc & ~1;
		fp = (db_addr_t) ddb_regs.tf_caller.r14;
		cur_intrframe = &ddb_regs.tf_ifr;
	} else {
		/*
		 * Otherwise, the user specified an "address".
		 */
		if (trace_thread) {
			/*
			 * Trace a specific process.
			 *
			 * XXX: Needs to understand multiple LWPs per proc.
			 */
			struct proc *p;
			struct lwp *l;
			if (lwpaddr) {
				l = (struct lwp *)addr;
				p = l->l_proc;
				(*pr)("trace: pid %d ", p->p_pid);
			} else {
				(*pr)("trace: pid %d ", (int)addr);
				p = p_find(addr, PFIND_LOCKED);
				if (p == NULL) {
					(*pr)("not found\n");
					return;
				}
				l = proc_representative_lwp(p, NULL, 0);
			}
			(*pr)("lid %d ", l->l_lid);
			if ((l->l_flag & LW_INMEM) == 0) {
				(*pr)("swapped out\n");
				return;
			}

			/*
			 * When the LWP is NOT current, its PC and FP
			 * are stashed in the LWP's PCB. Otherwise, just
			 * use the current stack frame.
			 */
			if (l != curlwp) {
				pc = (db_addr_t) l->l_addr->u_pcb.pcb_ctx.sf_pc;
				pc &= ~1;
				fp = (db_addr_t) l->l_addr->u_pcb.pcb_ctx.sf_fp;
				cur_intrframe = NULL;
			} else {
				pc = (db_addr_t) ddb_regs.tf_state.sf_spc & ~1;
				fp = (db_addr_t) ddb_regs.tf_caller.r14;
				cur_intrframe = &ddb_regs.tf_ifr;
			}
			(*pr)("at 0x%lx\n", fp);
		} else {
			(*pr)("sh5 trace requires known PC =eject=\n");
			return;
		}
	}

	pc_adj = 0;

	/*
	 * Walk the call stack until the PC or FP are not valid
	 */
	while (pc >= SH5_KSEG0_BASE && fp >= SH5_KSEG0_BASE &&
	    (pc & 3) == 0 && (fp & 7) == 0 &&
	    (pc != lastpc || fp != lastfp)) {
		/*
		 * Lookup the name of the current function
		 */
		symp = NULL;
		sym = db_search_symbol(pc - pc_adj, DB_STGY_PROC, &diff);
		if (sym == 0) {
			(*pr)("0x%lx: Symbol not found\n", (long)(pc - pc_adj));
			break;
		}
		symp = NULL;
		db_symbol_values(sym, &symp, NULL);
		if (symp == NULL) {
			(*pr)("0x%lx: No symbol string found\n");
			break;
		}

		/*
		 * There's no point even trying to grovel for function
		 * parameters. It's just Too Much Trouble.
		 * A determined hacker should be able to use the Frame
		 * Pointer value and a disassembly of the prologue to
		 * figure out what's what anyway.
		 */
		(*pr)("0x%lx: %s() at ", fp, symp);
		db_printsym(pc - pc_adj, DB_STGY_PROC, pr);
		(*pr)("\n");

		/*
		 * There really is no point trying to trace back through
		 * "idle" or "proc_trampoline". The former has no valid
		 * "context" anyway, and the latter is the first function
		 * called for each new LWP/kernel thread.
		 * Likewise for 'Lmapped_start'.
		 */
		if (strcmp(symp, "idle") == 0 ||
		    strcmp(symp, ___STRING(_C_LABEL(proc_trampoline))) == 0 ||
		    strcmp(symp, "Lmapped_start") == 0) {
			pc = fp = SH5_KSEG0_BASE;
			break;
		}

		lastpc = pc;
		lastfp = fp;

		if (strcmp(symp, "Lsh5_event_sync") == 0 ||
		    strcmp(symp, "Ltrapagain") == 0 ||
		    strcmp(symp, "Ltrapepilogue") == 0 ||
		    strcmp(symp, "Ltrapexit") == 0) {
			/*
			 * We're tracing back through a synchronous exception.
			 * The previous PC and FP are available from the
			 * 'struct trapframe' saved on the stack.
			 */
			struct trapframe *tf = (struct trapframe *)(intptr_t)fp;
			pc = (db_addr_t) tf->tf_state.sf_spc & ~1;
			fp = (db_addr_t) tf->tf_caller.r14;
			cur_intrframe = &tf->tf_ifr;
			pc_adj = 0;
			(*pr)("\tTrap Type: %s\n",
			    trap_type((int)tf->tf_state.sf_expevt));
			(*pr)("\tSPC=0x%lx, SSR=0x%lx, TEA=0x%lx, TRA=0x%lx",
			    (long)tf->tf_state.sf_spc,
			    (long)tf->tf_state.sf_ssr,
			    (long)tf->tf_state.sf_tea,
			    (long)tf->tf_state.sf_tra);
			if (dump_eframe)
				dump_trapframe(pr, "\n\t", tf);
			else
				(*pr)("\n");
		} else
		if (strcmp(symp, "Lsh5_event_interrupt") == 0 ||
		    strcmp(symp, "Lintrexit") == 0) {
			/*
			 * We're tracing back through an asynchronous
			 * exception (hardware interrupt). The previous PC
			 * and FP are available from the 'struct intrframe'
			 * saved on the stack.
			 */
			struct intrframe *tf = (struct intrframe *)(intptr_t)fp;
			pc = (db_addr_t) tf->if_state.sf_spc & ~1;
			fp = (db_addr_t) tf->if_caller.r14;
			cur_intrframe = tf;
			pc_adj = 0;
			(*pr)("\tSPC=0x%lx, SSR=0x%lx, INTEVT=0x%lx",
			    (long)tf->if_state.sf_spc,
			    (long)tf->if_state.sf_ssr,
			    (long)tf->if_state.sf_intevt);
			if (dump_eframe) {
				struct trapframe tfr;
				memset(&tfr, 0, sizeof(tfr));
				tfr.tf_ifr = *tf;
				dump_trapframe(pr, "\n\t", &tfr);
			} else
				(*pr)("\n");
		} else
		/*
		 * Looks like we have to grovel the current function's
		 * prologue to find out the next PC and FP. Start the
		 * search from "PC - pc_adj" to ensure we catch the actual
		 * call-site. Without this, we can fall foul of tail-calls
		 * and functions with the "__noreturn__" attribute
		 * (depending on alignment, we could pick up the symbol for
		 * the *next* function and, hence, get the wrong prologue).
		 */
		if (prev_frame(fp, pc - pc_adj, &nextfp, &nextpc)) {
			fp = nextfp;
			pc = nextpc & ~1;
			pc_adj = 4;
		} else {
			(*pr)("Can't find caller's stack frame.\n");
			lastpc = lastfp = 0;
			break;
		}
	}

	/*
	 * If the loop terminated due to a bad PC or FP value, print
	 * the reason why.
	 */
	if (pc < SH5_KSEG0_BASE) {
		(*pr)("Program counter is in user-space: 0x%lx(fp=0x%lx)\n",
		    pc, fp);
	} else
	if (fp < SH5_KSEG0_BASE) {
		(*pr)("Frame pointer is in user-space: 0x%lx(fp=0x%lx)\n",
		    pc, fp);
	} else
	if ((pc & 3) != 0)
		(*pr)("Program counter is invalid: 0x%lx(fp=0x%lx)\n", pc, fp);
	else
	if ((fp & 7) != 0)
		(*pr)("Frame pointer is invalid: 0x%lx(fp=0x%lx)\n", pc, fp);
	else
	if (lastpc == pc && lastfp == fp)
	       (*pr)("Program counter and Frame pointer bodge. In asm code?\n");
}

/*
 * Given a Program Counter address, find the address of the start of
 * the function (the prologue) which contains the PC's address.
 */
static db_addr_t
find_prologue(db_addr_t addr)
{
	db_sym_t sym;
	db_expr_t diff;
	const char *symp;

	addr &= ~3;
#if 0
again:
#endif
	symp = NULL;
	sym = db_search_symbol(addr, DB_STGY_PROC, &diff);
	if (sym == 0)
		return (0);

	symp = NULL;
	db_symbol_values(sym, &symp, NULL);

	if (symp == NULL)
		return (0);

#if 0
	/*
	 * Compensate for the <handy breakpoint location after LWP "wakes">
	 * symbol in ltsleep(), which screws up our search for the function's
	 * prologue.
	 */
	if (strcmp(symp, "bpendtsleep") == 0) {
		addr -= 4;
		goto again;
	}
#endif
	return (addr - diff);
}

/*
 * Given the current frame-pointer and program counter, figure out the
 * caller's frame-pointer and program counter.
 *
 * This involves much grovelling around in the current function's prologue
 * to determine where it stashed the caller's details in the stack frame.
 */
static int
prev_frame(db_addr_t curfp, db_addr_t curpc,
    db_addr_t *pprevfp, db_addr_t *pprevpc)
{
	db_addr_t prologue;
	db_addr_t prevfp;
	opcode_t op;
	long r14off, r18off;
	long lastfpoff, v;
	int i, found_frame_setup;

	/*
	 * We need to grovel the current function's prologue to figure out
	 * where the saved r18/r14 pair are stored in the function's stack.
	 * If we can't locate the prologue, we're stuffed.
	 */
	if ((prologue = find_prologue(curpc)) == 0)
		return (0);

	lastfpoff = r14off = r18off = 0;
	prevfp = 0;
	found_frame_setup = 0;

	/*
	 * Search at most PROLOGUE_LEN instructions.
	 */
	for (i = 0; i < PROLOGUE_LEN; i++, prologue += sizeof(opcode_t)) {
		/*
		 * Short-circuit, for the case where we're stopped right in the
		 * middle of the prologue (e.g. breakpoint at top of func.)
		 * We can just pull the required values straight out the frame.
		 */
		if (prologue >= curpc && cur_intrframe) {
			*pprevfp = cur_intrframe->if_caller.r14;
			*pprevpc = cur_intrframe->if_caller.r18 & ~1;
			return (1);
		}

		/*
		 * Fetch an opcode from the prologue.
		 */
		op = *((opcode_t *)(intptr_t)prologue);

		if ((op & OP_ADDI_R15_n_R15_M) == OP_ADDI_R15_n_R15) {
			/*
			 * Found "addi r15, -n, r15"
			 *
			 * By accumulating the offsets '-n', we can determine
			 * the size of stack frame the current function
			 * created. This is then used to 'pop' the stack to
			 * give us the base of the caller's frame.
			 */
			lastfpoff += OP_ADDI_n(op);
			prevfp = curfp - lastfpoff;
		} else
		if ((op & OP_SUB_R15_Rn_R15_M) == OP_SUB_R15_Rn_R15) {
			/*
			 * Found "sub r15, rN, r15"
			 *
			 * This is the same as the previous case, except this
			 * time we're subtracting a value in a register from
			 * the current stack pointer.
			 *
			 * We should already have seen the corresponding
			 * "movi imm, rN".
			 */
			lastfpoff -= (long) prologue_movi[OP_SUB_n(op)];
			prevfp = curfp - lastfpoff;
		} else
		if ((op & OP_ST_R15_n_R14_M) == OP_ST_R15_n_R14) {
			/*
			 * Found "st r15, n, r14"
			 *
			 * The prologue is saving the address of caller's
			 * stack frame. We will use this later to verify
			 * our idea of the size of the current function's
			 * frame.
			 */
			if (r14off || lastfpoff == 0)
				return (0);	/* Found a duplicate! */

			/*
			 * Record the offset (from the CALLER'S frame) at
			 * which the current function stored r14.
			 */
			r14off = OP_ST_n(op) + lastfpoff;
		} else
		if ((op & OP_ST_R15_n_R18_M) == OP_ST_R15_n_R18) {
			/*
			 * Found "st r15, n, r18"
			 *
			 * The prologue is saving the caller's return address
			 * in the stack frame.
			 */
			if (r18off || lastfpoff == 0)
				return (0);	/* Found a duplicate! */

			/*
			 * Record the offset (from the CALLER'S frame) at
			 * which the current function stored r18.
			 */
			r18off = OP_ST_n(op) + lastfpoff;
		} else
		if ((op & OP_PTABS_R18_TRn_M) == OP_PTABS_R18_TRn) {
			/*
			 * Found "ptabs r18, trn"
			 *
			 * This is for the benefit of Leaf functions, which
			 * generally don't need to save the return address in
			 * the frame. They tend to stuff the address straight
			 * into a branch-target register.
			 */
			if (r18off)
				return (0);	/* Found a duplicate */

			/*
			 * Fix up an "invalid" offset, which will be noticed
			 * later, so as not to grovel the frame.
			 */
			r18off = -1;

			/*
			 * Pick the value out of the specified branch-target
			 * register.
			 */
			switch (OP_PTABS_n(op)) {
			case 0:	*pprevpc = (db_addr_t)ddb_regs.tf_caller.tr0;
				break;
			case 1:	*pprevpc = (db_addr_t)ddb_regs.tf_caller.tr1;
				break;
			case 2:	*pprevpc = (db_addr_t)ddb_regs.tf_caller.tr2;
				break;
			case 3:	*pprevpc = (db_addr_t)ddb_regs.tf_caller.tr3;
				break;
			case 4:	*pprevpc = (db_addr_t)ddb_regs.tf_caller.tr4;
				break;
			case 5:	*pprevpc = (db_addr_t)ddb_regs.tf_callee.tr5;
				break;
			case 6:	*pprevpc = (db_addr_t)ddb_regs.tf_callee.tr6;
				break;
			case 7:	*pprevpc = (db_addr_t)ddb_regs.tf_callee.tr7;
				break;
			}
		} else
		if ((op & OP_MOVI_imm_Rn_M) == OP_MOVI_imm_Rn) {
			/*
			 * Found a "movi imm, Rn". There's a chance this
			 * value may be used to set up a stack frame if
			 * the frame size is > 511 bytes. So we stash it
			 * away somewhere for later use should it be
			 * needed.
			 */
			if (OP_MOVI_reg(op) < 63)
			       prologue_movi[OP_MOVI_reg(op)] = OP_MOVI_imm(op);
		} else
		if ((op & OP_ADD_R15_R63_R14_M) == OP_ADD_R15_R63_R14) {
			/*
			 * Found "add r15, r63, r14"
			 *
			 * Some prologues contain TWO "addi r15, -n, r15"
			 * instructions. We need to find both of them if
			 * we are to guess correctly the size of the frame.
			 * Fortunately, if we see this instruction, we can
			 * be sure the prologue has finished creating space
			 * for the frame. All that's left now is to continue
			 * searching for any remaining r14/r18 stores.
			 */
			found_frame_setup = 1;
		}

		/*
		 * Terminate the search if we've got all the required details.
		 */
		if (found_frame_setup && r14off && r18off && prevfp)
			break;
	}

	/*
	 * If we searched PROLOGUE_LEN opcodes but still didn't find anything
	 * useful, punt.
	 */
	if (r14off == 0 || r18off == 0 || prevfp == 0)
		return (0);

	/*
	 * Simply add r14off or r18off to prevfp to get the address of
	 * where the function stashed them.
	 *
	 * XXX: Should check for valid address
	 */

	/* Fetch the saved r14 (caller's frame pointer) */
	db_read_bytes(r14off + prevfp, sizeof(long), (char *)&v);
	*pprevfp = (db_addr_t)v;

	/*
	 * If we need to fetch the saved r18 ...
	 */
	if (r18off != -1) {
		db_read_bytes(r18off + prevfp, sizeof(long), (char *)&v);
		*pprevpc = (db_addr_t)v;
	}

	/*
	 * Make sure to clear the SHmedia flag in the PC
	 */
	*pprevpc &= ~1;

	return (1);
}
