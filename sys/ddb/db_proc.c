/*	$NetBSD: db_proc.c,v 1.3.6.2 2009/05/13 17:19:04 jym Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_proc.c,v 1.3.6.2 2009/05/13 17:19:04 jym Exp $");

#ifndef _KERNEL
#include <stdbool.h>
#endif

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/proc.h>
#ifdef _KERNEL	/* XXX */
#include <sys/kauth.h>
#endif

#include <ddb/ddb.h>

proc_t *
db_proc_first(void)
{

	return db_read_ptr("allproc");
}

proc_t *
db_proc_next(proc_t *p)
{

	db_read_bytes((db_addr_t)&p->p_list.le_next, sizeof(p), (char *)&p);
	return p;
}

proc_t *
db_proc_find(pid_t pid)
{
	proc_t *p;
	pid_t tp;

	for (p = db_proc_first(); p != NULL; p = db_proc_next(p)) {
		db_read_bytes((db_addr_t)&p->p_pid, sizeof(tp),
		    (char *)&tp);
		if (tp == pid) {
			return p;
		}
	}
	return NULL;
}

void
db_show_all_procs(db_expr_t addr, bool haddr, db_expr_t count,
		  const char *modif)
{
	static struct pgrp pgrp;
	static proc_t p;
	static lwp_t l;
	const char *mode, *ename;
	proc_t *pp;
	lwp_t *lp;
	char db_nbuf[MAXCOMLEN + 1], wbuf[MAXCOMLEN + 1];
	bool run;
	int cpuno;

	if (modif[0] == 0)
		mode = "l";			/* default == lwp mode */
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
		db_printf("PID  %10s %18s %18s %18s\n",
		    "COMMAND", "STRUCT PROC *", "UAREA *", "VMSPACE/VM_MAP");
		break;
	case 'l':
		db_printf("PID   %4s S %3s %9s %18s %18s %-8s\n",
		    "LID", "CPU", "FLAGS", "STRUCT LWP *", "NAME", "WAIT");
		break;
	case 'n':
		db_printf("PID  %8s %8s %10s S %7s %4s %16s %7s\n",
		    "PPID", "PGRP", "UID", "FLAGS", "LWPS", "COMMAND", "WAIT");
		break;
	case 'w':
		db_printf("PID  %4s %16s %8s %4s %-12s%s\n",
		    "LID", "COMMAND", "EMUL", "PRI", "WAIT-MSG",
		    "WAIT-CHANNEL");
		break;
	}

	for (pp = db_proc_first(); pp != NULL; pp = db_proc_next(pp)) {
		db_read_bytes((db_addr_t)pp, sizeof(p), (char *)&p);
		if (p.p_stat == 0) {
			continue;
		}
		lp = p.p_lwps.lh_first;
		if (lp != NULL) {
			db_read_bytes((db_addr_t)lp, sizeof(l), (char *)&l);
		}
		db_printf("%-5d", p.p_pid);

		switch (*mode) {
		case 'a':
			db_printf("%10.10s %18lx %18lx %18lx\n",
			    p.p_comm, (long)pp,
			    (long)(lp != NULL ? l.l_addr : 0),
			    (long)p.p_vmspace);
			break;
		case 'l':
			 while (lp != NULL) {
			 	if (l.l_name != NULL) {
					db_read_bytes((db_addr_t)l.l_name,
					    MAXCOMLEN, db_nbuf);
				} else {
					strlcpy(db_nbuf, p.p_comm,
					    sizeof(db_nbuf));
				}
				run = (l.l_stat == LSONPROC ||
				    (l.l_pflag & LP_RUNNING) != 0);
				if (l.l_cpu != NULL) {
					db_read_bytes((db_addr_t)
					    &l.l_cpu->ci_data.cpu_index,
					    sizeof(cpuno), (char *)&cpuno);
				} else
					cpuno = -1;
				if (l.l_wchan && l.l_wmesg) {
					db_read_bytes((db_addr_t)l.l_wmesg,
					    sizeof(wbuf), (char *)wbuf);
				} else {
					wbuf[0] = '\0';
				}
				db_printf("%c%4d %d %3d %9x %18lx %18s %-8s\n",
				    (run ? '>' : ' '), l.l_lid,
				    l.l_stat, cpuno, l.l_flag, (long)lp,
				    db_nbuf, wbuf);
				lp = LIST_NEXT((&l), l_sibling);
				if (lp != NULL) {
					db_printf("%-5d", p.p_pid);
					db_read_bytes((db_addr_t)lp, sizeof(l),
					    (char *)&l);
				}
			}
			break;
		case 'n':
			db_read_bytes((db_addr_t)p.p_pgrp, sizeof(pgrp),
			    (char *)&pgrp);
			if (lp != NULL && l.l_wchan && l.l_wmesg) {
				db_read_bytes((db_addr_t)l.l_wmesg,
				    sizeof(wbuf), (char *)wbuf);
			} else {
				wbuf[0] = '\0';
			}
			db_printf("%8d %8d %10d %d %#7x %4d %16s %7.7s\n",
			    p.p_pptr != NULL ? p.p_pid : -1, pgrp.pg_id,
#ifdef _KERNEL
			    kauth_cred_getuid(p.p_cred),
#else
			    /* XXX CRASH(8) */ 666,
#endif			    
			    p.p_stat, p.p_flag,
			    p.p_nlwps, p.p_comm,
			    (p.p_nlwps != 1) ? "*" : wbuf);
			break;

		case 'w':
			 while (lp != NULL) {
				if (l.l_wchan && l.l_wmesg) {
					db_read_bytes((db_addr_t)l.l_wmesg,
					    sizeof(wbuf), (char *)wbuf);
				} else {
					wbuf[0] = '\0';
				}
				db_read_bytes((db_addr_t)&p.p_emul->e_name,
				    sizeof(ename), (char *)&ename);
				db_read_bytes((db_addr_t)ename,
				    sizeof(db_nbuf), db_nbuf);
				db_printf(
				    "%4d %16s %8s %4d %-12s %-18lx\n",
				    l.l_lid, p.p_comm, db_nbuf,
				    l.l_priority, wbuf, (long)l.l_wchan);
				lp = LIST_NEXT((&l), l_sibling);
				if (lp != NULL) {
					db_printf("%-5d", p.p_pid);
					db_read_bytes((db_addr_t)lp, sizeof(l),
					    (char *)&l);
				}
			}
			break;
		}
	}
}

