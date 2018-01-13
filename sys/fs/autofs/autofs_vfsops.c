/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * Copyright (c) 2016 The DragonFly Project
 * Copyright (c) 2014 The FreeBSD Foundation
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tomohiro Kusumi <kusumi.tomohiro@gmail.com>.
 *
 * This software was developed by Edward Tomasz Napierala under sponsorship
 * from the FreeBSD Foundation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autofs_vfsops.c,v 1.3 2018/01/13 22:06:21 christos Exp $");


#include "autofs.h"
#include "autofs_mount.h"

#include <sys/stat.h>
#include <sys/sysctl.h>
#include <miscfs/genfs/genfs.h>

MODULE(MODULE_CLASS_VFS, autofs, NULL);

static int	autofs_statvfs(struct mount *, struct statvfs *);
static int	autofs_sysctl_create(void);

static void
autofs_init(void)
{

	KASSERT(!autofs_softc);

	autofs_softc = kmem_zalloc(sizeof(*autofs_softc), KM_SLEEP);

	pool_init(&autofs_request_pool, sizeof(struct autofs_request), 0, 0, 0,
	    "autofs_request", &pool_allocator_nointr, IPL_NONE);
	pool_init(&autofs_node_pool, sizeof(struct autofs_node), 0, 0, 0,
	    "autofs_node", &pool_allocator_nointr, IPL_NONE);

	TAILQ_INIT(&autofs_softc->sc_requests);
	cv_init(&autofs_softc->sc_cv, "autofscv");
	mutex_init(&autofs_softc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	autofs_softc->sc_dev_opened = false;

	autofs_sysctl_create();
	workqueue_create(&autofs_tmo_wq, "autofstmo",
	    autofs_timeout_wq, NULL, 0, 0, WQ_MPSAFE);
}

static void
autofs_done(void)
{
	KASSERT(autofs_softc);
	KASSERT(!autofs_softc->sc_dev_opened);

	workqueue_destroy(autofs_tmo_wq);

	struct autofs_softc *sc = autofs_softc;
	autofs_softc = NULL;

	cv_destroy(&sc->sc_cv);
	mutex_destroy(&sc->sc_lock);

	pool_destroy(&autofs_request_pool);
	pool_destroy(&autofs_node_pool);

	kmem_free(sc, sizeof(*sc));
}

static int
autofs_mount(struct mount *mp, const char *path, void *data, size_t *data_len)
{
	struct autofs_args *args = data;
	struct autofs_mount *amp;
	struct statvfs *sbp = &mp->mnt_stat;
	int error;

	if (!args)
		return EINVAL;

	/*
	 * MNT_GETARGS is unsupported.  Autofs is mounted via automount(8) by
	 * parsing /etc/auto_master instead of regular mount(8) variants with
	 * -o getargs support, thus not really needed either.
	 */
	if (mp->mnt_flag & MNT_GETARGS)
		return EOPNOTSUPP;

	if (mp->mnt_flag & MNT_UPDATE) {
		autofs_flush(VFSTOAUTOFS(mp));
		return 0;
	}

	/*
	 * Allocate the autofs mount.
	 */
	amp = kmem_zalloc(sizeof(*amp), KM_SLEEP);
	mp->mnt_data = amp;
	amp->am_mp = mp;

	/*
	 * Copy-in master_options string.
	 */
	error = copyinstr(args->master_options, amp->am_options,
	    sizeof(amp->am_options), NULL);
	if (error)
		goto fail;

	/*
	 * Copy-in master_prefix string.
	 */
	error = copyinstr(args->master_prefix, amp->am_prefix,
	    sizeof(amp->am_prefix), NULL);
	if (error)
		goto fail;

	/*
	 * Initialize the autofs mount.
	 */
	mutex_init(&amp->am_lock, MUTEX_DEFAULT, IPL_NONE);
	amp->am_last_ino = AUTOFS_ROOTINO;

	mutex_enter(&amp->am_lock);
	error = autofs_node_new(NULL, amp, ".", -1, &amp->am_root);
	if (error) {
		mutex_exit(&amp->am_lock);
		goto fail;
	}
	mutex_exit(&amp->am_lock);
	KASSERT(amp->am_root->an_ino == AUTOFS_ROOTINO);

	autofs_statvfs(mp, sbp);
	vfs_getnewfsid(mp);

	error = set_statvfs_info(path, UIO_USERSPACE, args->from, UIO_USERSPACE,
	    mp->mnt_op->vfs_name, mp, curlwp);
	if (error)
		goto fail;
	strlcpy(amp->am_from, sbp->f_mntfromname, sizeof(amp->am_from));
	strlcpy(amp->am_on, sbp->f_mntonname, sizeof(amp->am_on));

	return 0;

fail:
	kmem_free(amp, sizeof(*amp));
	return error;
}

static int
autofs_unmount(struct mount *mp, int mntflags)
{
	struct autofs_mount *amp = VFSTOAUTOFS(mp);
	int error, flags;

	flags = 0;
	if (mntflags & MNT_FORCE)
		flags |= FORCECLOSE;
	error = vflush(mp, NULL, flags);
	if (error) {
		AUTOFS_WARN("vflush failed with error %d", error);
		return error;
	}

	/*
	 * All vnodes are gone, and new one will not appear - so,
	 * no new triggerings.
	 */
	for (;;) {
		struct autofs_request *ar;
		int dummy;
		bool found;

		found = false;
		mutex_enter(&autofs_softc->sc_lock);
		TAILQ_FOREACH(ar, &autofs_softc->sc_requests, ar_next) {
			if (ar->ar_mount != amp)
				continue;
			ar->ar_error = ENXIO;
			ar->ar_done = true;
			ar->ar_in_progress = false;
			found = true;
		}
		if (found == false) {
			mutex_exit(&autofs_softc->sc_lock);
			break;
		}

		cv_broadcast(&autofs_softc->sc_cv);
		mutex_exit(&autofs_softc->sc_lock);

		tsleep(&dummy, 0, "autofs_umount", hz);
	}

	mutex_enter(&amp->am_lock);
	while (!RB_EMPTY(&amp->am_root->an_children)) {
		struct autofs_node *anp;
		anp = RB_MIN(autofs_node_tree, &amp->am_root->an_children);
		if (!RB_EMPTY(&anp->an_children)) {
			AUTOFS_DEBUG("%s: %s has children", __func__,
			    anp->an_name);
			mutex_exit(&amp->am_lock);
			return EBUSY;
		}
			
		autofs_node_delete(anp);
	}
	autofs_node_delete(amp->am_root);
	mp->mnt_data = NULL;
	mutex_exit(&amp->am_lock);

	mutex_destroy(&amp->am_lock);

	kmem_free(amp, sizeof(*amp));

	return 0;
}

static int
autofs_start(struct mount *mp, int flags)
{

	return 0;
}

static int
autofs_root(struct mount *mp, struct vnode **vpp)
{
	struct autofs_node *anp = VFSTOAUTOFS(mp)->am_root;
	int error;

	error = vcache_get(mp, &anp, sizeof(anp), vpp);
	if (error)
		return error;
	error = vn_lock(*vpp, LK_EXCLUSIVE);
	if (error) {
		vrele(*vpp);
		*vpp = NULL;
		return error;
	}

	return 0;
}

static int
autofs_statvfs(struct mount *mp, struct statvfs *sbp)
{

	sbp->f_bsize = S_BLKSIZE;
	sbp->f_frsize = S_BLKSIZE;
	sbp->f_iosize = 0;
	sbp->f_blocks = 0;
	sbp->f_bfree = 0;
	sbp->f_bavail = 0;
	sbp->f_bresvd = 0;
	sbp->f_files = 0;
	sbp->f_ffree = 0;
	sbp->f_favail = 0;
	sbp->f_fresvd = 0;

	copy_statvfs_info(sbp, mp);

	return 0;
}

static int
autofs_sync(struct mount *mp, int waitfor, kauth_cred_t uc)
{

	return 0;
}

static int
autofs_loadvnode(struct mount *mp, struct vnode *vp,
    const void *key, size_t key_len, const void **new_key)
{
	struct autofs_node *anp;

	KASSERT(key_len == sizeof(anp));
	memcpy(&anp, key, key_len);
	KASSERT(!anp->an_vnode);

	vp->v_tag = VT_AUTOFS;
	vp->v_type = VDIR;
	vp->v_op = autofs_vnodeop_p;
	vp->v_data = anp;

	if (anp->an_ino == AUTOFS_ROOTINO)
		vp->v_vflag |= VV_ROOT;

	anp->an_vnode = vp;
	uvm_vnp_setsize(vp, 0);

	*new_key = &vp->v_data;

	return 0;
}

static const struct vnodeopv_desc * const autofs_vnodeopv_descs[] = {
	&autofs_vnodeop_opv_desc,
	NULL,
};

static struct vfsops autofs_vfsops = {
	.vfs_name = MOUNT_AUTOFS,
	.vfs_min_mount_data = sizeof(struct autofs_args),
	.vfs_mount = autofs_mount,
	.vfs_start = autofs_start,
	.vfs_unmount = autofs_unmount,
	.vfs_root = autofs_root,
	.vfs_quotactl = (void *)eopnotsupp,
	.vfs_statvfs = autofs_statvfs,
	.vfs_sync = autofs_sync,
	.vfs_vget = (void *)eopnotsupp,
	.vfs_loadvnode = (void *)autofs_loadvnode,
	.vfs_newvnode = (void *)eopnotsupp,
	.vfs_fhtovp = (void *)eopnotsupp,
	.vfs_vptofh = (void *)eopnotsupp,
	.vfs_init = autofs_init,
	.vfs_reinit = (void *)eopnotsupp,
	.vfs_done = autofs_done,
	.vfs_mountroot = (void *)eopnotsupp,
	.vfs_snapshot = (void *)eopnotsupp,
	.vfs_extattrctl = (void *)eopnotsupp,
	.vfs_suspendctl = (void *)genfs_suspendctl,
	.vfs_renamelock_enter = (void *)eopnotsupp,
	.vfs_renamelock_exit = (void *)eopnotsupp,
	.vfs_fsync = (void *)eopnotsupp,
	.vfs_opv_descs = autofs_vnodeopv_descs
};

#define AUTOFS_SYSCTL_DEBUG		1
#define AUTOFS_SYSCTL_MOUNT_ON_STAT	2
#define AUTOFS_SYSCTL_TIMEOUT		3
#define AUTOFS_SYSCTL_CACHE		4
#define AUTOFS_SYSCTL_RETRY_ATTEMPTS	5
#define AUTOFS_SYSCTL_RETRY_DELAY	6
#define AUTOFS_SYSCTL_INTERRUPTIBLE	7

static struct sysctllog *autofs_sysctl_log;

static int
autofs_sysctl_create(void)
{
	int error;

	/*
	 * XXX the "33" below could be dynamic, thereby eliminating one
	 * more instance of the "number to vfs" mapping problem, but
	 * "33" is the order as taken from sys/mount.h
	 */
	error = sysctl_createv(&autofs_sysctl_log, 0, NULL, NULL,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "autofs",
	    SYSCTL_DESCR("Automounter filesystem"),
	    NULL, 0, NULL, 0,
	    CTL_VFS, 33, CTL_EOL);
	if (error)
		goto fail;

	error = sysctl_createv(&autofs_sysctl_log, 0, NULL, NULL,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "autofs_debug",
	    SYSCTL_DESCR("Enable debug messages"),
	    NULL, 0, &autofs_debug, 0,
	    CTL_VFS, 33, AUTOFS_SYSCTL_DEBUG, CTL_EOL);
	if (error)
		goto fail;

	error = sysctl_createv(&autofs_sysctl_log, 0, NULL, NULL,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "autofs_mount_on_stat",
	    SYSCTL_DESCR("Trigger mount on stat(2) on mountpoint"),
	    NULL, 0, &autofs_mount_on_stat, 0,
	    CTL_VFS, 33, AUTOFS_SYSCTL_MOUNT_ON_STAT, CTL_EOL);
	if (error)
		goto fail;

	error = sysctl_createv(&autofs_sysctl_log, 0, NULL, NULL,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "autofs_timeout",
	    SYSCTL_DESCR("Number of seconds to wait for automountd(8)"),
	    NULL, 0, &autofs_timeout, 0,
	    CTL_VFS, 33, AUTOFS_SYSCTL_TIMEOUT, CTL_EOL);
	if (error)
		goto fail;

	error = sysctl_createv(&autofs_sysctl_log, 0, NULL, NULL,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "autofs_cache",
	    SYSCTL_DESCR("Number of seconds to wait before reinvoking"),
	    NULL, 0, &autofs_cache, 0,
	    CTL_VFS, 33, AUTOFS_SYSCTL_CACHE, CTL_EOL);
	if (error)
		goto fail;

	error = sysctl_createv(&autofs_sysctl_log, 0, NULL, NULL,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "autofs_retry_attempts",
	    SYSCTL_DESCR("Number of attempts before failing mount"),
	    NULL, 0, &autofs_retry_attempts, 0,
	    CTL_VFS, 33, AUTOFS_SYSCTL_RETRY_ATTEMPTS, CTL_EOL);
	if (error)
		goto fail;

	error = sysctl_createv(&autofs_sysctl_log, 0, NULL, NULL,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "autofs_retry_delay",
	    SYSCTL_DESCR("Number of seconds before retrying"),
	    NULL, 0, &autofs_retry_delay, 0,
	    CTL_VFS, 33, AUTOFS_SYSCTL_RETRY_DELAY, CTL_EOL);
	if (error)
		goto fail;

	error = sysctl_createv(&autofs_sysctl_log, 0, NULL, NULL,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_INT, "autofs_interruptible",
	    SYSCTL_DESCR("Allow requests to be interrupted by signal"),
	    NULL, 0, &autofs_interruptible, 0,
	    CTL_VFS, 33, AUTOFS_SYSCTL_INTERRUPTIBLE, CTL_EOL);
	if (error)
		goto fail;

	return 0;
fail:
	AUTOFS_WARN("sysctl_createv failed with error %d", error);

	return error;
}

extern const struct cdevsw autofs_cdevsw;

static int
autofs_modcmd(modcmd_t cmd, void *arg)
{
#ifdef _MODULE
	devmajor_t bmajor = NODEVMAJOR, cmajor = NODEVMAJOR;
#endif
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
		error = vfs_attach(&autofs_vfsops);
		if (error)
			break;
#ifdef _MODULE
		error = devsw_attach("autofs", NULL, &bmajor, &autofs_cdevsw,
		    &cmajor);
		if (error) {
			vfs_detach(&autofs_vfsops);
			break;
		}
#endif
		break;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		KASSERT(autofs_softc);
		mutex_enter(&autofs_softc->sc_lock);
		if (autofs_softc->sc_dev_opened) {
			mutex_exit(&autofs_softc->sc_lock);
			error = EBUSY;
			break;
		}
		mutex_exit(&autofs_softc->sc_lock);

		error = devsw_detach(NULL, &autofs_cdevsw);
		if (error)
			break;
#endif
		error = vfs_detach(&autofs_vfsops);
		if (error)
			break;
		break;
	default:
		error = ENOTTY;
		break;
	}

	return error;
}
