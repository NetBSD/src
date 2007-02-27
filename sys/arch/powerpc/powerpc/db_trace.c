/*	$NetBSD: db_trace.c,v 1.41.2.1 2007/02/27 16:52:51 yamt Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: db_trace.c,v 1.41.2.1 2007/02/27 16:52:51 yamt Exp $");

#include "opt_ppcarch.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <machine/db_machdep.h>
#include <machine/pmap.h>
#include <powerpc/spr.h>

#include <ddb/db_access.h>
#include <ddb/db_interface.h>
#include <ddb/db_sym.h>
#include <ddb/db_variables.h>

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
#ifdef PPC_IBM4XX
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
	extern int trapexit[], sctrapexit[];
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
			struct user *u;

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
			u = l->l_addr;
			frame = (db_addr_t)u->u_pcb.pcb_sp;
			(*pr)("at %p\n", frame);
		} else
			frame = (db_addr_t)addr;
	} else {
		frame = (db_addr_t)ddb_regs.r[1];
	}
	for (;;) {
		if (frame < PAGE_SIZE)
			break;
		frame = *(db_addr_t *)frame;
	    next_frame:
		args = (db_addr_t *)(frame + 8);
		if (frame < PAGE_SIZE)
			break;
	        if (count-- == 0)
			break;

		lr = *(db_addr_t *)(frame + 4) - 4;
		if ((lr & 3) || (lr < 0x100)) {
			(*pr)("saved LR(0x%x) is invalid.", lr);
			break;
		}

		(*pr)("0x%08lx: ", frame);
		if (lr + 4 == (db_addr_t) trapexit ||
		    lr + 4 == (db_addr_t) sctrapexit) {
			const char *trapstr;
			struct trapframe *tf = (struct trapframe *) (frame+8);
			(*pr)("%s ", tf->srr1 & PSL_PR ? "user" : "kernel");
			if (lr + 4 == (db_addr_t) sctrapexit) {
				(*pr)("SC trap #%d by ", tf->fixreg[0]);
				goto print_trap;
			}
			switch (tf->exc) {
			case EXC_DSI:
#ifdef PPC_OEA
				(*pr)("DSI %s trap @ %#x by ",
				    tf->dsisr & DSISR_STORE ? "write" : "read",
				    tf->dar);
#endif
#ifdef PPC_IBM4XX
				(*pr)("DSI %s trap @ %#x by ",
				    tf->tf_xtra[TF_ESR] & ESR_DST ? "write" : "read",
				    tf->dar);
#endif
				goto print_trap;
			case EXC_ALI:
#ifdef PPC_OEA
				(*pr)("ALI trap @ %#x (DSISR %#x) ",
				    tf->dar, tf->dsisr);
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
			default: trapstr = NULL; break;
			}
			if (trapstr != NULL) {
				(*pr)("%s trap by ", trapstr);
			} else {
				(*pr)("trap %#x by ", tf->exc);
			}
		   print_trap:	
			lr = (db_addr_t) tf->srr0;
			diff = 0;
			symname = NULL;
			if (in_kernel && (tf->srr1 & PSL_PR) == 0) {
				sym = db_search_symbol(lr, DB_STGY_ANY, &diff);
				db_symbol_values(sym, &symname, 0);
			}
			if (symname == NULL || !strcmp(symname, "end")) {
				(*pr)("%p: srr1=%#x\n", lr, tf->srr1);
			} else {
				(*pr)("%s+%#x: srr1=%#x\n", symname,
				    diff, tf->srr1);
			}
			(*pr)("%-10s  r1=%#x cr=%#x xer=%#x ctr=%#x",
			    "", tf->fixreg[1], tf->cr, tf->xer, tf->ctr);
#ifdef PPC_OEA
			if (tf->exc == EXC_DSI)
				(*pr)(" dsisr=%#x", tf->dsisr);
			if ((mfpvr() >> 16) == MPC601)
				(*pr)(" mq=%#x", tf->tf_xtra[TF_MQ]);
#endif
#ifdef PPC_IBM4XX
			if (tf->exc == EXC_DSI)
				(*pr)(" dear=%#x", tf->dar);
			(*pr)(" esr=%#x pid=%#x", tf->tf_xtra[TF_ESR],
			    tf->tf_xtra[TF_PID]);
#endif
			(*pr)("\n");
			frame = (db_addr_t) tf->fixreg[1];
			in_kernel = !(tf->srr1 & PSL_PR);
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
				args[0], args[1], args[2], args[3],
				args[4], args[5], args[6], args[7]);
		(*pr)("\n");
	}
}
