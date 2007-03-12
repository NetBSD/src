/*	$NetBSD: procfs_vnops.c,v 1.148.2.2 2007/03/12 05:59:08 rmind Exp $	*/

/*-
 * Copyright (c) 2006, 2007 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * Copyright (c) 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry.
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
 *	@(#)procfs_vnops.c	8.18 (Berkeley) 5/21/95
 */

/*
 * Copyright (c) 1993 Jan-Simon Pendry
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry.
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
 *	@(#)procfs_vnops.c	8.18 (Berkeley) 5/21/95
 */

/*
 * procfs vnode interface
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: procfs_vnops.c,v 1.148.2.2 2007/03/12 05:59:08 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/dirent.h>
#include <sys/resourcevar.h>
#include <sys/stat.h>
#include <sys/ptrace.h>
#include <sys/kauth.h>

#include <uvm/uvm_extern.h>	/* for PAGE_SIZE */

#include <machine/reg.h>

#include <miscfs/genfs/genfs.h>
#include <miscfs/procfs/procfs.h>

/*
 * Vnode Operations.
 *
 */

static int procfs_validfile_linux(struct lwp *, struct mount *);
static int procfs_root_readdir_callback(struct proc *, void *);
static struct vnode *procfs_dir(pfstype, struct lwp *, struct proc *,
				char **, char *, int);

/*
 * This is a list of the valid names in the
 * process-specific sub-directories.  It is
 * used in procfs_lookup and procfs_readdir
 */
static const struct proc_target {
	u_char	pt_type;
	u_char	pt_namlen;
	const char	*pt_name;
	pfstype	pt_pfstype;
	int	(*pt_valid)(struct lwp *, struct mount *);
} proc_targets[] = {
#define N(s) sizeof(s)-1, s
	/*	  name		type		validp */
	{ DT_DIR, N("."),	PFSproc,	NULL },
	{ DT_DIR, N(".."),	PFSroot,	NULL },
	{ DT_DIR, N("fd"),	PFSfd,		NULL },
	{ DT_REG, N("file"),	PFSfile,	procfs_validfile },
	{ DT_REG, N("mem"),	PFSmem,		NULL },
	{ DT_REG, N("regs"),	PFSregs,	procfs_validregs },
	{ DT_REG, N("fpregs"),	PFSfpregs,	procfs_validfpregs },
	{ DT_REG, N("ctl"),	PFSctl,		NULL },
	{ DT_REG, N("stat"),	PFSstat,	procfs_validfile_linux },
	{ DT_REG, N("status"),	PFSstatus,	NULL },
	{ DT_REG, N("note"),	PFSnote,	NULL },
	{ DT_REG, N("notepg"),	PFSnotepg,	NULL },
	{ DT_REG, N("map"),	PFSmap,		procfs_validmap },
	{ DT_REG, N("maps"),	PFSmaps,	procfs_validmap },
	{ DT_REG, N("cmdline"), PFScmdline,	NULL },
	{ DT_REG, N("exe"),	PFSexe,		procfs_validfile },
	{ DT_LNK, N("cwd"),	PFScwd,		NULL },
	{ DT_LNK, N("root"),	PFSchroot,	NULL },
	{ DT_LNK, N("emul"),	PFSemul,	NULL },
#ifdef __HAVE_PROCFS_MACHDEP
	PROCFS_MACHDEP_NODETYPE_DEFNS
#endif
#undef N
};
static const int nproc_targets = sizeof(proc_targets) / sizeof(proc_targets[0]);

/*
 * List of files in the root directory. Note: the validate function will
 * be called with p == NULL for these ones.
 */
static const struct proc_target proc_root_targets[] = {
#define N(s) sizeof(s)-1, s
	/*	  name		    type	    validp */
	{ DT_REG, N("meminfo"),     PFSmeminfo,        procfs_validfile_linux },
	{ DT_REG, N("cpuinfo"),     PFScpuinfo,        procfs_validfile_linux },
	{ DT_REG, N("uptime"),      PFSuptime,         procfs_validfile_linux },
	{ DT_REG, N("mounts"),	    PFSmounts,	       procfs_validfile_linux },
	{ DT_REG, N("devices"),     PFSdevices,        procfs_validfile_linux },
#undef N
};
static const int nproc_root_targets =
    sizeof(proc_root_targets) / sizeof(proc_root_targets[0]);

int	procfs_lookup(void *);
#define	procfs_create	genfs_eopnotsupp
#define	procfs_mknod	genfs_eopnotsupp
int	procfs_open(void *);
int	procfs_close(void *);
int	procfs_access(void *);
int	procfs_getattr(void *);
int	procfs_setattr(void *);
#define	procfs_read	procfs_rw
#define	procfs_write	procfs_rw
#define	procfs_fcntl	genfs_fcntl
#define	procfs_ioctl	genfs_enoioctl
#define	procfs_poll	genfs_poll
#define procfs_revoke	genfs_revoke
#define	procfs_fsync	genfs_nullop
#define	procfs_seek	genfs_nullop
#define	procfs_remove	genfs_eopnotsupp
int	procfs_link(void *);
#define	procfs_rename	genfs_eopnotsupp
#define	procfs_mkdir	genfs_eopnotsupp
#define	procfs_rmdir	genfs_eopnotsupp
int	procfs_symlink(void *);
int	procfs_readdir(void *);
int	procfs_readlink(void *);
#define	procfs_abortop	genfs_abortop
int	procfs_inactive(void *);
int	procfs_reclaim(void *);
#define	procfs_lock	genfs_lock
#define	procfs_unlock	genfs_unlock
#define	procfs_bmap	genfs_badop
#define	procfs_strategy	genfs_badop
int	procfs_print(void *);
int	procfs_pathconf(void *);
#define	procfs_islocked	genfs_islocked
#define	procfs_advlock	genfs_einval
#define	procfs_bwrite	genfs_eopnotsupp
#define procfs_putpages	genfs_null_putpages

static int atoi(const char *, size_t);

/*
 * procfs vnode operations.
 */
int (**procfs_vnodeop_p)(void *);
const struct vnodeopv_entry_desc procfs_vnodeop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, procfs_lookup },		/* lookup */
	{ &vop_create_desc, procfs_create },		/* create */
	{ &vop_mknod_desc, procfs_mknod },		/* mknod */
	{ &vop_open_desc, procfs_open },		/* open */
	{ &vop_close_desc, procfs_close },		/* close */
	{ &vop_access_desc, procfs_access },		/* access */
	{ &vop_getattr_desc, procfs_getattr },		/* getattr */
	{ &vop_setattr_desc, procfs_setattr },		/* setattr */
	{ &vop_read_desc, procfs_read },		/* read */
	{ &vop_write_desc, procfs_write },		/* write */
	{ &vop_fcntl_desc, procfs_fcntl },		/* fcntl */
	{ &vop_ioctl_desc, procfs_ioctl },		/* ioctl */
	{ &vop_poll_desc, procfs_poll },		/* poll */
	{ &vop_revoke_desc, procfs_revoke },		/* revoke */
	{ &vop_fsync_desc, procfs_fsync },		/* fsync */
	{ &vop_seek_desc, procfs_seek },		/* seek */
	{ &vop_remove_desc, procfs_remove },		/* remove */
	{ &vop_link_desc, procfs_link },		/* link */
	{ &vop_rename_desc, procfs_rename },		/* rename */
	{ &vop_mkdir_desc, procfs_mkdir },		/* mkdir */
	{ &vop_rmdir_desc, procfs_rmdir },		/* rmdir */
	{ &vop_symlink_desc, procfs_symlink },		/* symlink */
	{ &vop_readdir_desc, procfs_readdir },		/* readdir */
	{ &vop_readlink_desc, procfs_readlink },	/* readlink */
	{ &vop_abortop_desc, procfs_abortop },		/* abortop */
	{ &vop_inactive_desc, procfs_inactive },	/* inactive */
	{ &vop_reclaim_desc, procfs_reclaim },		/* reclaim */
	{ &vop_lock_desc, procfs_lock },		/* lock */
	{ &vop_unlock_desc, procfs_unlock },		/* unlock */
	{ &vop_bmap_desc, procfs_bmap },		/* bmap */
	{ &vop_strategy_desc, procfs_strategy },	/* strategy */
	{ &vop_print_desc, procfs_print },		/* print */
	{ &vop_islocked_desc, procfs_islocked },	/* islocked */
	{ &vop_pathconf_desc, procfs_pathconf },	/* pathconf */
	{ &vop_advlock_desc, procfs_advlock },		/* advlock */
	{ &vop_putpages_desc, procfs_putpages },	/* putpages */
	{ NULL, NULL }
};
const struct vnodeopv_desc procfs_vnodeop_opv_desc =
	{ &procfs_vnodeop_p, procfs_vnodeop_entries };
/*
 * set things up for doing i/o on
 * the pfsnode (vp).  (vp) is locked
 * on entry, and should be left locked
 * on exit.
 *
 * for procfs we don't need to do anything
 * in particular for i/o.  all that is done
 * is to support exclusive open on process
 * memory images.
 */
int
procfs_open(v)
	void *v;
{
	struct vop_open_args /* {
		struct vnode *a_vp;
		int  a_mode;
		kauth_cred_t a_cred;
		struct lwp *a_l;
	} */ *ap = v;
	struct pfsnode *pfs = VTOPFS(ap->a_vp);
	struct lwp *l1;
	struct proc *p2;
	int error;

	if ((error = procfs_proc_lock(pfs->pfs_pid, &p2, ENOENT)) != 0)
		return error;

	l1 = ap->a_l;				/* tracer */

#define	M2K(m)	(((m) & FREAD) && ((m) & FWRITE) ? \
		 KAUTH_REQ_PROCESS_CANPROCFS_RW : \
		 (m) & FWRITE ? KAUTH_REQ_PROCESS_CANPROCFS_WRITE : \
		 KAUTH_REQ_PROCESS_CANPROCFS_READ)

	mutex_enter(&p2->p_mutex);
	error = kauth_authorize_process(l1->l_cred, KAUTH_PROCESS_CANPROCFS,
	    p2, pfs, KAUTH_ARG(M2K(ap->a_mode)), NULL);
	mutex_exit(&p2->p_mutex);
	if (error)
		return (error);

#undef M2K

	switch (pfs->pfs_type) {
	case PFSmem:
		if (((pfs->pfs_flags & FWRITE) && (ap->a_mode & O_EXCL)) ||
		    ((pfs->pfs_flags & O_EXCL) && (ap->a_mode & FWRITE))) {
			error = EBUSY;
			break;
		}

		if (!proc_isunder(p2, l1))
			return (EPERM);

		if (ap->a_mode & FWRITE)
			pfs->pfs_flags = ap->a_mode & (FWRITE|O_EXCL);

		break;

	case PFSregs:
	case PFSfpregs:
		if (!proc_isunder(p2, l1))
			return (EPERM);

		break;

	default:
		break;
	}

	procfs_proc_unlock(p2);
	return (error);
}

/*
 * close the pfsnode (vp) after doing i/o.
 * (vp) is not locked on entry or exit.
 *
 * nothing to do for procfs other than undo
 * any exclusive open flag (see _open above).
 */
int
procfs_close(v)
	void *v;
{
	struct vop_close_args /* {
		struct vnode *a_vp;
		int  a_fflag;
		kauth_cred_t a_cred;
		struct lwp *a_l;
	} */ *ap = v;
	struct pfsnode *pfs = VTOPFS(ap->a_vp);

	switch (pfs->pfs_type) {
	case PFSmem:
		if ((ap->a_fflag & FWRITE) && (pfs->pfs_flags & O_EXCL))
			pfs->pfs_flags &= ~(FWRITE|O_EXCL);
		break;

	default:
		break;
	}

	return (0);
}

/*
 * _inactive is called when the pfsnode
 * is vrele'd and the reference count goes
 * to zero.  (vp) will be on the vnode free
 * list, so to get it back vget() must be
 * used.
 *
 * for procfs, check if the process is still
 * alive and if it isn't then just throw away
 * the vnode by calling vgone().  this may
 * be overkill and a waste of time since the
 * chances are that the process will still be
 * there.
 *
 * (vp) is locked on entry, but must be unlocked on exit.
 */
int
procfs_inactive(v)
	void *v;
{
	struct vop_inactive_args /* {
		struct vnode *a_vp;
		struct proc *a_p;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct pfsnode *pfs = VTOPFS(vp);
	struct proc *p;
	int error;

	VOP_UNLOCK(vp, 0);

	error = procfs_proc_lock(pfs->pfs_pid, &p, ESRCH);
	if (error != 0 && (vp->v_flag & VXLOCK) == 0)
		vgone(vp);
	else
		procfs_proc_unlock(p);

	return (0);
}

/*
 * _reclaim is called when getnewvnode()
 * wants to make use of an entry on the vnode
 * free list.  at this time the filesystem needs
 * to free any private data and remove the node
 * from any private lists.
 */
int
procfs_reclaim(v)
	void *v;
{
	struct vop_reclaim_args /* {
		struct vnode *a_vp;
	} */ *ap = v;

	return (procfs_freevp(ap->a_vp));
}

/*
 * Return POSIX pathconf information applicable to special devices.
 */
int
procfs_pathconf(v)
	void *v;
{
	struct vop_pathconf_args /* {
		struct vnode *a_vp;
		int a_name;
		register_t *a_retval;
	} */ *ap = v;

	switch (ap->a_name) {
	case _PC_LINK_MAX:
		*ap->a_retval = LINK_MAX;
		return (0);
	case _PC_MAX_CANON:
		*ap->a_retval = MAX_CANON;
		return (0);
	case _PC_MAX_INPUT:
		*ap->a_retval = MAX_INPUT;
		return (0);
	case _PC_PIPE_BUF:
		*ap->a_retval = PIPE_BUF;
		return (0);
	case _PC_CHOWN_RESTRICTED:
		*ap->a_retval = 1;
		return (0);
	case _PC_VDISABLE:
		*ap->a_retval = _POSIX_VDISABLE;
		return (0);
	case _PC_SYNC_IO:
		*ap->a_retval = 1;
		return (0);
	default:
		return (EINVAL);
	}
	/* NOTREACHED */
}

/*
 * _print is used for debugging.
 * just print a readable description
 * of (vp).
 */
int
procfs_print(v)
	void *v;
{
	struct vop_print_args /* {
		struct vnode *a_vp;
	} */ *ap = v;
	struct pfsnode *pfs = VTOPFS(ap->a_vp);

	printf("tag VT_PROCFS, type %d, pid %d, mode %x, flags %lx\n",
	    pfs->pfs_type, pfs->pfs_pid, pfs->pfs_mode, pfs->pfs_flags);
	return 0;
}

int
procfs_link(v)
	void *v;
{
	struct vop_link_args /* {
		struct vnode *a_dvp;
		struct vnode *a_vp;
		struct componentname *a_cnp;
	} */ *ap = v;

	VOP_ABORTOP(ap->a_dvp, ap->a_cnp);
	vput(ap->a_dvp);
	return (EROFS);
}

int
procfs_symlink(v)
	void *v;
{
	struct vop_symlink_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
		char *a_target;
	} */ *ap = v;

	VOP_ABORTOP(ap->a_dvp, ap->a_cnp);
	vput(ap->a_dvp);
	return (EROFS);
}

/*
 * Works out the path to (and vnode of) the target process's current
 * working directory or chroot.  If the caller is in a chroot and
 * can't "reach" the target's cwd or root (or some other error
 * occurs), a "/" is returned for the path and a NULL pointer is
 * returned for the vnode.
 */
static struct vnode *
procfs_dir(pfstype t, struct lwp *caller, struct proc *target,
	   char **bpp, char *path, int len)
{
	struct vnode *vp, *rvp = caller->l_proc->p_cwdi->cwdi_rdir;
	char *bp;

	LOCK_ASSERT(mutex_owned(&target->p_mutex));

	bp = bpp ? *bpp : NULL;

	switch (t) {
	case PFScwd:
		vp = target->p_cwdi->cwdi_cdir;
		break;
	case PFSchroot:
		vp = target->p_cwdi->cwdi_rdir;
		break;
	case PFSexe:
		vp = target->p_textvp;
		break;
	default:
		return (NULL);
	}

	if (rvp == NULL)
		rvp = rootvnode;
	mutex_exit(&target->p_mutex);	/* XXXSMP */
	if (vp == NULL || getcwd_common(vp, rvp, bp ? &bp : NULL, path,
	    len / 2, 0, caller) != 0) {
		vp = NULL;
		if (bpp) {
/* 
			if (t == PFSexe) {
				snprintf(path, len, "%s/%d/file"
				    mp->mnt_stat.f_mntonname, pfs->pfs_pid);
			} else */ {
				bp = *bpp;
				*--bp = '/';
			}
		}
	}
	mutex_enter(&target->p_mutex);	/* XXXSMP */

	if (bpp)
		*bpp = bp;

	return (vp);
}

/*
 * Invent attributes for pfsnode (vp) and store
 * them in (vap).
 * Directories lengths are returned as zero since
 * any real length would require the genuine size
 * to be computed, and nothing cares anyway.
 *
 * this is relatively minimal for procfs.
 */
int
procfs_getattr(v)
	void *v;
{
	struct vop_getattr_args /* {
		struct vnode *a_vp;
		struct vattr *a_vap;
		kauth_cred_t a_cred;
		struct lwp *a_l;
	} */ *ap = v;
	struct pfsnode *pfs = VTOPFS(ap->a_vp);
	struct vattr *vap = ap->a_vap;
	struct proc *procp;
	char *path;
	int error;

	/* first check the process still exists */
	switch (pfs->pfs_type) {
	case PFSroot:
	case PFScurproc:
	case PFSself:
		procp = NULL;
		break;

	default:
		error = procfs_proc_lock(pfs->pfs_pid, &procp, ENOENT);
		if (error != 0)
			return (error);
		break;
	}

	switch (pfs->pfs_type) {
	case PFScwd:
	case PFSchroot:
	case PFSexe:
		MALLOC(path, char *, MAXPATHLEN + 4, M_TEMP,
		    M_WAITOK|M_CANFAIL);
		if (path == NULL && procp != NULL) {
			procfs_proc_unlock(procp);
			return (ENOMEM);
		}
		break;

	default:
		path = NULL;
		break;
	}

	if (procp != NULL) {
		mutex_enter(&procp->p_mutex);
		error = kauth_authorize_process(kauth_cred_get(),
		    KAUTH_PROCESS_CANSEE, procp, NULL, NULL, NULL);
		mutex_exit(&procp->p_mutex);
		if (error != 0) {
		    	procfs_proc_unlock(procp);
		    	if (path != NULL)
		    		free(path, M_TEMP);
			return (ENOENT);
		}
	}

	error = 0;

	/* start by zeroing out the attributes */
	VATTR_NULL(vap);

	/* next do all the common fields */
	vap->va_type = ap->a_vp->v_type;
	vap->va_mode = pfs->pfs_mode;
	vap->va_fileid = pfs->pfs_fileno;
	vap->va_flags = 0;
	vap->va_blocksize = PAGE_SIZE;

	/*
	 * Make all times be current TOD.
	 *
	 * It would be possible to get the process start
	 * time from the p_stats structure, but there's
	 * no "file creation" time stamp anyway, and the
	 * p_stats structure is not addressable if u. gets
	 * swapped out for that process.
	 */
	getnanotime(&vap->va_ctime);
	vap->va_atime = vap->va_mtime = vap->va_ctime;
	if (procp)
		TIMEVAL_TO_TIMESPEC(&procp->p_stats->p_start,
		    &vap->va_birthtime);
	else
		getnanotime(&vap->va_birthtime);

	switch (pfs->pfs_type) {
	case PFSmem:
	case PFSregs:
	case PFSfpregs:
#if defined(__HAVE_PROCFS_MACHDEP) && defined(PROCFS_MACHDEP_PROTECT_CASES)
	PROCFS_MACHDEP_PROTECT_CASES
#endif
		/*
		 * If the process has exercised some setuid or setgid
		 * privilege, then rip away read/write permission so
		 * that only root can gain access.
		 */
		if (procp->p_flag & PK_SUGID)
			vap->va_mode &= ~(S_IRUSR|S_IWUSR);
		/* FALLTHROUGH */
	case PFSctl:
	case PFSstatus:
	case PFSstat:
	case PFSnote:
	case PFSnotepg:
	case PFSmap:
	case PFSmaps:
	case PFScmdline:
	case PFSemul:
		vap->va_nlink = 1;
		vap->va_uid = kauth_cred_geteuid(procp->p_cred);
		vap->va_gid = kauth_cred_getegid(procp->p_cred);
		break;
	case PFSmeminfo:
	case PFSdevices:
	case PFScpuinfo:
	case PFSuptime:
	case PFSmounts:
		vap->va_nlink = 1;
		vap->va_uid = vap->va_gid = 0;
		break;

	default:
		break;
	}

	/*
	 * now do the object specific fields
	 *
	 * The size could be set from struct reg, but it's hardly
	 * worth the trouble, and it puts some (potentially) machine
	 * dependent data into this machine-independent code.  If it
	 * becomes important then this function should break out into
	 * a per-file stat function in the corresponding .c file.
	 */

	switch (pfs->pfs_type) {
	case PFSroot:
		/*
		 * Set nlink to 1 to tell fts(3) we don't actually know.
		 */
		vap->va_nlink = 1;
		vap->va_uid = 0;
		vap->va_gid = 0;
		vap->va_bytes = vap->va_size = DEV_BSIZE;
		break;

	case PFSself:
	case PFScurproc: {
		char bf[16];		/* should be enough */
		vap->va_nlink = 1;
		vap->va_uid = 0;
		vap->va_gid = 0;
		vap->va_bytes = vap->va_size =
		    snprintf(bf, sizeof(bf), "%ld", (long)curproc->p_pid);
		break;
	}

	case PFSfd:
		if (pfs->pfs_fd != -1) {
			struct file *fp;

			fp = fd_getfile(procp->p_fd, pfs->pfs_fd);
			if (fp == NULL) {
				error = EBADF;
				break;
			}
			FILE_USE(fp);
			vap->va_nlink = 1;
			vap->va_uid = kauth_cred_geteuid(fp->f_cred);
			vap->va_gid = kauth_cred_getegid(fp->f_cred);
			switch (fp->f_type) {
			case DTYPE_VNODE:
				vap->va_bytes = vap->va_size =
				    ((struct vnode *)fp->f_data)->v_size;
				break;
			default:
				vap->va_bytes = vap->va_size = 0;
				break;
			}
			FILE_UNUSE(fp, curlwp);
			break;
		}
		/*FALLTHROUGH*/
	case PFSproc:
		vap->va_nlink = 2;
		vap->va_uid = kauth_cred_geteuid(procp->p_cred);
		vap->va_gid = kauth_cred_getegid(procp->p_cred);
		vap->va_bytes = vap->va_size = DEV_BSIZE;
		break;

	case PFSfile:
		error = EOPNOTSUPP;
		break;

	case PFSmem:
		vap->va_bytes = vap->va_size =
			ctob(procp->p_vmspace->vm_tsize +
				    procp->p_vmspace->vm_dsize +
				    procp->p_vmspace->vm_ssize);
		break;

#if defined(PT_GETREGS) || defined(PT_SETREGS)
	case PFSregs:
		vap->va_bytes = vap->va_size = sizeof(struct reg);
		break;
#endif

#if defined(PT_GETFPREGS) || defined(PT_SETFPREGS)
	case PFSfpregs:
		vap->va_bytes = vap->va_size = sizeof(struct fpreg);
		break;
#endif

	case PFSctl:
	case PFSstatus:
	case PFSstat:
	case PFSnote:
	case PFSnotepg:
	case PFScmdline:
	case PFSmeminfo:
	case PFSdevices:
	case PFScpuinfo:
	case PFSuptime:
	case PFSmounts:
		vap->va_bytes = vap->va_size = 0;
		break;
	case PFSmap:
	case PFSmaps:
		/*
		 * Advise a larger blocksize for the map files, so that
		 * they may be read in one pass.
		 */
		vap->va_blocksize = 4 * PAGE_SIZE;
		vap->va_bytes = vap->va_size = 0;
		break;

	case PFScwd:
	case PFSchroot:
	case PFSexe: {
		char *bp;

		vap->va_nlink = 1;
		vap->va_uid = 0;
		vap->va_gid = 0;
		bp = path + MAXPATHLEN;
		*--bp = '\0';
		mutex_enter(&procp->p_mutex);
		(void)procfs_dir(pfs->pfs_type, curlwp, procp, &bp, path,
		     MAXPATHLEN);
		mutex_exit(&procp->p_mutex);
		vap->va_bytes = vap->va_size = strlen(bp);
		break;
	}

	case PFSemul:
		vap->va_bytes = vap->va_size = strlen(procp->p_emul->e_name);
		break;

#ifdef __HAVE_PROCFS_MACHDEP
	PROCFS_MACHDEP_NODETYPE_CASES
		error = procfs_machdep_getattr(ap->a_vp, vap, procp);
		break;
#endif

	default:
		panic("procfs_getattr");
	}

	if (procp != NULL)
		procfs_proc_unlock(procp);
	if (path != NULL)
		free(path, M_TEMP);

	return (error);
}

/*ARGSUSED*/
int
procfs_setattr(void *v)
{
	/*
	 * just fake out attribute setting
	 * it's not good to generate an error
	 * return, otherwise things like creat()
	 * will fail when they try to set the
	 * file length to 0.  worse, this means
	 * that echo $note > /proc/$pid/note will fail.
	 */

	return (0);
}

/*
 * implement access checking.
 *
 * actually, the check for super-user is slightly
 * broken since it will allow read access to write-only
 * objects.  this doesn't cause any particular trouble
 * but does mean that the i/o entry points need to check
 * that the operation really does make sense.
 */
int
procfs_access(v)
	void *v;
{
	struct vop_access_args /* {
		struct vnode *a_vp;
		int a_mode;
		kauth_cred_t a_cred;
		struct lwp *a_l;
	} */ *ap = v;
	struct vattr va;
	int error;

	if ((error = VOP_GETATTR(ap->a_vp, &va, ap->a_cred, ap->a_l)) != 0)
		return (error);

	return (vaccess(va.va_type, va.va_mode,
	    va.va_uid, va.va_gid, ap->a_mode, ap->a_cred));
}

/*
 * lookup.  this is incredibly complicated in the
 * general case, however for most pseudo-filesystems
 * very little needs to be done.
 *
 * Locking isn't hard here, just poorly documented.
 *
 * If we're looking up ".", just vref the parent & return it.
 *
 * If we're looking up "..", unlock the parent, and lock "..". If everything
 * went ok, and we're on the last component and the caller requested the
 * parent locked, try to re-lock the parent. We do this to prevent lock
 * races.
 *
 * For anything else, get the needed node. Then unlock the parent if not
 * the last component or not LOCKPARENT (i.e. if we wouldn't re-lock the
 * parent in the .. case).
 *
 * We try to exit with the parent locked in error cases.
 */
int
procfs_lookup(v)
	void *v;
{
	struct vop_lookup_args /* {
		struct vnode * a_dvp;
		struct vnode ** a_vpp;
		struct componentname * a_cnp;
	} */ *ap = v;
	struct componentname *cnp = ap->a_cnp;
	struct vnode **vpp = ap->a_vpp;
	struct vnode *dvp = ap->a_dvp;
	const char *pname = cnp->cn_nameptr;
	const struct proc_target *pt = NULL;
	struct vnode *fvp;
	pid_t pid, vnpid;
	struct pfsnode *pfs;
	struct proc *p = NULL;
	struct lwp *l = NULL;
	int i, error;
	pfstype type;

	*vpp = NULL;

	if (cnp->cn_nameiop == DELETE || cnp->cn_nameiop == RENAME)
		return (EROFS);

	if (cnp->cn_namelen == 1 && *pname == '.') {
		*vpp = dvp;
		VREF(dvp);
		return (0);
	}

	pfs = VTOPFS(dvp);
	switch (pfs->pfs_type) {
	case PFSroot:
		/*
		 * Shouldn't get here with .. in the root node.
		 */
		if (cnp->cn_flags & ISDOTDOT)
			return (EIO);

		for (i = 0; i < nproc_root_targets; i++) {
			pt = &proc_root_targets[i];
			/*
			 * check for node match.  proc is always NULL here,
			 * so call pt_valid with constant NULL lwp.
			 */
			if (cnp->cn_namelen == pt->pt_namlen &&
			    memcmp(pt->pt_name, pname, cnp->cn_namelen) == 0 &&
			    (pt->pt_valid == NULL ||
			     (*pt->pt_valid)(NULL, dvp->v_mount)))
				break;
		}

		if (i != nproc_root_targets) {
			error = procfs_allocvp(dvp->v_mount, vpp, 0,
			    pt->pt_pfstype, -1, NULL);
			return (error);
		}

		if (CNEQ(cnp, "curproc", 7)) {
			pid = curproc->p_pid;
			vnpid = 0;
			type = PFScurproc;
		} else if (CNEQ(cnp, "self", 4)) {
			pid = curproc->p_pid;
			vnpid = 0;
			type = PFSself;
		} else {
			pid = (pid_t)atoi(pname, cnp->cn_namelen);
			vnpid = pid;
			type = PFSproc;
		}

		if (procfs_proc_lock(pid, &p, ESRCH) != 0)
			break;
		error = procfs_allocvp(dvp->v_mount, vpp, vnpid, type, -1, p);
		procfs_proc_unlock(p);
		return (error);

	case PFSproc:
		/*
		 * do the .. dance. We unlock the directory, and then
		 * get the root dir. That will automatically return ..
		 * locked. Then if the caller wanted dvp locked, we
		 * re-lock.
		 */
		if (cnp->cn_flags & ISDOTDOT) {
			VOP_UNLOCK(dvp, 0);
			error = procfs_root(dvp->v_mount, vpp);
			vn_lock(dvp, LK_EXCLUSIVE | LK_RETRY);
			return (error);
		}

		if (procfs_proc_lock(pfs->pfs_pid, &p, ESRCH) != 0)
			break;

		for (pt = proc_targets, i = 0; i < nproc_targets; pt++, i++) {
			struct lwp *plwp;
			int found;

			mutex_enter(&p->p_smutex);
			plwp = proc_representative_lwp(p, NULL, 1);
			lwp_addref(plwp);
			mutex_exit(&p->p_smutex);
			found = cnp->cn_namelen == pt->pt_namlen &&
			    memcmp(pt->pt_name, pname, cnp->cn_namelen) == 0 &&
			    (pt->pt_valid == NULL
			      || (*pt->pt_valid)(plwp, dvp->v_mount));
			lwp_delref(plwp);
			if (found)
				break;
		}
		if (i == nproc_targets) {
			procfs_proc_unlock(p);
			break;
		}
		if (pt->pt_pfstype == PFSfile) {
			fvp = p->p_textvp;
			/* We already checked that it exists. */
			VREF(fvp);
			vn_lock(fvp, LK_EXCLUSIVE | LK_RETRY);
			*vpp = fvp;
			procfs_proc_unlock(p);
			return (0);
		}

		error = procfs_allocvp(dvp->v_mount, vpp, pfs->pfs_pid,
		    pt->pt_pfstype, -1, p);
		procfs_proc_unlock(p);
		return (error);

	case PFSfd: {
		int fd;
		struct file *fp;

		if ((error = procfs_proc_lock(pfs->pfs_pid, &p, ENOENT)) != 0)
			return error;

		/*
		 * do the .. dance. We unlock the directory, and then
		 * get the proc dir. That will automatically return ..
		 * locked. Then re-lock the directory.
		 */
		if (cnp->cn_flags & ISDOTDOT) {
			VOP_UNLOCK(dvp, 0);
			error = procfs_allocvp(dvp->v_mount, vpp, pfs->pfs_pid,
			    PFSproc, -1, p);
			vn_lock(dvp, LK_EXCLUSIVE | LK_RETRY);
			procfs_proc_unlock(p);
			return (error);
		}
		fd = atoi(pname, cnp->cn_namelen);

		mutex_enter(&p->p_mutex);
		fp = fd_getfile(p->p_fd, fd);
		mutex_exit(&p->p_mutex);
		if (fp == NULL) {
			procfs_proc_unlock(p);
			return ENOENT;
		}

		FILE_USE(fp);

		switch (fp->f_type) {
		case DTYPE_VNODE:
			fvp = (struct vnode *)fp->f_data;

			/* Don't show directories */
			if (fvp->v_type == VDIR)
				goto symlink;

			VREF(fvp);
			FILE_UNUSE(fp, l);
			vn_lock(fvp, LK_EXCLUSIVE | LK_RETRY |
			    (p == curproc ? LK_CANRECURSE : 0));
			*vpp = fvp;
			error = 0;
			break;
		default:
		symlink:
			FILE_UNUSE(fp, l);
			error = procfs_allocvp(dvp->v_mount, vpp, pfs->pfs_pid,
			    PFSfd, fd, p);
			break;
		}
		procfs_proc_unlock(p);
		return error;
	}
	default:
		return (ENOTDIR);
	}

	return (cnp->cn_nameiop == LOOKUP ? ENOENT : EROFS);
}

int
procfs_validfile(struct lwp *l, struct mount *mp)
{
	return l != NULL && l->l_proc != NULL && l->l_proc->p_textvp != NULL;
}

static int
procfs_validfile_linux(l, mp)
	struct lwp *l;
	struct mount *mp;
{
	int flags;

	flags = VFSTOPROC(mp)->pmnt_flags;
	return (flags & PROCFSMNT_LINUXCOMPAT) &&
	    (l == NULL || l->l_proc == NULL || procfs_validfile(l, mp));
}

struct procfs_root_readdir_ctx {
	struct uio *uiop;
	off_t *cookies;
	int ncookies;
	off_t off;
	off_t startoff;
	int error;
};

static int
procfs_root_readdir_callback(struct proc *p, void *arg)
{
	struct procfs_root_readdir_ctx *ctxp = arg;
	struct dirent d;
	struct uio *uiop;
	int error;

	uiop = ctxp->uiop;
	if (uiop->uio_resid < UIO_MX)
		return -1; /* no space */

	if (ctxp->off < ctxp->startoff) {
		ctxp->off++;
		return 0;
	}

	if (kauth_authorize_process(kauth_cred_get(),
	    KAUTH_PROCESS_CANSEE, p, NULL, NULL, NULL) != 0)
		return 0;

	memset(&d, 0, UIO_MX);
	d.d_reclen = UIO_MX;
	d.d_fileno = PROCFS_FILENO(p->p_pid, PFSproc, -1);
	d.d_namlen = snprintf(d.d_name,
	    UIO_MX - offsetof(struct dirent, d_name), "%ld", (long)p->p_pid);
	d.d_type = DT_DIR;

	mutex_exit(&proclist_lock);
	error = uiomove(&d, UIO_MX, uiop);
	mutex_enter(&proclist_lock);
	if (error) {
		ctxp->error = error;
		return -1;
	}

	ctxp->ncookies++;
	if (ctxp->cookies)
		*(ctxp->cookies)++ = ctxp->off + 1;
	ctxp->off++;

	return 0;
}

/*
 * readdir returns directory entries from pfsnode (vp).
 *
 * the strategy here with procfs is to generate a single
 * directory entry at a time (struct dirent) and then
 * copy that out to userland using uiomove.  a more efficent
 * though more complex implementation, would try to minimize
 * the number of calls to uiomove().  for procfs, this is
 * hardly worth the added code complexity.
 *
 * this should just be done through read()
 */
int
procfs_readdir(v)
	void *v;
{
	struct vop_readdir_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		kauth_cred_t a_cred;
		int *a_eofflag;
		off_t **a_cookies;
		int *a_ncookies;
	} */ *ap = v;
	struct uio *uio = ap->a_uio;
	struct dirent d;
	struct pfsnode *pfs;
	off_t i;
	int error;
	off_t *cookies = NULL;
	int ncookies;
	struct vnode *vp;
	const struct proc_target *pt;
	struct procfs_root_readdir_ctx ctx;
	struct lwp *l;

	vp = ap->a_vp;
	pfs = VTOPFS(vp);

	if (uio->uio_resid < UIO_MX)
		return (EINVAL);
	if (uio->uio_offset < 0)
		return (EINVAL);

	error = 0;
	i = uio->uio_offset;
	memset(&d, 0, UIO_MX);
	d.d_reclen = UIO_MX;
	ncookies = uio->uio_resid / UIO_MX;

	switch (pfs->pfs_type) {
	/*
	 * this is for the process-specific sub-directories.
	 * all that is needed to is copy out all the entries
	 * from the procent[] table (top of this file).
	 */
	case PFSproc: {
		struct proc *p;

		if (i >= nproc_targets)
			return 0;

		if (procfs_proc_lock(pfs->pfs_pid, &p, ESRCH) != 0)
			break;

		if (ap->a_ncookies) {
			ncookies = min(ncookies, (nproc_targets - i));
			cookies = malloc(ncookies * sizeof (off_t),
			    M_TEMP, M_WAITOK);
			*ap->a_cookies = cookies;
		}

		for (pt = &proc_targets[i];
		     uio->uio_resid >= UIO_MX && i < nproc_targets; pt++, i++) {
			if (pt->pt_valid) {
				/* XXXSMP locking */
				mutex_enter(&p->p_smutex);
				l = proc_representative_lwp(p, NULL, 1);
				mutex_exit(&p->p_smutex);
				if ((*pt->pt_valid)(l, vp->v_mount) == 0)
					continue;
			}

			d.d_fileno = PROCFS_FILENO(pfs->pfs_pid,
			    pt->pt_pfstype, -1);
			d.d_namlen = pt->pt_namlen;
			memcpy(d.d_name, pt->pt_name, pt->pt_namlen + 1);
			d.d_type = pt->pt_type;

			if ((error = uiomove(&d, UIO_MX, uio)) != 0)
				break;
			if (cookies)
				*cookies++ = i + 1;
		}

		procfs_proc_unlock(p);
	    	break;
	}
	case PFSfd: {
		struct proc *p;
		struct filedesc	*fdp;
		struct file *fp;
		int lim, nc = 0;

		if ((error = procfs_proc_lock(pfs->pfs_pid, &p, ESRCH)) != 0)
			return error;

		if (kauth_authorize_process(kauth_cred_get(),
		    KAUTH_PROCESS_CANSEE, p, NULL, NULL, NULL) != 0) {
		    	procfs_proc_unlock(p);
			return ESRCH;
		}

		fdp = p->p_fd;	/* XXXSMP */

		lim = min((int)p->p_rlimit[RLIMIT_NOFILE].rlim_cur, maxfiles);
		if (i >= lim) {
		    	procfs_proc_unlock(p);
			return 0;
		}

		if (ap->a_ncookies) {
			ncookies = min(ncookies, (fdp->fd_nfiles + 2 - i));
			cookies = malloc(ncookies * sizeof (off_t),
			    M_TEMP, M_WAITOK);
			*ap->a_cookies = cookies;
		}

		for (; i < 2 && uio->uio_resid >= UIO_MX; i++) {
			pt = &proc_targets[i];
			d.d_namlen = pt->pt_namlen;
			d.d_fileno = PROCFS_FILENO(pfs->pfs_pid,
			    pt->pt_pfstype, -1);
			(void)memcpy(d.d_name, pt->pt_name, pt->pt_namlen + 1);
			d.d_type = pt->pt_type;
			if ((error = uiomove(&d, UIO_MX, uio)) != 0)
				break;
			if (cookies)
				*cookies++ = i + 1;
			nc++;
		}
		if (error) {
			ncookies = nc;
			break;
		}
		for (; uio->uio_resid >= UIO_MX && i < fdp->fd_nfiles; i++) {
			/* check the descriptor exists */
			if ((fp = fd_getfile(fdp, i - 2)) == NULL)
				continue;
			simple_unlock(&fp->f_slock);

			d.d_fileno = PROCFS_FILENO(pfs->pfs_pid, PFSfd, i - 2);
			d.d_namlen = snprintf(d.d_name, sizeof(d.d_name),
			    "%lld", (long long)(i - 2));
			d.d_type = VREG;
			if ((error = uiomove(&d, UIO_MX, uio)) != 0)
				break;
			if (cookies)
				*cookies++ = i + 1;
			nc++;
		}
		ncookies = nc;
		procfs_proc_unlock(p);
		break;
	}

	/*
	 * this is for the root of the procfs filesystem
	 * what is needed are special entries for "curproc"
	 * and "self" followed by an entry for each process
	 * on allproc.
	 */

	case PFSroot: {
		int nc = 0;

		if (ap->a_ncookies) {
			/*
			 * XXX Potentially allocating too much space here,
			 * but I'm lazy. This loop needs some work.
			 */
			cookies = malloc(ncookies * sizeof (off_t),
			    M_TEMP, M_WAITOK);
			*ap->a_cookies = cookies;
		}
		error = 0;
		/* 0 ... 3 are static entries. */
		for (; i <= 3 && uio->uio_resid >= UIO_MX; i++) {
			switch (i) {
			case 0:		/* `.' */
			case 1:		/* `..' */
				d.d_fileno = PROCFS_FILENO(0, PFSroot, -1);
				d.d_namlen = i + 1;
				memcpy(d.d_name, "..", d.d_namlen);
				d.d_name[i + 1] = '\0';
				d.d_type = DT_DIR;
				break;

			case 2:
				d.d_fileno = PROCFS_FILENO(0, PFScurproc, -1);
				d.d_namlen = sizeof("curproc") - 1;
				memcpy(d.d_name, "curproc", sizeof("curproc"));
				d.d_type = DT_LNK;
				break;

			case 3:
				d.d_fileno = PROCFS_FILENO(0, PFSself, -1);
				d.d_namlen = sizeof("self") - 1;
				memcpy(d.d_name, "self", sizeof("self"));
				d.d_type = DT_LNK;
				break;
			}

			if ((error = uiomove(&d, UIO_MX, uio)) != 0)
				break;
			nc++;
			if (cookies)
				*cookies++ = i + 1;
		}
		/* 4 ... are process entries. */
		ctx.uiop = uio;
		ctx.error = 0;
		ctx.off = 4;
		ctx.startoff = i;
		ctx.cookies = cookies;
		ctx.ncookies = nc;
		proclist_foreach_call(&allproc,
		    procfs_root_readdir_callback, &ctx);
		cookies = ctx.cookies;
		nc = ctx.ncookies;
		error = ctx.error;
		if (error)
			break;

		/* misc entries. */
		if (i < ctx.off)
			i = ctx.off;
		if (i >= ctx.off + nproc_root_targets)
			break;
		for (pt = &proc_root_targets[i - ctx.off];
		    uio->uio_resid >= UIO_MX &&
		    pt < &proc_root_targets[nproc_root_targets];
		    pt++, i++) {
			if (pt->pt_valid &&
			    (*pt->pt_valid)(NULL, vp->v_mount) == 0)
				continue;
			d.d_fileno = PROCFS_FILENO(0, pt->pt_pfstype, -1);
			d.d_namlen = pt->pt_namlen;
			memcpy(d.d_name, pt->pt_name, pt->pt_namlen + 1);
			d.d_type = pt->pt_type;

			if ((error = uiomove(&d, UIO_MX, uio)) != 0)
				break;
			nc++;
			if (cookies)
				*cookies++ = i + 1;
		}

		ncookies = nc;
		break;
	}

	default:
		error = ENOTDIR;
		break;
	}

	if (ap->a_ncookies) {
		if (error) {
			if (cookies)
				free(*ap->a_cookies, M_TEMP);
			*ap->a_ncookies = 0;
			*ap->a_cookies = NULL;
		} else
			*ap->a_ncookies = ncookies;
	}
	uio->uio_offset = i;
	return (error);
}

/*
 * readlink reads the link of `curproc' and others
 */
int
procfs_readlink(v)
	void *v;
{
	struct vop_readlink_args *ap = v;
	char bf[16];		/* should be enough */
	char *bp = bf;
	char *path = NULL;
	int len;
	int error = 0;
	struct pfsnode *pfs = VTOPFS(ap->a_vp);
	struct proc *pown;

	if (pfs->pfs_fileno == PROCFS_FILENO(0, PFScurproc, -1))
		len = snprintf(bf, sizeof(bf), "%ld", (long)curproc->p_pid);
	else if (pfs->pfs_fileno == PROCFS_FILENO(0, PFSself, -1))
		len = snprintf(bf, sizeof(bf), "%s", "curproc");
	else if (pfs->pfs_fileno == PROCFS_FILENO(pfs->pfs_pid, PFScwd, -1) ||
	    pfs->pfs_fileno == PROCFS_FILENO(pfs->pfs_pid, PFSchroot, -1) ||
	    pfs->pfs_fileno == PROCFS_FILENO(pfs->pfs_pid, PFSexe, -1)) {
		if ((error = procfs_proc_lock(pfs->pfs_pid, &pown, ESRCH)) != 0)
			return error;
		MALLOC(path, char *, MAXPATHLEN + 4, M_TEMP,
		    M_WAITOK|M_CANFAIL);
		if (path == NULL) {
			procfs_proc_unlock(pown);
			return (ENOMEM);
		}
		bp = path + MAXPATHLEN;
		*--bp = '\0';
		mutex_enter(&pown->p_mutex);
		(void)procfs_dir(PROCFS_TYPE(pfs->pfs_fileno), curlwp, pown,
		    &bp, path, MAXPATHLEN);
		mutex_exit(&pown->p_mutex);
		procfs_proc_unlock(pown);
		len = strlen(bp);
	} else {
		struct file *fp;
		struct vnode *vxp, *vp;

		if ((error = procfs_proc_lock(pfs->pfs_pid, &pown, ESRCH)) != 0)
			return error;
		mutex_enter(&pown->p_mutex);
		fp = fd_getfile(pown->p_fd, pfs->pfs_fd);
		mutex_exit(&pown->p_mutex);
		if (error != 0) {
			procfs_proc_unlock(pown);
			return (EBADF);
		}
		FILE_USE(fp);
		switch (fp->f_type) {
		case DTYPE_VNODE:
			vxp = (struct vnode *)fp->f_data;
			if (vxp->v_type != VDIR) {
				FILE_UNUSE(fp, curlwp);
				error = EINVAL;
				break;
			}
			if ((path = malloc(MAXPATHLEN, M_TEMP, M_WAITOK))
			    == NULL) {
				FILE_UNUSE(fp, curlwp);
				error = ENOMEM;
				break;
			}
			bp = path + MAXPATHLEN;
			*--bp = '\0';
			vp = curproc->p_cwdi->cwdi_rdir;	/* XXXSMP */
			if (vp == NULL)
				vp = rootvnode;
			error = getcwd_common(vxp, vp, &bp, path,
			    MAXPATHLEN / 2, 0, curlwp);
			FILE_UNUSE(fp, curlwp);
			if (error)
				break;
			len = strlen(bp);
			break;

		case DTYPE_MISC:
			len = snprintf(bf, sizeof(bf), "%s", "[misc]");
			break;

		case DTYPE_KQUEUE:
			len = snprintf(bf, sizeof(bf), "%s", "[kqueue]");
			break;

		default:
			error = EINVAL;
			break;
		}	
		procfs_proc_unlock(pown);
	}

	if (error == 0)
		error = uiomove(bp, len, ap->a_uio);
	if (path)
		free(path, M_TEMP);
	return error;
}

/*
 * convert decimal ascii to int
 */
static int
atoi(b, len)
	const char *b;
	size_t len;
{
	int p = 0;

	while (len--) {
		char c = *b++;
		if (c < '0' || c > '9')
			return -1;
		p = 10 * p + (c - '0');
	}

	return p;
}
