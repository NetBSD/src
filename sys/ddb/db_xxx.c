/*	$NetBSD: db_xxx.c,v 1.69 2013/01/06 03:34:52 christos Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: db_xxx.c,v 1.69 2013/01/06 03:34:52 christos Exp $");

#ifdef _KERNEL_OPT
#include "opt_kgdb.h"
#include "opt_aio.h"
#include "opt_mqueue.h"
#endif

#ifndef _KERNEL
#include <stdbool.h>
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/msgbuf.h>
#include <sys/callout.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/lockdebug.h>
#include <sys/signalvar.h>
#include <sys/resourcevar.h>
#include <sys/pool.h>
#include <sys/uio.h>
#include <sys/kauth.h>
#include <sys/mqueue.h>
#include <sys/vnode.h>
#include <sys/module.h>
#include <sys/cpu.h>
#include <sys/vmem.h>

#include <ddb/ddb.h>
#include <ddb/db_user.h>

#ifdef KGDB
#include <sys/kgdb.h>
#endif

void
db_kill_proc(db_expr_t addr, bool haddr,
    db_expr_t count, const char *modif)
{
#ifdef _KERNEL	/* XXX CRASH(8) */
	struct proc *p;
	ksiginfo_t	ksi;
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
	/* We might stop when the mutex is held or when not */
	t = mutex_tryenter(proc_lock);
#ifdef DIAGNOSTIC
	if (!t) {
	       db_error("could not acquire proc_lock mutex\n");
	       /*NOTREACHED*/
	}
#endif
	p = proc_find((pid_t)pid);
	if (p == NULL) {
		if (t)
			mutex_exit(proc_lock);
		db_error("no such proc\n");
		/*NOTREACHED*/
	}
	KSI_INIT(&ksi);
	ksi.ksi_signo = sig;
	ksi.ksi_code = SI_USER;
	ksi.ksi_pid = 0;
	ksi.ksi_uid = 0;
	mutex_enter(p->p_lock);
	kpsignal2(p, &ksi);
	mutex_exit(p->p_lock);
	if (t)
		mutex_exit(proc_lock);
#else
	db_printf("This command is not currently supported.\n");
#endif
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
db_show_files_cmd(db_expr_t addr, bool haddr,
	      db_expr_t count, const char *modif)
{
#ifdef _KERNEL	/* XXX CRASH(8) */
	struct proc *p;
	int i;
	filedesc_t *fdp;
	fdfile_t *ff;
	file_t *fp;
	struct vnode *vn;
	bool full = false;
	fdtab_t *dt;

	if (modif[0] == 'f')
		full = true;

	p = (struct proc *) (uintptr_t) addr;

	fdp = p->p_fd;
	dt = fdp->fd_dt;
	for (i = 0; i < dt->dt_nfiles; i++) {
		if ((ff = dt->dt_ff[i]) == NULL)
			continue;

		fp = ff->ff_file;

		/* Only look at vnodes... */
		if ((fp != NULL) && (fp->f_type == DTYPE_VNODE)) {
			if (fp->f_data != NULL) {
				vn = (struct vnode *) fp->f_data;
				vfs_vnode_print(vn, full, db_printf);

#ifdef LOCKDEBUG
				db_printf("\nv_uobj.vmobjlock lock details:\n");
				lockdebug_lock_print(vn->v_uobj.vmobjlock,
					     db_printf);
				db_printf("\n");
#endif
			}
		}
	}
#endif
}

#ifdef AIO
void
db_show_aio_jobs(db_expr_t addr, bool haddr,
    db_expr_t count, const char *modif)
{

	aio_print_jobs(db_printf);
}
#endif

#ifdef MQUEUE
void
db_show_mqueue_cmd(db_expr_t addr, bool haddr,
    db_expr_t count, const char *modif)
{

#ifdef _KERNEL	/* XXX CRASH(8) */
	mqueue_print_list(db_printf);
#endif
}
#endif

void
db_show_module_cmd(db_expr_t addr, bool haddr,
    db_expr_t count, const char *modif)
{

#ifdef _KERNEL	/* XXX CRASH(8) */
	module_print_list(db_printf);
#endif
}

void
db_show_all_pools(db_expr_t addr, bool haddr,
    db_expr_t count, const char *modif)
{

#ifdef _KERNEL	/* XXX CRASH(8) */
	pool_printall(modif, db_printf);
#endif
}

void
db_show_all_vmems(db_expr_t addr, bool have_addr,
    db_expr_t count, const char *modif)
{

#ifdef _KERNEL	/* XXX CRASH(8) */
	vmem_printall(modif, db_printf);
#endif
}

void
db_dmesg(db_expr_t addr, bool haddr, db_expr_t count, const char *modif)
{
	struct kern_msgbuf mb, *mbp;
	db_expr_t print;
	int newl, skip, i;
	char *p, *bufdata, ch;

	if (!db_read_int("msgbufenabled")) {
		db_printf("message buffer not available\n");
		return;
	}
	mbp = (struct kern_msgbuf *)db_read_ptr("msgbufp");
	db_read_bytes((db_addr_t)mbp, sizeof(mb), (char *)&mb);
	if (mb.msg_magic != MSG_MAGIC) {
		db_printf("message buffer not available\n");
		return;
	}

	bufdata = &mbp->msg_bufc[0];

	if (haddr && addr < mb.msg_bufs)
		print = addr;
	else
		print = mb.msg_bufs;

	for (newl = skip = i = 0, p = bufdata + mb.msg_bufx;
	    i < mb.msg_bufs; i++, p++) {
		if (p == bufdata + mb.msg_bufs)
			p = bufdata;
		if (i < mb.msg_bufs - print) {
			continue;
		}
		db_read_bytes((db_addr_t)p, sizeof(ch), &ch);
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

#ifdef _KERNEL	/* XXX CRASH(8) */
	sched_print_runqueue(db_printf);
#endif
}

void
db_show_panic(db_expr_t addr, bool haddr, db_expr_t count, const char *modif)
{
#ifdef _KERNEL	/* XXX CRASH(8) */
        int s;

	s = splhigh();

	db_printf("Panic string: %s\n", panicstr);

	(void)splx(s);
#endif
}
