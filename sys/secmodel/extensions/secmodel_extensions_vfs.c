/*	$NetBSD: secmodel_extensions_vfs.c,v 1.1 2023/04/22 13:54:19 riastradh Exp $	*/

/*-
 * Copyright (c) 2011 Elad Efrat <elad@NetBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: secmodel_extensions_vfs.c,v 1.1 2023/04/22 13:54:19 riastradh Exp $");

#include <sys/types.h>
#include <sys/param.h>

#include <sys/kauth.h>
#include <sys/vnode.h>

#include <secmodel/secmodel.h>
#include <secmodel/extensions/extensions.h>
#include <secmodel/extensions/extensions_impl.h>

static int dovfsusermount;
static int hardlink_check_uid;
static int hardlink_check_gid;

static kauth_listener_t l_system, l_vnode;

static int secmodel_extensions_system_cb(kauth_cred_t, kauth_action_t,
    void *, void *, void *, void *, void *);
static int secmodel_extensions_vnode_cb(kauth_cred_t, kauth_action_t,
    void *, void *, void *, void *, void *);

void
secmodel_extensions_vfs_start(void)
{

	l_system = kauth_listen_scope(KAUTH_SCOPE_SYSTEM,
	    secmodel_extensions_system_cb, NULL);
	l_vnode = kauth_listen_scope(KAUTH_SCOPE_VNODE,
	    secmodel_extensions_vnode_cb, NULL);
}

void
secmodel_extensions_vfs_stop(void)
{

	kauth_unlisten_scope(l_system);
	kauth_unlisten_scope(l_vnode);
}

void
secmodel_extensions_vfs_sysctl(struct sysctllog **clog,
    const struct sysctlnode *rnode)
{

	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "usermount",
		       SYSCTL_DESCR("Whether unprivileged users may mount "
				    "filesystems"),
		       sysctl_extensions_user_handler, 0, &dovfsusermount, 0,
		       CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "hardlink_check_uid",
		       SYSCTL_DESCR("Whether unprivileged users can hardlink "\
			    "to files they don't own"),
		       sysctl_extensions_user_handler, 0,
		       &hardlink_check_uid, 0,
		       CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "hardlink_check_gid",
		       SYSCTL_DESCR("Whether unprivileged users can hardlink "\
			    "to files that are not in their " \
			    "group membership"),
		       sysctl_extensions_user_handler, 0,
		       &hardlink_check_gid, 0,
		       CTL_CREATE, CTL_EOL);

	/* Compatibility: vfs.generic.usermount */
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "generic",
		       SYSCTL_DESCR("Non-specific vfs related information"),
		       NULL, 0, NULL, 0,
		       CTL_VFS, VFS_GENERIC, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "usermount",
		       SYSCTL_DESCR("Whether unprivileged users may mount "
				    "filesystems"),
		       sysctl_extensions_user_handler, 0, &dovfsusermount, 0,
		       CTL_VFS, VFS_GENERIC, VFS_USERMOUNT, CTL_EOL);
}

static int
secmodel_extensions_system_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1, void *arg2, void *arg3)
{
	vnode_t *vp;
	struct vattr va;
	struct mount *mp;
	u_long flags;
	int result;
	enum kauth_system_req req;
	int error;

	req = (enum kauth_system_req)(uintptr_t)arg0;
	result = KAUTH_RESULT_DEFER;

	switch (action) {
	case KAUTH_SYSTEM_MOUNT:
		if (dovfsusermount == 0)
			break;
		switch (req) {
		case KAUTH_REQ_SYSTEM_MOUNT_NEW:
			vp = (vnode_t *)arg1;
			mp = vp->v_mount;
			flags = (u_long)arg2;

			/*
			 * Ensure that the user owns the directory onto which
			 * the mount is attempted.
			 */
			vn_lock(vp, LK_SHARED | LK_RETRY);
			error = VOP_GETATTR(vp, &va, cred);
			VOP_UNLOCK(vp);
			if (error)
				break;

			if (va.va_uid != kauth_cred_geteuid(cred))
				break;

			error = usermount_common_policy(mp, flags);
			if (error)
				break;

			result = KAUTH_RESULT_ALLOW;

			break;

		case KAUTH_REQ_SYSTEM_MOUNT_UNMOUNT:
			mp = arg1;

			/* Must own the mount. */
			if (mp->mnt_stat.f_owner == kauth_cred_geteuid(cred))
				result = KAUTH_RESULT_ALLOW;

			break;

		case KAUTH_REQ_SYSTEM_MOUNT_UPDATE:
			mp = arg1;
			flags = (u_long)arg2;

			/* Must own the mount. */
			if (mp->mnt_stat.f_owner == kauth_cred_geteuid(cred) &&
				usermount_common_policy(mp, flags) == 0)
				result = KAUTH_RESULT_ALLOW;

			break;

		default:
			break;
		}
		break;

	default:
		break;
	}

	return (result);
}

static int
secmodel_extensions_vnode_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1, void *arg2, void *arg3)
{
	int error;
	bool isroot;
	struct vattr va;

	if ((action & KAUTH_VNODE_ADD_LINK) == 0)
		return KAUTH_RESULT_DEFER;

	error = VOP_GETATTR((vnode_t *)arg0, &va, cred);
	if (error)
		goto checkroot;

	if (hardlink_check_uid && kauth_cred_geteuid(cred) != va.va_uid)
		goto checkroot;

	if (hardlink_check_gid && kauth_cred_groupmember(cred, va.va_gid) != 0)
		goto checkroot;

	return KAUTH_RESULT_DEFER;
checkroot:
	error = secmodel_eval("org.netbsd.secmodel.suser", "is-root",
	    cred, &isroot);
	if (error || !isroot)
		return KAUTH_RESULT_DENY;

	return KAUTH_RESULT_DEFER;
}

