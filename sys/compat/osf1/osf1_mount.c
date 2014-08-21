/*	$NetBSD: osf1_mount.c,v 1.51 2014/08/21 06:40:35 maxv Exp $	*/

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

/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: osf1_mount.c,v 1.51 2014/08/21 06:40:35 maxv Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/syscallargs.h>

#include <compat/osf1/osf1.h>
#include <compat/osf1/osf1_syscallargs.h>
#include <compat/common/compat_util.h>
#include <compat/osf1/osf1_cvt.h>

#include <net/if.h>
#include <netinet/in.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsmount.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/ufsmount.h>

#include <machine/vmparam.h>

#define	OSF1_MNT_WAIT		0x1
#define	OSF1_MNT_NOWAIT		0x2

#define	OSF1_MNT_FORCE		0x1
#define	OSF1_MNT_NOFORCE	0x2

/* acceptable flags for various calls */
#define	OSF1_GETFSSTAT_FLAGS	(OSF1_MNT_WAIT|OSF1_MNT_NOWAIT)
#define	OSF1_MOUNT_FLAGS	0xffffffff			/* XXX */
#define	OSF1_UNMOUNT_FLAGS	(OSF1_MNT_FORCE|OSF1_MNT_NOFORCE)


static int	osf1_mount_mfs(struct lwp *, const struct osf1_sys_mount_args *);
static int	osf1_mount_nfs(struct lwp *, const struct osf1_sys_mount_args *);

int
osf1_sys_fstatfs(struct lwp *l, const struct osf1_sys_fstatfs_args *uap, register_t *retval)
{
	file_t *fp;
	struct mount *mp;
	struct statvfs *sp;
	struct osf1_statfs osfs;
	int error;

	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(SCARG(uap, fd), &fp)))
		return (error);
	mp = ((struct vnode *)fp->f_data)->v_mount;
	sp = &mp->mnt_stat;
	if ((error = VFS_STATVFS(mp, sp)))
		goto out;
	sp->f_flag = mp->mnt_flag & MNT_VISFLAGMASK;
	osf1_cvt_statfs_from_native(sp, &osfs);
	error = copyout(&osfs, SCARG(uap, buf), min(sizeof osfs,
	    SCARG(uap, len)));
 out:
 	fd_putfile(SCARG(uap, fd));
	return (error);
}

int
osf1_sys_getfsstat(struct lwp *l, const struct osf1_sys_getfsstat_args *uap, register_t *retval)
{
	struct mount *mp, *nmp;
	struct statvfs *sp;
	struct osf1_statfs osfs;
	char *osf_sfsp;
	long count, maxcount, error;

	if (SCARG(uap, flags) & ~OSF1_GETFSSTAT_FLAGS)
		return (EINVAL);

	maxcount = SCARG(uap, bufsize) / sizeof(struct osf1_statfs);
	osf_sfsp = (void *)SCARG(uap, buf);
	mutex_enter(&mountlist_lock);
	for (count = 0, mp = TAILQ_FIRST(&mountlist);
	    mp != NULL;
	    mp = nmp) {
		if (vfs_busy(mp, &nmp)) {
			continue;
		}
		if (osf_sfsp && count < maxcount) {
			sp = &mp->mnt_stat;
			/*
			 * If OSF1_MNT_NOWAIT is specified, do not refresh the
			 * fsstat cache.  OSF1_MNT_WAIT overrides
			 * OSF1_MNT_NOWAIT.
			 */
			if (((SCARG(uap, flags) & OSF1_MNT_NOWAIT) == 0 ||
			    (SCARG(uap, flags) & OSF1_MNT_WAIT)) &&
			    (error = VFS_STATVFS(mp, sp)) == 0) {
				sp->f_flag = mp->mnt_flag & MNT_VISFLAGMASK;
				osf1_cvt_statfs_from_native(sp, &osfs);
				if ((error = copyout(&osfs, osf_sfsp,
				    sizeof (struct osf1_statfs)))) {
				    	vfs_unbusy(mp, false, NULL);
					return (error);
				}
				osf_sfsp += sizeof (struct osf1_statfs);
			}
		}
		count++;
		vfs_unbusy(mp, false, &nmp);
	}
	mutex_exit(&mountlist_lock);
	if (osf_sfsp && count > maxcount)
		*retval = maxcount;
	else
		*retval = count;
	return (0);
}

int
osf1_sys_mount(struct lwp *l, const struct osf1_sys_mount_args *uap, register_t *retval)
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
osf1_sys_statfs(struct lwp *l, const struct osf1_sys_statfs_args *uap, register_t *retval)
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
	return copyout(&osfs, SCARG(uap, buf), min(sizeof osfs,
	    SCARG(uap, len)));
}

int
osf1_sys_unmount(struct lwp *l, const struct osf1_sys_unmount_args *uap, register_t *retval)
{
	struct sys_unmount_args a;

	SCARG(&a, path) = SCARG(uap, path);

	if (SCARG(uap, flags) & ~OSF1_UNMOUNT_FLAGS)
		return (EINVAL);
	SCARG(&a, flags) = 0;
	if ((SCARG(uap, flags) & OSF1_MNT_FORCE) &&
	    (SCARG(uap, flags) & OSF1_MNT_NOFORCE) == 0)
		SCARG(&a, flags) |= MNT_FORCE;

	return sys_unmount(l, &a, retval);
}

static int
osf1_mount_mfs(struct lwp *l, const struct osf1_sys_mount_args *uap)
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

	return do_sys_mount(l, vfs_getopsbyname("mfs"), NULL, SCARG(uap, path),
	    SCARG(uap, flags), &bsd_ma, UIO_SYSSPACE, sizeof bsd_ma, &dummy);
}

static int
osf1_mount_nfs(struct lwp *l, const struct osf1_sys_mount_args *uap)
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

	return do_sys_mount(l, vfs_getopsbyname("nfs"), NULL, SCARG(uap, path),
	    SCARG(uap, flags), &bsd_na, UIO_SYSSPACE, sizeof bsd_na, &dummy);
}
