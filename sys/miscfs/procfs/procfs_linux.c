/*      $NetBSD: procfs_linux.c,v 1.7 2003/05/28 18:03:16 christos Exp $      */

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: procfs_linux.c,v 1.7 2003/05/28 18:03:16 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/tty.h>

#include <miscfs/procfs/procfs.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm.h>

#define PGTOB(p)	((unsigned long)(p) << PAGE_SHIFT)
#define PGTOKB(p)	((unsigned long)(p) << (PAGE_SHIFT - 10))

/*
 * Linux compatible /proc/meminfo. Only active when the -o linux
 * mountflag is used.
 */
int
procfs_domeminfo(struct proc *curp, struct proc *p, struct pfsnode *pfs,
		 struct uio *uio)
{
	char buf[512], *cp;
	int len, error;

	len = snprintf(buf, sizeof buf,
		"        total:    used:    free:  shared: buffers: cached:\n"
		"Mem:  %8lu %8lu %8lu %8lu %8lu %8lu\n"
		"Swap: %8lu %8lu %8lu\n"
		"MemTotal:  %8lu kB\n"
		"MemFree:   %8lu kB\n"
		"MemShared: %8lu kB\n"
		"Buffers:   %8lu kB\n"
		"Cached:    %8lu kB\n"
		"SwapTotal: %8lu kB\n" 
		"SwapFree:  %8lu kB\n",
		PGTOB(uvmexp.npages),
		PGTOB(uvmexp.npages - uvmexp.free),
		PGTOB(uvmexp.free),
		0L,
		PGTOB(uvmexp.filepages),
		PGTOB(uvmexp.anonpages + uvmexp.filepages + uvmexp.execpages),
		PGTOB(uvmexp.swpages),
		PGTOB(uvmexp.swpginuse),
		PGTOB(uvmexp.swpages - uvmexp.swpginuse),
		PGTOKB(uvmexp.npages),
		PGTOKB(uvmexp.free),
		0L,
		PGTOKB(uvmexp.filepages),
		PGTOKB(uvmexp.anonpages + uvmexp.filepages + uvmexp.execpages),
		PGTOKB(uvmexp.swpages),
		PGTOKB(uvmexp.swpages - uvmexp.swpginuse));

	if (len == 0)
		return 0;

	len -= uio->uio_offset;
	cp = buf + uio->uio_offset;
	len = imin(len, uio->uio_resid);
	if (len <= 0)
		error = 0;
	else
		error = uiomove(cp, len, uio);
	return error;
}

/*
 * Linux compatible /proc/<pid>/stat. Only active when the -o linux
 * mountflag is used.
 */
int
procfs_do_pid_stat(struct proc *p, struct lwp *l, struct pfsnode *pfs,
		 struct uio *uio)
{
	char buf[512], *cp;
	int len, error;
	struct tty *tty = p->p_session->s_ttyp;
	struct rusage *ru = &p->p_stats->p_ru;
	struct rusage *cru = &p->p_stats->p_cru;
	struct vm_map *map = &p->p_vmspace->vm_map;
	struct vm_map_entry *entry, *last = NULL;
	unsigned long stext = 0, etext = 0, sstack = 0;

	if (map != &curproc->p_vmspace->vm_map)
		vm_map_lock_read(map);
	for (entry = map->header.next; entry != &map->header;
	    entry = entry->next) {
		if (UVM_ET_ISSUBMAP(entry))
			continue;
		/* assume text is the first entry */
		if (stext == etext) {
			stext = entry->start;
			etext = entry->end;
		}
		last = entry;
	}
	/* assume stack is the last entry */
	if (last != NULL)
		sstack = last->start;

	if (map != &curproc->p_vmspace->vm_map)
		vm_map_unlock_read(map);

	len = snprintf(buf, sizeof(buf), 
	    "%d (%s) %c %d %d %d %d %d "
	    "%u "
	    "%lu %lu %lu %lu %lu %lu %lu %lu "
	    "%d %d %d "
	    "%lu %lu %lu %lu %qu "
	    "%lu %lu %lu "
	    "%u %u "
	    "%u %u %u %u "
	    "%lu %lu %lu %d %d\n",

	    p->p_pid,
	    p->p_comm,
	    "0IR3SZD"[(p->p_stat > 6) ? 0 : (int)p->p_stat],
	    p->p_pptr->p_pid,

	    p->p_pgid,
	    p->p_session->s_sid,
	    tty ? tty->t_dev : 0,
	    tty ? tty->t_pgrp->pg_id : 0,

	    p->p_flag,

	    ru->ru_minflt,
	    cru->ru_minflt,
	    ru->ru_majflt,
	    cru->ru_majflt,
	    ru->ru_utime.tv_sec,
	    ru->ru_stime.tv_sec,
	    cru->ru_utime.tv_sec,
	    cru->ru_stime.tv_sec,

	    p->p_nice,					/* XXX: priority */
	    p->p_nice,
	    0,

	    p->p_rtime.tv_sec,
	    p->p_stats->p_start.tv_sec,
	    ru->ru_ixrss + ru->ru_idrss + ru->ru_isrss,
	    ru->ru_maxrss,
	    p->p_rlimit[RLIMIT_RSS].rlim_cur,

	    stext,					/* start code */
	    etext,					/* end code */
	    sstack,					/* mm start stack */
	    0,						/* XXX: pc */
	    0,						/* XXX: sp */
	    p->p_sigctx.ps_siglist.__bits[0],		/* pending */
	    p->p_sigctx.ps_sigmask.__bits[0],		/* blocked */
	    p->p_sigctx.ps_sigignore.__bits[0],		/* ignored */
	    p->p_sigctx.ps_sigcatch.__bits[0],		/* caught */

	    (unsigned long)(intptr_t)l->l_wchan,
	    ru->ru_nvcsw,
	    ru->ru_nivcsw,
	    p->p_exitsig,
	    0);						/* XXX: processor */

	if (len == 0)
		return 0;

	len -= uio->uio_offset;
	cp = buf + uio->uio_offset;
	len = imin(len, uio->uio_resid);
	if (len <= 0)
		error = 0;
	else
		error = uiomove(cp, len, uio);
	return error;
}

int
procfs_docpuinfo(struct proc *curp, struct proc *p, struct pfsnode *pfs,
		 struct uio *uio)
{
	char buf[512], *cp;
	int len, error;

	len = sizeof buf;
	if (procfs_getcpuinfstr(buf, &len) < 0)
		return EIO;

	if (len == 0)
		return 0;

	len -= uio->uio_offset;
	cp = buf + uio->uio_offset;
	len = imin(len, uio->uio_resid);
	if (len <= 0)
		error = 0;
	else
		error = uiomove(cp, len, uio);
	return error;
}

int
procfs_douptime(struct proc *curp, struct proc *p, struct pfsnode *pfs,
		 struct uio *uio)
{
	char buf[512], *cp;
	int len, error;
	struct timeval runtime;
	u_int64_t idle;

	timersub(&curcpu()->ci_schedstate.spc_runtime, &boottime, &runtime);
	idle = curcpu()->ci_schedstate.spc_cp_time[CP_IDLE];
/*###251 [cc] unterminated string or character constant%%%*/
	len = sprintf(buf, "%lu.%02lu %" PRIu64 ".%02" PRIu64 "\n",
		      runtime.tv_sec, runtime.tv_usec / 10000,
		      idle / hz, (((idle % hz) * 100) / hz) % 100);

	if (len == 0)
		return 0;

	len -= uio->uio_offset;
	cp = buf + uio->uio_offset;
	len = imin(len, uio->uio_resid);
	if (len <= 0)
		error = 0;
	else
		error = uiomove(cp, len, uio);
	return error;
}
