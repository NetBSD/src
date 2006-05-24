/* $NetBSD: trap.c,v 1.2.10.2 2006/05/24 15:47:59 tron Exp $ */

/*-
 * Copyright (c) 2005 Marcel Moolenaar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 2000, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center, by Charles M. Hannum, and by Ross Harvey.
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


#include "opt_ddb.h"

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: trap.c,v 1.2.10.2 2006/05/24 15:47:59 tron Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/sa.h>
#include <sys/savar.h>

#include <sys/userret.h>

#include <uvm/uvm_extern.h>

#include <machine/frame.h>
#include <machine/md_var.h>
#include <machine/cpu.h>
#include <machine/ia64_cpu.h>
#include <machine/fpu.h>
#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#include <ia64/disasm/disasm.h>


static const char *ia64_vector_names[] = {
	"VHPT Translation",			/* 0 */
	"Instruction TLB",			/* 1 */
	"Data TLB",				/* 2 */
	"Alternate Instruction TLB",		/* 3 */
	"Alternate Data TLB",			/* 4 */
	"Data Nested TLB",			/* 5 */
	"Instruction Key Miss",			/* 6 */
	"Data Key Miss",			/* 7 */
	"Dirty-Bit",				/* 8 */
	"Instruction Access-Bit",		/* 9 */
	"Data Access-Bit",			/* 10 */
	"Break Instruction",			/* 11 */
	"External Interrupt",			/* 12 */
	"Reserved 13",				/* 13 */
	"Reserved 14",				/* 14 */
	"Reserved 15",				/* 15 */
	"Reserved 16",				/* 16 */
	"Reserved 17",				/* 17 */
	"Reserved 18",				/* 18 */
	"Reserved 19",				/* 19 */
	"Page Not Present",			/* 20 */
	"Key Permission",			/* 21 */
	"Instruction Access Rights",		/* 22 */
	"Data Access Rights",			/* 23 */
	"General Exception",			/* 24 */
	"Disabled FP-Register",			/* 25 */
	"NaT Consumption",			/* 26 */
	"Speculation",				/* 27 */
	"Reserved 28",				/* 28 */
	"Debug",				/* 29 */
	"Unaligned Reference",			/* 30 */
	"Unsupported Data Reference",		/* 31 */
	"Floating-point Fault",			/* 32 */
	"Floating-point Trap",			/* 33 */
	"Lower-Privilege Transfer Trap",	/* 34 */
	"Taken Branch Trap",			/* 35 */
	"Single Step Trap",			/* 36 */
	"Reserved 37",				/* 37 */
	"Reserved 38",				/* 38 */
	"Reserved 39",				/* 39 */
	"Reserved 40",				/* 40 */
	"Reserved 41",				/* 41 */
	"Reserved 42",				/* 42 */
	"Reserved 43",				/* 43 */
	"Reserved 44",				/* 44 */
	"IA-32 Exception",			/* 45 */
	"IA-32 Intercept",			/* 46 */
	"IA-32 Interrupt",			/* 47 */
	"Reserved 48",				/* 48 */
	"Reserved 49",				/* 49 */
	"Reserved 50",				/* 50 */
	"Reserved 51",				/* 51 */
	"Reserved 52",				/* 52 */
	"Reserved 53",				/* 53 */
	"Reserved 54",				/* 54 */
	"Reserved 55",				/* 55 */
	"Reserved 56",				/* 56 */
	"Reserved 57",				/* 57 */
	"Reserved 58",				/* 58 */
	"Reserved 59",				/* 59 */
	"Reserved 60",				/* 60 */
	"Reserved 61",				/* 61 */
	"Reserved 62",				/* 62 */
	"Reserved 63",				/* 63 */
	"Reserved 64",				/* 64 */
	"Reserved 65",				/* 65 */
	"Reserved 66",				/* 66 */
	"Reserved 67",				/* 67 */
};

struct bitname {
	uint64_t mask;
	const char* name;
};

static void
printbits(uint64_t mask, struct bitname *bn, int count)
{
	int i, first = 1;
	uint64_t bit;

	for (i = 0; i < count; i++) {
		/*
		 * Handle fields wider than one bit.
		 */
		bit = bn[i].mask & ~(bn[i].mask - 1);
		if (bn[i].mask > bit) {
			if (first)
				first = 0;
			else
				printf(",");
			printf("%s=%ld", bn[i].name,
			       (mask & bn[i].mask) / bit);
		} else if (mask & bit) {
			if (first)
				first = 0;
			else
				printf(",");
			printf("%s", bn[i].name);
		}
	}
}

struct bitname psr_bits[] = {
	{IA64_PSR_BE,	"be"},
	{IA64_PSR_UP,	"up"},
	{IA64_PSR_AC,	"ac"},
	{IA64_PSR_MFL,	"mfl"},
	{IA64_PSR_MFH,	"mfh"},
	{IA64_PSR_IC,	"ic"},
	{IA64_PSR_I,	"i"},
	{IA64_PSR_PK,	"pk"},
	{IA64_PSR_DT,	"dt"},
	{IA64_PSR_DFL,	"dfl"},
	{IA64_PSR_DFH,	"dfh"},
	{IA64_PSR_SP,	"sp"},
	{IA64_PSR_PP,	"pp"},
	{IA64_PSR_DI,	"di"},
	{IA64_PSR_SI,	"si"},
	{IA64_PSR_DB,	"db"},
	{IA64_PSR_LP,	"lp"},
	{IA64_PSR_TB,	"tb"},
	{IA64_PSR_RT,	"rt"},
	{IA64_PSR_CPL,	"cpl"},
	{IA64_PSR_IS,	"is"},
	{IA64_PSR_MC,	"mc"},
	{IA64_PSR_IT,	"it"},
	{IA64_PSR_ID,	"id"},
	{IA64_PSR_DA,	"da"},
	{IA64_PSR_DD,	"dd"},
	{IA64_PSR_SS,	"ss"},
	{IA64_PSR_RI,	"ri"},
	{IA64_PSR_ED,	"ed"},
	{IA64_PSR_BN,	"bn"},
	{IA64_PSR_IA,	"ia"},
};

static void
printpsr(uint64_t psr)
{
	printbits(psr, psr_bits, sizeof(psr_bits)/sizeof(psr_bits[0]));
}

struct bitname isr_bits[] = {
	{IA64_ISR_CODE,	"code"},
	{IA64_ISR_VECTOR, "vector"},
	{IA64_ISR_X,	"x"},
	{IA64_ISR_W,	"w"},
	{IA64_ISR_R,	"r"},
	{IA64_ISR_NA,	"na"},
	{IA64_ISR_SP,	"sp"},
	{IA64_ISR_RS,	"rs"},
	{IA64_ISR_IR,	"ir"},
	{IA64_ISR_NI,	"ni"},
	{IA64_ISR_SO,	"so"},
	{IA64_ISR_EI,	"ei"},
	{IA64_ISR_ED,	"ed"},
};

static void printisr(uint64_t isr)
{
	printbits(isr, isr_bits, sizeof(isr_bits)/sizeof(isr_bits[0]));
}

static void
printtrap(int vector, struct trapframe *tf, int isfatal, int user)
{

	printf("\n");
	printf("%s %s trap (cpu %lu):\n", isfatal? "fatal" : "handled",
	       user ? "user" : "kernel", curcpu()->ci_cpuid);
	printf("\n");
	printf("    trap vector = 0x%x (%s)\n",
	       vector, ia64_vector_names[vector]);
	printf("    cr.iip      = 0x%lx\n", tf->tf_special.iip);
	printf("    cr.ipsr     = 0x%lx (", tf->tf_special.psr);
	printpsr(tf->tf_special.psr);
	printf(")\n");
	printf("    cr.isr      = 0x%lx (", tf->tf_special.isr);
	printisr(tf->tf_special.isr);
	printf(")\n");
	printf("    cr.ifa      = 0x%lx\n", tf->tf_special.ifa);
	if (tf->tf_special.psr & IA64_PSR_IS) {
		printf("    ar.cflg     = 0x%lx\n", ia64_get_cflg());
		printf("    ar.csd      = 0x%lx\n", ia64_get_csd());
		printf("    ar.ssd      = 0x%lx\n", ia64_get_ssd());
	}
	printf("    curlwp   = %p\n", curlwp);
	if (curproc != NULL)
		printf("        pid = %d, comm = %s\n",
		       curproc->p_pid, curproc->p_comm);
	printf("\n");
}

/*
 * We got a trap caused by a break instruction and the immediate was 0.
 * This indicates that we may have a break.b with some non-zero immediate.
 * The break.b doesn't cause the immediate to be put in cr.iim.  Hence,
 * we need to disassemble the bundle and return the immediate found there.
 * This may be a 0 value anyway.  Return 0 for any error condition.  This
 * will result in a SIGILL, which is pretty much the best thing to do.
 */
static uint64_t
trap_decode_break(struct trapframe *tf)
{
	struct asm_bundle bundle;
	struct asm_inst *inst;
	int slot;

	if (!asm_decode(tf->tf_special.iip, &bundle))
		return (0);

	slot = ((tf->tf_special.psr & IA64_PSR_RI) == IA64_PSR_RI_0) ? 0 :
            ((tf->tf_special.psr & IA64_PSR_RI) == IA64_PSR_RI_1) ? 1 : 2;
	inst = bundle.b_inst + slot;

	/*
	 * Sanity checking: It must be a break instruction and the operand
	 * that has the break value must be an immediate.
	 */
	if (inst->i_op != ASM_OP_BREAK ||
	    inst->i_oper[1].o_type != ASM_OPER_IMM)
		return (0);

	return (inst->i_oper[1].o_value);
}


/*
 * Start a new LWP
 */
void
startlwp(arg)
	void *arg;
{
	return;
}

void
upcallret(struct lwp *l)
{
	return;
}

#ifdef DDB
	int call_debugger = 1;

/*
 * Enter the debugger due to a trap.
 */

int
ia64_trap(int type, int code, db_regs_t *regs)
{

	/* XXX: Switch stacks ? */

	/* Debugger is not re-entrant. */

	ddb_regp = regs;
	db_trap(type, code);
	return 1; /* XXX: Always handled ??? */

}

#endif

void
trap_panic(int vector, struct trapframe *tf)
{

	printtrap(vector, tf, 1, TRAPF_USERMODE(tf));

#ifdef DDB
	if (ia64_trap(vector, 0, tf)) return;
#endif
	panic("trap");

	return;
}

/*
 *
 */
int
do_ast(struct trapframe *tf)
{
	return 0;
}

/*
 * Trap is called from exception.s to handle most types of processor traps.
 */
/*ARGSUSED*/
void
trap(int vector, struct trapframe *tf)
{

	struct proc *p;
	struct lwp *l;
	uint64_t ucode;
	int sig, user;
	u_int sticks;
	ksiginfo_t ksi;

	user = TRAPF_USERMODE(tf) ? 1 : 0;

	l = curlwp;

	ucode = 0;

#if 0
	printtrap(vector, tf, 0, TRAPF_USERMODE(tf));
#endif
	if (user) {
		ia64_set_fpsr(IA64_FPSR_DEFAULT);
		sticks = p->p_sticks;
		l->l_md.md_tf = tf;
		p = l->l_proc;
	} else {
		sticks = 0;		/* XXX bogus -Wuninitialized warning */
		p = NULL;
	}
	sig = 0;
	switch (vector) {
	case IA64_VEC_VHPT:
		/*
		 * This one is tricky. We should hardwire the VHPT, but
		 * don't at this time. I think we're mostly lucky that
		 * the VHPT is mapped.
		 */
		trap_panic(vector, tf);
		break;
	case IA64_VEC_ITLB:
	case IA64_VEC_DTLB:
	case IA64_VEC_EXT_INTR:
		/* We never call trap() with these vectors. */
		trap_panic(vector, tf);
		break;

	case IA64_VEC_ALT_ITLB:
	case IA64_VEC_ALT_DTLB:
		/*
		 * These should never happen, because regions 0-4 use the
		 * VHPT. If we get one of these it means we didn't program
		 * the region registers correctly.
		 */
		trap_panic(vector, tf);
		break;

	case IA64_VEC_NESTED_DTLB:
		/*
		 * We never call trap() with this vector. We may want to
		 * do that in the future in case the nested TLB handler
		 * could not find the translation it needs. In that case
		 * we could switch to a special (hardwired) stack and
		 * come here to produce a nice panic().
		 */
		trap_panic(vector, tf);
		break;

	case IA64_VEC_IKEY_MISS:
	case IA64_VEC_DKEY_MISS:
	case IA64_VEC_KEY_PERMISSION:
		/*
		 * We don't use protection keys, so we should never get
		 * these faults.
		 */
		trap_panic(vector, tf);
		break;

	case IA64_VEC_DIRTY_BIT:
	case IA64_VEC_INST_ACCESS:
	case IA64_VEC_DATA_ACCESS:
		/*
		 * We get here if we read or write to a page of which the
		 * PTE does not have the access bit or dirty bit set and
		 * we can not find the PTE in our datastructures. This
		 * either means we have a stale PTE in the TLB, or we lost
		 * the PTE in our datastructures.
		 */
		trap_panic(vector, tf);
		break;

	case IA64_VEC_BREAK:
		if (user) {
			ucode = (int)tf->tf_special.ifa & 0x1FFFFF;
			if (ucode == 0) {
				/*
				 * A break.b doesn't cause the immediate to be
				 * stored in cr.iim (and saved in the TF in
				 * tf_special.ifa).  We need to decode the
				 * instruction to find out what the immediate
				 * was.  Note that if the break instruction
				 * didn't happen to be a break.b, but any
				 * other break with an immediate of 0, we
				 * will do unnecessary work to get the value
				 * we already had.  Not an issue, because a
				 * break 0 is invalid.
				 */
				ucode = trap_decode_break(tf);
			}
			if (ucode < 0x80000) {
				/* Software interrupts. */
				switch (ucode) {
				case 0:		/* Unknown error. */
					sig = SIGILL;
					break;
				case 1:		/* Integer divide by zero. */
					sig = SIGFPE;
					ucode = FPE_INTDIV;
					break;
				case 2:		/* Integer overflow. */
					sig = SIGFPE;
					ucode = FPE_INTOVF;
					break;
				case 3:		/* Range check/bounds check. */
					sig = SIGFPE;
					ucode = FPE_FLTSUB;
					break;
				case 6: 	/* Decimal overflow. */
				case 7: 	/* Decimal divide by zero. */
				case 8: 	/* Packed decimal error. */
				case 9: 	/* Invalid ASCII digit. */
				case 10:	/* Invalid decimal digit. */
					sig = SIGFPE;
					ucode = FPE_FLTINV;
					break;
				case 4:		/* Null pointer dereference. */
				case 5:		/* Misaligned data. */
				case 11:	/* Paragraph stack overflow. */
					sig = SIGSEGV;
					break;
				default:
					sig = SIGILL;
					break;
				}
			} else if (ucode < 0x100000) {
				/* Debugger breakpoint. */
				tf->tf_special.psr &= ~IA64_PSR_SS;
				sig = SIGTRAP;
#if 0
			} else if (ucode == 0x100000) {
				break_syscall(tf);
				return;		/* do_ast() already called. */

			} else if (ucode == 0x180000) {
				mcontext_t mc;

				error = copyin((void*)tf->tf_scratch.gr8,
				    &mc, sizeof(mc));
				if (!error) {
					set_mcontext(td, &mc);
					return;	/* Don't call do_ast()!!! */
				}
				sig = SIGSEGV;
				ucode = tf->tf_scratch.gr8;
#endif
			} else
				sig = SIGILL;
		} else {
			trap_panic(vector, tf);
			goto out;
		}
		break;

/* XXX: Fill in the rest */

	case IA64_VEC_DEBUG:
	case IA64_VEC_SINGLE_STEP_TRAP:
		tf->tf_special.psr &= ~IA64_PSR_SS;
		if (!user) {
			trap_panic(vector, tf);
			goto out;
		}
		sig = SIGTRAP;
		break;



	default:
		/* Reserved vectors get here. Should never happen of course. */
		trap_panic(vector, tf);
		break;
	}

	printf("sig = %d", sig);
	KASSERT(sig != 0);

	KSI_INIT(&ksi);
	ksi.ksi_signo = sig;
	ksi.ksi_code = ucode;
	trapsignal(l, &ksi);

#if 1
out:
#endif

	if (user) {
		mi_userret(l);
	}


	return;
}
