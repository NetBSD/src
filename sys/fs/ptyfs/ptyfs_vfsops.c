/*	$NetBSD: ptyfs_vfsops.c,v 1.42.18.3 2014/08/27 14:53:26 msaitoh Exp $	*/

/*
 * Copyright (c) 1992, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
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
 */

/*
 * Pseudo-tty Filesystem
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ptyfs_vfsops.c,v 1.42.18.3 2014/08/27 14:53:26 msaitoh Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/stat.h>
#include <sys/dirent.h>
#include <sys/malloc.h>
#include <sys/syslog.h>
#include <sys/select.h>
#include <sys/filedesc.h>
#include <sys/tty.h>
#include <sys/pty.h>
#include <sys/kauth.h>
#include <sys/module.h>

#include <fs/ptyfs/ptyfs.h>
#include <miscfs/genfs/genfs.h>
#include <miscfs/specfs/specdev.h>

MODULE(MODULE_CLASS_VFS, ptyfs, NULL);

MALLOC_JUSTDEFINE(M_PTYFSMNT, "ptyfs mount", "ptyfs mount structures");
MALLOC_JUSTDEFINE(M_PTYFSTMP, "ptyfs temp", "ptyfs temporary structures");

VFS_PROTOS(ptyfs);

static struct sysctllog *ptyfs_sysctl_log;

static int ptyfs__allocvp(struct ptm_pty *, struct lwp *, struct vnode **,
    dev_t, char);
static int ptyfs__makename(struct ptm_pty *, struct lwp *, char *, size_t,
    dev_t, char);
static void ptyfs__getvattr(struct ptm_pty *, struct lwp *, struct vattr *);

/*
 * ptm glue: When we mount, we make ptm point to us.
 */
struct ptm_pty *ptyfs_save_ptm;
static int ptyfs_count;

struct ptm_pty ptm_ptyfspty = {
	ptyfs__allocvp,
	ptyfs__makename,
	ptyfs__getvattr,
	NULL
};

static const char *
ptyfs__getpath(struct lwp *l, const struct mount *mp)
{
#define MAXBUF (sizeof(mp->mnt_stat.f_mntonname) + 32)
	struct cwdinfo *cwdi = l->l_proc->p_cwdi;
	char *buf;
	const char *rv;
	size_t len;
	char *bp;
	int error;

	rv = mp->mnt_stat.f_mntonname;
	if (cwdi->cwdi_rdir == NULL)
		return rv;

	buf = malloc(MAXBUF, M_TEMP, M_WAITOK);
	bp = buf + MAXBUF;
	*--bp = '\0';
	error = getcwd_common(cwdi->cwdi_rdir, rootvnode, &bp,
	    buf, MAXBUF / 2, 0, l);
	if (error)	/* XXX */
		goto out;

	len = strlen(bp);
	if (len < sizeof(mp->mnt_stat.f_mntonname))	/* XXX */
		rv += len;
out:
	free(buf, M_TEMP);
	return rv;
}

static int
ptyfs__makename(struct ptm_pty *pt, struct lwp *l, char *tbuf, size_t bufsiz,
    dev_t dev, char ms)
{
	struct mount *mp = pt->arg;
	size_t len;

	switch (ms) {
	case 'p':
		/* We don't provide access to the master, should we? */
		len = snprintf(tbuf, bufsiz, "/dev/null");
		break;
	case 't':
		len = snprintf(tbuf, bufsiz, "%s/%llu", ptyfs__getpath(l, mp),
		    (unsigned long long)minor(dev));
		break;
	default:
		return EINVAL;
	}

	return len >= bufsiz ? ENOSPC : 0;
}


static int
/*ARGSUSED*/
ptyfs__allocvp(struct ptm_pty *pt, struct lwp *l, struct vnode **vpp,
    dev_t dev, char ms)
{
	struct mount *mp = pt->arg;
	ptyfstype type;

	switch (ms) {
	case 'p':
		type = PTYFSptc;
		break;
	case 't':
		type = PTYFSpts;
		break;
	default:
		return EINVAL;
	}

	return ptyfs_allocvp(mp, vpp, type, minor(dev), l);
}


static void
ptyfs__getvattr(struct ptm_pty *pt, struct lwp *l, struct vattr *vattr)
{
	struct mount *mp = pt->arg;
	struct ptyfsmount *pmnt = VFSTOPTY(mp);
	vattr_null(vattr);
	/* get real uid */
	vattr->va_uid = kauth_cred_getuid(l->l_cred);
	vattr->va_gid = pmnt->pmnt_gid;
	vattr->va_mode = pmnt->pmnt_mode;
}


void
ptyfs_init(void)
{

	malloc_type_attach(M_PTYFSMNT);
	malloc_type_attach(M_PTYFSTMP);
	ptyfs_hashinit();
}

void
ptyfs_reinit(void)
{
	ptyfs_hashreinit();
}

void
ptyfs_done(void)
{

	ptyfs_hashdone();
	malloc_type_detach(M_PTYFSTMP);
	malloc_type_detach(M_PTYFSMNT);
}

#define OSIZE sizeof(struct { int f; gid_t g; mode_t m; })
/*
 * Mount the Pseudo tty params filesystem
 */
int
ptyfs_mount(struct mount *mp, const char *path, void *data, size_t *data_len)
{
	struct lwp *l = curlwp;
	int error = 0;
	struct ptyfsmount *pmnt;
	struct ptyfs_args *args = data;

	if (args == NULL)
		return EINVAL;
	if (*data_len != sizeof *args) {
		if (*data_len != OSIZE || args->version >= PTYFS_ARGSVERSION)
			return EINVAL;
	}

	if (UIO_MX & (UIO_MX - 1)) {
		log(LOG_ERR, "ptyfs: invalid directory entry size");
		return EINVAL;
	}

	if (mp->mnt_flag & MNT_GETARGS) {
		pmnt = VFSTOPTY(mp);
		if (pmnt == NULL)
			return EIO;
		args->mode = pmnt->pmnt_mode;
		args->gid = pmnt->pmnt_gid;
		if (args->version >= PTYFS_ARGSVERSION) {
			args->flags = pmnt->pmnt_flags;
			*data_len = sizeof *args;
		} else {
			*data_len = OSIZE;
		}
		return 0;
	}

#if 0
	/* Don't allow more than one mount */
	if (ptyfs_count)
		return EBUSY;
#endif

	if (mp->mnt_flag & MNT_UPDATE)
		return EOPNOTSUPP;

	if (args->version > PTYFS_ARGSVERSION)
		return EINVAL;

	pmnt = malloc(sizeof(struct ptyfsmount), M_PTYFSMNT, M_WAITOK);

	mp->mnt_data = pmnt;
	pmnt->pmnt_gid = args->gid;
	pmnt->pmnt_mode = args->mode;
	if (args->version >= PTYFS_ARGSVERSION)
		pmnt->pmnt_flags = args->flags;
	else
		pmnt->pmnt_flags = 0;
	mp->mnt_flag |= MNT_LOCAL;
	vfs_getnewfsid(mp);

	if ((error = set_statvfs_info(path, UIO_USERSPACE, "ptyfs",
	    UIO_SYSSPACE, mp->mnt_op->vfs_name, mp, l)) != 0) {
		free(pmnt, M_PTYFSMNT);
		return error;
	}

	/* Point pty access to us */
	if (ptyfs_count == 0) {
		ptm_ptyfspty.arg = mp;
		ptyfs_save_ptm = pty_sethandler(&ptm_ptyfspty);
	}
	ptyfs_count++;
	return 0;
}

/*ARGSUSED*/
int
ptyfs_start(struct mount *mp, int flags)
{
	return 0;
}

/*ARGSUSED*/
int
ptyfs_unmount(struct mount *mp, int mntflags)
{
	int error;
	int flags = 0;

	if (mntflags & MNT_FORCE)
		flags |= FORCECLOSE;

	if ((error = vflush(mp, 0, flags)) != 0)
		return error;

	ptyfs_count--;
	if (ptyfs_count == 0) {
		/* Restore where pty access was pointing */
		(void)pty_sethandler(ptyfs_save_ptm);
		ptyfs_save_ptm = NULL;
		ptm_ptyfspty.arg = NULL;
	}

	/*
	 * Finally, throw away the ptyfsmount structure
	 */
	free(mp->mnt_data, M_PTYFSMNT);
	mp->mnt_data = NULL;

	return 0;
}

int
ptyfs_root(struct mount *mp, struct vnode **vpp)
{
	/* setup "." */
	return ptyfs_allocvp(mp, vpp, PTYFSroot, 0, NULL);
}

/*ARGSUSED*/
int
ptyfs_sync(struct mount *mp, int waitfor,
    kauth_cred_t uc)
{
	return 0;
}

/*
 * Kernfs flat namespace lookup.
 * Currently unsupported.
 */
/*ARGSUSED*/
int
ptyfs_vget(struct mount *mp, ino_t ino,
    struct vnode **vpp)
{
	return EOPNOTSUPP;
}

extern const struct vnodeopv_desc ptyfs_vnodeop_opv_desc;

const struct vnodeopv_desc * const ptyfs_vnodeopv_descs[] = {
	&ptyfs_vnodeop_opv_desc,
	NULL,
};

struct vfsops ptyfs_vfsops = {
	MOUNT_PTYFS,
	sizeof (struct ptyfs_args),
	ptyfs_mount,
	ptyfs_start,
	ptyfs_unmount,
	ptyfs_root,
	(void *)eopnotsupp,		/* vfs_quotactl */
	genfs_statvfs,
	ptyfs_sync,
	ptyfs_vget,
	(void *)eopnotsupp,		/* vfs_fhtovp */
	(void *)eopnotsupp,		/* vfs_vptofp */
	ptyfs_init,
	ptyfs_reinit,
	ptyfs_done,
	NULL,				/* vfs_mountroot */
	(void *)eopnotsupp,
	(void *)eopnotsupp,
	(void *)eopnotsupp,		/* vfs_suspendctl */
	genfs_renamelock_enter,
	genfs_renamelock_exit,
	(void *)eopnotsupp,
	ptyfs_vnodeopv_descs,
	0,
	{ NULL, NULL },
};

static int
ptyfs_modcmd(modcmd_t cmd, void *arg)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		error = vfs_attach(&ptyfs_vfsops);
		if (error != 0)
			break;
		sysctl_createv(&ptyfs_sysctl_log, 0, NULL, NULL,
			       CTLFLAG_PERMANENT,
			       CTLTYPE_NODE, "vfs", NULL,
			       NULL, 0, NULL, 0,
			       CTL_VFS, CTL_EOL);
		sysctl_createv(&ptyfs_sysctl_log, 0, NULL, NULL,
			       CTLFLAG_PERMANENT,
			       CTLTYPE_NODE, "ptyfs",
			       SYSCTL_DESCR("Pty file system"),
			       NULL, 0, NULL, 0,
			       CTL_VFS, 23, CTL_EOL);
		/*
		 * XXX the "23" above could be dynamic, thereby eliminating
		 * one more instance of the "number to vfs" mapping problem,
		 * but "23" is the order as taken from sys/mount.h
		 */
		break;
	case MODULE_CMD_FINI:
		error = vfs_detach(&ptyfs_vfsops);
		if (error != 0)
			break;
		sysctl_teardown(&ptyfs_sysctl_log);
		break;
	default:
		error = ENOTTY;
		break;
	}

	return (error);
}
