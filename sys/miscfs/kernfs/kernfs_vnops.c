/*
 * Copyright (c) 1992 The Regents of the University of California
 * Copyright (c) 1990, 1992 Jan-Simon Pendry
 * All rights reserved.
 *
 * This code is derived from software donated to Berkeley by
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
 *	Id: kernfs_vnops.c,v 4.1 1994/01/02 14:41:30 jsp Exp
 *
 *	$Id: kernfs_vnops.c,v 1.17 1994/01/05 11:05:12 cgd Exp $
 */

/*
 * Kernel parameter filesystem
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/buf.h>
#include <miscfs/kernfs/kernfs.h>

#include <ufs/dir.h>		/* For readdir() XXX */

#define KSTRING	256		/* Largest I/O available via this filesystem */
#define	UIO_MX 32

struct kern_target {
	char *kt_name;
	void *kt_data;
#define	KTT_NULL 1
#define	KTT_TIME 5
#define KTT_INT	17
#define	KTT_STRING 31
#define KTT_HOSTNAME 47
#define KTT_AVENRUN 53
	int kt_tag;
#define	KTM_RO	0
#define	KTM_RO_MODE \
		((VREAD) | (VREAD >> 3) | (VREAD >> 6))
#define	KTM_RW	43
#define	KTM_RW_MODE \
		((VWRITE) | KTM_RO_MODE)
#define KTM_DIR_MODE \
		((VREAD|VEXEC) | ((VREAD|VEXEC) >> 3) | ((VREAD|VEXEC) >> 6))
	int kt_rw;
	enum vtype kt_vtype;
} kern_targets[] = {
/* NOTE: The name must be less than UIO_MX-16 chars in length */
	/* name		data		tag		ro/rw */
	{ ".",		0,		KTT_NULL,	KTM_RO,	VDIR },
	{ "copyright",	copyright,	KTT_STRING,	KTM_RO,	VREG },
	{ "hostname",	0,		KTT_HOSTNAME,	KTM_RW,	VREG },
	{ "hz",		&hz,		KTT_INT,	KTM_RO,	VREG },
	{ "loadavg",	0,		KTT_AVENRUN,	KTM_RO,	VREG },
	{ "physmem",	&physmem,	KTT_INT,	KTM_RO,	VREG },
#if 0
	{ "root",	0,		KTT_NULL,	KTM_RO,	VDIR },
#endif
	{ "rootdev",	0,		KTT_NULL,	KTM_RO,	VBLK },
#ifdef notdef
	{ "rrootdev",	0,		KTT_NULL,	KTM_RO,	VCHR },
#endif
	{ "time",	0,		KTT_TIME,	KTM_RO,	VREG },
	{ "version",	version,	KTT_STRING,	KTM_RO,	VREG },
};

static int nkern_targets = sizeof(kern_targets) / sizeof(kern_targets[0]);

static int
kernfs_xread(kt, buf, len, lenp)
	struct kern_target *kt;
	char *buf;
	int len;
	int *lenp;
{
	int xlen;

	switch (kt->kt_tag) {
	case KTT_TIME: {
		struct timeval tv;
		microtime(&tv);
		sprintf(buf, "%d %d\n", tv.tv_sec, tv.tv_usec);
		break;
	}

	case KTT_INT: {
		int *ip = kt->kt_data;
		sprintf(buf, "%d\n", *ip);
		break;
	}

	case KTT_STRING: {
		char *cp = kt->kt_data;
		int xlen = strlen(cp) + 1;

		if (xlen >= len)
			return (EINVAL);

		bcopy(cp, buf, xlen);
		break;
	}

	case KTT_HOSTNAME: {
		char *cp = hostname;
		int xlen = hostnamelen;

		if (xlen >= (len-2))
			return (EINVAL);

		bcopy(cp, buf, xlen);
		buf[xlen] = '\n';
		buf[xlen+1] = '\0';
		break;
	}

	case KTT_AVENRUN:
		sprintf(buf, "%d %d %d %d\n",
		    averunnable.ldavg[0],
		    averunnable.ldavg[1],
		    averunnable.ldavg[2],
		    averunnable.fscale);
		break;

	default:
		return (EINVAL);
	}

	*lenp = strlen(buf);
	return (0);
}

static int
kernfs_xwrite(kt, buf, len)
	struct kern_target *kt;
	char *buf;
	int len;
{
	switch (kt->kt_tag) {
	case KTT_HOSTNAME: {
		if (buf[len-1] == '\n')
			--len;
		bcopy(buf, hostname, len);
		hostname[len] = '\0';
		hostnamelen = len;
		return (0);
	}

	default:
		return (EIO);
	}
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
kernfs_access(vp, mode, cred, p)
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
 * vp is the current namei directory
 * ndp is the name to locate in that directory...
 */
kernfs_lookup(dvp, ndp, p)
	struct vnode *dvp;
	struct nameidata *ndp;
	struct proc *p;
{
	char *pname = ndp->ni_ptr;
	int error = ENOENT;
	int i;
	struct vnode *fvp;

#ifdef KERNFS_DIAGNOSTIC
	printf("kernfs_lookup(%s)\n", pname);
#endif
	if (ndp->ni_namelen == 1 && *pname == '.') {
		ndp->ni_dvp = dvp;
		ndp->ni_vp = dvp;
		VREF(dvp);
		/*VOP_LOCK(dvp);*/
		return (0);
	}

#if 0
	if (ndp->ni_namelen == 4 && bcmp(pname, "root", 4) == 0) {
		ndp->ni_dvp = rootdir;
		ndp->ni_vp = rootdir;
		VREF(rootdir);
		VREF(rootdir);
		VOP_LOCK(rootdir);
		return (0);
	}
#endif
	
	/*
	 * /kern/rootdev is the root device
	 */
	if (ndp->ni_namelen == 7 && bcmp(pname, "rootdev", 7) == 0) {
		if (vfinddev(rootdev, VBLK, &fvp))
			return (ENXIO);
		ndp->ni_dvp = dvp;
		ndp->ni_vp = fvp;
		VREF(fvp);
		VOP_LOCK(fvp);
		return (0);
	}

	for (i = 0; i < nkern_targets; i++) {
		struct kern_target *kt = &kern_targets[i];
		if (ndp->ni_namelen == strlen(kt->kt_name) &&
		    bcmp(kt->kt_name, pname, ndp->ni_namelen) == 0) {
			error = 0;
			break;
		}
	}

#ifdef KERNFS_DIAGNOSTIC
	printf("kernfs_lookup: i = %d, error = %d\n", i, error);
#endif

	if (error)
		goto bad;

#ifdef KERNFS_DIAGNOSTIC
	printf("kernfs_lookup: allocate new vnode\n");
#endif
	error = getnewvnode(VT_KERNFS, dvp->v_mount, &kernfs_vnodeops, &fvp);
	if (error)
		goto bad;
	VTOKERN(fvp)->kf_kt = &kern_targets[i];
	fvp->v_type = VTOKERN(fvp)->kf_kt->kt_vtype;
	ndp->ni_dvp = dvp;
	ndp->ni_vp = fvp;
#ifdef KERNFS_DIAGNOSTIC
	printf("kernfs_lookup: newvp = %x\n", fvp);
#endif
	return (0);

bad:;
	ndp->ni_dvp = dvp;
	ndp->ni_vp = NULL;
#ifdef KERNFS_DIAGNOSTIC
	printf("kernfs_lookup: error = %d\n", error);
#endif
	return (error);
}

kernfs_open(vp, mode, cred, p)
	struct vnode *vp;
	int mode;
	struct ucred *cred;
	struct proc *p;
{
	int error;
	struct filedesc *fdp;
	struct file *fp;
	int dfd;
	int fd;

#ifdef KERNFS_DIAGNOSTIC
	printf("kernfs_open\n");
#endif

	/*
	 * Can always open the root (modulo perms)
	 */
	if (vp->v_flag & VROOT)
		return (0);

#ifdef KERNFS_DIAGNOSTIC
	printf("kernfs_open, mode = %x, file = %s\n",
			mode, VTOKERN(vp)->kf_kt->kt_name);
#endif

	if ((mode & FWRITE) && VTOKERN(vp)->kf_kt->kt_rw != KTM_RW)
		return (EACCES);

	return (0);
}

kernfs_getattr(vp, vap, cred, p)
	struct vnode *vp;
	struct vattr *vap;
	struct ucred *cred;
	struct proc *p;
{
	int error = 0;
	char strbuf[KSTRING];
	struct kern_target *kt = VTOKERN(vp)->kf_kt;

	bzero((caddr_t) vap, sizeof(*vap));
	vattr_null(vap);
	vap->va_uid = 0;
	vap->va_gid = 0;
	vap->va_fsid = vp->v_mount->mnt_stat.f_fsid.val[0];
	/* vap->va_qsize = 0; */
	vap->va_blocksize = DEV_BSIZE;
	microtime(&vap->va_atime);
	vap->va_mtime = vap->va_atime;
	vap->va_ctime = vap->va_ctime;
	vap->va_gen = 0;
	vap->va_flags = 0;
	vap->va_rdev = 0;
	/* vap->va_qbytes = 0; */
	vap->va_bytes = 0;

	if (vp->v_flag & VROOT) {
#ifdef KERNFS_DIAGNOSTIC
		printf("kernfs_getattr: stat rootdir\n");
#endif
		vap->va_type = VDIR;
		vap->va_mode = KTM_DIR_MODE;
		vap->va_nlink = 2;
		vap->va_fileid = 2;
		vap->va_size = DEV_BSIZE;
	} else {
#ifdef KERNFS_DIAGNOSTIC
		printf("kernfs_getattr: stat target %s\n", kt->kt_name);
#endif
		vap->va_type = kt->kt_vtype;
		vap->va_mode = (kt->kt_rw ? KTM_RW_MODE : KTM_RO_MODE);
		vap->va_nlink = 1;
		vap->va_fileid = 3 + (kt - kern_targets) / sizeof(*kt);
		error = kernfs_xread(kt, strbuf, sizeof(strbuf), &vap->va_size);
	}

	vp->v_type = vap->va_type;
#ifdef KERNFS_DIAGNOSTIC
	printf("kernfs_getattr: return error %d\n", error);
#endif
	return (error);
}

kernfs_setattr(vp, vap, cred, p)
	struct vnode *vp;
	struct vattr *vap;
	struct ucred *cred;
	struct proc *p;
{

	/*
	 * Silently ignore attribute changes.
	 * This allows for open with truncate to have no
	 * effect until some data is written.  I want to
	 * do it this way because all writes are atomic.
	 */
	return (0);
}

static int
kernfs_read(vp, uio, ioflag, cred)
	struct vnode *vp;
	struct uio *uio;
	int ioflag;
	struct ucred *cred;
{
	struct kern_target *kt = VTOKERN(vp)->kf_kt;
	char strbuf[KSTRING];
	int off = uio->uio_offset;
	int len = 0;
	char *cp = strbuf;
	int error;
#ifdef KERNFS_DIAGNOSTIC
	printf("kern_read %s\n", kt->kt_name);
#endif

	error = kernfs_xread(kt, strbuf, sizeof(strbuf), &len);
	if (error)
		return (error);
	cp = strbuf + off;
	len -= off;
	return (uiomove(cp, len, uio));
}

static int
kernfs_write(vp, uio, ioflag, cred)
	struct vnode *vp;
	struct uio *uio;
	int ioflag;
	struct ucred *cred;
{
	struct kern_target *kt = VTOKERN(vp)->kf_kt;
	char strbuf[KSTRING];
	int len = uio->uio_resid;
	char *cp = strbuf;
	int xlen;
	int error;

	if (uio->uio_offset != 0)
		return (EINVAL);

	xlen = min(uio->uio_resid, KSTRING-1);
	error = uiomove(strbuf, xlen, uio);
	if (error)
		return (error);

	if (uio->uio_resid != 0)
		return (EIO);

	strbuf[xlen] = '\0';
	xlen = strlen(strbuf);
	return (kernfs_xwrite(kt, strbuf, xlen));
}

kernfs_readdir(vp, uio, cred, eofflagp)
	struct vnode *vp;
	struct uio *uio;
	struct ucred *cred;
	int *eofflagp;
{
	struct filedesc *fdp;
	int i;
	int error;

	i = uio->uio_offset / UIO_MX;
	error = 0;
	while (uio->uio_resid > 0) {
#ifdef KERNFS_DIAGNOSTIC
		printf("kernfs_readdir: i = %d\n", i);
#endif
		if (i >= nkern_targets) {
			*eofflagp = 1;
			break;
		}
		{
			struct direct d;
			struct direct *dp = &d;
			struct kern_target *kt = &kern_targets[i];

			bzero((caddr_t) dp, UIO_MX);

			dp->d_namlen = strlen(kt->kt_name);
			bcopy(kt->kt_name, dp->d_name, dp->d_namlen+1);

#ifdef KERNFS_DIAGNOSTIC
			printf("kernfs_readdir: name = %s, len = %d\n",
					dp->d_name, dp->d_namlen);
#endif
			/*
			 * Fill in the remaining fields
			 */
			dp->d_reclen = UIO_MX;
			dp->d_ino = i + 3;
			/*
			 * And ship to userland
			 */
			error = uiomove((caddr_t) dp, UIO_MX, uio);
			if (error)
				break;
		}
		i++;
	}

	uio->uio_offset = i * UIO_MX;

	return (error);
}

kernfs_inactive(vp, p)
	struct vnode *vp;
	struct proc *p;
{
	/*
	 * Clear out the v_type field to avoid
	 * nasty things happening in vgone().
	 */
	vp->v_type = VNON;
#ifdef KERNFS_DIAGNOSTIC
	printf("kernfs_inactive(%x)\n", vp);
#endif
	return (0);
}

/*
 * Print out the contents of a kernfs vnode.
 */
/* ARGSUSED */
kernfs_print(vp)
	struct vnode *vp;
{
	printf("tag VT_NON, kernfs vnode\n");
}

/*
 * kernfs vnode unsupported operation
 */
kernfs_enotsupp()
{
	return (EOPNOTSUPP);
}

/*
 * kernfs "should never get here" operation
 */
kernfs_badop()
{
	panic("kernfs: bad op");
	/* NOTREACHED */
}

/*
 * kernfs vnode null operation
 */
kernfs_nullop()
{
	return (0);
}

#define kernfs_create ((int (*) __P(( \
		struct nameidata *ndp, \
		struct vattr *vap, \
		struct proc *p))) kernfs_enotsupp)
#define kernfs_mknod ((int (*) __P(( \
		struct nameidata *ndp, \
		struct vattr *vap, \
		struct ucred *cred, \
		struct proc *p))) kernfs_enotsupp)
#define kernfs_close ((int (*) __P(( \
		struct vnode *vp, \
		int fflag, \
		struct ucred *cred, \
		struct proc *p))) nullop)
#define	kernfs_ioctl ((int (*) __P(( \
		struct vnode *vp, \
		int command, \
		caddr_t data, \
		int fflag, \
		struct ucred *cred, \
		struct proc *p))) kernfs_enotsupp)
#define	kernfs_select ((int (*) __P(( \
		struct vnode *vp, \
		int which, \
		int fflags, \
		struct ucred *cred, \
		struct proc *p))) kernfs_enotsupp)
#define kernfs_mmap ((int (*) __P(( \
		struct vnode *vp, \
		int fflags, \
		struct ucred *cred, \
		struct proc *p))) kernfs_enotsupp)
#define kernfs_fsync ((int (*) __P(( \
		struct vnode *vp, \
		int fflags, \
		struct ucred *cred, \
		int waitfor, \
		struct proc *p))) nullop)
#define kernfs_seek ((int (*) __P(( \
		struct vnode *vp, \
		off_t oldoff, \
		off_t newoff, \
		struct ucred *cred))) nullop)
#define kernfs_remove ((int (*) __P(( \
		struct nameidata *ndp, \
		struct proc *p))) kernfs_enotsupp)
#define kernfs_link ((int (*) __P(( \
		struct vnode *vp, \
		struct nameidata *ndp, \
		struct proc *p))) kernfs_enotsupp)
#define kernfs_rename ((int (*) __P(( \
		struct nameidata *fndp, \
		struct nameidata *tdnp, \
		struct proc *p))) kernfs_enotsupp)
#define kernfs_mkdir ((int (*) __P(( \
		struct nameidata *ndp, \
		struct vattr *vap, \
		struct proc *p))) kernfs_enotsupp)
#define kernfs_rmdir ((int (*) __P(( \
		struct nameidata *ndp, \
		struct proc *p))) kernfs_enotsupp)
#define kernfs_symlink ((int (*) __P(( \
		struct nameidata *ndp, \
		struct vattr *vap, \
		char *target, \
		struct proc *p))) kernfs_enotsupp)
#define kernfs_readlink ((int (*) __P(( \
		struct vnode *vp, \
		struct uio *uio, \
		struct ucred *cred))) kernfs_enotsupp)
#define kernfs_abortop ((int (*) __P(( \
		struct nameidata *ndp))) nullop)
#ifdef KERNFS_DIAGNOSTIC
int kernfs_reclaim(vp)
struct vnode *vp;
{
	printf("kernfs_reclaim(%x)\n", vp);
	return (0);
}
#else
#define kernfs_reclaim ((int (*) __P(( \
		struct vnode *vp))) nullop)
#endif
#define	kernfs_lock ((int (*) __P(( \
		struct vnode *vp))) nullop)
#define kernfs_unlock ((int (*) __P(( \
		struct vnode *vp))) nullop)
#define	kernfs_bmap ((int (*) __P(( \
		struct vnode *vp, \
		daddr_t bn, \
		struct vnode **vpp, \
		daddr_t *bnp))) kernfs_badop)
#define	kernfs_strategy ((int (*) __P(( \
		struct buf *bp))) kernfs_badop)
#define kernfs_islocked ((int (*) __P(( \
		struct vnode *vp))) nullop)
#define kernfs_advlock ((int (*) __P(( \
		struct vnode *vp, \
		caddr_t id, \
		int op, \
		struct flock *fl, \
		int flags))) kernfs_enotsupp)

struct vnodeops kernfs_vnodeops = {
	kernfs_lookup,	/* lookup */
	kernfs_create,	/* create */
	kernfs_mknod,	/* mknod */
	kernfs_open,	/* open */
	kernfs_close,	/* close */
	kernfs_access,	/* access */
	kernfs_getattr,	/* getattr */
	kernfs_setattr,	/* setattr */
	kernfs_read,	/* read */
	kernfs_write,	/* write */
	kernfs_ioctl,	/* ioctl */
	kernfs_select,	/* select */
	kernfs_mmap,	/* mmap */
	kernfs_fsync,	/* fsync */
	kernfs_seek,	/* seek */
	kernfs_remove,	/* remove */
	kernfs_link,	/* link */
	kernfs_rename,	/* rename */
	kernfs_mkdir,	/* mkdir */
	kernfs_rmdir,	/* rmdir */
	kernfs_symlink,	/* symlink */
	kernfs_readdir,	/* readdir */
	kernfs_readlink,	/* readlink */
	kernfs_abortop,	/* abortop */
	kernfs_inactive,	/* inactive */
	kernfs_reclaim,	/* reclaim */
	kernfs_lock,	/* lock */
	kernfs_unlock,	/* unlock */
	kernfs_bmap,	/* bmap */
	kernfs_strategy,	/* strategy */
	kernfs_print,	/* print */
	kernfs_islocked,	/* islocked */
	kernfs_advlock,	/* advlock */
};
