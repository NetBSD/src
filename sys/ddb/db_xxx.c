/*	$NetBSD: db_xxx.c,v 1.46.8.2 2007/07/19 21:54:52 dsl Exp $	*/

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
 * 3. Neither the name of the University nor the names of its contributors
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_xxx.c,v 1.46.8.2 2007/07/19 21:54:52 dsl Exp $");

#include "opt_kgdb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/msgbuf.h>

#include <sys/callout.h>
#include <sys/signalvar.h>
#include <sys/resourcevar.h>
#include <sys/pool.h>
#include <sys/kauth.h>

#include <machine/db_machdep.h>

#include <ddb/db_access.h>
#include <ddb/db_command.h>
#include <ddb/db_interface.h>
#include <ddb/db_lex.h>
#include <ddb/db_output.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#ifdef KGDB
#include <sys/kgdb.h>
#endif

void
db_kill_proc(db_expr_t addr, bool haddr,
    db_expr_t count, const char *modif)
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

#ifdef KGDB
void
db_kgdb_cmd(db_expr_t addr, bool haddr,
    db_expr_t count, const char *modif)
{
	kgdb_active++;
	kgdb_trap(db_trap_type, DDB_REGS);
	kgdb_active--;
}
#endif

void
db_show_aio_jobs(db_expr_t addr, bool haddr,
    db_expr_t count, const char *modif)
{
	aio_print_jobs(db_printf);
}

void
db_show_all_procs(db_expr_t addr, bool haddr,
    db_expr_t count, const char *modif)
{
	int i;

	const char *mode;
	struct proc *p, *pp, *cp;
	struct lwp *l, *cl;
	struct timeval tv[2];
	const struct proclist_desc *pd;
	char nbuf[MAXCOMLEN + 1];

	if (modif[0] == 0)
		mode = "n";			/* default == normal mode */
	else
		mode = strchr("mawln", modif[0]);

	if (mode == NULL || *mode == 'm') {
		db_printf("usage: show all procs [/a] [/l] [/n] [/w]\n");
		db_printf("\t/a == show process address info\n");
		db_printf("\t/l == show LWP info\n");
		db_printf("\t/n == show normal process info [default]\n");
		db_printf("\t/w == show process wait/emul info\n");
		return;
	}

	switch (*mode) {
	case 'a':
		db_printf(" PID       %10s %18s %18s %18s\n",
		    "COMMAND", "STRUCT PROC *", "UAREA *", "VMSPACE/VM_MAP");
		break;
	case 'l':
		db_printf(" PID        %4s S %9s %18s %18s %-8s\n",
		    "LID", "FLAGS", "STRUCT LWP *", "NAME", "WAIT");
		break;
	case 'n':
		db_printf(" PID       %8s %8s %10s S %7s %4s %16s %7s\n",
		    "PPID", "PGRP", "UID", "FLAGS", "LWPS", "COMMAND", "WAIT");
		break;
	case 'w':
		db_printf(" PID       %10s %8s %4s %5s %5s %-12s%s\n",
		    "COMMAND", "EMUL", "PRI", "UTIME", "STIME",
		    "WAIT-MSG", "WAIT-CHANNEL");
		break;
	}

	/* XXX LOCKING XXX */
	pd = proclists;
	cp = curproc;
	cl = curlwp;
	for (pd = proclists; pd->pd_list != NULL; pd++) {
		LIST_FOREACH(p, pd->pd_list, p_list) {
			pp = p->p_pptr;
			if (p->p_stat == 0) {
				continue;
			}
			l = LIST_FIRST(&p->p_lwps);
			db_printf("%c%-10d", (cp == p ? '>' : ' '), p->p_pid);

			switch (*mode) {

			case 'a':
				db_printf("%10.10s %18lx %18lx %18lx\n",
				    p->p_comm, (long)p,
				    (long)(l != NULL ? l->l_addr : 0),
				    (long)p->p_vmspace);
				break;
			case 'l':
				 while (l != NULL) {
				 	if (l->l_name != NULL) {
				 		snprintf(nbuf, sizeof(nbuf),
				 		    "%s", l->l_name);
					} else
				 		snprintf(nbuf, sizeof(nbuf),
				 		    "%s", p->p_comm);
					db_printf("%c%4d %d %9x %18lx %18s %-8s\n",
					    (cl == l ? '>' : ' '), l->l_lid,
					    l->l_stat, l->l_flag, (long)l, nbuf,
					    (l->l_wchan && l->l_wmesg) ?
					    l->l_wmesg : "");

					l = LIST_NEXT(l, l_sibling);
					if (l)
						db_printf("%11s","");
				}
				break;
			case 'n':
				db_printf("%8d %8d %10d %d %#7x %4d %16s %7.7s\n",
				    pp ? pp->p_pid : -1, p->p_pgrp->pg_id,
				    kauth_cred_getuid(p->p_cred), p->p_stat, p->p_flag,
				    p->p_nlwps, p->p_comm,
				    (p->p_nlwps != 1) ? "*" : (
				    (l->l_wchan && l->l_wmesg) ?
				    l->l_wmesg : ""));
				break;

			case 'w':
				db_printf("%10s %8s %4d", p->p_comm,
				    p->p_emul->e_name,
				    (l != NULL) ? l->l_priority : -1);
				calcru(p, &tv[0], &tv[1], NULL, NULL);
				for (i = 0; i < 2; ++i) {
					db_printf("%4ld.%1ld",
					    (long)tv[i].tv_sec,
					    (long)tv[i].tv_usec/100000);
				}
				if (p->p_nlwps == 1) {
				if (l->l_wchan && l->l_wmesg) {
					db_printf(" %-12s", l->l_wmesg);
					db_printsym(
					    (db_expr_t)(intptr_t)l->l_wchan,
					    DB_STGY_XTRN, db_printf);
				} } else {
					db_printf(" * ");
				}
				db_printf("\n");
				break;

			}
		}
	}
}

void
db_show_all_pools(db_expr_t addr, bool haddr,
    db_expr_t count, const char *modif)
{

	pool_printall(modif, db_printf);
}

void
db_dmesg(db_expr_t addr, bool haddr, db_expr_t count,
    const char *modif)
{
	struct kern_msgbuf *mbp;
	db_expr_t print;
	int ch, newl, skip, i;
	char *p, *bufdata;

        if (!msgbufenabled || msgbufp->msg_magic != MSG_MAGIC) {
		db_printf("message buffer not available\n");
		return;
	}

	mbp = msgbufp;
	bufdata = &mbp->msg_bufc[0];

	if (haddr && addr < mbp->msg_bufs)
		print = addr;
	else
		print = mbp->msg_bufs;

	for (newl = skip = i = 0, p = bufdata + mbp->msg_bufx;
	    i < mbp->msg_bufs; i++, p++) {
		if (p == bufdata + mbp->msg_bufs)
			p = bufdata;
		if (i < mbp->msg_bufs - print)
			continue;
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

void
db_show_sched_qs(db_expr_t addr, bool haddr,
    db_expr_t count, const char *modif)
{

	sched_print_runqueue(db_printf);
}
