/*	$NetBSD: linux_osf1.c,v 1.3.6.1 2022/08/03 11:11:33 martin Exp $	*/

/*
 * Copyright (c) 1999 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_osf1.c,v 1.3.6.1 2022/08/03 11:11:33 martin Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/syscallargs.h>
#include <sys/vfs_syscalls.h>

#include <machine/alpha.h>
#include <machine/cpuconf.h>
#include <machine/rpb.h>
#include <machine/fpu.h>

#include <compat/common/compat_util.h>
#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/arch/alpha/linux_signal.h>
#include <compat/linux/arch/alpha/linux_osf1.h>
#include <compat/linux/linux_syscallargs.h>

#include <net/if.h>
#include <netinet/in.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsmount.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/ufsmount.h>

#include <machine/vmparam.h>

const struct emul_flags_xtab osf1_wait_options_xtab[] = {
    {	OSF1_WNOHANG,		OSF1_WNOHANG,		WNOHANG		},
    {	OSF1_WUNTRACED,		OSF1_WUNTRACED,		WUNTRACED	},
    {	0								}
};

const struct emul_flags_xtab osf1_nfs_mount_flags_xtab[] = {
    {	OSF1_NFSMNT_SOFT,	OSF1_NFSMNT_SOFT,	NFSMNT_SOFT,	},
    {	OSF1_NFSMNT_WSIZE,	OSF1_NFSMNT_WSIZE,	NFSMNT_WSIZE,	},
    {	OSF1_NFSMNT_RSIZE,	OSF1_NFSMNT_RSIZE,	NFSMNT_RSIZE,	},
    {	OSF1_NFSMNT_TIMEO,	OSF1_NFSMNT_TIMEO,	NFSMNT_TIMEO,	},
    {	OSF1_NFSMNT_RETRANS,	OSF1_NFSMNT_RETRANS,	NFSMNT_RETRANS,	},
#if 0 /* no equivalent; needs special handling, see below */
    {	OSF1_NFSMNT_HOSTNAME,	OSF1_NFSMNT_HOSTNAME,	???,		},
#endif
    {	OSF1_NFSMNT_INT,	OSF1_NFSMNT_INT,	NFSMNT_INT,	},
    {	OSF1_NFSMNT_NOCONN,	OSF1_NFSMNT_NOCONN,	NFSMNT_NOCONN,	},
#if 0 /* no equivalents */
    {	OSF1_NFSMNT_NOAC,	OSF1_NFSMNT_NOAC,	???,		},
    {	OSF1_NFSMNT_ACREGMIN,	OSF1_NFSMNT_ACREGMIN,	???,		},
    {	OSF1_NFSMNT_ACREGMAX,	OSF1_NFSMNT_ACREGMAX,	???,		},
    {	OSF1_NFSMNT_ACDIRMIN,	OSF1_NFSMNT_ACDIRMIN,	???,		},
    {	OSF1_NFSMNT_ACDIRMAX,	OSF1_NFSMNT_ACDIRMAX,	???,		},
    {	OSF1_NFSMNT_NOCTO,	OSF1_NFSMNT_NOCTO,	???,		},
    {	OSF1_NFSMNT_POSIX,	OSF1_NFSMNT_POSIX,	???,		},
    {	OSF1_NFSMNT_AUTO,	OSF1_NFSMNT_AUTO,	???,		},
    {	OSF1_NFSMNT_SEC,	OSF1_NFSMNT_SEC,	???,		},
    {	OSF1_NFSMNT_TCP,	OSF1_NFSMNT_TCP,	???,		},
    {	OSF1_NFSMNT_PROPLIST,	OSF1_NFSMNT_PROPLIST,	???,		},
#endif
    {	0								}
};

static void
osf1_cvt_rusage_from_native(const struct rusage *ru, struct osf1_rusage *oru)
{

	memset(oru, 0, sizeof(*oru));

	oru->ru_utime.tv_sec = ru->ru_utime.tv_sec;
	oru->ru_utime.tv_usec = ru->ru_utime.tv_usec;

	oru->ru_stime.tv_sec = ru->ru_stime.tv_sec;
	oru->ru_stime.tv_usec = ru->ru_stime.tv_usec;

	oru->ru_maxrss = ru->ru_maxrss;
	oru->ru_ixrss = ru->ru_ixrss;
	oru->ru_idrss = ru->ru_idrss;
	oru->ru_isrss = ru->ru_isrss;
	oru->ru_minflt = ru->ru_minflt;
	oru->ru_majflt = ru->ru_majflt;
	oru->ru_nswap = ru->ru_nswap;
	oru->ru_inblock = ru->ru_inblock;
	oru->ru_oublock = ru->ru_oublock;
	oru->ru_msgsnd = ru->ru_msgsnd;
	oru->ru_msgrcv = ru->ru_msgrcv;
	oru->ru_nsignals = ru->ru_nsignals;
	oru->ru_nvcsw = ru->ru_nvcsw;
	oru->ru_nivcsw = ru->ru_nivcsw;
}

static void
osf1_cvt_statfs_from_native(const struct statvfs *bsfs, struct osf1_statfs *osfs)
{

	memset(osfs, 0, sizeof(*osfs));
	if (!strncmp(MOUNT_FFS, bsfs->f_fstypename, sizeof(bsfs->f_fstypename)))
		osfs->f_type = OSF1_MOUNT_UFS;
	else if (!strncmp(MOUNT_NFS, bsfs->f_fstypename, sizeof(bsfs->f_fstypename)))
		osfs->f_type = OSF1_MOUNT_NFS;
	else if (!strncmp(MOUNT_MFS, bsfs->f_fstypename, sizeof(bsfs->f_fstypename)))
		osfs->f_type = OSF1_MOUNT_MFS;
	else
		/* uh oh...  XXX = PC, CDFS, PROCFS, etc. */
		osfs->f_type = OSF1_MOUNT_ADDON;
	osfs->f_flags = bsfs->f_flag;		/* XXX translate */
	osfs->f_fsize = bsfs->f_frsize;
	osfs->f_bsize = bsfs->f_bsize;
	osfs->f_blocks = bsfs->f_blocks;
	osfs->f_bfree = bsfs->f_bfree;
	osfs->f_bavail = bsfs->f_bavail;
	osfs->f_files = bsfs->f_files;
	osfs->f_ffree = bsfs->f_ffree;
	memcpy(&osfs->f_fsid, &bsfs->f_fsidx, sizeof osfs->f_fsid);
	/* osfs->f_spare zeroed above */
	memcpy(osfs->f_mntonname, bsfs->f_mntonname, sizeof osfs->f_mntonname);
	memcpy(osfs->f_mntfromname, bsfs->f_mntfromname,
	    sizeof osfs->f_mntfromname);
	/* XXX osfs->f_xxx should be filled in... */
}

/* --------------------------------------------------------------------- */

int
linux_sys_osf1_wait4(struct lwp *l, const struct linux_sys_osf1_wait4_args *uap, register_t *retval)
{
	struct osf1_rusage osf1_rusage;
	struct rusage netbsd_rusage;
	unsigned long leftovers;
	int error, status;
	int options = SCARG(uap, options);
	int pid = SCARG(uap, pid);

	/* translate options */
	options = emul_flags_translate(osf1_wait_options_xtab,
	    options, &leftovers);
	if (leftovers != 0)
		return (EINVAL);

	error = do_sys_wait(&pid, &status, options,
	    SCARG(uap, rusage) != NULL ? &netbsd_rusage : NULL);

	retval[0] = pid;
	if (pid == 0)
		return error;

	if (SCARG(uap, rusage)) {
		osf1_cvt_rusage_from_native(&netbsd_rusage, &osf1_rusage);
		error = copyout(&osf1_rusage, SCARG(uap, rusage),
		    sizeof osf1_rusage);
	}

	if (error == 0 && SCARG(uap, status))
		error = copyout(&status, SCARG(uap, status), sizeof(status));

	return error;
}

#define	OSF1_MNT_WAIT		0x1
#define	OSF1_MNT_NOWAIT		0x2

#define	OSF1_MNT_FORCE		0x1
#define	OSF1_MNT_NOFORCE	0x2

/* acceptable flags for various calls */
#define	OSF1_GETFSSTAT_FLAGS	(OSF1_MNT_WAIT|OSF1_MNT_NOWAIT)
#define	OSF1_MOUNT_FLAGS	0xffffffff			/* XXX */
#define	OSF1_UNMOUNT_FLAGS	(OSF1_MNT_FORCE|OSF1_MNT_NOFORCE)

static int
osf1_mount_mfs(struct lwp *l, const struct linux_sys_osf1_mount_args *uap)
{
	struct osf1_mfs_args osf_ma;
	struct mfs_args bsd_ma;
	int error;
	register_t dummy;

	if ((error = copyin(SCARG(uap, data), &osf_ma, sizeof osf_ma)))
		return error;

	memset(&bsd_ma, 0, sizeof bsd_ma);
	bsd_ma.fspec = osf_ma.name;
	/* XXX export args */
	bsd_ma.base = osf_ma.base;
	bsd_ma.size = osf_ma.size;

	return do_sys_mount(l, "mfs", UIO_SYSSPACE, SCARG(uap, path),
	    SCARG(uap, flags), &bsd_ma, UIO_SYSSPACE, sizeof bsd_ma, &dummy);
}

static int
osf1_mount_nfs(struct lwp *l, const struct linux_sys_osf1_mount_args *uap)
{
	struct osf1_nfs_args osf_na;
	struct nfs_args bsd_na;
	int error;
	unsigned long leftovers;
	register_t dummy;

	if ((error = copyin(SCARG(uap, data), &osf_na, sizeof osf_na)))
		return error;

	memset(&bsd_na, 0, sizeof bsd_na);
	bsd_na.addr = (struct sockaddr *)osf_na.addr;
	bsd_na.addrlen = sizeof (struct sockaddr_in);
	bsd_na.fh = osf_na.fh;

        /* translate flags */
        bsd_na.flags = emul_flags_translate(osf1_nfs_mount_flags_xtab,
            osf_na.flags, &leftovers);
	if (leftovers & OSF1_NFSMNT_HOSTNAME) {
		leftovers &= ~OSF1_NFSMNT_HOSTNAME;
		bsd_na.hostname = osf_na.hostname;
	} else {
		/* XXX FILL IN HOST NAME WITH IPADDR? */
	}
	if (leftovers & OSF1_NFSMNT_TCP) {
		leftovers &= ~OSF1_NFSMNT_TCP;
		bsd_na.sotype = SOCK_DGRAM;
		bsd_na.proto = 0;
	} else {
		bsd_na.sotype = SOCK_STREAM;
		bsd_na.proto = 0;
	}
        if (leftovers != 0)
                return (EINVAL);

	/* copy structure elements based on flags */
	if (bsd_na.flags & NFSMNT_WSIZE)
		bsd_na.wsize = osf_na.wsize;
	if (bsd_na.flags & NFSMNT_RSIZE)
		bsd_na.rsize = osf_na.rsize;
	if (bsd_na.flags & NFSMNT_TIMEO)
		bsd_na.timeo = osf_na.timeo;
	if (bsd_na.flags & NFSMNT_RETRANS)
		bsd_na.retrans = osf_na.retrans;

	return do_sys_mount(l, "nfs", UIO_SYSSPACE, SCARG(uap, path),
	    SCARG(uap, flags), &bsd_na, UIO_SYSSPACE, sizeof bsd_na, &dummy);
}

int
linux_sys_osf1_mount(struct lwp *l, const struct linux_sys_osf1_mount_args *uap, register_t *retval)
{

	if (SCARG(uap, flags) & ~OSF1_MOUNT_FLAGS)
		return (EINVAL);

	/* XXX - xlate flags */

	switch (SCARG(uap, type)) {
	case OSF1_MOUNT_NFS:
		return osf1_mount_nfs(l, uap);
		break;

	case OSF1_MOUNT_MFS:
		return osf1_mount_mfs(l, uap);

	default:
		return (EINVAL);
	}
}

int
linux_sys_osf1_set_program_attributes(struct lwp *l, const struct linux_sys_osf1_set_program_attributes_args *uap, register_t *retval)
{
	struct proc *p = l->l_proc;
	segsz_t tsize, dsize;

	tsize = btoc(SCARG(uap, tsize));
	dsize = btoc(SCARG(uap, dsize));

	if (dsize > p->p_rlimit[RLIMIT_DATA].rlim_cur)
		return (ENOMEM);
	if (tsize > MAXTSIZ)
		return (ENOMEM);

	/* XXXSMP unlocked */
	p->p_vmspace->vm_taddr = SCARG(uap, taddr);
	p->p_vmspace->vm_tsize = tsize;
	p->p_vmspace->vm_daddr = SCARG(uap, daddr);
	p->p_vmspace->vm_dsize = dsize;

	return (0);
}

int
linux_sys_osf1_setitimer(struct lwp *l, const struct linux_sys_osf1_setitimer_args *uap, register_t *retval)
{
	struct osf1_itimerval o_itv, o_oitv;
	struct itimerval b_itv, b_oitv;
	int which;
	int error;

	switch (SCARG(uap, which)) {
	case OSF1_ITIMER_REAL:
		which = ITIMER_REAL;
		break;

	case OSF1_ITIMER_VIRTUAL:
		which = ITIMER_VIRTUAL;
		break;

	case OSF1_ITIMER_PROF:
		which = ITIMER_PROF;
		break;

	default:
		return (EINVAL);
	}

	/* get the OSF/1 itimerval argument */
	error = copyin(SCARG(uap, itv), &o_itv, sizeof o_itv);
	if (error != 0)
		return error;

	/* fill in and the NetBSD timeval */
	memset(&b_itv, 0, sizeof b_itv);
	b_itv.it_interval.tv_sec = o_itv.it_interval.tv_sec;
	b_itv.it_interval.tv_usec = o_itv.it_interval.tv_usec;
	b_itv.it_value.tv_sec = o_itv.it_value.tv_sec;
	b_itv.it_value.tv_usec = o_itv.it_value.tv_usec;

	if (SCARG(uap, oitv) != NULL) {
		dogetitimer(l->l_proc, which, &b_oitv);
		if (error)
			return error;
	}
		
	error = dosetitimer(l->l_proc, which, &b_itv);

	if (error == 0 || SCARG(uap, oitv) == NULL)
		return error;

	/* fill in and copy out the old timeval */
	memset(&o_oitv, 0, sizeof o_oitv);
	o_oitv.it_interval.tv_sec = b_oitv.it_interval.tv_sec;
	o_oitv.it_interval.tv_usec = b_oitv.it_interval.tv_usec;
	o_oitv.it_value.tv_sec = b_oitv.it_value.tv_sec;
	o_oitv.it_value.tv_usec = b_oitv.it_value.tv_usec;

	return copyout(&o_oitv, SCARG(uap, oitv), sizeof o_oitv);
}

int
linux_sys_osf1_select(struct lwp *l, const struct linux_sys_osf1_select_args *uap,
    register_t *retval)
{
	struct osf1_timeval otv;
	struct timespec ats, *ts = NULL;
	int error;

	if (SCARG(uap, tv)) {
		/* get the OSF/1 timeval argument */
		error = copyin(SCARG(uap, tv), &otv, sizeof otv);
		if (error != 0)
			return error;

		ats.tv_sec = otv.tv_sec;
		ats.tv_nsec = otv.tv_usec * 1000;
		ts = &ats;
	}

	return selcommon(retval, SCARG(uap, nd), SCARG(uap, in),
	    SCARG(uap, ou), SCARG(uap, ex), ts, NULL);
}

int
linux_sys_osf1_gettimeofday(struct lwp *l, const struct linux_sys_osf1_gettimeofday_args *uap, register_t *retval)
{
	struct osf1_timeval otv;
	struct osf1_timezone otz;
	struct timeval tv;
	int error;

	microtime(&tv);
	memset(&otv, 0, sizeof otv);
	otv.tv_sec = tv.tv_sec;
	otv.tv_usec = tv.tv_usec;
	error = copyout(&otv, SCARG(uap, tv), sizeof otv);

	if (error == 0 && SCARG(uap, tzp) != NULL) {
		memset(&otz, 0, sizeof otz);
		error = copyout(&otz, SCARG(uap, tzp), sizeof otz);
	}
	return (error);
}

int
linux_sys_osf1_getrusage(struct lwp *l, const struct linux_sys_osf1_getrusage_args *uap, register_t *retval)
{
	int error, who;
	struct osf1_rusage osf1_rusage;
	struct rusage ru;
	struct proc *p = l->l_proc;


	switch (SCARG(uap, who)) {
	case OSF1_RUSAGE_SELF:
		who = RUSAGE_SELF;
		break;

	case OSF1_RUSAGE_CHILDREN:
		who = RUSAGE_CHILDREN;
		break;

	case OSF1_RUSAGE_THREAD:		/* XXX not supported */
	default:
		return EINVAL;
	}

	error = getrusage1(p, who, &ru);
	if (error != 0)
		return error;

	osf1_cvt_rusage_from_native(&ru, &osf1_rusage);

	return copyout(&osf1_rusage, SCARG(uap, rusage), sizeof osf1_rusage);
}

int
linux_sys_osf1_settimeofday(struct lwp *l, const struct linux_sys_osf1_settimeofday_args *uap, register_t *retval)
{
	struct osf1_timeval otv;
	struct timeval tv, *tvp;
	int error = 0;

	if (SCARG(uap, tv) == NULL)
		tvp = NULL;
	else {
		/* get the OSF/1 timeval argument */
		error = copyin(SCARG(uap, tv), &otv, sizeof otv);
		if (error != 0)
			return error;

		tv.tv_sec = otv.tv_sec;
		tv.tv_usec = otv.tv_usec;
		tvp = &tv;
	}

	/* NetBSD ignores the timezone field */

	return settimeofday1(tvp, false, (const void *)SCARG(uap, tzp), l, true);
}

int
linux_sys_osf1_utimes(struct lwp *l, const struct linux_sys_osf1_utimes_args *uap, register_t *retval)
{
	struct osf1_timeval otv;
	struct timeval tv[2], *tvp;
	int error;

	if (SCARG(uap, tptr) == NULL)
		tvp = NULL;
	else {
		/* get the OSF/1 timeval argument */
		error = copyin(SCARG(uap, tptr), &otv, sizeof otv);
		if (error != 0)
			return error;

		/* fill in and copy out the NetBSD timeval */
		tv[0].tv_sec = otv.tv_sec;
		tv[0].tv_usec = otv.tv_usec;
		/* Set access and modified to the same time */
		tv[1].tv_sec = otv.tv_sec;
		tv[1].tv_usec = otv.tv_usec;
		tvp = tv;
	}

	return do_sys_utimes(l, NULL, SCARG(uap, path), FOLLOW,
			    tvp, UIO_SYSSPACE);
}

int
linux_sys_osf1_statfs(struct lwp *l, const struct linux_sys_osf1_statfs_args *uap, register_t *retval)
{
	struct mount *mp;
	struct statvfs *sp;
	struct osf1_statfs osfs;
	int error;
	struct vnode *vp;

	error = namei_simple_user(SCARG(uap, path),
				NSM_FOLLOW_TRYEMULROOT, &vp);
	if (error != 0)
		return (error);
	mp = vp->v_mount;
	sp = &mp->mnt_stat;
	vrele(vp);
	if ((error = VFS_STATVFS(mp, sp)))
		return (error);
	sp->f_flag = mp->mnt_flag & MNT_VISFLAGMASK;
	osf1_cvt_statfs_from_native(sp, &osfs);
	return copyout(&osfs, SCARG(uap, buf), uimin(sizeof osfs,
	    SCARG(uap, len)));
}

int
linux_sys_osf1_fstatfs(struct lwp *l, const struct linux_sys_osf1_fstatfs_args *uap, register_t *retval)
{
	file_t *fp;
	struct mount *mp;
	struct statvfs *sp;
	struct osf1_statfs osfs;
	int error;

	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(SCARG(uap, fd), &fp)))
		return (error);
	mp = fp->f_vnode->v_mount;
	sp = &mp->mnt_stat;
	if ((error = VFS_STATVFS(mp, sp)))
		goto out;
	sp->f_flag = mp->mnt_flag & MNT_VISFLAGMASK;
	osf1_cvt_statfs_from_native(sp, &osfs);
	error = copyout(&osfs, SCARG(uap, buf), uimin(sizeof osfs,
	    SCARG(uap, len)));
 out:
 	fd_putfile(SCARG(uap, fd));
	return (error);
}

int
linux_sys_osf1_sysinfo(struct lwp *l, const struct linux_sys_osf1_sysinfo_args *uap, register_t *retval)
{
	const char *string;
	size_t slen;
	int error;

	error = 0;
	switch (SCARG(uap, cmd)) {
	case OSF1_SI_SYSNAME:
		string = ostype;
		break;

	case OSF1_SI_HOSTNAME:
		string = hostname;
		break;

	case OSF1_SI_RELEASE:
		string = osrelease;
		break;

	case OSF1_SI_VERSION:
		goto should_handle;

	case OSF1_SI_MACHINE:
		string = MACHINE;
		break;

	case OSF1_SI_ARCHITECTURE:
		string = MACHINE_ARCH;
		break;

	case OSF1_SI_HW_SERIAL:
		string = "666";			/* OSF/1 emulation?  YES! */
		break;

	case OSF1_SI_HW_PROVIDER:
		string = "unknown";
		break;

	case OSF1_SI_SRPC_DOMAIN:
		goto dont_care;

	case OSF1_SI_SET_HOSTNAME:
		goto should_handle;

	case OSF1_SI_SET_SYSNAME:
		goto should_handle;

	case OSF1_SI_SET_SRPC_DOMAIN:
		goto dont_care;

	default:
should_handle:
		printf("osf1_sys_sysinfo(%d, %p, 0x%lx)\n", SCARG(uap, cmd),
		    SCARG(uap, buf), SCARG(uap,len));
dont_care:
		return (EINVAL);
	};

	slen = strlen(string) + 1;
	if (SCARG(uap, buf)) {
		error = copyout(string, SCARG(uap, buf),
				uimin(slen, SCARG(uap, len)));
		if (!error && (SCARG(uap, len) > 0) && (SCARG(uap, len) < slen))
			error = ustore_char(SCARG(uap, buf)
					    + SCARG(uap, len) - 1, 0);
	}
	if (!error)
		retval[0] = slen;

	return (error);
}

int
linux_sys_osf1_usleep_thread(struct lwp *l, const struct linux_sys_osf1_usleep_thread_args *uap, register_t *retval)
{
	struct osf1_timeval otv, endotv;
	struct timeval tv, ntv, endtv;
	u_long ticks;
	int error;

	if ((error = copyin(SCARG(uap, sleep), &otv, sizeof otv)))
		return (error);
	tv.tv_sec = otv.tv_sec;
	tv.tv_usec = otv.tv_usec;

	ticks = howmany((u_long)tv.tv_sec * 1000000 + tv.tv_usec, tick);
	if (ticks == 0)
		ticks = 1;

	getmicrotime(&tv);

	tsleep(l, PUSER|PCATCH, "uslpthrd", ticks);	/* XXX */

	if (SCARG(uap, slept) != NULL) {
		getmicrotime(&ntv);
		timersub(&ntv, &tv, &endtv);
		if (endtv.tv_sec < 0 || endtv.tv_usec < 0)
			endtv.tv_sec = endtv.tv_usec = 0;

		memset(&endotv, 0, sizeof(endotv));
		endotv.tv_sec = endtv.tv_sec;
		endotv.tv_usec = endtv.tv_usec;
		error = copyout(&endotv, SCARG(uap, slept), sizeof endotv);
	}
	return (error);
}

int
linux_sys_osf1_getsysinfo(struct lwp *l, const struct linux_sys_osf1_getsysinfo_args *uap, register_t *retval)
{
	extern int ncpus;
	int error;
	int unit;
	long percpu;
	long proctype;
	u_int64_t fpflags;
	struct osf1_cpu_info cpuinfo;
	const void *data;
	size_t datalen;

	error = 0;

	switch(SCARG(uap, op))
	{
	case OSF_GET_MAX_UPROCS:
		data = &maxproc;
		datalen = sizeof(maxproc);
		break;
	case OSF_GET_PHYSMEM:
		data = &physmem;
		datalen = sizeof(physmem);
		break;
	case OSF_GET_MAX_CPU:
	case OSF_GET_CPUS_IN_BOX:
		data = &ncpus;
		datalen = sizeof(ncpus);
		break;
	case OSF_GET_IEEE_FP_CONTROL:
		if (((fpflags = alpha_read_fp_c(l)) & IEEE_INHERIT) != 0) {
			fpflags |= 1ULL << 63;
			fpflags &= ~IEEE_INHERIT;
		}
		data = &fpflags;
		datalen = sizeof(fpflags);
		break;
	case OSF_GET_CPU_INFO:
		memset(&cpuinfo, 0, sizeof(cpuinfo));
#ifdef __alpha__
		unit = alpha_pal_whami();
#else
		unit = 0; /* XXX */
#endif
		cpuinfo.current_cpu = unit;
		cpuinfo.cpus_in_box = ncpus;
		cpuinfo.cpu_type = LOCATE_PCS(hwrpb, unit)->pcs_proc_type;
		cpuinfo.ncpus = ncpus;
		cpuinfo.cpus_present = ncpus;
		cpuinfo.cpus_running = ncpus;
		cpuinfo.cpu_binding = 1;
		cpuinfo.cpu_ex_binding = 0;
		cpuinfo.mhz = hwrpb->rpb_cc_freq / 1000000;
		data = &cpuinfo;
		datalen = sizeof(cpuinfo);
		break;
	case OSF_GET_PROC_TYPE:
#ifdef __alpha__
		unit = alpha_pal_whami();
		proctype = LOCATE_PCS(hwrpb, unit)->pcs_proc_type;
#else
		proctype = 0;	/* XXX */
#endif
		data = &proctype;
		datalen = sizeof(percpu);
		break;
	case OSF_GET_HWRPB: /* note -- osf/1 doesn't have rpb_tbhint[8] */
		data = hwrpb;
		datalen = hwrpb->rpb_size;
		break;
	case OSF_GET_PLATFORM_NAME:
		data = platform.model;
		datalen = strlen(platform.model) + 1;
		break;
	default:
		printf("osf1_getsysinfo called with unknown op=%ld\n",
		       SCARG(uap, op));
		/* return EINVAL; */
		return 0;
	}

	if (SCARG(uap, nbytes) < datalen)
		return (EINVAL);
	error = copyout(data, SCARG(uap, buffer), datalen);
	if (!error)
		retval[0] = 1;
	return (error);
}

int
linux_sys_osf1_setsysinfo(struct lwp *l, const struct linux_sys_osf1_setsysinfo_args *uap, register_t *retval)
{
	u_int64_t temp;
	int error;

	error = 0;

	switch(SCARG(uap, op)) {
	case OSF_SET_IEEE_FP_CONTROL:

		if ((error = copyin(SCARG(uap, buffer), &temp, sizeof(temp))))
			break;
		if (temp >> 63 != 0)
			temp |= IEEE_INHERIT;
		alpha_write_fp_c(l, temp);
		break;
	default:
		uprintf("osf1_setsysinfo called with op=%ld\n", SCARG(uap, op));
		//error = EINVAL;
	}
	retval[0] = 0;
	return (error);
}
