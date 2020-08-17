/*	$NetBSD: mips_stacktrace.c,v 1.4 2020/08/17 21:50:14 mrg Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: NetBSD: trap.c,v 1.255 2020/07/13 09:00:40 simonb Exp
 * from: Utah Hdr: trap.c 1.32 91/04/06
 *
 *	@(#)trap.c	8.5 (Berkeley) 1/11/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mips_stacktrace.c,v 1.4 2020/08/17 21:50:14 mrg Exp $");

#ifdef _KERNEL_OPT
#include "opt_ddb.h"
#include "opt_kgdb.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>

#include <mips/locore.h>
#include <mips/mips_opcode.h>
#include <mips/stacktrace.h>

#if defined(_KMEMUSER) && !defined(DDB)
#define DDB 1
#endif

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_user.h>
#include <ddb/db_access.h>
#endif

#ifdef KGDB
#include <sys/kgdb.h>
#endif

#ifndef DDB_TRACE

#if defined(DEBUG) || defined(DDB) || defined(KGDB) || defined(geo)

extern char start[], edata[], verylocore[];
#ifdef MIPS1
extern char mips1_kern_gen_exception[];
extern char mips1_user_gen_exception[];
extern char mips1_kern_intr[];
extern char mips1_user_intr[];
extern char mips1_systemcall[];
#endif
#ifdef MIPS3
extern char mips3_kern_gen_exception[];
extern char mips3_user_gen_exception[];
extern char mips3_kern_intr[];
extern char mips3_user_intr[];
extern char mips3_systemcall[];
#endif
#ifdef MIPS32
extern char mips32_kern_gen_exception[];
extern char mips32_user_gen_exception[];
extern char mips32_kern_intr[];
extern char mips32_user_intr[];
extern char mips32_systemcall[];
#endif
#ifdef MIPS32R2
extern char mips32r2_kern_gen_exception[];
extern char mips32r2_user_gen_exception[];
extern char mips32r2_kern_intr[];
extern char mips32r2_user_intr[];
extern char mips32r2_systemcall[];
#endif
#ifdef MIPS64
extern char mips64_kern_gen_exception[];
extern char mips64_user_gen_exception[];
extern char mips64_kern_intr[];
extern char mips64_user_intr[];
extern char mips64_systemcall[];
#endif
#ifdef MIPS64R2
extern char mips64r2_kern_gen_exception[];
extern char mips64r2_user_gen_exception[];
extern char mips64r2_kern_intr[];
extern char mips64r2_user_intr[];
extern char mips64r2_systemcall[];
#endif

#define	MIPS_JR_RA	0x03e00008	/* instruction code for jr ra */
#define	MIPS_JR_K0	0x03400008	/* instruction code for jr k0 */
#define	MIPS_ERET	0x42000018	/* instruction code for eret */

int main(void *);	/* XXX */

/*
 * Functions ``special'' enough to print by name
 */
#define Name(_fn)  { (void*)_fn, # _fn }
static const struct { void *addr; const char *name;} names[] = {
#ifdef _KERNEL
	Name(stacktrace),
	Name(stacktrace_subr),
	Name(main),
	Name(trap),

#ifdef MIPS1	/*  r2000 family  (mips-I CPU) */
	Name(mips1_kern_gen_exception),
	Name(mips1_user_gen_exception),
	Name(mips1_systemcall),
	Name(mips1_kern_intr),
	Name(mips1_user_intr),
#endif	/* MIPS1 */

#if defined(MIPS3)			/* r4000 family (mips-III CPU) */
	Name(mips3_kern_gen_exception),
	Name(mips3_user_gen_exception),
	Name(mips3_systemcall),
	Name(mips3_kern_intr),
	Name(mips3_user_intr),
#endif	/* MIPS3 */

#if defined(MIPS32)			/* MIPS32 family (mips-III CPU) */
	Name(mips32_kern_gen_exception),
	Name(mips32_user_gen_exception),
	Name(mips32_systemcall),
	Name(mips32_kern_intr),
	Name(mips32_user_intr),
#endif	/* MIPS32 */

#if defined(MIPS32R2)			/* MIPS32R2 family (mips-III CPU) */
	Name(mips32r2_kern_gen_exception),
	Name(mips32r2_user_gen_exception),
	Name(mips32r2_systemcall),
	Name(mips32r2_kern_intr),
	Name(mips32r2_user_intr),
#endif	/* MIPS32R2 */

#if defined(MIPS64)			/* MIPS64 family (mips-III CPU) */
	Name(mips64_kern_gen_exception),
	Name(mips64_user_gen_exception),
	Name(mips64_systemcall),
	Name(mips64_kern_intr),
	Name(mips64_user_intr),
#endif	/* MIPS64 */

#if defined(MIPS64R2)			/* MIPS64R2 family (mips-III CPU) */
	Name(mips64r2_kern_gen_exception),
	Name(mips64r2_user_gen_exception),
	Name(mips64r2_systemcall),
	Name(mips64r2_kern_intr),
	Name(mips64r2_user_intr),
#endif	/* MIPS64R2 */

	Name(cpu_idle),
	Name(cpu_switchto),
#endif /* _KERNEL */
	{0, 0}
};


static bool
kdbpeek(vaddr_t addr, unsigned *valp)
{
	if (addr & 3) {
		printf("kdbpeek: unaligned address %#"PRIxVADDR"\n", addr);
		/* We might have been called from DDB, so do not go there. */
		return false;
	} else if (addr == 0) {
		printf("kdbpeek: NULL\n");
		return false;
	} else {
#if _KERNEL
		*valp = *(unsigned *)addr;
#else
		db_read_bytes((db_addr_t)addr, sizeof(unsigned), (char *)valp);
#endif
		return true;
	}
}

static mips_reg_t
kdbrpeek(vaddr_t addr, size_t n)
{
	mips_reg_t rc = 0;

	if (addr & (n - 1)) {
		printf("kdbrpeek: unaligned address %#"PRIxVADDR"\n", addr);
#if _KERNEL
		/* We might have been called from DDB, so do not go there. */
		stacktrace();
#endif
		rc = -1;
	} else if (addr == 0) {
		printf("kdbrpeek: NULL\n");
		rc = 0xdeadfeed;
	} else {
		if (sizeof(mips_reg_t) == 8 && n == 8)
#if _KERNEL
			db_read_bytes((db_addr_t)addr, sizeof(int64_t), (char *)&rc);
		else
			db_read_bytes((db_addr_t)addr, sizeof(int32_t), (char *)&rc);
#else
			rc = *(int64_t *)addr;
 		else
			rc = *(int32_t *)addr;
#endif
	}
	return rc;
}

/*
 * Map a function address to a string name, if known; or a hex string.
 */
static const char *
fn_name(vaddr_t addr)
{
	static char buf[17];
	int i = 0;
#ifdef DDB
	db_expr_t diff;
	db_sym_t sym;
	const char *symname;
#endif

#ifdef DDB
	diff = 0;
	symname = NULL;
	sym = db_search_symbol(addr, DB_STGY_ANY, &diff);
	db_symbol_values(sym, &symname, 0);
	if (symname && diff == 0)
		return (symname);
#endif
	for (i = 0; names[i].name; i++)
		if (names[i].addr == (void*)addr)
			return (names[i].name);
	snprintf(buf, sizeof(buf), "%#"PRIxVADDR, addr);
	return (buf);
}

/*
 * Do a stack backtrace.
 * (*printfn)()  prints the output to either the system log,
 * the console, or both.
 */
void
stacktrace_subr(mips_reg_t a0, mips_reg_t a1, mips_reg_t a2, mips_reg_t a3,
    vaddr_t pc, vaddr_t sp, vaddr_t fp, vaddr_t ra,
    void (*printfn)(const char*, ...))
{
	vaddr_t va, subr;
	unsigned instr, mask;
	InstFmt i;
	int more, stksize;
	unsigned int frames =  0;
	int foundframesize = 0;
	mips_reg_t regs[32] = {
		[_R_ZERO] = 0,
		[_R_A0] = a0, [_R_A1] = a1, [_R_A2] = a2, [_R_A3] = a3,
		[_R_RA] = ra,
	};
#ifdef DDB
	db_expr_t diff;
	db_sym_t sym;
#endif

/* Jump here when done with a frame, to start a new one */
loop:
	stksize = 0;
	subr = 0;
	mask = 1;
	if (frames++ > 100) {
		(*printfn)("\nstackframe count exceeded\n");
		/* return breaks stackframe-size heuristics with gcc -O2 */
		goto finish;	/*XXX*/
	}

	/* check for bad SP: could foul up next frame */
	if ((sp & (sizeof(sp)-1)) || (intptr_t)sp >= 0) {
		(*printfn)("SP 0x%x: not in kernel\n", sp);
		ra = 0;
		subr = 0;
		goto done;
	}

	/* Check for bad PC */
	if (pc & 3 || (intptr_t)pc >= 0 || (intptr_t)pc >= (intptr_t)edata) {
		(*printfn)("PC 0x%x: not in kernel space\n", pc);
		ra = 0;
		goto done;
	}

#ifdef DDB
	/*
	 * Check the kernel symbol table to see the beginning of
	 * the current subroutine.
	 */
	diff = 0;
	sym = db_search_symbol(pc, DB_STGY_ANY, &diff);
	if (sym != DB_SYM_NULL && diff == 0) {
		/* check func(foo) __attribute__((__noreturn__)) case */
		if (!kdbpeek(pc - 2 * sizeof(unsigned), &instr))
			return;
		i.word = instr;
		if (i.JType.op == OP_JAL) {
			sym = db_search_symbol(pc - sizeof(int),
			    DB_STGY_ANY, &diff);
			if (sym != DB_SYM_NULL && diff != 0)
				diff += sizeof(int);
		}
	}
	if (sym == DB_SYM_NULL) {
		ra = 0;
		goto done;
	}
	va = pc - diff;
#else
	/*
	 * Find the beginning of the current subroutine by scanning backwards
	 * from the current PC for the end of the previous subroutine.
	 *
	 * XXX This won't work well because nowadays gcc is so aggressive
	 *     as to reorder instruction blocks for branch-predict.
	 *     (i.e. 'jr ra' wouldn't indicate the end of subroutine)
	 */
	va = pc;
	do {
		va -= sizeof(int);
		if (va <= (vaddr_t)verylocore)
			goto finish;
		if (!kdbpeek(va, &instr))
			return;
		if (instr == MIPS_ERET)
			goto mips3_eret;
	} while (instr != MIPS_JR_RA && instr != MIPS_JR_K0);
	/* skip back over branch & delay slot */
	va += sizeof(int);
mips3_eret:
	va += sizeof(int);
	/* skip over nulls which might separate .o files */
	instr = 0;
	while (instr == 0) {
		if (!kdbpeek(va, &instr))
			return;
		va += sizeof(int);
	}
#endif
	subr = va;

	/* scan forwards to find stack size and any saved registers */
	stksize = 0;
	more = 3;
	mask &= 0x40ff0001;	/* if s0-s8 are valid, leave then as valid */
	foundframesize = 0;
	for (va = subr; more; va += sizeof(int),
			      more = (more == 3) ? 3 : more - 1) {
		/* stop if hit our current position */
		if (va >= pc)
			break;
		if (!kdbpeek(va, &instr))
			return;
		i.word = instr;
		switch (i.JType.op) {
		case OP_SPECIAL:
			switch (i.RType.func) {
			case OP_JR:
			case OP_JALR:
				more = 2; /* stop after next instruction */
				break;

			case OP_ADD:
			case OP_ADDU:
			case OP_DADD:
			case OP_DADDU:
				if (!(mask & (1 << i.RType.rd))
				    || !(mask & (1 << i.RType.rt)))
					break;
				if (i.RType.rd != _R_ZERO)
					break;
				mask |= (1 << i.RType.rs);
				regs[i.RType.rs] = regs[i.RType.rt];
				if (i.RType.func >= OP_DADD)
					break;
				regs[i.RType.rs] = (int32_t)regs[i.RType.rs];
				break;

			case OP_SYSCALL:
			case OP_BREAK:
				more = 1; /* stop now */
				break;
			}
			break;

		case OP_REGIMM:
		case OP_J:
		case OP_JAL:
		case OP_BEQ:
		case OP_BNE:
		case OP_BLEZ:
		case OP_BGTZ:
			more = 2; /* stop after next instruction */
			break;

		case OP_COP0:
		case OP_COP1:
		case OP_COP2:
		case OP_COP3:
			switch (i.RType.rs) {
			case OP_BCx:
			case OP_BCy:
				more = 2; /* stop after next instruction */
			};
			break;

		case OP_SW:
#if !defined(__mips_o32)
		case OP_SD:
#endif
		{
			size_t size = (i.JType.op == OP_SW) ? 4 : 8;

			/* look for saved registers on the stack */
			if (i.IType.rs != _R_SP)
				break;
			switch (i.IType.rt) {
			case _R_A0: /* a0 */
			case _R_A1: /* a1 */
			case _R_A2: /* a2 */
			case _R_A3: /* a3 */
			case _R_S0: /* s0 */
			case _R_S1: /* s1 */
			case _R_S2: /* s2 */
			case _R_S3: /* s3 */
			case _R_S4: /* s4 */
			case _R_S5: /* s5 */
			case _R_S6: /* s6 */
			case _R_S7: /* s7 */
			case _R_S8: /* s8 */
			case _R_RA: /* ra */
				regs[i.IType.rt] =
				    kdbrpeek(sp + (int16_t)i.IType.imm, size);
				mask |= (1 << i.IType.rt);
				break;
			}
			break;
		}

		case OP_ADDI:
		case OP_ADDIU:
#if !defined(__mips_o32)
		case OP_DADDI:
		case OP_DADDIU:
#endif
			/* look for stack pointer adjustment */
			if (i.IType.rs != _R_SP || i.IType.rt != _R_SP)
				break;
			/* don't count pops for mcount */
			if (!foundframesize) {
				stksize = - ((short)i.IType.imm);
				foundframesize = 1;
			}
			break;
		}
	}
done:
	if (mask & (1 << _R_RA))
		ra = regs[_R_RA];
	(*printfn)("%#"PRIxVADDR": %s+%"PRIxVADDR" (%"PRIxREGISTER",%"PRIxREGISTER",%"PRIxREGISTER",%"PRIxREGISTER") ra %"PRIxVADDR" sz %d\n",
		sp, fn_name(subr), pc - subr,
		regs[_R_A0], regs[_R_A1], regs[_R_A2], regs[_R_A3],
		ra, stksize);

	if (ra) {
		if (pc == ra && stksize == 0)
			(*printfn)("stacktrace: loop!\n");
		else {
			pc = ra;
			sp += stksize;
			ra = 0;
			goto loop;
		}
	} else {
finish:
#ifdef _KERNEL
		(*printfn)("User-level: pid %d.%d\n",
		    curlwp->l_proc->p_pid, curlwp->l_lid);
#else
		(*printfn)("User-level: FIXME\n");
#endif
	}
}

#endif /* DEBUG || DDB || KGDB || geo */
#endif /* DDB_TRACE */
