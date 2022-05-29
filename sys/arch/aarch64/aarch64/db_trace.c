/* $NetBSD: db_trace.c,v 1.16 2022/05/29 23:43:49 ryo Exp $ */

/*
 * Copyright (c) 2017 Ryo Shimizu <ryo@nerv.org>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: db_trace.c,v 1.16 2022/05/29 23:43:49 ryo Exp $");

#include <sys/param.h>
#include <sys/proc.h>

#include <aarch64/db_machdep.h>
#include <aarch64/machdep.h>
#include <aarch64/armreg.h>
#include <aarch64/vmparam.h>

#include <arm/cpufunc.h>

#include <uvm/uvm_extern.h>

#include <ddb/db_access.h>
#include <ddb/db_command.h>
#include <ddb/db_output.h>
#include <ddb/db_variables.h>
#include <ddb/db_sym.h>
#include <ddb/db_proc.h>
#include <ddb/db_lwp.h>
#include <ddb/db_extern.h>
#include <ddb/db_interface.h>

#define MAXBACKTRACE	128	/* against infinite loop */


__CTASSERT(VM_MIN_ADDRESS == 0);
#define IN_USER_VM_ADDRESS(addr)	\
	((addr) < VM_MAX_ADDRESS)
#define IN_KERNEL_VM_ADDRESS(addr)	\
	((VM_MIN_KERNEL_ADDRESS <= (addr)) && ((addr) < VM_MAX_KERNEL_ADDRESS))


static bool __unused
is_lwp(void *p)
{
	lwp_t *lwp;

	for (lwp = db_lwp_first(); lwp != NULL; lwp = db_lwp_next(lwp)) {
		if (lwp == p)
			return true;
	}
	return false;
}

static const char *
getlwpnamebysp(uint64_t sp)
{
#if defined(_KERNEL)
	lwp_t *lwp;

	for (lwp = db_lwp_first(); lwp != NULL; lwp = db_lwp_next(lwp)) {
		uint64_t uarea = uvm_lwp_getuarea(lwp);
		if ((uarea <= sp) && (sp < (uarea + USPACE))) {
			return lwp->l_name;
		}
	}
#endif
	return "unknown";
}

#define TRACEFLAG_LOOKUPLWP	0x00000001

static void
pr_traceaddr(const char *prefix, uint64_t frame, uint64_t pc, int flags,
    void (*pr)(const char *, ...) __printflike(1, 2))
{
	db_expr_t offset;
	db_sym_t sym;
	const char *name;

	sym = db_search_symbol(pc, DB_STGY_ANY, &offset);
	if (sym != DB_SYM_NULL) {
		db_symbol_values(sym, &name, NULL);

		if (flags & TRACEFLAG_LOOKUPLWP) {
			(*pr)("%s %016lx %s %s() at %016lx ",
			    prefix, frame, getlwpnamebysp(frame), name, pc);
		} else {
			(*pr)("%s %016lx %s() at %016lx ",
			    prefix, frame, name, pc);
		}
		db_printsym(pc, DB_STGY_PROC, pr);
		(*pr)("\n");
	} else {
		if (flags & TRACEFLAG_LOOKUPLWP) {
			(*pr)("%s %016lx %s ?() at %016lx\n",
			    prefix, frame, getlwpnamebysp(frame), pc);
		} else {
			(*pr)("%s %016lx ?() at %016lx\n", prefix, frame, pc);
		}
	}
}

void
db_stack_trace_print(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif, void (*pr)(const char *, ...) __printflike(1, 2))
{
	uint64_t lr, fp, lastlr, lastfp;
	struct trapframe *tf = NULL;
	int flags = 0;
	bool trace_user = false;
	bool trace_thread = false;
	bool trace_lwp = false;

	for (; *modif != '\0'; modif++) {
		switch (*modif) {
		case 'a':
			trace_lwp = true;
			trace_thread = false;
			break;
		case 'l':
			break;
		case 't':
			trace_thread = true;
			trace_lwp = false;
			break;
		case 'u':
			trace_user = true;
			break;
		case 'x':
			flags |= TRACEFLAG_LOOKUPLWP;
			break;
		default:
			pr("usage: bt[/ulx] [frame-address][,count]\n");
			pr("       bt/t[ulx] [pid][,count]\n");
			pr("       bt/a[ulx] [lwpaddr][,count]\n");
			pr("\n");
			pr("       /x      reverse lookup lwp name from sp\n");
			return;
		}
	}

#if defined(_KERNEL)
	if (!have_addr) {
		if (trace_lwp) {
			addr = (db_expr_t)curlwp;
		} else if (trace_thread) {
			addr = curlwp->l_proc->p_pid;
		} else {
			tf = DDB_REGS;
		}
	}
#endif

	if (trace_thread) {
		proc_t *pp;

		if ((pp = db_proc_find((pid_t)addr)) == 0) {
			(*pr)("trace: pid %d: not found\n", (int)addr);
			return;
		}
		db_read_bytes((db_addr_t)pp + offsetof(proc_t, p_lwps.lh_first),
		    sizeof(addr), (char *)&addr);
		trace_thread = false;
		trace_lwp = true;
	}

#if 0
	/* "/a" is abbreviated? */
	if (!trace_lwp && is_lwp(addr))
		trace_lwp = true;
#endif

	if (trace_lwp) {
		struct lwp l;
		pid_t pid;

		db_read_bytes(addr, sizeof(l), (char *)&l);
		db_read_bytes((db_addr_t)l.l_proc + offsetof(proc_t, p_pid),
		    sizeof(pid), (char *)&pid);

#if defined(_KERNEL)
		if (addr == (db_expr_t)curlwp) {
			fp = (uint64_t)&DDB_REGS->tf_reg[29];	/* &reg[29]={fp,lr} */
			tf = DDB_REGS;
			(*pr)("trace: pid %d lid %d (curlwp) at tf %p\n",
			    pid, l.l_lid, tf);
		} else
#endif
		{
			struct pcb *pcb = lwp_getpcb(&l);

			db_read_bytes((db_addr_t)pcb +
			    offsetof(struct pcb, pcb_tf),
			    sizeof(tf), (char *)&tf);
			if (tf != 0) {
				db_read_bytes((db_addr_t)&tf->tf_reg[29],
				    sizeof(fp), (char *)&fp);
				(*pr)("trace: pid %d lid %d at tf %p (in pcb)\n",
				    pid, l.l_lid, tf);
			}
#if defined(MULTIPROCESSOR) && defined(_KERNEL)
			else if (l.l_stat == LSONPROC ||
			    (l.l_pflag & LP_RUNNING) != 0) {

				/* running lwp on other cpus */
				extern struct trapframe *db_readytoswitch[];
				u_int index;

				db_read_bytes((db_addr_t)l.l_cpu +
				    offsetof(struct cpu_info, ci_index),
				    sizeof(index), (char *)&index);
				tf = db_readytoswitch[index];

				(*pr)("trace: pid %d lid %d at tf %p (in kdb_trap)\n",
				    pid, l.l_lid, tf);
			}
#endif
			else {
				(*pr)("trace: no trapframe found for lwp: %p\n", (void *)addr);
			}
		}
	} else if (tf == NULL) {
		fp = addr;
		pr("trace fp %016lx\n", fp);
	} else {
		pr("trace tf %p\n", tf);
	}

	if (count > MAXBACKTRACE)
		count = MAXBACKTRACE;

	if (tf != NULL) {
#if defined(_KERNEL)
		if (tf->tf_sp == 0) {
			(*pr)("---- switchframe %p (%zu bytes) ----\n",
			    tf, sizeof(*tf));
			dump_switchframe(tf, pr);
		} else {
			(*pr)("---- %s: trapframe %p (%zu bytes) ----\n",
			    (tf->tf_esr == -1) ? "Interrupt" :
			    eclass_trapname(__SHIFTOUT(tf->tf_esr, ESR_EC)),
			    tf, sizeof(*tf));
			dump_trapframe(tf, pr);
		}
		(*pr)("------------------------"
		      "------------------------\n");

#endif
		lastfp = lastlr = lr = fp = 0;

		db_read_bytes((db_addr_t)&tf->tf_pc, sizeof(lr), (char *)&lr);
		db_read_bytes((db_addr_t)&tf->tf_reg[29], sizeof(fp), (char *)&fp);
		lr = aarch64_strip_pac(lr);

		pr_traceaddr("fp", fp, lr - 4, flags, pr);
	}

	for (; (count > 0) && (fp != 0); count--) {

		lastfp = fp;
		fp = lr = 0;
		/*
		 * normal stack frame
		 *  fp[0]  saved fp(x29) value
		 *  fp[1]  saved lr(x30) value
		 */
		db_read_bytes(lastfp + 0, sizeof(fp), (char *)&fp);
		db_read_bytes(lastfp + 8, sizeof(lr), (char *)&lr);
		lr = aarch64_strip_pac(lr);

		if (lr == 0 || (!trace_user && IN_USER_VM_ADDRESS(lr)))
			break;

#if defined(_KERNEL)
		extern char el1_trap[];	/* XXX */
		extern char el0_trap[];	/* XXX */
		if (((char *)(lr - 4) == (char *)el0_trap) ||
		    ((char *)(lr - 4) == (char *)el1_trap)) {

			tf = (struct trapframe *)fp;

			lastfp = (uint64_t)tf;
			lastlr = lr;
			lr = fp = 0;
			db_read_bytes((db_addr_t)&tf->tf_pc, sizeof(lr),
			    (char *)&lr);
			if (lr == 0) {
				/*
				 * The exception may have been from a
				 * jump to null, so the null pc we
				 * would return to is useless.  Try
				 * x[30] instead -- that will be the
				 * return address for the jump.
				 */
				db_read_bytes((db_addr_t)&tf->tf_reg[30],
				    sizeof(lr), (char *)&lr);
			}
			db_read_bytes((db_addr_t)&tf->tf_reg[29], sizeof(fp),
			    (char *)&fp);
			lr = aarch64_strip_pac(lr);

			pr_traceaddr("tf", (db_addr_t)tf, lastlr - 4, flags, pr);

			if (lr == 0)
				break;

			(*pr)("---- %s: trapframe %p (%zu bytes) ----\n",
			    (tf->tf_esr == -1) ? "Interrupt" :
			    eclass_trapname(__SHIFTOUT(tf->tf_esr, ESR_EC)),
			    tf, sizeof(*tf));
			dump_trapframe(tf, pr);
			(*pr)("------------------------"
			      "------------------------\n");
			tf = NULL;

			if (!trace_user && IN_USER_VM_ADDRESS(lr))
				break;

			pr_traceaddr("fp", fp, lr, flags, pr);

		} else
#endif
		{
			pr_traceaddr("fp", fp, lr - 4, flags, pr);
		}
	}
}
