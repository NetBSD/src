/* $NetBSD: secmodel_securelevel.c,v 1.29 2013/01/28 00:51:30 jym Exp $ */
/*-
 * Copyright (c) 2006 Elad Efrat <elad@NetBSD.org>
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

/*
 * This file contains kauth(9) listeners needed to implement the traditional
 * NetBSD securelevel.
 *
 * The securelevel is a system-global indication on what operations are
 * allowed or not. It affects all users, including root.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: secmodel_securelevel.c,v 1.29 2013/01/28 00:51:30 jym Exp $");

#ifdef _KERNEL_OPT
#include "opt_insecure.h"
#endif /* _KERNEL_OPT */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/kauth.h>

#include <sys/conf.h>
#include <sys/mount.h>
#include <sys/sysctl.h>
#include <sys/vnode.h>
#include <sys/module.h>
#include <sys/timevar.h>

#include <miscfs/specfs/specdev.h>

#include <secmodel/secmodel.h>
#include <secmodel/securelevel/securelevel.h>

MODULE(MODULE_CLASS_SECMODEL, securelevel, NULL);

static int securelevel;

static kauth_listener_t l_system, l_process, l_network, l_machdep, l_device,
    l_vnode;

static secmodel_t securelevel_sm;
static struct sysctllog *securelevel_sysctl_log;

/*
 * Sysctl helper routine for securelevel. Ensures that the value only rises
 * unless the caller is init.
 */
int
secmodel_securelevel_sysctl(SYSCTLFN_ARGS)
{
	int newsecurelevel, error;
	struct sysctlnode node;

	newsecurelevel = securelevel;
	node = *rnode;
	node.sysctl_data = &newsecurelevel;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return (error);

	if ((newsecurelevel < securelevel) && (l->l_proc != initproc))
		return (EPERM);

	securelevel = newsecurelevel;

	return (error);
}

void
sysctl_security_securelevel_setup(struct sysctllog **clog)
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

        /* Compatibility: security.models.bsd44.securelevel */
	sysctl_createv(clog, 0, &rnode2, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "securelevel",
		       SYSCTL_DESCR("System security level"),
		       secmodel_securelevel_sysctl, 0, NULL, 0,
		       CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, &rnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "securelevel", NULL,
		       NULL, 0, NULL, 0,
		       CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "name", NULL,
		       NULL, 0, __UNCONST(SECMODEL_SECURELEVEL_NAME), 0,
		       CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "securelevel",
		       SYSCTL_DESCR("System security level"),
		       secmodel_securelevel_sysctl, 0, NULL, 0,
		       CTL_CREATE, CTL_EOL);

	/* Compatibility: kern.securelevel */
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "kern", NULL,
		       NULL, 0, NULL, 0,
		       CTL_KERN, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "securelevel",
		       SYSCTL_DESCR("System security level"),
		       secmodel_securelevel_sysctl, 0, NULL, 0,
		       CTL_KERN, KERN_SECURELVL, CTL_EOL);
}

void
secmodel_securelevel_init(void)
{
#ifdef INSECURE
	securelevel = -1;
#else
	securelevel = 0;
#endif /* INSECURE */
}

void
secmodel_securelevel_start(void)
{
	l_system = kauth_listen_scope(KAUTH_SCOPE_SYSTEM,
	    secmodel_securelevel_system_cb, NULL);
	l_process = kauth_listen_scope(KAUTH_SCOPE_PROCESS,
	    secmodel_securelevel_process_cb, NULL);
	l_network = kauth_listen_scope(KAUTH_SCOPE_NETWORK,
	    secmodel_securelevel_network_cb, NULL);
	l_machdep = kauth_listen_scope(KAUTH_SCOPE_MACHDEP,
	    secmodel_securelevel_machdep_cb, NULL);
	l_device = kauth_listen_scope(KAUTH_SCOPE_DEVICE,
	    secmodel_securelevel_device_cb, NULL);
	l_vnode = kauth_listen_scope(KAUTH_SCOPE_VNODE,
	    secmodel_securelevel_vnode_cb, NULL);
}

void
secmodel_securelevel_stop(void)
{
	kauth_unlisten_scope(l_system);
	kauth_unlisten_scope(l_process);
	kauth_unlisten_scope(l_network);
	kauth_unlisten_scope(l_machdep);
	kauth_unlisten_scope(l_device);
	kauth_unlisten_scope(l_vnode);
}

static int
securelevel_eval(const char *what, void *arg, void *ret)
{
	int error = 0;

	if (strcasecmp(what, "is-securelevel-above") == 0) {
		int level = (int)(uintptr_t)arg;
		bool *bp = ret;

		*bp = (securelevel > level);
	} else {
		error = ENOENT;
	}

	return error;
}

static int
securelevel_modcmd(modcmd_t cmd, void *arg)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
		secmodel_securelevel_init();
		error = secmodel_register(&securelevel_sm,
		    SECMODEL_SECURELEVEL_ID, SECMODEL_SECURELEVEL_NAME,
		    NULL, securelevel_eval, NULL);
		if (error != 0)
			printf("securelevel_modcmd::init: secmodel_register "
			    "returned %d\n", error);

		secmodel_securelevel_start();
		sysctl_security_securelevel_setup(&securelevel_sysctl_log);
		break;

	case MODULE_CMD_FINI:
		sysctl_teardown(&securelevel_sysctl_log);
		secmodel_securelevel_stop();

		error = secmodel_deregister(securelevel_sm);
		if (error != 0)
			printf("securelevel_modcmd::fini: secmodel_deregister "
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

/*
 * kauth(9) listener
 *
 * Security model: Traditional NetBSD
 * Scope: System
 * Responsibility: Securelevel
 */
int
secmodel_securelevel_system_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1, void *arg2, void *arg3)
{
	int result;
	enum kauth_system_req req;

	result = KAUTH_RESULT_DEFER;
	req = (enum kauth_system_req)arg0;

	switch (action) {
	case KAUTH_SYSTEM_CHSYSFLAGS:
		/* Deprecated. */
		if (securelevel > 0)
			result = KAUTH_RESULT_DENY;
		break;

	case KAUTH_SYSTEM_TIME:
		switch (req) {
		case KAUTH_REQ_SYSTEM_TIME_RTCOFFSET:
			if (securelevel > 0)
				result = KAUTH_RESULT_DENY;
			break;

		case KAUTH_REQ_SYSTEM_TIME_SYSTEM: {
			struct timespec *ts = arg1;
			struct timespec *delta = arg2;

			if (securelevel > 1 && time_wraps(ts, delta))
				result = KAUTH_RESULT_DENY;

			break;
		}

		default:
			break;
		}
		break;

	case KAUTH_SYSTEM_MAP_VA_ZERO:
		if (securelevel > 0)
			result = KAUTH_RESULT_DENY;
		break;

	case KAUTH_SYSTEM_MODULE:
		if (securelevel > 0)
			result = KAUTH_RESULT_DENY;
		break;

	case KAUTH_SYSTEM_MOUNT:
		switch (req) {
		case KAUTH_REQ_SYSTEM_MOUNT_NEW:
			if (securelevel > 1)
				result = KAUTH_RESULT_DENY;

			break;

		case KAUTH_REQ_SYSTEM_MOUNT_UPDATE:
			if (securelevel > 1) {
				struct mount *mp = arg1;
				u_long flags = (u_long)arg2;

				/* Can only degrade from read/write to read-only. */
				if (flags != (mp->mnt_flag | MNT_RDONLY | MNT_RELOAD |
				    MNT_FORCE | MNT_UPDATE))
					result = KAUTH_RESULT_DENY;
			}

			break;

		default:
			break;
		}

		break;

	case KAUTH_SYSTEM_SYSCTL:
		switch (req) {
		case KAUTH_REQ_SYSTEM_SYSCTL_ADD:
		case KAUTH_REQ_SYSTEM_SYSCTL_DELETE:
		case KAUTH_REQ_SYSTEM_SYSCTL_DESC:
			if (securelevel > 0)
				result = KAUTH_RESULT_DENY;
			break;

		default:
			break;
		}
		break;

	case KAUTH_SYSTEM_SETIDCORE:
		if (securelevel > 0)
			result = KAUTH_RESULT_DENY;
		break;

	case KAUTH_SYSTEM_DEBUG:
		switch (req) {
		case KAUTH_REQ_SYSTEM_DEBUG_IPKDB:
			if (securelevel > 0)
				result = KAUTH_RESULT_DENY;
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

/*
 * kauth(9) listener
 *
 * Security model: Traditional NetBSD
 * Scope: Process
 * Responsibility: Securelevel
 */
int
secmodel_securelevel_process_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1, void *arg2, void *arg3)
{
	struct proc *p;
	int result;

	result = KAUTH_RESULT_DEFER;
	p = arg0;

	switch (action) {
	case KAUTH_PROCESS_PROCFS: {
		enum kauth_process_req req;

		req = (enum kauth_process_req)arg2;
		switch (req) {
		case KAUTH_REQ_PROCESS_PROCFS_READ:
			break;

		case KAUTH_REQ_PROCESS_PROCFS_RW:
		case KAUTH_REQ_PROCESS_PROCFS_WRITE:
			if ((p == initproc) && (securelevel > -1))
				result = KAUTH_RESULT_DENY;

			break;

		default:
			break;
		}

		break;
		}

	case KAUTH_PROCESS_PTRACE:
		if ((p == initproc) && (securelevel > -1))
			result = KAUTH_RESULT_DENY;

		break;

	case KAUTH_PROCESS_CORENAME:
		if (securelevel > 1)
			result = KAUTH_RESULT_DENY;
		break;

	default:
		break;
	}

	return (result);
}

/*
 * kauth(9) listener
 *
 * Security model: Traditional NetBSD
 * Scope: Network
 * Responsibility: Securelevel
 */
int
secmodel_securelevel_network_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1, void *arg2, void *arg3)
{
	int result;
	enum kauth_network_req req;

	result = KAUTH_RESULT_DEFER;
	req = (enum kauth_network_req)arg0;

	switch (action) {
	case KAUTH_NETWORK_FIREWALL:
		switch (req) {
		case KAUTH_REQ_NETWORK_FIREWALL_FW:
		case KAUTH_REQ_NETWORK_FIREWALL_NAT:
			if (securelevel > 1)
				result = KAUTH_RESULT_DENY;
			break;

		default:
			break;
		}
		break;

	case KAUTH_NETWORK_FORWSRCRT:
		if (securelevel > 0)
			result = KAUTH_RESULT_DENY;
		break;

	default:
		break;
	}

	return (result);
}

/*
 * kauth(9) listener
 *
 * Security model: Traditional NetBSD
 * Scope: Machdep
 * Responsibility: Securelevel
 */
int
secmodel_securelevel_machdep_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1, void *arg2, void *arg3)
{
	int result;

	result = KAUTH_RESULT_DEFER;

	switch (action) {
	case KAUTH_MACHDEP_IOPERM_SET:
	case KAUTH_MACHDEP_IOPL:
		if (securelevel > 0)
			result = KAUTH_RESULT_DENY;
		break;

	case KAUTH_MACHDEP_UNMANAGEDMEM:
		if (securelevel > 0)
			result = KAUTH_RESULT_DENY;
		break;

	case KAUTH_MACHDEP_CPU_UCODE_APPLY:
		if (securelevel > 1)
			result = KAUTH_RESULT_DENY;
		break;

	default:
		break;
	}

	return (result);
}

/*
 * kauth(9) listener
 *
 * Security model: Traditional NetBSD
 * Scope: Device
 * Responsibility: Securelevel
 */
int
secmodel_securelevel_device_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1, void *arg2, void *arg3)
{
	int result;

	result = KAUTH_RESULT_DEFER;

	switch (action) {
	case KAUTH_DEVICE_RAWIO_SPEC: {
		struct vnode *vp;
		enum kauth_device_req req;

		req = (enum kauth_device_req)arg0;
		vp = arg1;

		KASSERT(vp != NULL);

		/* Handle /dev/mem and /dev/kmem. */
		if (iskmemvp(vp)) {
			switch (req) {
			case KAUTH_REQ_DEVICE_RAWIO_SPEC_READ:
				break;

			case KAUTH_REQ_DEVICE_RAWIO_SPEC_WRITE:
			case KAUTH_REQ_DEVICE_RAWIO_SPEC_RW:
				if (securelevel > 0)
					result = KAUTH_RESULT_DENY;

				break;

			default:
				break;
			}

			break;
		}

		switch (req) {
		case KAUTH_REQ_DEVICE_RAWIO_SPEC_READ:
			break;

		case KAUTH_REQ_DEVICE_RAWIO_SPEC_WRITE:
		case KAUTH_REQ_DEVICE_RAWIO_SPEC_RW: {
			int error;

			error = rawdev_mounted(vp, NULL);

			/* Not a disk. */
			if (error == EINVAL)
				break;

			if (error && securelevel > 0)
				result = KAUTH_RESULT_DENY;

			if (securelevel > 1)
				result = KAUTH_RESULT_DENY;

			break;
			}

		default:
			break;
		}

		break;
		}

	case KAUTH_DEVICE_RAWIO_PASSTHRU:
		if (securelevel > 0) {
			u_long bits;

			bits = (u_long)arg0;

			KASSERT(bits != 0);
			KASSERT((bits & ~KAUTH_REQ_DEVICE_RAWIO_PASSTHRU_ALL) == 0);

			if (bits & ~KAUTH_REQ_DEVICE_RAWIO_PASSTHRU_READCONF)
				result = KAUTH_RESULT_DENY;
		}

		break;

	case KAUTH_DEVICE_GPIO_PINSET:
		if (securelevel > 0)
			result = KAUTH_RESULT_DENY;
		break;

	case KAUTH_DEVICE_RND_ADDDATA_ESTIMATE:
		if (securelevel > 0)
			result = KAUTH_RESULT_DENY;
		break;

	default:
		break;
	}

	return (result);
}

int
secmodel_securelevel_vnode_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1, void *arg2, void *arg3)
{
	int result;

	result = KAUTH_RESULT_DEFER;

	if ((action & KAUTH_VNODE_WRITE_SYSFLAGS) &&
	    (action & KAUTH_VNODE_HAS_SYSFLAGS)) {
		if (securelevel > 0)
			result = KAUTH_RESULT_DENY;
	}

	return (result);
}

