/*	$NetBSD: db_trace.c,v 1.6 2022/12/24 14:14:52 uwe Exp $	*/

/*
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
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
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_trace.c,v 1.6 2022/12/24 14:14:52 uwe Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>

#include <uvm/uvm_prot.h>
#include <uvm/uvm_pmap.h>

#include <machine/frame.h>
#include <machine/trap.h>
#include <machine/intrdefs.h>
#include <machine/pmap.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_access.h>
#include <ddb/db_variables.h>
#include <ddb/db_output.h>
#include <ddb/db_interface.h>
#include <ddb/db_user.h>
#include <ddb/db_proc.h>
#include <ddb/db_command.h>
#include <x86/db_machdep.h>

int
db_x86_regop(const struct db_variable *vp, db_expr_t *val, int opcode)
{
	db_expr_t *regaddr =
	    (db_expr_t *)(((uint8_t *)DDB_REGS) + ((size_t)vp->valuep));

	switch (opcode) {
	case DB_VAR_GET:
		*val = *regaddr;
		break;
	case DB_VAR_SET:
		*regaddr = *val;
		break;
	default:
		db_printf("db_x86_regop: unknown op %d", opcode);
		db_error(NULL);
	}
	return 0;
}

/*
 * Stack trace.
 */

#if 0
db_addr_t db_trap_symbol_value = 0;
db_addr_t db_syscall_symbol_value = 0;
db_addr_t db_kdintr_symbol_value = 0;
bool db_trace_symbols_found = false;

void db_find_trace_symbols(void);

void
db_find_trace_symbols(void)
{
	db_expr_t value;

	if (db_value_of_name("_trap", &value))
		db_trap_symbol_value = (db_addr_t) value;
	if (db_value_of_name("_kdintr", &value))
		db_kdintr_symbol_value = (db_addr_t) value;
	if (db_value_of_name("_syscall", &value))
		db_syscall_symbol_value = (db_addr_t) value;
	db_trace_symbols_found = true;
}
#endif

#define set_frame_callpc() do {				\
		frame = (long *)ddb_regs.tf_bp;		\
		callpc = (db_addr_t)ddb_regs.tf_ip;	\
	} while (/*CONSTCCOND*/0)

void
db_stack_trace_print(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif, void (*pr)(const char *, ...))
{
	long *frame, *lastframe;
	long *retaddr, *arg0;
	long *argp;
	db_addr_t callpc;
	int is_trap;
	bool kernel_only = true;
	bool trace_thread = false;
	bool lwpaddr = false;

#if 0
	if (!db_trace_symbols_found)
		db_find_trace_symbols();
#endif

	{
		const char *cp = modif;
		char c;

		while ((c = *cp++) != 0) {
			if (c == 'a') {
				lwpaddr = true;
				trace_thread = true;
			}
			if (c == 't')
				trace_thread = true;
			if (c == 'u')
				kernel_only = false;
		}
	}

	if (have_addr && trace_thread) {
		struct pcb *pcb;
		proc_t p;
		lwp_t l;

		if (lwpaddr) {
			db_read_bytes(addr, sizeof(l),
			    (char *)&l);
			db_read_bytes((db_addr_t)l.l_proc,
			    sizeof(p), (char *)&p);
			(*pr)("trace: pid %d ", p.p_pid);
		} else {
			proc_t	*pp;

			(*pr)("trace: pid %d ", (int)addr);
			if ((pp = db_proc_find((pid_t)addr)) == 0) {
				(*pr)("not found\n");
				return;
			}
			db_read_bytes((db_addr_t)pp, sizeof(p), (char *)&p);
			addr = (db_addr_t)p.p_lwps.lh_first;
			db_read_bytes(addr, sizeof(l), (char *)&l);
		}
		(*pr)("lid %d ", l.l_lid);
		pcb = lwp_getpcb(&l);
#ifdef _KERNEL
		if (l.l_proc == curproc && (lwp_t *)addr == curlwp)
			set_frame_callpc();
		else
#endif
		{
			db_read_bytes((db_addr_t)&pcb->pcb_bp,
			    sizeof(frame), (char *)&frame);
			db_read_bytes((db_addr_t)(frame + 1),
			    sizeof(callpc), (char *)&callpc);
			db_read_bytes((db_addr_t)frame,
			    sizeof(frame), (char *)&frame);
		}
		(*pr)("at %p\n", frame);
	} else if (have_addr) {
		frame = (long *)addr;
		db_read_bytes((db_addr_t)(frame + 1),
		    sizeof(callpc), (char *)&callpc);
		db_read_bytes((db_addr_t)frame,
		    sizeof(frame), (char *)&frame);
	} else {
		set_frame_callpc();
	}

	retaddr = frame + 1;
	arg0 = frame + 2;

	lastframe = NULL;
	while (count && frame != 0) {
		int narg;
		const char *name;
		db_expr_t offset;
		db_sym_t sym;
		char *argnames[MAXNARG], **argnp = NULL;
		db_addr_t lastcallpc;

		name = "?";
		is_trap = NONE;
		offset = 0;
		sym = db_frame_info(frame, callpc, &name, &offset, &is_trap,
		    &narg);

		if (lastframe == NULL && sym == DB_SYM_NULL && callpc != 0) {
			/* Symbol not found, peek at code */
			u_long instr = db_get_value(callpc, 4, false);

			offset = 1;
			if (
#ifdef __x86_64__
			   (instr == 0xe5894855 ||
					/* enter: pushq %rbp, movq %rsp, %rbp */
			    (instr & 0x00ffffff) == 0x0048e589
					/* enter+1: movq %rsp, %rbp */)
#else
			   ((instr & 0x00ffffff) == 0x00e58955 ||
					/* enter: pushl %ebp, movl %esp, %ebp */
			    (instr & 0x0000ffff) == 0x0000e589
					/* enter+1: movl %esp, %ebp */)
#endif
			    )
			{
				offset = 0;
			}
		}

		if (is_trap == NONE) {
			if (db_sym_numargs(sym, &narg, argnames))
				argnp = argnames;
			else
				narg = db_numargs(frame);
		}

		(*pr)("%s(", name);

		if (lastframe == NULL && offset == 0 && !have_addr) {
			/*
			 * We have a breakpoint before the frame is set up
			 * Use %[er]sp instead
			 */
			argp = (long *)&((struct x86_frame *)
			    (ddb_regs.tf_sp-sizeof(long)))->f_arg0;
		} else {
			argp = frame + 2;
		}

		while (narg) {
			if (argnp)
				(*pr)("%s=", *argnp++);
			(*pr)("%lx", db_get_value((long)argp, sizeof(long),
			    false));
			argp++;
			if (--narg != 0)
				(*pr)(",");
		}
		(*pr)(") at ");
		db_printsym(callpc, DB_STGY_PROC, pr);
		(*pr)("\n");

		if (lastframe == NULL && offset == 0 && !have_addr) {
			/* Frame really belongs to next callpc */
			struct x86_frame *fp = (void *)
			    (ddb_regs.tf_sp-sizeof(long));
			lastframe = (long *)fp;
			callpc = (db_addr_t)
			    db_get_value((db_addr_t)&fp->f_retaddr,
				sizeof(long), false);

			continue;
		}

		lastframe = frame;
		lastcallpc = callpc;
		if (!db_nextframe(&frame, &retaddr, &arg0, &callpc,
		    frame + 2, is_trap, pr))
			break;

		if (INKERNEL((long)frame)) {
			/* staying in kernel */
#ifdef __i386__
			if (!db_intrstack_p(frame) &&
			    db_intrstack_p(lastframe)) {
				(*pr)("--- switch to interrupt stack ---\n");
			} else
#endif
			if (frame < lastframe ||
			    (frame == lastframe && callpc == lastcallpc)) {
				(*pr)("Bad frame pointer: %p\n", frame);
				break;
			}
		} else if (INKERNEL((long)lastframe)) {
			/* switch from user to kernel */
			if (kernel_only)
				break;	/* kernel stack only */
		} else {
			/* in user */
			if (frame <= lastframe) {
				(*pr)("Bad user frame pointer: %p\n", frame);
				break;
			}
		}
		--count;
	}

	if (count && is_trap != NONE) {
		db_printsym(callpc, DB_STGY_XTRN, pr);
		(*pr)(":\n");
	}
}
