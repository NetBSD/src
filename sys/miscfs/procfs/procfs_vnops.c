/*
 * Copyright (c) 1993 The Regents of the University of California.
 * Copyright (c) 1993 Jan-Simon Pendry
 * All rights reserved.
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
 * From:
 *	Id: procfs_vnops.c,v 4.2 1994/01/02 15:28:44 jsp Exp
 *
 *	$Id: procfs_vnops.c,v 1.10 1994/01/05 08:00:09 cgd Exp $
 */

/*
 * procfs vnode interface
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/malloc.h>
#include <sys/resourcevar.h>
#include <miscfs/procfs/procfs.h>
#include <vm/vm.h>	/* for page_size */

/*
 * Vnode Operations.
 *
 */

/*
 * This is a list of the valid names in the
 * process-specific sub-directories.  It is
 * used in procfs_lookup and procfs_readdir
 */
static struct pfsnames {
	u_short	d_namlen;
	char	d_name[PROCFS_NAMELEN];
	pfstype	d_pfstype;
} procent[] = {
#define N(s) sizeof(s)-1, s
	/* namlen, nam, type */
	{  N("file"),   Pfile },
	{  N("mem"),    Pmem },
	{  N("regs"),   Pregs },
	{  N("ctl"),    Pctl },
	{  N("status"), Pstatus },
	{  N("note"),   Pnote },
	{  N("notepg"), Pnotepg },
#undef N
};
#define Nprocent (sizeof(procent)/sizeof(procent[0]))

static pid_t atopid __P((const char *, u_int));

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
procfs_open(vp, mode, cred, p)
	struct vnode *vp;
	int mode;
	struct ucred *cred;
	struct proc *p;
{
	struct pfsnode *pfs = VTOPFS(vp);

	switch (pfs->pfs_type) {
	case Pmem:
		if (PFIND(pfs->pfs_pid) == 0)
			return (ENOENT);	/* was ESRCH, jsp */

		if ((pfs->pfs_flags & FWRITE) && (mode & O_EXCL) ||
				(pfs->pfs_flags & O_EXCL) && (mode & FWRITE))
			return (EBUSY);


		if (mode & FWRITE)
			pfs->pfs_flags = (mode & (FWRITE|O_EXCL));

		return (0);

	default:
		break;
	}

	return (0);
}

/*
 * close the pfsnode (vp) after doing i/o.
 * (vp) is not locked on entry or exit.
 *
 * nothing to do for procfs other than undo
 * any exclusive open flag (see _open above).
 */
procfs_close(vp, flag, cred, p)
	struct vnode *vp;
	int flag;
	struct ucred *cred;
	struct proc *p;
{
	struct pfsnode *pfs = VTOPFS(vp);

	switch (pfs->pfs_type) {
	case Pmem:
		if ((flag & FWRITE) && (pfs->pfs_flags & O_EXCL))
			pfs->pfs_flags &= ~(FWRITE|O_EXCL);
		break;
	}

	return (0);
}

/*
 * do an ioctl operation on pfsnode (vp).
 * (vp) is not locked on entry or exit.
 */
procfs_ioctl(vp, com, data, fflag, cred, p)
	struct vnode *vp;
	int com;
	caddr_t data;
	int fflag;
	struct ucred *cred;
	struct proc *p;
{

	return (ENOTTY);
}

/*
 * do block mapping for pfsnode (vp).
 * since we don't use the buffer cache
 * for procfs this function should never
 * be called.  in any case, it's not clear
 * what part of the kernel ever makes use
 * of this function.  for sanity, this is the
 * usual no-op bmap, although returning
 * (EIO) would be a reasonable alternative.
 */
procfs_bmap(vp, bn, vpp, bnp)
	struct vnode *vp;
	daddr_t bn;
	struct vnode **vpp;
	daddr_t *bnp;
{

	if (vpp != NULL)
		*vpp = vp;
	if (bnp != NULL)
		*bnp = bn;
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
 * there and PFIND is not free.
 *
 * (vp) is not locked on entry or exit.
 */
procfs_inactive(vp, p)
	struct vnode *vp;
	struct proc *p;
{
	struct pfsnode *pfs = VTOPFS(vp);

	if (PFIND(pfs->pfs_pid) == 0)
		vgone(vp);

	return (0);
}

/*
 * _reclaim is called when getnewvnode()
 * wants to make use of an entry on the vnode
 * free list.  at this time the filesystem needs
 * to free any private data and remove the node
 * from any private lists.
 */
procfs_reclaim(vp)
	struct vnode *vp;
{
	int error;

	error = procfs_freevp(vp);
	return (error);
}

/*
 * _print is used for debugging.
 * just print a readable description
 * of (vp).
 */
procfs_print(vp)
	struct vnode *vp;
{
	struct pfsnode *pfs = VTOPFS(vp);

	printf("tag VT_PROCFS, pid %d, mode %x, flags %x\n",
		pfs->pfs_pid,
		pfs->pfs_mode, pfs->pfs_flags);
}

/*
 * _abortop is called when operations such as
 * rename and create fail.  this entry is responsible
 * for undoing any side-effects caused by the lookup.
 * this will always include freeing the pathname buffer.
 */
procfs_abortop(ndp)
	struct nameidata *ndp;
{

	if ((ndp->ni_nameiop & (HASBUF | SAVESTART)) == HASBUF)
		FREE(ndp->ni_pnbuf, M_NAMEI);
	return (0);
}

/*
 * generic entry point for unsupported operations
 */
procfs_badop()
{

	return (EIO);
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
procfs_getattr(vp, vap, cred, p)
	struct vnode *vp;
	struct vattr *vap;
	struct ucred *cred;
	struct proc *p;
{
	struct pfsnode *pfs = VTOPFS(vp);
	struct proc *procp;
	int error;

	/* first check the process still exists */
	procp = PFIND(pfs->pfs_pid);
	if (procp == 0)
		return (ENOENT);

	error = 0;

	/* start by zeroing out the attributes */
	VATTR_NULL(vap);

	/* next do all the common fields */
	vap->va_type = vp->v_type;
	vap->va_mode = pfs->pfs_mode;
	vap->va_fileid = pfs->pfs_fileno;
	vap->va_flags = 0;
	vap->va_blocksize = page_size;
	vap->va_bytes = vap->va_size = 0;

	/*
	 * Make all times be current TOD.
	 * It would be possible to get the process start
	 * time from the p_stat structure, but there's
	 * no "file creation" time stamp anyway, and the
	 * p_stat structure is not addressible if u. gets
	 * swapped out for that process.
	 */
	microtime(&vap->va_ctime);
	vap->va_atime = vap->va_mtime = vap->va_ctime;

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
	case Proot:
		vap->va_nlink = 2;
		vap->va_uid = 0;
		vap->va_gid = 0;
		break;

	case Pproc:
		vap->va_nlink = 2;
		vap->va_uid = procp->p_ucred->cr_uid;
		vap->va_gid = procp->p_ucred->cr_gid;
		break;

	case Pfile:
		error = EOPNOTSUPP;
		break;

	case Pmem:
		vap->va_nlink = 1;
		vap->va_bytes = vap->va_size =
			ctob(procp->p_vmspace->vm_tsize +
				    procp->p_vmspace->vm_dsize +
				    procp->p_vmspace->vm_ssize);
		vap->va_uid = procp->p_ucred->cr_uid;
		vap->va_gid = procp->p_ucred->cr_gid;
		break;

	case Pregs:
	case Pctl:
	case Pstatus:
	case Pnote:
	case Pnotepg:
		vap->va_nlink = 1;
		vap->va_uid = procp->p_ucred->cr_uid;
		vap->va_gid = procp->p_ucred->cr_gid;
		break;

	default:
		panic("procfs_getattr");
	}

	return (error);
}

procfs_setattr(vp, vap, cred, p)
	struct vnode *vp;
	struct vattr *vap;
	struct ucred *cred;
	struct proc *p;
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
 * something very similar to this code is duplicated
 * throughout the 4bsd kernel and should be moved
 * into kern/vfs_subr.c sometime.
 *
 * actually, the check for super-user is slightly
 * broken since it will allow read access to write-only
 * objects.  this doesn't cause any particular trouble
 * but does mean that the i/o entry points need to check
 * that the operation really does make sense.
 */
procfs_access(vp, mode, cred, p)
	struct vnode *vp;
	int mode;
	struct ucred *cred;
	struct proc *p;
{
	struct vattr *vap;
	struct vattr vattr;
	int error;

	/*
	 * If you're the super-user,
	 * you always get access.
	 */
	if (cred->cr_uid == (uid_t) 0)
		return (0);
	vap = &vattr;
	if (error = VOP_GETATTR(vp, vap, cred, p))
		return (error);

	/*
	 * Access check is based on only one of owner, group, public.
	 * If not owner, then check group. If not a member of the
	 * group, then check public access.
	 */
	if (cred->cr_uid != vap->va_uid) {
		gid_t *gp;
		int i;

		mode >>= 3;
		gp = cred->cr_groups;
		for (i = 0; i < cred->cr_ngroups; i++, gp++)
			if (vap->va_gid == *gp)
				goto found;
		mode >>= 3;
found:
		;
	}

	if ((vap->va_mode & mode) == mode)
		return (0);

	return (EACCES);
}

/*
 * lookup.  this is incredibly complicated in the
 * general case, however for most pseudo-filesystems
 * very little needs to be done.
 *
 * (dvp) is the directory in which the lookup takes place.
 * (ndp) contains all the information about the type of
 *       lookup being done.
 *
 * (dvp) is locked on entry.
 * the job of lookup is to set ndp->ni_dvp, and ndp->ni_vp.
 * (this changes in 4.4 where all we want is the equivalent
 * of ndp->ni_vp.)
 *
 * unless you want to get a migraine, just make sure your
 * filesystem doesn't do any locking of its own.  otherwise
 * read and inwardly digest ufs_lookup().
 */
procfs_lookup(dvp, ndp, p)
	struct vnode *dvp;
	struct nameidata *ndp;
	struct proc *p;
{
	char *pname = ndp->ni_ptr;
	int error = 0;
	int flag;
	pid_t pid;
	struct vnode *nvp;
	struct pfsnode *pfs;
	struct proc *procp;
	int mode;
	pfstype pfs_type;
	int i;

	if (ndp->ni_namelen == 1 && *pname == '.') {
		ndp->ni_vp = dvp;
		ndp->ni_dvp = dvp;
		VREF(dvp);
		return (0);
	}

	ndp->ni_dvp = dvp;
	ndp->ni_vp = NULL;

	pfs = VTOPFS(dvp);
	switch (pfs->pfs_type) {
	case Proot:
		if (ndp->ni_isdotdot)
			return (EIO);

		if (NDEQ(ndp, "curproc", 7))
			pid = p->p_pid;
		else
			pid = atopid(pname, ndp->ni_namelen);
		if (pid == NO_PID)
			return (ENOENT);

		procp = PFIND(pid);
		if (procp == 0)
			return (ENOENT);

		error = procfs_allocvp(dvp->v_mount, &nvp, pid, Pproc);
		if (error)
			return (error);

		nvp->v_type = VDIR;
		pfs = VTOPFS(nvp);

		ndp->ni_vp = nvp;
		return (0);

	case Pproc:
		if (ndp->ni_isdotdot) {
			ndp->ni_dvp = dvp;
			error = procfs_root(dvp->v_mount, &ndp->ni_vp);
			return (error);
		}

		procp = PFIND(pfs->pfs_pid);
		if (procp == 0)
			return (ENOENT);

		for (i = 0; i < Nprocent; i++) {
			struct pfsnames *dp = &procent[i];

			if (ndp->ni_namelen == dp->d_namlen &&
			    bcmp(pname, dp->d_name, dp->d_namlen) == 0) {
			    	pfs_type = dp->d_pfstype;
				goto found;
			}
		}
		return (ENOENT);

	found:
		if (pfs_type == Pfile) {
			nvp = procfs_findtextvp(procp);
			if (nvp) {
				VREF(nvp);
				VOP_LOCK(nvp);
			} else {
				error = ENXIO;
			}
		} else {
			error = procfs_allocvp(dvp->v_mount, &nvp,
					pfs->pfs_pid, pfs_type);
			if (error)
				return (error);

			nvp->v_type = VREG;
			pfs = VTOPFS(nvp);
		}
		ndp->ni_vp = nvp;
		return (error);

	default:
		return (ENOTDIR);
	}
}

/*
 * readdir returns directory entries from pfsnode (vp).
 *
 * the strategy here with procfs is to generate a single
 * directory entry at a time (struct pfsdent) and then
 * copy that out to userland using uiomove.  a more efficent
 * though more complex implementation, would try to minimize
 * the number of calls to uiomove().  for procfs, this is
 * hardly worth the added code complexity.
 *
 * this should just be done through read()
 */
procfs_readdir(vp, uio, cred, eofflagp, cookies, ncookies)
        struct vnode *vp;
        struct uio *uio;
        struct ucred *cred;
        int *eofflagp;
	u_int *cookies;
	int ncookies;
{
	struct pfsdent d;
	struct pfsdent *dp = &d;
	struct pfsnode *pfs;
	int error;
	int count;
	int i;

	pfs = VTOPFS(vp);

	if (uio->uio_resid < UIO_MX)
		return (EINVAL);
	if (uio->uio_offset & (UIO_MX-1))
		return (EINVAL);
	if (uio->uio_offset < 0)
		return (EINVAL);

	error = 0;
	count = 0;
	i = uio->uio_offset / UIO_MX;

	switch (pfs->pfs_type) {
	/*
	 * this is for the process-specific sub-directories.
	 * all that is needed to is copy out all the entries
	 * from the procent[] table (top of this file).
	 */
	case Pproc: {
		while (uio->uio_resid >= UIO_MX) {
			struct pfsnames *dt;

			if (i >= Nprocent) {
				*eofflagp = 1;
				break;
			}

			dt = &procent[i];
			dp->d_reclen = UIO_MX;
			dp->d_fileno = PROCFS_FILENO(pfs->pfs_pid, dt->d_pfstype);
			dp->d_namlen = dt->d_namlen;
			bcopy(dt->d_name, dp->d_name, sizeof(dt->d_name)-1);
			error = uiomove((caddr_t) dp, UIO_MX, uio);
			if (error)
				break;
			count += UIO_MX;
			i++;
		}

	    	break;

	    }

	/*
	 * this is for the root of the procfs filesystem
	 * what is needed is a special entry for "curproc"
	 * followed by an entry for each process on allproc
#ifdef PROCFS_ZOMBIE
	 * and zombproc.
#endif
	 */

	case Proot: {
		int pcnt;
#ifdef PROCFS_ZOMBIE
		int doingzomb = 0;
#endif
		struct proc *p;

		p = (struct proc *) allproc;

#define PROCFS_XFILES	1	/* number of other entries, like "curproc" */
		pcnt = PROCFS_XFILES;

		while (p && uio->uio_resid >= UIO_MX) {
			bzero((char *) dp, UIO_MX);
			dp->d_reclen = UIO_MX;

			switch (i) {
			case 0:
				/* ship out entry for "curproc" */
				dp->d_fileno = PROCFS_FILENO(PID_MAX+1, Pproc);
				dp->d_namlen = 7;
				bcopy("curproc", dp->d_name, dp->d_namlen+1);
				break;

			default:
				if (pcnt >= i) {
					dp->d_fileno = PROCFS_FILENO(p->p_pid, Pproc);
					dp->d_namlen = sprintf(dp->d_name, "%ld", (long) p->p_pid);
				}

				p = p->p_nxt;

#ifdef PROCFS_ZOMBIE
				if (p == 0 && doingzomb == 0) {
					doingzomb = 1;
					p = zombproc;
				}
#endif

				if (pcnt++ < i)
					continue;

				break;
			}
			error = uiomove((caddr_t) dp, UIO_MX, uio);
			if (error)
				break;
			count += UIO_MX;
			i++;
		}

		break;

	    }

	default:
		error = ENOTDIR;
		break;
	}

	uio->uio_offset = i * UIO_MX;
	if (count == 0)
		*eofflagp = 1;

	return (error);
}

/*
 * convert decimal ascii to pid_t
 */
static pid_t
atopid(b, len)
	const char *b;
	u_int len;
{
	pid_t p = 0;

	while (len--) {
		char c = *b++;
		if (c < '0' || c > '9')
			return (NO_PID);
		p = 10 * p + (c - '0');
		if (p > PID_MAX)
			return (NO_PID);
	}

	return (p);
}

/*
 * procfs vnode operations.
 */
struct vnodeops procfs_vnodeops = {
	procfs_lookup,		/* lookup */
	procfs_create,		/* create */
	procfs_mknod,		/* mknod */
	procfs_open,		/* open */
	procfs_close,		/* close */
	procfs_access,		/* access */
	procfs_getattr,		/* getattr */
	procfs_setattr,		/* setattr */
	procfs_read,		/* read */
	procfs_write,		/* write */
	procfs_ioctl,		/* ioctl */
	procfs_select,		/* select */
	procfs_mmap,		/* mmap */
	procfs_fsync,		/* fsync */
	procfs_seek,		/* seek */
	procfs_remove,		/* remove */
	procfs_link,		/* link */
	procfs_rename,		/* rename */
	procfs_mkdir,		/* mkdir */
	procfs_rmdir,		/* rmdir */
	procfs_symlink,		/* symlink */
	procfs_readdir,		/* readdir */
	procfs_readlink,	/* readlink */
	procfs_abortop,		/* abortop */
	procfs_inactive,	/* inactive */
	procfs_reclaim,		/* reclaim */
	procfs_lock,		/* lock */
	procfs_unlock,		/* unlock */
	procfs_bmap,		/* bmap */
	procfs_strategy,	/* strategy */
	procfs_print,		/* print */
	procfs_islocked,	/* islocked */
	procfs_advlock,		/* advlock */
};
