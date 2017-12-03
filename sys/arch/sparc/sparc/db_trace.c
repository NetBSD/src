/*	$NetBSD: db_trace.c,v 1.34.14.1 2017/12/03 11:36:43 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: db_trace.c,v 1.34.14.1 2017/12/03 11:36:43 jdolecek Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/cpu.h>

#include <machine/db_machdep.h>

#include <ddb/db_access.h>
#include <ddb/db_user.h>
#include <ddb/db_sym.h>
#include <ddb/db_interface.h>
#include <ddb/db_output.h>

#define INKERNEL(va)	(((vaddr_t)(va)) >= USRSTACK)
#ifdef _KERNEL
#define ONINTSTACK(fr)	(						\
	(u_int)(fr) <  (u_int)ddb_cpuinfo->eintstack &&		 	\
	(u_int)(fr) >= (u_int)ddb_cpuinfo->eintstack - INT_STACK_SIZE	\
)
#else
#define ONINTSTACK(fr)	(0)
#endif

void
db_stack_trace_print(db_expr_t addr, bool have_addr,
		     db_expr_t count, const char *modif,
		     void (*pr)(const char *, ...))
{
	struct frame	*frame, *prevframe;
	db_addr_t	pc;
	bool		kernel_only = true;
	bool		trace_thread = false;
	bool		lwpaddr = false;
	const char	*cp = modif;
	char		c;

#ifdef _KERNEL
	if (ddb_cpuinfo == NULL)
		ddb_cpuinfo = curcpu();
#endif

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

	if (!have_addr) {
		frame = (struct frame *)DDB_TF->tf_out[6];
		pc = DDB_TF->tf_pc;
	} else {
		if (trace_thread) {
			struct proc *p;
			struct pcb *pcb;
			struct lwp *l;
			if (lwpaddr) {
				l = (struct lwp *)addr;
				p = l->l_proc;
				(*pr)("trace: pid %d ", p->p_pid);
			} else {
				(*pr)("trace: pid %d ", (int)addr);
#ifdef _KERNEL
				p = proc_find_raw(addr);
				if (p == NULL) {
					(*pr)("not found\n");
					return;
				}
				l = LIST_FIRST(&p->p_lwps);
				KASSERT(l != NULL);
#else
				(*pr)("no proc_find_raw() in crash\n");
				return;
#endif
			}
			(*pr)("lid %d ", l->l_lid);
			pcb = lwp_getpcb(l);
			frame = (struct frame *)pcb->pcb_sp;
			pc = pcb->pcb_pc;
			(*pr)("at %p\n", frame);
		} else {
			frame = (struct frame *)addr;
			pc = 0;
		}
	}

	while (count--) {
		int		i;
		db_expr_t	offset;
		const char	*name;
		db_addr_t	prevpc;

#ifdef _KERNEL
#define FR(framep,field) (INKERNEL(framep)			\
				? (u_int)(framep)->field	\
				: fuword(&(framep)->field))
#else
/* XXX fix me, this is probably wrong */
#define FR(framep,field) ((u_int)(framep)->field)
#endif

		/* Fetch return address and arguments frame */
		prevpc = (db_addr_t)FR(frame, fr_pc);
		prevframe = (struct frame *)FR(frame, fr_fp);

		/*
		 * Switch to frame that contains arguments
		 */
		if (prevframe == NULL || (!INKERNEL(prevframe) && kernel_only))
			return;

		if ((ONINTSTACK(frame) && !ONINTSTACK(prevframe)) ||
		    (INKERNEL(frame) && !INKERNEL(prevframe))) {
			/* We're crossing a trap frame; pc = %l1 */
			prevpc = (db_addr_t)FR(frame, fr_local[1]);
		}

		name = NULL;
		if (INKERNEL(pc))
			db_find_sym_and_offset(pc, &name, &offset);
		if (name == NULL)
			(*pr)("0x%lx(", pc);
		else
			(*pr)("%s(", name);

		/*
		 * Print %i0..%i5, hope these still reflect the
		 * actual arguments somewhat...
		 */
		for (i = 0; i < 6; i++)
			(*pr)("0x%x%s", FR(frame, fr_arg[i]),
				(i < 5) ? ", " : ") at ");
		if (INKERNEL(prevpc))
			db_printsym(prevpc, DB_STGY_PROC, pr);
		else
			(*pr)("0x%lx", prevpc);
		(*pr)("\n");

		pc = prevpc;
		frame = prevframe;
	}
}
