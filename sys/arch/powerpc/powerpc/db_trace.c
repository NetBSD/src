/*	$NetBSD: db_trace.c,v 1.63 2023/04/13 06:39:23 riastradh Exp $	*/
/*	$OpenBSD: db_trace.c,v 1.3 1997/03/21 02:10:48 niklas Exp $	*/

/*
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_trace.c,v 1.63 2023/04/13 06:39:23 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "opt_ppcarch.h"
#endif

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <machine/db_machdep.h>
#include <machine/pmap.h>

#include <powerpc/pcb.h>
#include <powerpc/psl.h>
#include <powerpc/spr.h>
#if defined (PPC_OEA) || defined(PPC_OEA64) || defined (PPC_OEA64_BRIDGE)
#include <powerpc/oea/spr.h>
#elif defined(PPC_IBM4XX)
#include <powerpc/ibm4xx/spr.h>
#elif defined(PPC_BOOKE)
#include <powerpc/booke/spr.h>
#else
#ifdef _KERNEL
#error unknown powerpc variants
#endif
#endif

#ifndef _KERNEL			/* crash(8) */
#include <unistd.h>
#define	PAGE_SIZE	((unsigned)sysconf(_SC_PAGESIZE))
#endif

#include <ddb/db_access.h>
#include <ddb/db_interface.h>
#include <ddb/db_proc.h>
#include <ddb/db_sym.h>
#include <ddb/db_variables.h>

#define	R(P)								      \
({									      \
	__typeof__(*(P)) __db_tmp;					      \
	db_read_bytes((db_addr_t)(P), sizeof(*(P)), (char *)&__db_tmp);	      \
	__db_tmp;							      \
})

const struct db_variable db_regs[] = {
	{ "r0",  (long *)&ddb_regs.r[0],  FCN_NULL, NULL },
	{ "r1",  (long *)&ddb_regs.r[1],  FCN_NULL, NULL },
	{ "r2",  (long *)&ddb_regs.r[2],  FCN_NULL, NULL },
	{ "r3",  (long *)&ddb_regs.r[3],  FCN_NULL, NULL },
	{ "r4",  (long *)&ddb_regs.r[4],  FCN_NULL, NULL },
	{ "r5",  (long *)&ddb_regs.r[5],  FCN_NULL, NULL },
	{ "r6",  (long *)&ddb_regs.r[6],  FCN_NULL, NULL },
	{ "r7",  (long *)&ddb_regs.r[7],  FCN_NULL, NULL },
	{ "r8",  (long *)&ddb_regs.r[8],  FCN_NULL, NULL },
	{ "r9",  (long *)&ddb_regs.r[9],  FCN_NULL, NULL },
	{ "r10", (long *)&ddb_regs.r[10], FCN_NULL, NULL },
	{ "r11", (long *)&ddb_regs.r[11], FCN_NULL, NULL },
	{ "r12", (long *)&ddb_regs.r[12], FCN_NULL, NULL },
	{ "r13", (long *)&ddb_regs.r[13], FCN_NULL, NULL },
	{ "r14", (long *)&ddb_regs.r[14], FCN_NULL, NULL },
	{ "r15", (long *)&ddb_regs.r[15], FCN_NULL, NULL },
	{ "r16", (long *)&ddb_regs.r[16], FCN_NULL, NULL },
	{ "r17", (long *)&ddb_regs.r[17], FCN_NULL, NULL },
	{ "r18", (long *)&ddb_regs.r[18], FCN_NULL, NULL },
	{ "r19", (long *)&ddb_regs.r[19], FCN_NULL, NULL },
	{ "r20", (long *)&ddb_regs.r[20], FCN_NULL, NULL },
	{ "r21", (long *)&ddb_regs.r[21], FCN_NULL, NULL },
	{ "r22", (long *)&ddb_regs.r[22], FCN_NULL, NULL },
	{ "r23", (long *)&ddb_regs.r[23], FCN_NULL, NULL },
	{ "r24", (long *)&ddb_regs.r[24], FCN_NULL, NULL },
	{ "r25", (long *)&ddb_regs.r[25], FCN_NULL, NULL },
	{ "r26", (long *)&ddb_regs.r[26], FCN_NULL, NULL },
	{ "r27", (long *)&ddb_regs.r[27], FCN_NULL, NULL },
	{ "r28", (long *)&ddb_regs.r[28], FCN_NULL, NULL },
	{ "r29", (long *)&ddb_regs.r[29], FCN_NULL, NULL },
	{ "r30", (long *)&ddb_regs.r[30], FCN_NULL, NULL },
	{ "r31", (long *)&ddb_regs.r[31], FCN_NULL, NULL },
	{ "iar", (long *)&ddb_regs.iar,   FCN_NULL, NULL },
	{ "msr", (long *)&ddb_regs.msr,   FCN_NULL, NULL },
	{ "lr",  (long *)&ddb_regs.lr,    FCN_NULL, NULL },
	{ "ctr", (long *)&ddb_regs.ctr,   FCN_NULL, NULL },
	{ "cr",  (long *)&ddb_regs.cr,    FCN_NULL, NULL },
	{ "xer", (long *)&ddb_regs.xer,   FCN_NULL, NULL },
	{ "mq",  (long *)&ddb_regs.mq,    FCN_NULL, NULL },
#ifdef PPC_IBM4XX		/* XXX crash(8) */
	{ "dear", (long *)&ddb_regs.dear, FCN_NULL, NULL },
	{ "esr", (long *)&ddb_regs.esr,   FCN_NULL, NULL },
	{ "pid", (long *)&ddb_regs.pid,   FCN_NULL, NULL },
#endif
};
const struct db_variable * const db_eregs = db_regs + sizeof (db_regs)/sizeof (db_regs[0]);

/*
 *	Frame tracing.
 */
void
db_stack_trace_print(db_expr_t addr, bool have_addr, db_expr_t count,
	const char *modif, void (*pr)(const char *, ...))
{
	db_addr_t frame, lr, *args;
	db_expr_t diff;
	db_sym_t sym;
	const char *symname;
	const char *cp = modif;
	char c;
	bool kernel_only = true;
	bool trace_thread = false;
	bool lwpaddr = false;
#ifdef _KERNEL
	extern int trapexit[], sctrapexit[];
#ifdef PPC_BOOKE
	extern int intrcall[];
#endif
#else
	extern void *trapexit, *sctrapexit, *intrcall;
#endif
	bool full = false;
	bool in_kernel = true;

	while ((c = *cp++) != 0) {
		if (c == 'a') {
			lwpaddr = true;
			trace_thread = true;
		}
		if (c == 't')
			trace_thread = true;
		if (c == 'u')
			kernel_only = false;
		if (c == 'f')
			full = true;
	}

	if (have_addr) {
		if (trace_thread) {
			struct proc *p;
			struct lwp *l;
			struct pcb *pcb;

			if (lwpaddr) {
				l = (struct lwp *)addr;
				p = R(&l->l_proc);
				(*pr)("trace: pid %d ", R(&p->p_pid));
			} else {
				(*pr)("trace: pid %d ", (int)addr);
				p = db_proc_find((pid_t)addr);
				if (p == NULL) {
					(*pr)("not found\n");
					return;
				}
				l = R(&LIST_FIRST(&p->p_lwps));
				if (l == NULL) {
					(*pr)("trace: no LWP?\n");
					return;
				}
			}
			(*pr)("lid %d ", R(&l->l_lid));
			pcb = R(&l->l_addr); /* lwp_getpcb */
			frame = (db_addr_t)R(&pcb->pcb_sp);
			(*pr)("at %p\n", frame);
		} else
			frame = (db_addr_t)addr;
	} else {
		frame = (db_addr_t)ddb_regs.r[1];
	}
	for (;;) {
		if (frame < PAGE_SIZE)
			break;
		frame = R((db_addr_t *)frame);
	    next_frame:
		args = (db_addr_t *)(frame + 8);
		if (frame < PAGE_SIZE)
			break;
	        if (count-- == 0)
			break;

		lr = R((db_addr_t *)(frame + 4)) - 4;
		if ((lr & 3) || (lr < 0x100)) {
			(*pr)("saved LR(0x%x) is invalid.", lr);
			break;
		}

		(*pr)("0x%08lx: ", frame);
		if (lr + 4 == (db_addr_t) trapexit ||
#if !defined(_KERNEL) || defined(PPC_BOOKE) /* XXX crash(8) */
		    lr + 4 == (db_addr_t) intrcall ||
#endif
		    lr + 4 == (db_addr_t) sctrapexit) {
			const char *trapstr;
			struct trapframe *tf =
			    &((struct ktrapframe *)frame)->ktf_tf;
			(*pr)("%s ",
			    R(&tf->tf_srr1) & PSL_PR ? "user" : "kernel");
			if (lr + 4 == (db_addr_t) sctrapexit) {
				(*pr)("SC trap #%d by ", R(&tf->tf_fixreg[0]));
				goto print_trap;
			}
			switch (R(&tf->tf_exc)) {
			case EXC_DSI:
#ifdef PPC_OEA			/* XXX crash(8) */
				(*pr)("DSI %s trap @ %#x by ",
				    (R(&tf->tf_dsisr) & DSISR_STORE
					? "write"
					: "read"),
				    R(&tf->tf_dar));
#endif
#ifdef PPC_IBM4XX		/* XXX crash(8) */
				trapstr = "DSI";
dsi:
				(*pr)("%s %s trap @ %#x by ", trapstr,
				    (R(&tf->tf_esr) & ESR_DST
					? "write"
					: "read"),
				    R(&tf->tf_dear));
#endif
				goto print_trap;
			case EXC_ALI:
#ifdef PPC_OEA			/* XXX crash(8) */
				(*pr)("ALI trap @ %#x (DSISR %#x) ",
				    R(&tf->tf_dar), R(&tf->tf_dsisr));
				goto print_trap;
#else
				trapstr = "ALI"; break;
#endif
			case EXC_ISI: trapstr = "ISI"; break;
			case EXC_PGM: trapstr = "PGM"; break;
			case EXC_SC: trapstr = "SC"; break;
			case EXC_EXI: trapstr = "EXI"; break;
			case EXC_MCHK: trapstr = "MCHK"; break;
			case EXC_VEC: trapstr = "VEC"; break;
			case EXC_FPU: trapstr = "FPU"; break;
			case EXC_FPA: trapstr = "FPA"; break;
			case EXC_DECR: trapstr = "DECR"; break;
			case EXC_BPT: trapstr = "BPT"; break;
			case EXC_TRC: trapstr = "TRC"; break;
			case EXC_RUNMODETRC: trapstr = "RUNMODETRC"; break;
			case EXC_PERF: trapstr = "PERF"; break;
			case EXC_SMI: trapstr = "SMI"; break;
			case EXC_RST: trapstr = "RST"; break;
			case EXC_DTMISS: trapstr = "DTMISS";
#ifdef PPC_IBM4XX		/* XXX crash(8) */
				goto dsi;
#endif
				break;
			case EXC_ITMISS: trapstr = "ITMISS"; break;
			case EXC_FIT: trapstr = "FIT"; break;
			case EXC_PIT: trapstr = "PIT"; break;
			case EXC_WDOG: trapstr = "WDOG"; break;
			default: trapstr = NULL; break;
			}
			if (trapstr != NULL) {
				(*pr)("%s trap by ", trapstr);
			} else {
				(*pr)("trap %#x by ", R(&tf->tf_exc));
			}
		   print_trap:
			lr = (db_addr_t)R(&tf->tf_srr0);
			diff = 0;
			symname = NULL;
			if (in_kernel && (R(&tf->tf_srr1) & PSL_PR) == 0) {
				sym = db_search_symbol(lr, DB_STGY_ANY, &diff);
				db_symbol_values(sym, &symname, 0);
			}
			if (symname == NULL || !strcmp(symname, "end")) {
				(*pr)("%p: srr1=%#x\n", lr, R(&tf->tf_srr1));
			} else {
				(*pr)("%s+%#x: srr1=%#x\n", symname,
				    diff, R(&tf->tf_srr1));
			}
			(*pr)("%-10s  r1=%#x cr=%#x xer=%#x ctr=%#x",
			    "",
			    R(&tf->tf_fixreg[1]),
			    R(&tf->tf_cr),
			    R(&tf->tf_xer),
			    R(&tf->tf_ctr));
#ifdef PPC_OEA			/* XXX crash(8) */
			if (R(&tf->tf_exc) == EXC_DSI)
				(*pr)(" dsisr=%#x", R(&tf->tf_dsisr));
#ifdef PPC_OEA601		/* XXX crash(8) */
			if ((mfpvr() >> 16) == MPC601) /* XXX crash(8) */
				(*pr)(" mq=%#x", R(&tf->tf_mq));
#endif /* PPC_OEA601 */
#endif /* PPC_OEA */
#ifdef PPC_IBM4XX		/* XXX crash(8) */
			if (R(&tf->tf_exc) == EXC_DSI ||
			    R(&tf->tf_exc) == EXC_DTMISS)
				(*pr)(" dear=%#x", R(&tf->tf_dear));
			(*pr)(" esr=%#x pid=%#x", R(&tf->tf_esr),
			    R(&tf->tf_pid));
#endif
			(*pr)("\n");
			frame = (db_addr_t)R(&tf->tf_fixreg[1]);
			in_kernel = !(R(&tf->tf_srr1) & PSL_PR);
			if (kernel_only && !in_kernel)
				break;
			goto next_frame;
		}

		diff = 0;
		symname = NULL;
		if (in_kernel) {
			sym = db_search_symbol(lr, DB_STGY_ANY, &diff);
			db_symbol_values(sym, &symname, 0);
		}
		if (symname == NULL || !strcmp(symname, "end"))
			(*pr)("at %p", lr);
		else
			(*pr)("at %s+%#x", symname, diff);
		if (full)
			/* Print all the args stored in that stackframe. */
			(*pr)("(%lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx)",
			    R(&args[0]), R(&args[1]), R(&args[2]), R(&args[3]),
			    R(&args[4]), R(&args[5]), R(&args[6]), R(&args[7]));
		(*pr)("\n");
	}
}
