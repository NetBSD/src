/*      $NetBSD: procfs_linux.c,v 1.36 2007/05/24 05:33:08 dogcow Exp $      */

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
__KERNEL_RCSID(0, "$NetBSD: procfs_linux.c,v 1.36 2007/05/24 05:33:08 dogcow Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/exec.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/tty.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/conf.h>

#include <miscfs/procfs/procfs.h>
#include <compat/linux/common/linux_exec.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm.h>

extern struct devsw_conv *devsw_conv;
extern int max_devsw_convs;

#define PGTOB(p)	((unsigned long)(p) << PAGE_SHIFT)
#define PGTOKB(p)	((unsigned long)(p) << (PAGE_SHIFT - 10))

#define LBFSZ (8 * 1024)

static void
get_proc_size_info(struct lwp *l, unsigned long *stext, unsigned long *etext, unsigned long *sstack)
{
	struct proc *p = l->l_proc;
	struct vmspace *vm;
	struct vm_map *map;
	struct vm_map_entry *entry;

	*stext = 0;
	*etext = 0;
	*sstack = 0;

	proc_vmspace_getref(p, &vm);
	map = &vm->vm_map;
	vm_map_lock_read(map);

	for (entry = map->header.next; entry != &map->header;
	    entry = entry->next) {
		if (UVM_ET_ISSUBMAP(entry))
			continue;
		/* assume text is the first entry */
		if (*stext == *etext) {
			*stext = entry->start;
			*etext = entry->end;
			break;
		}
	}
#ifdef LINUX_USRSTACK
	if (strcmp(p->p_emul->e_name, "linux") == 0 &&
	    LINUX_USRSTACK < USRSTACK)
		*sstack = (unsigned long) LINUX_USRSTACK;
	else
#endif
		*sstack = (unsigned long) USRSTACK;

	/*
	 * jdk 1.6 compares low <= addr && addr < high
	 * if we put addr == high, then the test fails
	 * so eat one page.
	 */
	*sstack -= PAGE_SIZE;

	vm_map_unlock_read(map);
	uvmspace_free(vm);
}

/*
 * Linux compatible /proc/meminfo. Only active when the -o linux
 * mountflag is used.
 */
int
procfs_domeminfo(struct lwp *curl, struct proc *p,
    struct pfsnode *pfs, struct uio *uio)
{
	char *bf;
	int len;
	int error = 0;

	bf = malloc(LBFSZ, M_TEMP, M_WAITOK);

	len = snprintf(bf, LBFSZ,
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
		goto out;

	error = uiomove_frombuf(bf, len, uio);
out:
	free(bf, M_TEMP);
	return error;
}

/*
 * Linux compatible /proc/devices. Only active when the -o linux
 * mountflag is used.
 */
int
procfs_dodevices(struct lwp *curl, struct proc *p,
    struct pfsnode *pfs, struct uio *uio)
{
	char *bf;
	int offset = 0;
	int i, error = ENAMETOOLONG;

	/* XXX elad - may need filtering. */

	bf = malloc(LBFSZ, M_TEMP, M_WAITOK);

	offset += snprintf(&bf[offset], LBFSZ - offset, "Character devices:\n");
	if (offset >= LBFSZ)
		goto out;

	for (i = 0; i < max_devsw_convs; i++) {
		if ((devsw_conv[i].d_name == NULL) || 
		    (devsw_conv[i].d_cmajor == -1))
			continue;

		offset += snprintf(&bf[offset], LBFSZ - offset, 
		    "%3d %s\n", devsw_conv[i].d_cmajor, devsw_conv[i].d_name);
		if (offset >= LBFSZ)
			goto out;
	}

	offset += snprintf(&bf[offset], LBFSZ - offset, "\nBlock devices:\n");
	if (offset >= LBFSZ)
		goto out;

	for (i = 0; i < max_devsw_convs; i++) {
		if ((devsw_conv[i].d_name == NULL) || 
		    (devsw_conv[i].d_bmajor == -1))
			continue;

		offset += snprintf(&bf[offset], LBFSZ - offset, 
		    "%3d %s\n", devsw_conv[i].d_bmajor, devsw_conv[i].d_name);
		if (offset >= LBFSZ)
			goto out;
	}

	error = uiomove_frombuf(bf, offset, uio);
out:
	free(bf, M_TEMP);
	return error;
}

/*
 * Linux compatible /proc/stat. Only active when the -o linux
 * mountflag is used.
 */
int
procfs_docpustat(struct lwp *curl, struct proc *p,
    struct pfsnode *pfs, struct uio *uio)
{
	struct timeval	 runtime;
        struct cpu_info *ci;
        CPU_INFO_ITERATOR cii;
	char		*bf;
	int	 	 error;
	int	 	 len;
	int	 	 i;

	error = ENAMETOOLONG;
	bf = malloc(LBFSZ, M_TEMP, M_WAITOK);

	len = snprintf(bf, LBFSZ,
		"cpu %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 "\n",
		curcpu()->ci_schedstate.spc_cp_time[CP_USER],
		curcpu()->ci_schedstate.spc_cp_time[CP_NICE],
		curcpu()->ci_schedstate.spc_cp_time[CP_SYS] /*+ [CP_INTR]*/,
		curcpu()->ci_schedstate.spc_cp_time[CP_IDLE]);
	if (len == 0)
		goto out;

	i = 0;
	for (CPU_INFO_FOREACH(cii, ci)) {
		len += snprintf(&bf[len], LBFSZ - len, 
			"cpu%d %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64
			"\n", i,
			ci->ci_schedstate.spc_cp_time[CP_USER],
			ci->ci_schedstate.spc_cp_time[CP_NICE],
			ci->ci_schedstate.spc_cp_time[CP_SYS],
			ci->ci_schedstate.spc_cp_time[CP_IDLE]);
		if (len >= LBFSZ)
			goto out;
		i += 1;
	}

	timersub(&curcpu()->ci_schedstate.spc_runtime, &boottime, &runtime);
	len += snprintf(&bf[len], LBFSZ - len,
			"disk 0 0 0 0\n"
			"page %u %u\n"
			"swap %u %u\n"
			"intr %u\n"
			"ctxt %u\n"
			"btime %lld\n",
			uvmexp.pageins, uvmexp.pdpageouts,
			uvmexp.pgswapin, uvmexp.pgswapout,
			uvmexp.intrs,
			uvmexp.swtch,
			(long long)boottime.tv_sec);
	if (len >= LBFSZ)
		goto out;

	error = uiomove_frombuf(bf, len, uio);
out:
	free(bf, M_TEMP);
	return error;
}

/*
 * Linux compatible /proc/loadavg. Only active when the -o linux
 * mountflag is used.
 */
int
procfs_doloadavg(struct lwp *curl, struct proc *p,
    struct pfsnode *pfs, struct uio *uio)
{
	char	*bf;
	int 	 error;
	int 	 len;

	error = ENAMETOOLONG;
	bf = malloc(LBFSZ, M_TEMP, M_WAITOK);

	averunnable.fscale = FSCALE;
	len = snprintf(bf, LBFSZ,
	        "%d.%02d %d.%02d %d.%02d %d/%d %d\n",
		(int)(averunnable.ldavg[0] / averunnable.fscale),
		(int)(averunnable.ldavg[0] * 100 / averunnable.fscale % 100),
		(int)(averunnable.ldavg[1] / averunnable.fscale),
		(int)(averunnable.ldavg[1] * 100 / averunnable.fscale % 100),
		(int)(averunnable.ldavg[2] / averunnable.fscale),
		(int)(averunnable.ldavg[2] * 100 / averunnable.fscale % 100),
		1,		/* number of ONPROC processes */
		nprocs,
		30000);		/* last pid */
	if (len == 0)
		goto out;

	error = uiomove_frombuf(bf, len, uio);
out:
	free(bf, M_TEMP);
	return error;
}

/*
 * Linux compatible /proc/<pid>/statm. Only active when the -o linux
 * mountflag is used.
 */
int
procfs_do_pid_statm(struct lwp *curl, struct lwp *l,
    struct pfsnode *pfs, struct uio *uio)
{
	unsigned long	 stext = 0, etext = 0, sstack = 0;
	struct proc	*p = l->l_proc;
	struct rusage	*ru = &p->p_stats->p_ru;
	char		*bf;
	int	 	 error;
	int	 	 len;

	error = ENAMETOOLONG;
	bf = malloc(LBFSZ, M_TEMP, M_WAITOK);

	get_proc_size_info(l, &stext, &etext, &sstack);

	len = snprintf(bf, LBFSZ,
	        "%lu %lu %lu %lu %lu %lu %lu\n",
		(ru->ru_ixrss + ru->ru_idrss + ru->ru_isrss), /* size */
		ru->ru_maxrss,	/* resident */
		ru->ru_ixrss,	/* shared */
		stext,
		etext,
		sstack,
		(unsigned long) 0);
	if (len == 0)
		goto out;

	error = uiomove_frombuf(bf, len, uio);
out:
	free(bf, M_TEMP);
	return error;
}

/*
 * Linux compatible /proc/<pid>/stat. Only active when the -o linux
 * mountflag is used.
 */
int
procfs_do_pid_stat(struct lwp *curl, struct lwp *l,
    struct pfsnode *pfs, struct uio *uio)
{
	char *bf;
	struct proc *p = l->l_proc;
	int len;
	struct tty *tty = p->p_session->s_ttyp;
	struct rusage *ru = &p->p_stats->p_ru;
	struct rusage *cru = &p->p_stats->p_cru;
	unsigned long stext = 0, etext = 0, sstack = 0;
	struct timeval rt;
	int error = 0;

	bf = malloc(LBFSZ, M_TEMP, M_WAITOK);

	get_proc_size_info(l, &stext, &etext, &sstack);

	mutex_enter(&proclist_lock);
	mutex_enter(&p->p_mutex);
	mutex_enter(&p->p_smutex);

	calcru(p, NULL, NULL, NULL, &rt);

	len = snprintf(bf, LBFSZ,
	    "%d (%s) %c %d %d %d %d %d "
	    "%u "
	    "%lu %lu %lu %lu %lu %lu %lu %lu "
	    "%d %d %d "
	    "%lu %lu %lu %lu %" PRIu64 " "
	    "%lu %lu %lu "
	    "%u %u "
	    "%u %u %u %u "
	    "%lu %lu %lu %d %d\n",

	    p->p_pid,
	    p->p_comm,
	    "0IR3SZD"[(p->p_stat > 6) ? 0 : (int)p->p_stat],
	    (p->p_pptr != NULL) ? p->p_pptr->p_pid : 0,

	    p->p_pgid,
	    p->p_session->s_sid,
	    tty ? tty->t_dev : 0,
	    (tty && tty->t_pgrp) ? tty->t_pgrp->pg_id : 0,

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

	    rt.tv_sec,
	    p->p_stats->p_start.tv_sec,
	    ru->ru_ixrss + ru->ru_idrss + ru->ru_isrss,
	    ru->ru_maxrss,
	    p->p_rlimit[RLIMIT_RSS].rlim_cur,

	    stext,					/* start code */
	    etext,					/* end code */
	    sstack,					/* mm start stack */
	    0,						/* XXX: pc */
	    0,						/* XXX: sp */
	    p->p_sigpend.sp_set.__bits[0],		/* XXX: pending */
	    0,						/* XXX: held */
	    p->p_sigctx.ps_sigignore.__bits[0],		/* ignored */
	    p->p_sigctx.ps_sigcatch.__bits[0],		/* caught */

	    (unsigned long)(intptr_t)l->l_wchan,
	    ru->ru_nvcsw,
	    ru->ru_nivcsw,
	    p->p_exitsig,
	    0);						/* XXX: processor */

	mutex_exit(&p->p_smutex);
	mutex_exit(&p->p_mutex);
	mutex_exit(&proclist_lock);

	if (len == 0)
		goto out;

	error = uiomove_frombuf(bf, len, uio);
out:
	free(bf, M_TEMP);
	return error;
}

int
procfs_docpuinfo(struct lwp *curl, struct proc *p,
    struct pfsnode *pfs, struct uio *uio)
{
	int len = LBFSZ;
	char *bf = malloc(len, M_TEMP, M_WAITOK);
	int error;

	if (procfs_getcpuinfstr(bf, &len) < 0) {
		error = ENOSPC;
		goto done;
	}

	if (len == 0) {
		error = 0;
		goto done;
	}

	error = uiomove_frombuf(bf, len, uio);
done:
	free(bf, M_TEMP);
	return error;
}

int
procfs_douptime(struct lwp *curl, struct proc *p,
    struct pfsnode *pfs, struct uio *uio)
{
	char *bf;
	int len;
	struct timeval runtime;
	u_int64_t idle;
	int error = 0;

	bf = malloc(LBFSZ, M_TEMP, M_WAITOK);

	timersub(&curcpu()->ci_schedstate.spc_runtime, &boottime, &runtime);
	idle = curcpu()->ci_schedstate.spc_cp_time[CP_IDLE];
	len = snprintf(bf, LBFSZ,
	    "%lu.%02lu %" PRIu64 ".%02" PRIu64 "\n",
	    runtime.tv_sec, runtime.tv_usec / 10000,
	    idle / hz, (((idle % hz) * 100) / hz) % 100);

	if (len == 0)
		goto out;

	error = uiomove_frombuf(bf, len, uio);
out:
	free(bf, M_TEMP);
	return error;
}

int
procfs_domounts(struct lwp *curl, struct proc *p,
    struct pfsnode *pfs, struct uio *uio)
{
	char *bf, *mtab = NULL;
	const char *fsname;
	size_t len, mtabsz = 0;
	struct mount *mp, *nmp;
	struct statvfs *sfs;
	int error = 0;

	/* XXX elad - may need filtering. */

	bf = malloc(LBFSZ, M_TEMP, M_WAITOK);
	simple_lock(&mountlist_slock);
	for (mp = CIRCLEQ_FIRST(&mountlist); mp != (void *)&mountlist;
	     mp = nmp) {
		if (vfs_busy(mp, LK_NOWAIT, &mountlist_slock)) {
			nmp = CIRCLEQ_NEXT(mp, mnt_list);
			continue;
		}

		sfs = &mp->mnt_stat;

		/* Linux uses different names for some filesystems */
		fsname = sfs->f_fstypename;
		if (strcmp(fsname, "procfs") == 0)
			fsname = "proc";
		else if (strcmp(fsname, "ext2fs") == 0)
			fsname = "ext2";

		len = snprintf(bf, LBFSZ, "%s %s %s %s%s%s%s%s%s 0 0\n",
			sfs->f_mntfromname,
			sfs->f_mntonname,
			fsname,
			(mp->mnt_flag & MNT_RDONLY) ? "ro" : "rw",
			(mp->mnt_flag & MNT_NOSUID) ? ",nosuid" : "",
			(mp->mnt_flag & MNT_NOEXEC) ? ",noexec" : "",
			(mp->mnt_flag & MNT_NODEV) ? ",nodev" : "",
			(mp->mnt_flag & MNT_SYNCHRONOUS) ? ",sync" : "",
			(mp->mnt_flag & MNT_NOATIME) ? ",noatime" : ""
			);

		mtab = realloc(mtab, mtabsz + len, M_TEMP, M_WAITOK);
		memcpy(mtab + mtabsz, bf, len);
		mtabsz += len;

		simple_lock(&mountlist_slock);
		nmp = CIRCLEQ_NEXT(mp, mnt_list);
		vfs_unbusy(mp);
	}
	simple_unlock(&mountlist_slock);
	free(bf, M_TEMP);

	if (mtabsz > 0) {
		error = uiomove_frombuf(mtab, mtabsz, uio);
		free(mtab, M_TEMP);
	}

	return error;
}
