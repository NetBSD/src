/*	$NetBSD: db_trace.c,v 1.5 2009/11/03 05:07:26 snj Exp $	*/

/*	$OpenBSD: db_interface.c,v 1.16 2001/03/22 23:31:45 mickey Exp $	*/

/*
 * Copyright (c) 1999-2003 Michael Shalayeff
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_trace.c,v 1.5 2009/11/03 05:07:26 snj Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h> 

#include <machine/db_machdep.h>

#include <ddb/db_access.h>
#include <ddb/db_sym.h>
#include <ddb/db_interface.h>

void
db_stack_trace_print(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif, void (*pr)(const char *, ...))
{
	register_t *fp, pc, rp;
	bool kernel_only = true;
	bool trace_thread = false;
	bool lwpaddr = false;
	db_sym_t sym;
	db_expr_t off;
	const char *name;
	const char *cp = modif;
	char c;

	if (count < 0)
		count = 65536;

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
		fp = (register_t *)ddb_regs.tf_r3;
		pc = ddb_regs.tf_iioq_head;
		rp = ddb_regs.tf_rp;
	} else {
		if (trace_thread) {
			struct proc *p;
			struct user *u;
			struct lwp *l;
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
				l = LIST_FIRST(&p->p_lwps);
				KASSERT(l != NULL);
			}
			(*pr)("lid %d ", l->l_lid);
			u = l->l_addr;
			if (p == curproc && l == curlwp) {
				fp = (int *)ddb_regs.tf_r3;
				pc = ddb_regs.tf_iioq_head;
				rp = ddb_regs.tf_rp;
			} else {
				/* cpu_switchto fp, and return point */
				fp = (int *)(u->u_pcb.pcb_ksp -
				    (HPPA_FRAME_SIZE + 16*4));
				pc = 0;
				rp = fp[-5];
			}
			(*pr)("at %p\n", fp);
		} else {
			pc = 0;
			fp = (register_t *)addr;
			rp = fp[-5];
		}
	}

	while (fp && count--) {

#ifdef DDB_DEBUG
		pr(">> %08x %08x %08x\t", fp, pc, rp);
#endif

		if (USERMODE(pc))
			return;

		sym = db_search_symbol(pc, DB_STGY_ANY, &off);
		db_symbol_values (sym, &name, NULL);

		pr("%s() at ", name);
		db_printsym(pc, DB_STGY_PROC, pr);
		pr("\n");

		/* XXX NH - unwind info here */
		/* aue = ue_find(pc); */

		/*
		 * get rp?
		 * fp -= ue_total_frame_size(aue)
		 */

		/*
		 * if a terminal frame then report the trapframe
		 * and continue after it (if not the last one).
		 */
		if (!fp[0]) {
			register_t *scargs;
			struct trapframe *tf;
			int scoff;

			/* Stack space for syscall args */
			scoff = HPPA_FRAME_ROUND(HPPA_FRAME_SIZE + HPPA_FRAME_MAXARGS);

			scargs = (register_t *)((char *)fp - scoff);
			tf = (struct trapframe *)((char *)scargs - sizeof(*tf));

			if (tf->tf_flags & TFF_SYS)
				pr("-- syscall #%d(%x, %x, %x, %x, ...)\n",
				    tf->tf_t1, scargs[1], scargs[2],
				    scargs[3], scargs[4]);
			else
				pr("-- trap #%d (%p) %s\n", tf->tf_flags & 0x3f, tf,
				    (tf->tf_flags & T_USER)? " from user" : "");

			if (!(tf->tf_flags & TFF_LAST)) {
				fp = (register_t *)tf->tf_r3;
				pc = tf->tf_iioq_head;
				rp = tf->tf_rp;
			} else {
				pc = 0;
				fp = 0;
			}
		} else {
			/* next frame */
			fp = (register_t *)fp[0];
			pc = rp;
			rp = fp[-5];
		}
	}

	if (count && pc) {
		db_printsym(pc, DB_STGY_XTRN, pr);
		pr(":\n");
	}
}
