/*	$NetBSD: db_xxx.c,v 1.2 1997/10/24 18:26:36 chuck Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1989, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	from: kern_proc.c	8.4 (Berkeley) 1/4/94
 */

/*
 * Miscellaneous DDB functions that are intimate (xxx) with various
 * data structures and functions used by the kernel (proc, callout).
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>

#include <sys/callout.h>
#include <sys/signalvar.h>

#include <machine/db_machdep.h>

#include <ddb/db_access.h>
#include <ddb/db_command.h>
#include <ddb/db_interface.h>
#include <ddb/db_lex.h>
#include <ddb/db_output.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

void
db_kill_proc(addr, haddr, count, modif)
	db_expr_t addr;
	int haddr;
	db_expr_t count;
	char *modif;
{
	struct proc *p;
	db_expr_t pid, sig;
	int t;

	/* What pid? */
	if (!db_expression(&pid)) {
		db_error("pid?\n");
	    /*NOTREACHED*/
	}
	/* What sig? */
	t = db_read_token();
	if (t == tCOMMA) {
		if (!db_expression(&sig)) {
			db_error("sig?\n");
			/*NOTREACHED*/
		}
	} else {
		db_unread_token(t);
		sig = 15;
	}
	if (db_read_token() != tEOL) {
	    db_error("?\n");
	    /*NOTREACHED*/
	}

	p = pfind((pid_t)pid);
	if (p == NULL) {
		db_error("no such proc\n");
	    /*NOTREACHED*/
	}
	psignal(p, (int)sig);
}

void
db_show_all_procs(addr, haddr, count, modif)
	db_expr_t addr;
	int haddr;
	db_expr_t count;
	char *modif;
{
	char *mode;
	int doingzomb = 0;
	struct proc *p, *pp;
    
	if (modif[0] == 0)
		modif[0] = 'n';			/* default == normal mode */

	mode = strchr("mawn", modif[0]);
	if (mode == NULL || *mode == 'm') {
		db_printf("usage: show all procs [/a] [/n] [/w]\n");
		db_printf("\t/a == show process address info\n");
		db_printf("\t/n == show normal process info [default]\n");
		db_printf("\t/w == show process wait/emul info\n");
		return;
	}
	
	p = allproc.lh_first;

	switch (*mode) {

	case 'a':
		db_printf("PID        %10s %18s %18s %18s\n",
		    "COMMAND", "STRUCT PROC *", "UAREA *", "VMSPACE/VM_MAP");
		break;
	case 'n':
		db_printf("PID        %10s %10s %10s S %7s %16s %7s\n",
		    "PPID", "PGRP", "UID", "FLAGS", "COMMAND", "WAIT");
		break;
	case 'w':
		db_printf("PID        %16s %8s %18s %s\n",
		    "COMMAND", "EMUL", "WAIT-CHANNEL", "WAIT-MSG");
		break;
	}

	while (p != 0) {
		pp = p->p_pptr;
		if (p->p_stat) {

			db_printf("%-10d ", p->p_pid);

			switch (*mode) {

			case 'a':
				db_printf("%10.10s %18p %18p %18p\n",
				    p->p_comm, p, p->p_addr, p->p_vmspace);
				break;

			case 'n':
				db_printf("%10d %10d %10d %d %#7x %16s %7.7s\n",
				    pp ? pp->p_pid : -1, p->p_pgrp->pg_id,
				    p->p_cred->p_ruid, p->p_stat, p->p_flag,
				    p->p_comm, (p->p_wchan && p->p_wmesg) ?
					p->p_wmesg : "");
				break;

			case 'w':
				db_printf("%16s %8s %18p %s\n", p->p_comm,
				    p->p_emul->e_name, p->p_wchan,
				    (p->p_wchan && p->p_wmesg) ? 
					p->p_wmesg : "");
				break;

			}
		}
		p = p->p_list.le_next;
		if (p == 0 && doingzomb == 0) {
			doingzomb = 1;
			p = zombproc.lh_first;
		}
	}
}

void
db_show_callout(addr, haddr, count, modif)
	db_expr_t addr; 
	int haddr; 
	db_expr_t count;
	char *modif;
{
	register struct callout *p1;
	register int	cum;
	register int	s;
	db_expr_t	offset;
	char		*name;

	db_printf("      cum     ticks      arg  func\n");
	s = splhigh();
	for (cum = 0, p1 = calltodo.c_next; p1; p1 = p1->c_next) {
		register int t = p1->c_time;

		if (t > 0)
			cum += t;

		db_find_sym_and_offset((db_addr_t)p1->c_func, &name, &offset);
		if (name == NULL)
			name = "?";

		db_printf("%9d %9d %p  %s (%p)\n",
			  cum, t, p1->c_arg, name, p1->c_func);
	}
	splx(s);
}
