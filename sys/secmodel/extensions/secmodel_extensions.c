/* $NetBSD: secmodel_extensions.c,v 1.2.2.1 2013/02/08 23:04:26 riz Exp $ */
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
__KERNEL_RCSID(0, "$NetBSD: secmodel_extensions.c,v 1.2.2.1 2013/02/08 23:04:26 riz Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/kauth.h>

#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/socketvar.h>
#include <sys/sysctl.h>
#include <sys/proc.h>
#include <sys/module.h>

#include <secmodel/secmodel.h>
#include <secmodel/extensions/extensions.h>

MODULE(MODULE_CLASS_SECMODEL, extensions, NULL);

/* static */ int dovfsusermount;
static int curtain;
static int user_set_cpu_affinity;

static kauth_listener_t l_system, l_process, l_network;

static secmodel_t extensions_sm;
static struct sysctllog *extensions_sysctl_log;

static void secmodel_extensions_init(void);
static void secmodel_extensions_start(void);
static void secmodel_extensions_stop(void);

static void sysctl_security_extensions_setup(struct sysctllog **);
static int  sysctl_extensions_user_handler(SYSCTLFN_PROTO);
static int  sysctl_extensions_curtain_handler(SYSCTLFN_PROTO);
static bool is_securelevel_above(int);

static int secmodel_extensions_system_cb(kauth_cred_t, kauth_action_t,
    void *, void *, void *, void *, void *);
static int secmodel_extensions_process_cb(kauth_cred_t, kauth_action_t,
    void *, void *, void *, void *, void *);
static int secmodel_extensions_network_cb(kauth_cred_t, kauth_action_t,
    void *, void *, void *, void *, void *);

static void
sysctl_security_extensions_setup(struct sysctllog **clog)
{
	const struct sysctlnode *rnode, *rnode2;

	sysctl_createv(clog, 0, NULL, &rnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "security", NULL,
		       NULL, 0, NULL, 0,
		       CTL_SECURITY, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, &rnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "models", NULL,
		       NULL, 0, NULL, 0,
		       CTL_CREATE, CTL_EOL);

	/* Compatibility: security.models.bsd44 */
	rnode2 = rnode;
	sysctl_createv(clog, 0, &rnode2, &rnode2,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "bsd44", NULL,
		       NULL, 0, NULL, 0,
		       CTL_CREATE, CTL_EOL);

        /* Compatibility: security.models.bsd44.curtain */
	sysctl_createv(clog, 0, &rnode2, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "curtain",
		       SYSCTL_DESCR("Curtain information about objects to "\
		       		    "users not owning them."),
		       sysctl_extensions_curtain_handler, 0, &curtain, 0,
		       CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, &rnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "extensions", NULL,
		       NULL, 0, NULL, 0,
		       CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "name", NULL,
		       NULL, 0, __UNCONST(SECMODEL_EXTENSIONS_NAME), 0,
		       CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "usermount",
		       SYSCTL_DESCR("Whether unprivileged users may mount "
				    "filesystems"),
		       sysctl_extensions_user_handler, 0, &dovfsusermount, 0,
		       CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "curtain",
		       SYSCTL_DESCR("Curtain information about objects to "\
		       		    "users not owning them."),
		       sysctl_extensions_curtain_handler, 0, &curtain, 0,
		       CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "user_set_cpu_affinity",
		       SYSCTL_DESCR("Whether unprivileged users may control "\
		       		    "CPU affinity."),
		       sysctl_extensions_user_handler, 0,
		       &user_set_cpu_affinity, 0,
		       CTL_CREATE, CTL_EOL);

	/* Compatibility: vfs.generic.usermount */
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "vfs", NULL,
		       NULL, 0, NULL, 0,
		       CTL_VFS, CTL_EOL);

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

	/* Compatibility: security.curtain */
	sysctl_createv(clog, 0, NULL, &rnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "security", NULL,
		       NULL, 0, NULL, 0,
		       CTL_SECURITY, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "curtain",
		       SYSCTL_DESCR("Curtain information about objects to "\
		       		    "users not owning them."),
		       sysctl_extensions_curtain_handler, 0, &curtain, 0,
		       CTL_CREATE, CTL_EOL);
}

static int
sysctl_extensions_curtain_handler(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	int val, error;

	val = *(int *)rnode->sysctl_data;

	node = *rnode;
	node.sysctl_data = &val;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	/* shortcut */
	if (val == *(int *)rnode->sysctl_data)
		return 0;

	/* curtain cannot be disabled when securelevel is above 0 */
	if (val == 0 && is_securelevel_above(0)) {
		return EPERM;
	}

	*(int *)rnode->sysctl_data = val;
	return 0;
}

/*
 * Generic sysctl extensions handler for user mount and set CPU affinity
 * rights. Checks the following conditions:
 * - setting value to 0 is always permitted (decrease user rights)
 * - setting value != 0 is not permitted when securelevel is above 0 (increase
 *   user rights).
 */
static int
sysctl_extensions_user_handler(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	int val, error;

	val = *(int *)rnode->sysctl_data;

	node = *rnode;
	node.sysctl_data = &val;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	/* shortcut */
	if (val == *(int *)rnode->sysctl_data)
		return 0;

	/* we cannot grant more rights to users when securelevel is above 0 */
	if (val != 0 && is_securelevel_above(0)) {
		return EPERM;
	}

	*(int *)rnode->sysctl_data = val;
	return 0;
}

/*
 * Query secmodel_securelevel(9) to know whether securelevel is strictly
 * above 'level' or not.
 * Returns true if it is, false otherwise (when securelevel is absent or
 * securelevel is at or below 'level').
 */
static bool
is_securelevel_above(int level)
{
	bool above;
	int error;

	error = secmodel_eval("org.netbsd.secmodel.securelevel",
	    "is-securelevel-above", KAUTH_ARG(level), &above);
	if (error == 0 && above)
		return true;
	else
		return false;
}

static void
secmodel_extensions_init(void)
{

	curtain = 0;
	user_set_cpu_affinity = 0;
}

static void
secmodel_extensions_start(void)
{

	l_system = kauth_listen_scope(KAUTH_SCOPE_SYSTEM,
	    secmodel_extensions_system_cb, NULL);
	l_process = kauth_listen_scope(KAUTH_SCOPE_PROCESS,
	    secmodel_extensions_process_cb, NULL);
	l_network = kauth_listen_scope(KAUTH_SCOPE_NETWORK,
	    secmodel_extensions_network_cb, NULL);
}

static void
secmodel_extensions_stop(void)
{

	kauth_unlisten_scope(l_system);
	kauth_unlisten_scope(l_process);
	kauth_unlisten_scope(l_network);
}

static int
extensions_modcmd(modcmd_t cmd, void *arg)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
		error = secmodel_register(&extensions_sm,
		    SECMODEL_EXTENSIONS_ID, SECMODEL_EXTENSIONS_NAME,
		    NULL, NULL, NULL);
		if (error != 0)
			printf("extensions_modcmd::init: secmodel_register "
			    "returned %d\n", error);

		secmodel_extensions_init();
		secmodel_extensions_start();
		sysctl_security_extensions_setup(&extensions_sysctl_log);
		break;

	case MODULE_CMD_FINI:
		sysctl_teardown(&extensions_sysctl_log);
		secmodel_extensions_stop();

		error = secmodel_deregister(extensions_sm);
		if (error != 0)
			printf("extensions_modcmd::fini: secmodel_deregister "
			    "returned %d\n", error);

		break;

	case MODULE_CMD_AUTOUNLOAD:
		error = EPERM;
		break;

	default:
		error = ENOTTY;
		break;
	}

	return (error);
}

static int
secmodel_extensions_system_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1, void *arg2, void *arg3)
{
	struct mount *mp;
	u_long flags;
	int result;
	enum kauth_system_req req;

	req = (enum kauth_system_req)arg0;
	result = KAUTH_RESULT_DEFER;

	if (action != KAUTH_SYSTEM_MOUNT || dovfsusermount == 0)
		return result;

	switch (req) {
	case KAUTH_REQ_SYSTEM_MOUNT_NEW:
		mp = ((struct vnode *)arg1)->v_mount;
		flags = (u_long)arg2;

		if (usermount_common_policy(mp, flags) == 0)
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

	return (result);
}

static int
secmodel_extensions_process_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1, void *arg2, void *arg3)
{
	int result;
	enum kauth_process_req req;

	result = KAUTH_RESULT_DEFER;
	req = (enum kauth_process_req)arg1;

	switch (action) {
	case KAUTH_PROCESS_CANSEE:
		switch (req) {
		case KAUTH_REQ_PROCESS_CANSEE_ARGS:
		case KAUTH_REQ_PROCESS_CANSEE_ENTRY:
		case KAUTH_REQ_PROCESS_CANSEE_OPENFILES:
			if (curtain != 0) {
				struct proc *p = arg0;

				/*
				 * Only process' owner and root can see
				 * through curtain
				 */
				if (!kauth_cred_uidmatch(cred, p->p_cred)) {
					int error;
					bool isroot = false;

					error = secmodel_eval(
					    "org.netbsd.secmodel.suser",
					    "is-root", cred, &isroot);
					if (error == 0 && !isroot)
						result = KAUTH_RESULT_DENY;
				}
			}

			break;

		default:
			break;
		}

		break;

	case KAUTH_PROCESS_SCHEDULER_SETAFFINITY:
		if (user_set_cpu_affinity != 0) {
			struct proc *p = arg0;

			if (kauth_cred_uidmatch(cred, p->p_cred))
				result = KAUTH_RESULT_ALLOW;
		}
		break;

	default:
		break;
	}

	return (result);
}

static int
secmodel_extensions_network_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1, void *arg2, void *arg3)
{
	int result;
	enum kauth_network_req req;

	result = KAUTH_RESULT_DEFER;
	req = (enum kauth_network_req)arg0;

	if (action != KAUTH_NETWORK_SOCKET ||
	    req != KAUTH_REQ_NETWORK_SOCKET_CANSEE)
		return result;

	if (curtain != 0) {
		struct socket *so = (struct socket *)arg1;

		if (!kauth_cred_uidmatch(cred, so->so_cred)) {
			int error;
			bool isroot = false;

			error = secmodel_eval("org.netbsd.secmodel.suser",
			    "is-root", cred, &isroot);
			if (error == 0 && !isroot)
				result = KAUTH_RESULT_DENY;
		}
	}

	return (result);
}
