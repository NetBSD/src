/*	$NetBSD: db_interface.c,v 1.11 2003/03/19 11:37:57 scw Exp $	*/

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

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/systm.h>

#include <dev/cons.h>

#include <machine/db_machdep.h>
#include <machine/frame.h>

#include <ddb/db_sym.h>
#include <ddb/db_command.h>
#include <ddb/db_extern.h>
#include <ddb/db_access.h>
#include <ddb/db_output.h>
#include <ddb/db_variables.h>
#include <ddb/ddbvar.h>

int     db_active = 0;

db_regs_t ddb_regs;

static db_expr_t reg_zero;
static db_expr_t reg_kcr0;
static db_expr_t reg_kcr1;

static int db_var_reg(const struct db_variable *, db_expr_t *, int);

const struct db_variable db_regs[] = {
	{ "usr", (long *)&ddb_regs.tf_state.sf_usr, db_var_reg },
	{ "sr",  (long *)&ddb_regs.tf_state.sf_ssr, db_var_reg },
	{ "pc",  (long *)&ddb_regs.tf_state.sf_spc, db_var_reg },
	{ "kcr0",(void *)&reg_kcr0, db_var_reg },
	{ "kcr1",(void *)&reg_kcr1, db_var_reg },

	{ "tr0", (long *)&ddb_regs.tf_caller.tr0, db_var_reg },
	{ "tr1", (long *)&ddb_regs.tf_caller.tr1, db_var_reg },
	{ "tr2", (long *)&ddb_regs.tf_caller.tr2, db_var_reg },
	{ "tr3", (long *)&ddb_regs.tf_caller.tr3, db_var_reg },
	{ "tr4", (long *)&ddb_regs.tf_caller.tr4, db_var_reg },

	{ "tr5", (long *)&ddb_regs.tf_callee.tr5, db_var_reg },
	{ "tr6", (long *)&ddb_regs.tf_callee.tr6, db_var_reg },
	{ "tr7", (long *)&ddb_regs.tf_callee.tr7, db_var_reg },

	{ "r0",  (long *)&ddb_regs.tf_caller.r0,  db_var_reg },
	{ "r1",  (long *)&ddb_regs.tf_caller.r1,  db_var_reg },
	{ "r2",  (long *)&ddb_regs.tf_caller.r2,  db_var_reg },
	{ "r3",  (long *)&ddb_regs.tf_caller.r3,  db_var_reg },
	{ "r4",  (long *)&ddb_regs.tf_caller.r4,  db_var_reg },
	{ "r5",  (long *)&ddb_regs.tf_caller.r5,  db_var_reg },
	{ "r6",  (long *)&ddb_regs.tf_caller.r6,  db_var_reg },
	{ "r7",  (long *)&ddb_regs.tf_caller.r7,  db_var_reg },
	{ "r8",  (long *)&ddb_regs.tf_caller.r8,  db_var_reg },
	{ "r9",  (long *)&ddb_regs.tf_caller.r9,  db_var_reg },
	{ "r10", (long *)&ddb_regs.tf_caller.r10, db_var_reg },
	{ "r11", (long *)&ddb_regs.tf_caller.r11, db_var_reg },
	{ "r12", (long *)&ddb_regs.tf_caller.r12, db_var_reg },
	{ "r13", (long *)&ddb_regs.tf_caller.r13, db_var_reg },
	{ "r14", (long *)&ddb_regs.tf_caller.r14, db_var_reg },
	{ "r15", (long *)&ddb_regs.tf_caller.r15, db_var_reg },
	{ "r16", (long *)&ddb_regs.tf_caller.r16, db_var_reg },
	{ "r17", (long *)&ddb_regs.tf_caller.r17, db_var_reg },
	{ "r18", (long *)&ddb_regs.tf_caller.r18, db_var_reg },
	{ "r19", (long *)&ddb_regs.tf_caller.r19, db_var_reg },
	{ "r20", (long *)&ddb_regs.tf_caller.r20, db_var_reg },
	{ "r21", (long *)&ddb_regs.tf_caller.r21, db_var_reg },
	{ "r22", (long *)&ddb_regs.tf_caller.r22, db_var_reg },
	{ "r23", (long *)&ddb_regs.tf_caller.r23, db_var_reg },
	{ "r24", (void *)&reg_zero,               db_var_reg },
	{ "r25", (long *)&ddb_regs.tf_caller.r25, db_var_reg },
	{ "r26", (long *)&ddb_regs.tf_caller.r26, db_var_reg },
	{ "r27", (long *)&ddb_regs.tf_caller.r27, db_var_reg },

	{ "r28", (long *)&ddb_regs.tf_callee.r28, db_var_reg },
	{ "r29", (long *)&ddb_regs.tf_callee.r29, db_var_reg },
	{ "r30", (long *)&ddb_regs.tf_callee.r30, db_var_reg },
	{ "r31", (long *)&ddb_regs.tf_callee.r31, db_var_reg },
	{ "r32", (long *)&ddb_regs.tf_callee.r32, db_var_reg },
	{ "r33", (long *)&ddb_regs.tf_callee.r33, db_var_reg },
	{ "r34", (long *)&ddb_regs.tf_callee.r34, db_var_reg },
	{ "r35", (long *)&ddb_regs.tf_callee.r35, db_var_reg },

	{ "r36", (long *)&ddb_regs.tf_caller.r36, db_var_reg },
	{ "r37", (long *)&ddb_regs.tf_caller.r37, db_var_reg },
	{ "r38", (long *)&ddb_regs.tf_caller.r38, db_var_reg },
	{ "r39", (long *)&ddb_regs.tf_caller.r39, db_var_reg },
	{ "r40", (long *)&ddb_regs.tf_caller.r40, db_var_reg },
	{ "r41", (long *)&ddb_regs.tf_caller.r41, db_var_reg },
	{ "r42", (long *)&ddb_regs.tf_caller.r42, db_var_reg },
	{ "r43", (long *)&ddb_regs.tf_caller.r43, db_var_reg },

	{ "r44", (long *)&ddb_regs.tf_callee.r44, db_var_reg },
	{ "r45", (long *)&ddb_regs.tf_callee.r45, db_var_reg },
	{ "r46", (long *)&ddb_regs.tf_callee.r46, db_var_reg },
	{ "r47", (long *)&ddb_regs.tf_callee.r47, db_var_reg },
	{ "r48", (long *)&ddb_regs.tf_callee.r48, db_var_reg },
	{ "r49", (long *)&ddb_regs.tf_callee.r49, db_var_reg },
	{ "r50", (long *)&ddb_regs.tf_callee.r50, db_var_reg },
	{ "r51", (long *)&ddb_regs.tf_callee.r51, db_var_reg },
	{ "r52", (long *)&ddb_regs.tf_callee.r52, db_var_reg },
	{ "r53", (long *)&ddb_regs.tf_callee.r53, db_var_reg },
	{ "r54", (long *)&ddb_regs.tf_callee.r54, db_var_reg },
	{ "r55", (long *)&ddb_regs.tf_callee.r55, db_var_reg },
	{ "r56", (long *)&ddb_regs.tf_callee.r56, db_var_reg },
	{ "r57", (long *)&ddb_regs.tf_callee.r57, db_var_reg },
	{ "r58", (long *)&ddb_regs.tf_callee.r58, db_var_reg },
	{ "r59", (long *)&ddb_regs.tf_callee.r59, db_var_reg },

	{ "r60", (long *)&ddb_regs.tf_caller.r60, db_var_reg },
	{ "r61", (long *)&ddb_regs.tf_caller.r61, db_var_reg },
	{ "r62", (long *)&ddb_regs.tf_caller.r62, db_var_reg },
	{ "r63", (void *)&reg_zero,               db_var_reg }
};
const struct db_variable * const db_eregs = db_regs + sizeof(db_regs)/sizeof(db_regs[0]);

/*
 * SH5-specific commands
 */
static void db_sh5_tlb(db_expr_t, int, db_expr_t, char *);
static void db_sh5_fpr(db_expr_t, int, db_expr_t, char *);

const struct db_command db_machine_command_table[] = {
	{"tlb",		db_sh5_tlb,	0,	0},
	{"fpr",		db_sh5_fpr,	0,	0},
	{NULL,}
};

static void print_tlb_entry(int, pteh_t, ptel_t);


static int
db_var_reg(const struct db_variable *varp, db_expr_t *valp, int op)
{
	db_expr_t *ep = (db_expr_t *) varp->valuep;
	register_t *rp = (register_t *) varp->valuep;

	if (op == DB_VAR_GET)
		if (ep == &reg_kcr0) {
			register_t kcr0;
			__asm __volatile("getcon kcr0, %0" : "=r"(kcr0));
			*valp = kcr0;
		} else
		if (ep == &reg_kcr1) {
			register_t kcr1;
			__asm __volatile("getcon kcr1, %0" : "=r"(kcr1));
			*valp = kcr1;
		} else
			*valp = *ep;
	else {
		/* Disallow writing to registers which are not saved/writable */
		if (ep == &reg_zero)
			return(0);

		if (ep == &reg_kcr0) {
			register_t kcr0 = *valp;
			__asm __volatile("putcon %0, kcr0" :: "r"(kcr0));
		} else
		if (ep == &reg_kcr1) {
			register_t kcr1 = *valp;
			__asm __volatile("putcon %0, kcr1" :: "r"(kcr1));
		} else {

			*ep = *valp;

			/*
			 * If writing to a branch target register, or the
			 * program counter, ensure bit zero is set to force
			 * SHMedia mode.
			 *
			 * XXX: what if running some SHcompact code in kernel?
			 */
			if ((rp >= &ddb_regs.tf_caller.tr0 &&
			     rp <= &ddb_regs.tf_caller.tr4) ||
			    (rp >= &ddb_regs.tf_callee.tr5 &&
			     rp <= &ddb_regs.tf_callee.tr7) ||
			    rp == &ddb_regs.tf_state.sf_spc)
				*ep |= 0x1;
		}
	}

	return (0);
}

int already_in_db;

void
cpu_Debugger(void)
{

	asm volatile("brk");
}

int
kdb_trap(int type, void *v)
{
	struct trapframe *frame = v;
	int s;

        switch (type) {
	case T_BREAK:
	case -1:
		break;
	default:
		if (!db_onpanic && db_recover == 0)
			return 0;
                if (db_recover != 0) {
			db_error("Faulted in DDB; continuing...\n");
			/*NOTREACHED*/
		}
	}

	s = splhigh();
	already_in_db = 1;
	ddb_regs = *frame;
	db_active++;
	cnpollc(1);
	db_trap(type, 0);
	cnpollc(0);
	db_active--;

	if (IS_BREAKPOINT_TRAP(type, 0)) {
		int bkpt;
		bkpt = db_get_value(PC_REGS(DDB_REGS), BKPT_SIZE, FALSE);
		if (bkpt == BKPT_INST)
			PC_ADVANCE(DDB_REGS);
	}

	*frame = ddb_regs;
	already_in_db = 0;
	splx(s);

	return (1);
}

boolean_t
inst_branch(int inst)
{
	/*
	 * Deal with conditional branches
	 */
	switch (inst & 0xfc0f010f) {
	case 0x64010000:	/* BEQ  Rm, Rn, TRc */
	case 0xe4010000:	/* BEQI Rm, imm, TRc */
	case 0x64030000:	/* BGE  Rm, Rn, TRc */
	case 0x640b0000:	/* BGEU Rm, Rn, TRc */
	case 0x64070000:	/* BGT  Rm, Rn, TRc */
	case 0x640f0000:	/* BGTU Rm, Rn, TRc */
	case 0x64050000:	/* BNE  Rm, Rn, TRc */
	case 0xe4050000:	/* BNEI Rm, imm, TRc */
		return (TRUE);
	}

	return (FALSE);
}

boolean_t
inst_unconditional_flow_transfer(int inst)
{
	/*
	 * Deal with blink <anything>
	 */
	if ((inst & 0xff8ffc0f) == 0x4401fc00)
		return (TRUE);

	return (FALSE);
}

db_addr_t
branch_taken(int inst, db_addr_t pc, db_regs_t *regs)
{
	int tr = -1;
	register_t trv;

	/*
	 * Check for conditional branches
	 */
	switch (inst & 0xfc0f010f) {
	case 0x64010000:	/* BEQ  Rm, Rn, TRc */
	case 0xe4010000:	/* BEQI Rm, imm, TRc */
	case 0x64030000:	/* BGE  Rm, Rn, TRc */
	case 0x640b0000:	/* BGEU Rm, Rn, TRc */
	case 0x64070000:	/* BGT  Rm, Rn, TRc */
	case 0x640f0000:	/* BGTU Rm, Rn, TRc */
	case 0x64050000:	/* BNE  Rm, Rn, TRc */
	case 0xe4050000:	/* BNEI Rm, imm, TRc */
		tr = (inst >> 4) & 0x7;
		break;

	default:
		break;
	}

	if (tr < 0 && ((inst & M_RET) == I_RET || (inst & M_CALL) == I_CALL))
		tr = (inst >> 20) & 0x7;

	if (tr < 0)
		db_error("inst_branch: Invalid branch instruction\n");

	trv = 0; /* XXX: Stupid compiler */
	switch (tr & 7) {
	case 0:	trv = regs->tf_caller.tr0; break;
	case 1:	trv = regs->tf_caller.tr1; break;
	case 2:	trv = regs->tf_caller.tr2; break;
	case 3:	trv = regs->tf_caller.tr3; break;
	case 4:	trv = regs->tf_caller.tr4; break;
	case 5:	trv = regs->tf_callee.tr5; break;
	case 6:	trv = regs->tf_callee.tr6; break;
	case 7:	trv = regs->tf_callee.tr7; break;
	}

	trv &= ~1;

	return ((db_addr_t)(uintptr_t)trv);
}

int
inst_load(int inst)
{
	return (0);
}

int
inst_store(int inst)
{
	return (0);
}

/*
 * Dump the contents of both TLBs.
 *
 * XXX: This should make use of a CPU-specific backend so that it is
 * independent of TLB implementation.
 */
#define	ITLB_REG(r)	((r) * 16)
#define	DTLB_REG(r)	(0x800000 + ((r) * 16))

static void
db_sh5_tlb(db_expr_t addr, int have_addr, db_expr_t count, char *modif)
{
	register_t tlbreg;
	ptel_t ptel;
	pteh_t pteh;
	int i, flagv, flagi, flagd;

	flagv = (strchr(modif, 'v') != NULL);
	flagi = (strchr(modif, 'i') != NULL);
	flagd = (strchr(modif, 'd') != NULL);

	if (flagi == 0 && flagd == 0)
		flagi = flagd = 1;

	if (flagi) {
		db_printf("ITLB:\n");
		for (i = 0; i < 64; i++) {
			tlbreg = ITLB_REG(i);

			asm volatile("getcfg %1, 0, %0" :
			    "=r"(pteh) : "r"(tlbreg));

			asm volatile("getcfg %1, 1, %0" :
			    "=r"(ptel) : "r"(tlbreg));

			if (flagv == 0 && (pteh & SH5_PTEH_V) == 0)
				continue;

			print_tlb_entry(i, pteh, ptel);
		}
	}

	if (flagd) {
		db_printf("DTLB:\n");
		for (i = 0; i < 64; i++) {
			tlbreg = DTLB_REG(i);

			asm volatile("getcfg %1, 0, %0" :
			    "=r"(pteh) : "r"(tlbreg));

			asm volatile("getcfg %1, 1, %0" :
			    "=r"(ptel) : "r"(tlbreg));

			if (flagv == 0 && (pteh & SH5_PTEH_V) == 0)
				continue;

			print_tlb_entry(i, pteh, ptel);
		}
	}
}

static void
db_sh5_fpr(db_expr_t addr, int have_addr, db_expr_t count, char *modif)
{
	struct switchframe sw, *swp;
	struct lwp *l;
	u_int usr;
	int i, flagc, flagf;

	flagc = (strchr(modif, 'c') != NULL);
	flagf = (strchr(modif, 'f') != NULL);

	if (have_addr) {
		struct proc *p = pfind(addr);
		if (p == NULL) {
			db_printf("Invalid PID\n");
			return;
		}
		l = LIST_FIRST(&p->p_lwps);	/* XXX: Not much use... */
	} else
		l = curlwp;

	if (flagc || l == NULL || (l->l_md.md_flags & MDP_FPSAVED) == 0) {
		/*
		 * Fetch "Current" FP reg contents (at time
		 * of entry to ddb), or if the current LWP's
		 * FP registers have not yet been saved.
		 */
		swp = &sw;
		usr = (u_int)ddb_regs.tf_state.sf_usr;
		i = sh5_fpsave(usr, (struct pcb *)swp);
	} else {
		/*
		 * Fetch FP registers for current LWP, as
		 * stashed in the PCB.
		 */
		swp = &l->l_addr->u_pcb.pcb_ctx;
		usr = (u_int)l->l_md.md_regs->tf_state.sf_usr;
		i = l->l_md.md_flags & (MDP_FPUSED | MDP_FPSAVED);
	}

	if (i == 0) {
		db_printf("The FPU appears to be disabled\n");
		return;
	}

	db_printf("fpscr: 0x%08x\n", (u_int)swp->sf_fpregs.fpscr);
	db_printf("  usr: 0x%04x\n", usr);

	if ((i & MDP_FPUSED) == 0) {
		db_printf("The current LWP hasn't used the FPU.");
		if (flagf == 0) {
			db_printf("\n");
			return;
		}
		db_printf(" Register dump forced.\n");
	}

	usr >>= 8;

	for (i = 0; i < 32; i++) {
		if (flagf || (usr & (1 << (i / 4))) != 0) {
			if (i < 5)
				db_printf("  dr%d: ", i * 2);
			else
				db_printf(" dr%d: ", i * 2);

			db_printf("0x%08x%08x\n",
			    (u_int)(swp->sf_fpregs.fp[i] >> 32),
			    (u_int)(swp->sf_fpregs.fp[i]));
		}
	}
}

static void
print_tlb_entry(int i, pteh_t pteh, ptel_t ptel)
{
	int asid;

	db_printf("%2d: %08x/%08x ", i, (u_int)pteh, (u_int)ptel);

	if ((pteh & SH5_PTEH_V)) {
		asid = (int)(pteh >> SH5_PTEH_ASID_SHIFT);
		asid &= SH5_PTEH_ASID_MASK;

		db_printf("%cASID:%02x ",
		    ((pteh & SH5_PTEH_SH) == 0) ? ' ' : '*', asid);
	} else {
		db_printf("INVALID\n");
		return;
	}

	db_printf("EPN:%08x PPN:%08x ", (u_int)(pteh & SH5_PTEH_EPN_MASK),
	    (u_int)(ptel & SH5_PTEH_EPN_MASK));

	switch (ptel & SH5_PTEL_SZ_MASK) {
	case SH5_PTEL_SZ_4KB:
		db_printf("SZ:4KB   ");
		break;

	case SH5_PTEL_SZ_64KB:
		db_printf("SZ:64KB  ");
		break;

	case SH5_PTEL_SZ_1MB:
		db_printf("SZ:1MB   ");
		break;

	case SH5_PTEL_SZ_512MB:
		db_printf("SZ:512MB ");
		break;
	}

	db_printf("PR:%c%c%c%c ",
	    (ptel & SH5_PTEL_PR_U) ? 'u' : 's',
	    (ptel & SH5_PTEL_PR_R) ? 'r' : '-',
	    (ptel & SH5_PTEL_PR_W) ? 'w' : '-',
	    (ptel & SH5_PTEL_PR_X) ? 'x' : '-');

	switch (ptel & SH5_PTEL_CB_MASK) {
	case SH5_PTEL_CB_NOCACHE:
		db_printf("OFF\n");
		break;

	case SH5_PTEL_CB_DEVICE:
		db_printf("DEV\n");
		break;

	case SH5_PTEL_CB_WRITEBACK:
		db_printf("WBK\n");
		break;

	case SH5_PTEL_CB_WRITETHRU:
		db_printf("WTH\n");
		break;
	}
}
