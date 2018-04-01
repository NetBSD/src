/* $NetBSD: db_trace.c,v 1.2 2018/04/01 04:35:03 ryo Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: db_trace.c,v 1.2 2018/04/01 04:35:03 ryo Exp $");

#include <sys/param.h>
#include <sys/proc.h>

#include <aarch64/db_machdep.h>
#include <aarch64/machdep.h>
#include <aarch64/armreg.h>
#include <aarch64/vmparam.h>

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


#define IN_USER_VM_ADDRESS(addr)	\
	((VM_MIN_ADDRESS <= (addr)) && ((addr) < VM_MAX_ADDRESS))
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
	lwp_t *lwp;

	for (lwp = db_lwp_first(); lwp != NULL; lwp = db_lwp_next(lwp)) {
		uint64_t uarea = uvm_lwp_getuarea(lwp);
		if ((uarea <= sp) && (sp < (uarea + USPACE))) {
			return lwp->l_name;
		}
	}
	return "unknown";
}

#define TRACEFLAG_LOOKUPLWP	0x00000001

static void
pr_traceaddr(const char *prefix, uint64_t frame, uint64_t pc, int flags,
    void (*pr)(const char *, ...))
{
	db_expr_t offset;
	db_sym_t sym;
	const char *name;

	sym = db_search_symbol(pc, DB_STGY_ANY, &offset);
	if (sym != DB_SYM_NULL) {
		db_symbol_values(sym, &name, NULL);

		if (flags & TRACEFLAG_LOOKUPLWP) {
			(*pr)("%s %016llx %s %s() at %016llx ",
			    prefix, frame, getlwpnamebysp(frame), name, pc);
		} else {
			(*pr)("%s %016llx %s() at %016llx ",
			    prefix, frame, name, pc);
		}
		db_printsym(pc, DB_STGY_PROC, pr);
		(*pr)("\n");
	} else {
		if (flags & TRACEFLAG_LOOKUPLWP) {
			(*pr)("%s %016llx %s ?() at %016llx\n",
			    prefix, frame, getlwpnamebysp(frame), pc);
		} else {
			(*pr)("%s %016llx ?() at %016llx\n", prefix, frame, pc);
		}
	}
}

void
db_stack_trace_print(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif, void (*pr)(const char *, ...))
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

	if (!have_addr) {
		if (trace_lwp) {
			addr = curlwp;
		} else if (trace_thread) {
			addr = curlwp->l_proc->p_pid;
		} else {
			tf = DDB_REGS;
		}
	}

	if (trace_thread) {
		proc_t *pp, p;

		if ((pp = db_proc_find((pid_t)addr)) == 0) {
			(*pr)("trace: pid %d: not found\n", (int)addr);
			return;
		}
		db_read_bytes((db_addr_t)pp, sizeof(p), (char *)&p);
		addr = (db_addr_t)p.p_lwps.lh_first;
		trace_thread = false;
		trace_lwp = true;
	}

#if 0
	/* "/a" is abbreviated? */
	if (!trace_lwp && is_lwp(addr))
		trace_lwp = true;
#endif

	if (trace_lwp) {
		proc_t p;
		struct lwp l;

		db_read_bytes(addr, sizeof(l), (char *)&l);
		db_read_bytes((db_addr_t)l.l_proc, sizeof(p), (char *)&p);

		if (addr == (db_addr_t)curlwp) {
			fp = &DDB_REGS->tf_reg[29];	/* &reg[29]={fp,lr} */
			tf = DDB_REGS;
			(*pr)("trace: pid %d lid %d (curlwp) at tf %016lx\n",
			    p.p_pid, l.l_lid, tf);
		} else {
			tf = l.l_md.md_ktf;
			db_read_bytes(&tf->tf_reg[29], sizeof(fp), (char *)&fp);
			(*pr)("trace: pid %d lid %d at tf %016lx\n",
			    p.p_pid, l.l_lid, tf);
		}
	} else if (tf == NULL) {
		fp = addr;
		pr("trace fp %016llx\n", fp);
	} else {
		pr("trace tf %016llx\n", tf);
	}

	if (count > MAXBACKTRACE)
		count = MAXBACKTRACE;

	if (tf != NULL) {
		(*pr)("---- trapframe %016llx (%d bytes) ----\n",
		    tf, sizeof(*tf));
		dump_trapframe(tf, pr);
		(*pr)("------------------------"
		      "------------------------\n");

		lastfp = lastlr = lr = fp = 0;
		db_read_bytes(&tf->tf_pc, sizeof(lr), (char *)&lr);
		db_read_bytes(&tf->tf_reg[29], sizeof(fp), (char *)&fp);

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

		if (!trace_user && IN_USER_VM_ADDRESS(lr))
			break;

		extern char el1_trap[];	/* XXX */
		extern char el0_trap[];	/* XXX */
		if (((char *)(lr - 4) == (char *)el0_trap) ||
		    ((char *)(lr - 4) == (char *)el1_trap)) {

			tf = (struct trapframe *)(lastfp + 16);

			lastfp = tf;
			lastlr = lr;
			lr = fp = 0;
			db_read_bytes(&tf->tf_pc, sizeof(lr), (char *)&lr);
			db_read_bytes(&tf->tf_reg[29], sizeof(fp), (char *)&fp);

			/*
			 * no need to display the frame of el0_trap
			 * of kernel thread
			 */
			if (((char *)(lastlr - 4) == (char *)el0_trap) &&
			    (lr == 0))
				break;

			pr_traceaddr("tf", tf, lastlr - 4, flags, pr);

			if (lr == 0)
				break;

			(*pr)("---- trapframe %016llx (%d bytes) ----\n",
			    tf, sizeof(*tf));
			dump_trapframe(tf, pr);
			(*pr)("------------------------"
			      "------------------------\n");
			tf = NULL;

			if (!trace_user && IN_USER_VM_ADDRESS(lr))
				break;

			pr_traceaddr("fp", fp, lr, flags, pr);

		} else {
			pr_traceaddr("fp", fp, lr - 4, flags, pr);
		}
	}
}
