/*	$NetBSD: darwin_mount.c,v 1.11 2007/04/22 08:29:55 dsl Exp $ */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: darwin_mount.c,v 1.11 2007/04/22 08:29:55 dsl Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/syscallargs.h>

#include <compat/sys/signal.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_vm.h>

#include <compat/darwin/darwin_audit.h>
#include <compat/darwin/darwin_mount.h>
#include <compat/darwin/darwin_syscallargs.h>

static void native_to_darwin_statvfs(const struct statvfs *,
    struct darwin_statfs *);

int
darwin_sys_fstatfs(struct lwp *l, void *v, register_t *retval)
{
	struct darwin_sys_fstatfs_args /* {
		syscallarg(int) fd;
		syscallarg(struct darwin_statfs *) buf;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct file *fp;
	struct mount *mp;
	struct statvfs *bs;
	struct darwin_statfs ds;
	int error;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)))
		return (error);

	mp = ((struct vnode *)fp->f_data)->v_mount;
	bs = &mp->mnt_stat;

	if ((error = VFS_STATVFS(mp, bs, l)) != 0)
		goto out;

	native_to_darwin_statvfs(bs, &ds);

	error = copyout(&ds, SCARG(uap, buf), sizeof(ds));

out:
	FILE_UNUSE(fp, l);
	return (error);
}

int
darwin_sys_getfsstat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct darwin_sys_getfsstat_args /* {
		syscallarg(struct darwin_statfs *) buf;
		syscallarg(long) bufsize;
		syscallarg(int) flags;
	} */ *uap = v;
	struct mount *mp, *nmp;
	struct statvfs *bs;
	struct darwin_statfs ds;
	struct darwin_statfs *uds;
	long count, maxcount, error;

	maxcount = SCARG(uap, bufsize) / sizeof(struct darwin_statfs);
	uds = SCARG(uap, buf);

	for (count = 0, mp = mountlist.cqh_first;
	    mp != (void *)&mountlist; mp = nmp) {
		nmp = mp->mnt_list.cqe_next;

		if ((uds != NULL) && (count < maxcount)) {
			bs = &mp->mnt_stat;

			if (((SCARG(uap, flags) & MNT_NOWAIT) == 0 ||
			    (SCARG(uap, flags) & MNT_WAIT)) &&
			    (error = VFS_STATVFS(mp, bs, l)))
				continue;

			native_to_darwin_statvfs(bs, &ds);

			if ((error = copyout(&ds, uds, sizeof(*uds))) != 0)
				return error;
			uds++;
		}
		count++;
	}

	if ((uds != NULL) && (count > maxcount))
		*retval = maxcount;
	else
		*retval = count;

	return 0;
}

int
darwin_sys_statfs(struct lwp *l, void *v, register_t *retval)
{
	struct darwin_sys_statfs_args /* {
		syscallarg(char *) path;
		syscallarg(struct statfs *) buf;
	} */ *uap = v;
	struct mount *mp;
	struct statvfs *bs;
	struct darwin_statfs ds;
	struct nameidata nd;
	int error;

	NDINIT(&nd, LOOKUP, FOLLOW | TRYEMULROOT, UIO_USERSPACE, SCARG(uap, path), l);
	if ((error = namei(&nd)) != 0)
		return error;

	mp = nd.ni_vp->v_mount;
	bs = &mp->mnt_stat;
	vrele(nd.ni_vp);

	if ((error = VFS_STATVFS(mp, bs, l)) != 0)
		return error;

	native_to_darwin_statvfs(bs, &ds);

	error = copyout(&ds, SCARG(uap, buf), sizeof(ds));

	return error;
}


static void
native_to_darwin_statvfs(bs, ds)
	const struct statvfs *bs;
	struct darwin_statfs *ds;
{
	long dflags = 0;
	long sflags = bs->f_flag & MNT_VISFLAGMASK;

	if (sflags|MNT_RDONLY)
		dflags |= DARWIN_MNT_RDONLY;
	if (sflags|MNT_SYNCHRONOUS)
		dflags |= DARWIN_MNT_SYNCHRONOUS;
	if (sflags|MNT_NOEXEC)
		dflags |= DARWIN_MNT_NOEXEC;
	if (sflags|MNT_NOSUID)
		dflags |= DARWIN_MNT_NOSUID;
	if (sflags|MNT_NODEV)
		dflags |= DARWIN_MNT_NODEV;
	if (sflags|MNT_UNION)
		dflags |= DARWIN_MNT_UNION;
	if (sflags|MNT_ASYNC)
		dflags |= DARWIN_MNT_ASYNC;
	if (sflags|MNT_IGNORE)
		dflags |= DARWIN_MNT_DONTBROWSE;

	ds->f_otype = 0;
	ds->f_oflags = dflags & 0xffff;
	ds->f_bsize = bs->f_bsize;
	ds->f_iosize = bs->f_iosize;
	ds->f_blocks = bs->f_blocks;
	ds->f_bfree = bs->f_bfree;
	ds->f_bavail = bs->f_bavail;
	ds->f_files = bs->f_files;
	ds->f_ffree = bs->f_ffree;
	(void)memcpy(&ds->f_fsid, &bs->f_fsidx, sizeof(ds->f_fsid));
	ds->f_owner = bs->f_owner;
	ds->f_reserved1 = 0;
	ds->f_type = 0;
	ds->f_flags = dflags;
	(void)strlcpy(ds->f_fstypename, bs->f_fstypename, DARWIN_MFSNAMELEN);
	(void)strlcpy(ds->f_mntonname, bs->f_mntonname, DARWIN_MNAMELEN);
	(void)strlcpy(ds->f_mntfromname, bs->f_mntfromname, DARWIN_MNAMELEN);

	return;
}
