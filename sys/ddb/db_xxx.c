/*	$NetBSD: db_xxx.c,v 1.11.4.2 2001/09/13 01:15:33 thorpej Exp $	*/

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
#include <sys/msgbuf.h>

#include <sys/callout.h>
#include <sys/signalvar.h>
#include <sys/resourcevar.h>

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
	int i;
	char *mode;
	struct proc *p, *pp;
	struct timeval tv[3];
	const struct proclist_desc *pd;
    
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
	
	switch (*mode) {

	case 'a':
		db_printf(" PID       %10s %18s %18s %18s\n",
		    "COMMAND", "STRUCT PROC *", "UAREA *", "VMSPACE/VM_MAP");
		break;
	case 'n':
		db_printf(" PID       %10s %10s %10s S %7s %16s %7s\n",
		    "PPID", "PGRP", "UID", "FLAGS", "COMMAND", "WAIT");
		break;
	case 'w':
		db_printf(" PID       %10s %8s %4s %5s %5s %-12s%s\n",
		    "COMMAND", "EMUL", "PRI", "UTIME", "STIME",
		    "WAIT-MSG", "WAIT-CHANNEL");
		break;
	}

	/* XXX LOCKING XXX */
	pd = proclists;
 loop:
	for (p = LIST_FIRST(pd->pd_list); p != NULL;
	     p = LIST_NEXT(p, p_list)) {
		pp = p->p_pptr;
		if (p->p_stat) {

			db_printf("%c%-10d", " >"[curproc == p], p->p_pid);

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
				db_printf("%10s %8s %4d", p->p_comm,
				    p->p_emul->e_name,p->p_priority);
				calcru(p, tv+0, tv+1, tv+2);
				for(i = 0; i < 2; ++i) {
					db_printf("%4ld.%1ld",
					    (long)tv[i].tv_sec,
					    (long)tv[i].tv_usec/100000);
				}
				if(p->p_wchan && p->p_wmesg) {
					db_printf(" %-12s", p->p_wmesg);
					db_printsym((db_expr_t)p->p_wchan,
					    DB_STGY_XTRN, db_printf);
				}
				db_printf("\n");
				break;

			}
		}
	}
	pd++;
	if (pd->pd_list != NULL)
		goto loop;
}

void
db_show_callout(addr, haddr, count, modif)
	db_expr_t addr; 
	int haddr; 
	db_expr_t count;
	char *modif;
{
	extern struct callout_queue *callwheel;
	extern int callwheelsize;
	uint64_t hint;
	int i;

	for (i = 0; i < callwheelsize; i++) {
		struct callout_queue *bucket = &callwheel[i];
		struct callout *c = TAILQ_FIRST(&bucket->cq_q);

		if (c) {
			db_printf("bucket %d (hint %llx):\n", i,
			    (long long) bucket->cq_hint);
			hint = UQUAD_MAX;
			while (c) {
				if (c->c_time < hint)
					hint = c->c_time;
				db_printf("%p: time %llx arg %p flags %x "
				    "func %p: ", c, (long long) c->c_time,
				    c->c_arg, c->c_flags, c->c_func);
				db_printsym((u_long)c->c_func, DB_STGY_PROC,
				    db_printf);
				db_printf("\n");
				c = TAILQ_NEXT(c, c_link);
			}
			if (bucket->cq_hint < hint)
				printf("** HINT IS STALE\n");
		}
	}
}

void
db_dmesg(addr, haddr, count, modif)
	db_expr_t addr; 
	int haddr; 
	db_expr_t count;
	char *modif;
{
	struct kern_msgbuf *mbp;
	int ch, newl, skip, i;
	char *p, *bufdata;

	mbp = msgbufp;
	bufdata = &mbp->msg_bufc[0];

	for (newl = skip = i = 0, p = bufdata + mbp->msg_bufx;
	    i < mbp->msg_bufs; i++, p++) {
		if (p == bufdata + mbp->msg_bufs)
			p = bufdata;
		ch = *p;
		/* Skip "\n<.*>" syslog sequences. */
		if (skip) {
			if (ch == '>')
				newl = skip = 0;
			continue;
		}
		if (newl && ch == '<') {
			skip = 1;
			continue;
		}
		if (ch == '\0')
			continue;
		newl = ch == '\n';
		db_printf("%c", ch);
	}
	if (!newl)
		db_printf("\n");
}
