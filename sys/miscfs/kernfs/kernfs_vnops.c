/*
 * Copyright (c) 1990, 1992 Jan-Simon Pendry
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
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
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
 *	$Id: kernfs_vnops.c,v 1.15.2.3 1994/01/06 15:08:17 pk Exp $
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

#include <ufs/dir.h>		/* For readdir() XXX */

#include <miscfs/kernfs/kernfs.h>

struct kernfs_target kernfs_targets[] = {
/* NOTE: The name must be less than UIO_MX-16 chars in length */
DIR_TARGET(".",		0,		KTT_NULL,	KTM_DIR_PERMS	)
DIR_TARGET("..",	0,		KTT_NULL,	KTM_DIR_PERMS	)
REG_TARGET("copyright",	copyright,	KTT_STRING,	KTM_RO_PERMS	)
REG_TARGET("hostname",	0,		KTT_HOSTNAME,	KTM_RW_PERMS	)
REG_TARGET("hz",	&hz,		KTT_INT,	KTM_RO_PERMS	)
REG_TARGET("loadavg",	0,		KTT_AVENRUN,	KTM_RO_PERMS	)
REG_TARGET("physmem",	&physmem,	KTT_INT,	KTM_RO_PERMS	)
#ifdef KERNFS_HAVE_ROOTDIR	
DIR_TARGET("root",	0,		KTT_NULL,	KTM_DIR_PERMS	)
#endif
BLK_TARGET("rootdev",	0,		KTT_NULL,	KTM_RO_PERMS	)
CHR_TARGET("rrootdev",	0,		KTT_NULL,	KTM_RO_PERMS	)
REG_TARGET("time",	0,		KTT_TIME,	KTM_RO_PERMS	)
REG_TARGET("version",	version,	KTT_STRING,	KTM_RO_PERMS	)
};

static int nkernfs_targets = sizeof(kernfs_targets) / sizeof(kernfs_targets[0]);

static int
kernfs_xread(kt, buf, len, lenp)
	struct kernfs_target *kt;
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

		if (xlen + 2 > len)	/* extra space for null and newline */
			return (EINVAL);

		bcopy(cp, buf, xlen);	/* safer than sprintf */
		buf[xlen] = '\n';
		buf[xlen+1] = '\0';
		break;
	}

	case KTT_AVENRUN:
		sprintf(buf, "%d %d %d %d\n",
				averunnable.ldavg[0],
				averunnable.ldavg[1],
				averunnable.ldavg[2],
				FSCALE);
		break;

	default:
		return (EINVAL);
	}

	*lenp = strlen(buf);
	return (0);
}

static int
kernfs_xwrite(kt, buf, len)
	struct kernfs_target *kt;
	char *buf;
	int len;
{
	switch (kt->kt_tag) {
	case KTT_HOSTNAME: {
		if (buf[len-1] == '\n')
			--len;
		bcopy(buf, hostname, len);
		/* kernfs_write set buf[value_passed_as_len] = \0.
		 * therefore, buf len (hostnamelen) = len.
		 */
		hostnamelen = len;
		hostname[hostnamelen] = '\0';	/* null end of string. */
		return (0);
	}

	default:
		return (EIO);
	}
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

#ifdef KERNFS_HAVE_ROOTDIR	
	if (ndp->ni_namelen == 4 && bcmp(pname, "root", 4) == 0) {
		ndp->ni_dvp = dvp;
		ndp->ni_vp = rootdir;
		VREF(rootdir);
		VOP_LOCK(rootdir);
		return (0);
	}
#endif

	/*
	 * /kern/rootdev is the root device
	 */
	if (ndp->ni_namelen == 7 && bcmp(pname, "rootdev", 7) == 0) {
		if (!rootvp) {
			error = ENOENT;
			goto bad;
		}
		ndp->ni_dvp = dvp;
		ndp->ni_vp = rootvp;
		VREF(rootvp);
		VOP_LOCK(rootvp);
		return (0);
	}

	/*
	 * /kern/rrootdev is the raw root device
	 */
	if (ndp->ni_namelen == 8 && bcmp(pname, "rrootdev", 7) == 0) {
		if (!rrootdevvp) {
			error = ENOENT;
			goto bad;
		}
		ndp->ni_dvp = dvp;
		ndp->ni_vp = rrootdevvp;
		VREF(rrootdevvp);
		VOP_LOCK(rrootdevvp);
		return (0);
	}

	for (i = 0; i < nkernfs_targets; i++) {
		struct kernfs_target *kt = &kernfs_targets[i];
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
	VTOKERN(fvp)->kf_kt = &kernfs_targets[i];
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
	/* if access succeeded, this always does, too */

	return (0);
}

/*
 * Check mode permission on target pointer. Mode is READ, WRITE or EXEC.
 * The mode is shifted to select the owner/group/other fields. The
 * super user is granted all permissions.
 */
kernfs_access(vp, mode, cred, p)
	struct vnode *vp;
	register int mode;
	struct ucred *cred;
	struct proc *p;
{
	struct kernfs_target *kt = VTOKERN(vp)->kf_kt;
	register gid_t *gp;
	int i, error;

#ifdef KERN_DIAGNOSTIC
	if (!VOP_ISLOCKED(vp)) {
		vprint("kernfs_access: not locked", vp);
		panic("kernfs_access: not locked");
	}
#endif
	/*
	 * If you're the super-user, you always get access.
	 */
	if (cred->cr_uid == 0)
		return (0);
	/*
	 * Access check is based on only one of owner, group, public.
	 * If not owner, then check group. If not a member of the
	 * group, then check public access.
	 */
	if (cred->cr_uid != /* kt->kt_uid XXX */ 0) {
		mode >>= 3;
		gp = cred->cr_groups;
		for (i = 0; i < cred->cr_ngroups; i++, gp++)
			if (/* kt->kt_gid XXX */ 0 == *gp)
				goto found;
		mode >>= 3;
found:
		;
	}
	if ((kt->kt_perms & mode) == mode)
		return (0);
	return (EACCES);
}

kernfs_getattr(vp, vap, cred, p)
	struct vnode *vp;
	struct vattr *vap;
	struct ucred *cred;
	struct proc *p;
{
	int error = 0;
	char strbuf[KSTRING];
	struct kernfs_target *kt = VTOKERN(vp)->kf_kt;

	bzero((caddr_t) vap, sizeof(*vap));
	vattr_null(vap);
	vap->va_uid = kt->kt_uid;
	vap->va_gid = kt->kt_gid;
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
	vap->va_type = kt->kt_vtype;
	vap->va_mode = kt->kt_perms;

	if (vp->v_flag & VROOT) {
#ifdef KERNFS_DIAGNOSTIC
		printf("kernfs_getattr: stat rootdir\n");
#endif
		vap->va_nlink = 2;
		vap->va_fileid = 2;
		vap->va_size = DEV_BSIZE;
	} else {
#ifdef KERNFS_DIAGNOSTIC
		printf("kernfs_getattr: stat target %s\n", kt->kt_name);
#endif
		vap->va_nlink = 1;
		vap->va_fileid = 3 + (kt - kernfs_targets) / sizeof(*kt);
		error = kernfs_xread(kt, strbuf, sizeof(strbuf), &vap->va_size);
	}

	vp->v_type = vap->va_type;
#ifdef KERNFS_DIAGNOSTIC
	printf("kernfs_getattr: return error %d\n", error);
#endif
	return (error);
}


/*
 * Change the mode on a file.
 */
kernfs_chmod1(vp, mode, p)
	register struct vnode *vp;
	register int mode;
	struct proc *p;
{
	register struct ucred *cred = p->p_ucred;
	register struct kernfs_target *kt = VTOKERN(vp)->kf_kt;
	int error;

	if ((mode & kt->kt_maxperms) != mode)	/* can't set ro var to rw */
		return (EPERM);

	if (cred->cr_uid != kt->kt_uid &&
	    (error = suser(cred, &p->p_acflag)))
		return (error);
	if (cred->cr_uid) {
		if (vp->v_type != VDIR && (mode & S_ISVTX))
			return (EFTYPE);
		if (!groupmember(kt->kt_gid, cred) && (mode & S_ISGID))
			return (EPERM);
	}
	kt->kt_perms &= ~07777;
	kt->kt_perms |= mode & 07777;
/*	ip->i_flag |= ICHG;*/
	return (0);
}

/*
 * Perform chown operation on kernfs_target kt
 */
kernfs_chown1(vp, uid, gid, p)
	register struct vnode *vp;
	uid_t uid;
	gid_t gid;
	struct proc *p;
{
	register struct kernfs_target *kt = VTOKERN(vp)->kf_kt;
	register struct ucred *cred = p->p_ucred;
	uid_t ouid;
	gid_t ogid;
	int error = 0;

	if (uid == (u_short)VNOVAL)
		uid = kt->kt_uid;
	if (gid == (u_short)VNOVAL)
		gid = kt->kt_gid;
	/*
	 * If we don't own the file, are trying to change the owner
	 * of the file, or are not a member of the target group,
	 * the caller must be superuser or the call fails.
	 */
	if ((cred->cr_uid != kt->kt_uid || uid != kt->kt_uid ||
	    !groupmember((gid_t)gid, cred)) &&
	    (error = suser(cred, &p->p_acflag)))
		return (error);
	ouid = kt->kt_uid;
	ogid = kt->kt_gid;

	kt->kt_uid = uid;
	kt->kt_gid = gid;

/*	if (ouid != uid || ogid != gid)
		ip->i_flag |= ICHG;*/
	if (ouid != uid && cred->cr_uid != 0)
		kt->kt_perms &= ~S_ISUID;
	if (ogid != gid && cred->cr_uid != 0)
		kt->kt_perms &= ~S_ISGID;
	return (0);
}

/*
 * Set attribute vnode op. called from several syscalls
 */
kernfs_setattr(vp, vap, cred, p)
	struct vnode *vp;
	struct vattr *vap;
	struct ucred *cred;
	struct proc *p;
{
	int error = 0;

	/*
	 * Check for unsetable attributes.
	 */
	if ((vap->va_type != VNON) || (vap->va_nlink != VNOVAL) ||
	    (vap->va_fsid != VNOVAL) || (vap->va_fileid != VNOVAL) ||
	    (vap->va_blocksize != VNOVAL) || (vap->va_rdev != VNOVAL) ||
	    ((int)vap->va_bytes != VNOVAL) || (vap->va_gen != VNOVAL)) {
		return (EINVAL);
	}
	/*
	 * Go through the fields and update iff not VNOVAL.
	 */
	if (vap->va_uid != (u_short)VNOVAL || vap->va_gid != (u_short)VNOVAL)
		if (error = kernfs_chown1(vp, vap->va_uid, vap->va_gid, p))
			return (error);
	if (vap->va_size != VNOVAL) {
		if (vp->v_type == VDIR)
			return (EISDIR);
		/* else just nod and smile... */
	}
	if (vap->va_atime.tv_sec != VNOVAL || vap->va_mtime.tv_sec != VNOVAL) {
/*		if (cred->cr_uid != ip->i_uid &&
		    (error = suser(cred, &p->p_acflag)))
			return (error);
		if (vap->va_atime.tv_sec != VNOVAL)
			ip->i_flag |= IACC;
		if (vap->va_mtime.tv_sec != VNOVAL)
			ip->i_flag |= IUPD;
		ip->i_flag |= ICHG;
		if (error = iupdat(ip, &vap->va_atime, &vap->va_mtime, 1))
			return (error);
*/
	}
	if (vap->va_mode != (u_short)VNOVAL)
		error = kernfs_chmod1(vp, (int)vap->va_mode, p);
	if (vap->va_flags != VNOVAL) {
/*		if (cred->cr_uid != ip->i_uid &&
		    (error = suser(cred, &p->p_acflag)))
			return (error);
		if (cred->cr_uid == 0) {
			ip->i_flags = vap->va_flags;
		} else {
			ip->i_flags &= 0xffff0000;
			ip->i_flags |= (vap->va_flags & 0xffff);
		}
		ip->i_flag |= ICHG;
*/
	}
	return (error);
}

static int
kernfs_read(vp, uio, ioflag, cred)
	struct vnode *vp;
	struct uio *uio;
	int ioflag;
	struct ucred *cred;
{
	struct kernfs_target *kt = VTOKERN(vp)->kf_kt;
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
	struct kernfs_target *kt = VTOKERN(vp)->kf_kt;
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
	return (kernfs_xwrite(kt, strbuf, xlen));
}

kernfs_readdir(vp, uio, cred, eofflagp, cookies, ncookies)
	struct vnode *vp;
	struct uio *uio;
	struct ucred *cred;
	int *eofflagp;
	u_int *cookies;
	int ncookies;
{
	struct filedesc *fdp;
	int i;
	int error;

	i = uio->uio_offset / UIO_MX;
	error = 0;
	while (uio->uio_resid > 0 && (!cookies || ncookies > 0)) {
#ifdef KERNFS_DIAGNOSTIC
		printf("kernfs_readdir: i = %d\n", i);
#endif
		if (i >= nkernfs_targets) {
			*eofflagp = 1;
			break;
		}
		{
			struct direct d;
			struct direct *dp = &d;
			struct kernfs_target *kt = &kernfs_targets[i];

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
			if (cookies) {
				*cookies = (i + 1) * UIO_MX;
				ncookies--;
			}
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
void
kernfs_print(vp)
	struct vnode *vp;
{
	printf("tag VT_KERNFS, kernfs vnode\n");
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
