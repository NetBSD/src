/* $NetBSD: db_trace.c,v 1.17 2022/06/02 05:09:01 ryo Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: db_trace.c,v 1.17 2022/06/02 05:09:01 ryo Exp $");

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

#ifdef _KERNEL
extern char el0_trap[];
extern char el1_trap[];
#else
/* see also usr.sbin/crash/arch/aarch64.c */
extern vaddr_t el0_trap;
extern vaddr_t el1_trap;
#endif

#define MAXBACKTRACE	128	/* against infinite loop */


__CTASSERT(VM_MIN_ADDRESS == 0);
#define IN_USER_VM_ADDRESS(addr)	\
	((addr) < VM_MAX_ADDRESS)
#define IN_KERNEL_VM_ADDRESS(addr)	\
	((VM_MIN_KERNEL_ADDRESS <= (addr)) && ((addr) < VM_MAX_KERNEL_ADDRESS))

static void
pr_frame(struct trapframe *tf, void (*pr)(const char *, ...) __printflike(1, 2))
{
	struct trapframe tf_buf;

	db_read_bytes((db_addr_t)tf, sizeof(tf_buf), (char *)&tf_buf);

	if (tf_buf.tf_sp == 0) {
		(*pr)("---- switchframe %p (%zu bytes) ----\n",
		    tf, sizeof(*tf));
		dump_switchframe(tf, pr);
	} else {
#ifdef _KERNEL
		(*pr)("---- %s: trapframe %p (%zu bytes) ----\n",
		    (tf_buf.tf_esr == (uint64_t)-1) ? "Interrupt" :
		    eclass_trapname(__SHIFTOUT(tf_buf.tf_esr, ESR_EC)),
		    tf, sizeof(*tf));
#else
		(*pr)("---- trapframe %p (%zu bytes) ----\n", tf, sizeof(*tf));
#endif
		dump_trapframe(tf, pr);
	}
	(*pr)("------------------------"
	      "------------------------\n");
}

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

static vaddr_t
db_lwp_getuarea(lwp_t *l)
{
	void *laddr;
	db_read_bytes((db_addr_t)&l->l_addr, sizeof(laddr), (char *)&laddr);
	if (laddr == 0)
		return 0;
	return (vaddr_t)((char *)laddr - UAREA_PCB_OFFSET);
}

static const char *
getlwpnamebysp(uint64_t sp)
{
	static char c_name[MAXCOMLEN];
	lwp_t *lwp;
	struct proc *pp;
	char *lname;

	for (lwp = db_lwp_first(); lwp != NULL; lwp = db_lwp_next(lwp)) {
		uint64_t uarea = db_lwp_getuarea(lwp);
		if ((uarea <= sp) && (sp < (uarea + USPACE))) {
			db_read_bytes((db_addr_t)&lwp->l_name, sizeof(lname),
			    (char *)&lname);
			if (lname != NULL) {
				db_read_bytes((db_addr_t)lname, sizeof(c_name),
			    c_name);
				return c_name;
			}
			db_read_bytes((db_addr_t)&lwp->l_proc, sizeof(pp),
			    (char *)&pp);
			if (pp != NULL) {
				db_read_bytes((db_addr_t)&pp->p_comm,
				    sizeof(c_name), c_name);
				return c_name;
			}
			break;
		}
	}
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
		pr_frame(tf, pr);
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

			pr_frame(tf, pr);
			tf = NULL;

			if (!trace_user && IN_USER_VM_ADDRESS(lr))
				break;

			pr_traceaddr("fp", fp, lr, flags, pr);

		} else {
			pr_traceaddr("fp", fp, lr - 4, flags, pr);
		}
	}
}
