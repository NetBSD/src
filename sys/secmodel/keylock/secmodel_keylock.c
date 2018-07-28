/* $NetBSD: secmodel_keylock.c,v 1.8.28.1 2018/07/28 04:38:11 pgoyette Exp $ */
/*-
 * Copyright (c) 2009 Marc Balmer <marc@msys.ch>
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
 * This file contains kauth(9) listeners needed to implement an experimental
 * keylock based security scheme.
 *
 * The position of the keylock is a system-global indication on what
 * operations are allowed or not. It affects all users, including root.
 *
 * Rules:
 *
 * - If the number of possible keylock positions is 0, assume there is no
 *   keylock present, do not dissallow any action, i.e. do nothing
 *
 * - If the number of possible keylock positions is greater than 0, but the
 *   current lock position is 0, assume tampering with the lock and forbid
 *   all actions.
 *
 * - If the lock is in the lowest position, assume the system is locked and
 *   forbid most actions.
 *
 * - If the lock is in the highest position, assume the system to be open and
 *   forbid nothing.
 *
 * - If the security.models.keylock.order sysctl is set to a value != 0,
 *   reverse this order.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: secmodel_keylock.c,v 1.8.28.1 2018/07/28 04:38:11 pgoyette Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/kauth.h>

#include <sys/conf.h>
#include <sys/mount.h>
#include <sys/sysctl.h>
#include <sys/vnode.h>
#include <sys/timevar.h>

#include <dev/keylock.h>

#include <miscfs/specfs/specdev.h>

#include <secmodel/secmodel.h>
#include <secmodel/keylock/keylock.h>

static kauth_listener_t l_system, l_process, l_network, l_machdep, l_device;

static secmodel_t keylock_sm;

SYSCTL_SETUP(sysctl_security_keylock_setup,
    "sysctl security keylock setup")
{
	const struct sysctlnode *rnode;

	sysctl_createv(clog, 0, NULL, &rnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "models", NULL,
		       NULL, 0, NULL, 0,
		       CTL_SECURITY, CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, &rnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "keylock",
		       SYSCTL_DESCR("Keylock security model"),
		       NULL, 0, NULL, 0,
		       CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "name", NULL,
		       NULL, 0, __UNCONST("Keylock"), 0,
		       CTL_CREATE, CTL_EOL);
}

void
secmodel_keylock_init(void)
{
	int error = secmodel_register(&keylock_sm,
	    "org.netbsd.secmodel.keylock",
	    "NetBSD Security Model: Keylock", NULL, NULL, NULL);
	if (error != 0)
		printf("secmodel_keylock_init: secmodel_register "
		    "returned %d\n", error);
}

void
secmodel_keylock_start(void)
{
	l_system = kauth_listen_scope(KAUTH_SCOPE_SYSTEM,
	    secmodel_keylock_system_cb, NULL);
	l_process = kauth_listen_scope(KAUTH_SCOPE_PROCESS,
	    secmodel_keylock_process_cb, NULL);
	l_network = kauth_listen_scope(KAUTH_SCOPE_NETWORK,
	    secmodel_keylock_network_cb, NULL);
	l_machdep = kauth_listen_scope(KAUTH_SCOPE_MACHDEP,
	    secmodel_keylock_machdep_cb, NULL);
	l_device = kauth_listen_scope(KAUTH_SCOPE_DEVICE,
	    secmodel_keylock_device_cb, NULL);
}

void
secmodel_keylock_stop(void)
{
	int error;

	kauth_unlisten_scope(l_system);
	kauth_unlisten_scope(l_process);
	kauth_unlisten_scope(l_network);
	kauth_unlisten_scope(l_machdep);
	kauth_unlisten_scope(l_device);

	error = secmodel_deregister(keylock_sm);
	if (error != 0)
		printf("secmodel_keylock_stop: secmodel_deregister "
		    "returned %d\n", error);
}

/*
 * kauth(9) listener
 *
 * Security model: Multi-position keylock
 * Scope: System
 * Responsibility: Keylock
 */
int
secmodel_keylock_system_cb(kauth_cred_t cred,
    kauth_action_t action, void *cookie, void *arg0, void *arg1,
    void *arg2, void *arg3)
{
	int result;
	enum kauth_system_req req;
	int kstate;

	kstate = keylock_state();
	if (kstate == KEYLOCK_ABSENT)
		return KAUTH_RESULT_DEFER;
	else if (kstate == KEYLOCK_TAMPER)
		return KAUTH_RESULT_DENY;

	result = KAUTH_RESULT_DEFER;
	req = (enum kauth_system_req)arg0;

	switch (action) {
	case KAUTH_SYSTEM_CHSYSFLAGS:
		if (kstate == KEYLOCK_CLOSE)
			result = KAUTH_RESULT_DENY;
		break;

	case KAUTH_SYSTEM_TIME:
		switch (req) {
		case KAUTH_REQ_SYSTEM_TIME_RTCOFFSET:
			if (kstate == KEYLOCK_CLOSE)
				result = KAUTH_RESULT_DENY;
			break;

		case KAUTH_REQ_SYSTEM_TIME_SYSTEM: {
			struct timespec *ts = arg1;
			struct timespec *delta = arg2;

			if (keylock_position() > 1 && time_wraps(ts, delta))
				result = KAUTH_RESULT_DENY;
			break;
		}
		default:
			break;
		}
		break;

	case KAUTH_SYSTEM_MODULE:
		if (kstate == KEYLOCK_CLOSE)
			result = KAUTH_RESULT_DENY;
		break;

	case KAUTH_SYSTEM_MOUNT:
		switch (req) {
		case KAUTH_REQ_SYSTEM_MOUNT_NEW:
			if (kstate == KEYLOCK_CLOSE)
				result = KAUTH_RESULT_DENY;

			break;

		case KAUTH_REQ_SYSTEM_MOUNT_UPDATE:
			if (kstate == KEYLOCK_CLOSE) {
				struct mount *mp = arg1;
				u_long flags = (u_long)arg2;

				/*
				 * Can only degrade from read/write to
				 * read-only.
				 */
				if (flags != (mp->mnt_flag | MNT_RDONLY |
				    MNT_RELOAD | MNT_FORCE | MNT_UPDATE))
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
			if (kstate == KEYLOCK_CLOSE)
				result = KAUTH_RESULT_DENY;
			break;
		default:
			break;
		}
		break;

	case KAUTH_SYSTEM_SETIDCORE:
		if (kstate == KEYLOCK_CLOSE)
			result = KAUTH_RESULT_DENY;
		break;

	case KAUTH_SYSTEM_DEBUG:
		break;
	}

	return result;
}

/*
 * kauth(9) listener
 *
 * Security model: Multi-position keylock
 * Scope: Process
 * Responsibility: Keylock
 */
int
secmodel_keylock_process_cb(kauth_cred_t cred,
    kauth_action_t action, void *cookie, void *arg0,
    void *arg1, void *arg2, void *arg3)
{
	struct proc *p;
	int result, kstate;

	kstate = keylock_state();
	if (kstate == KEYLOCK_ABSENT)
		return KAUTH_RESULT_DEFER;
	else if (kstate == KEYLOCK_TAMPER)
		return KAUTH_RESULT_DENY;

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
			if ((p == initproc) && (kstate != KEYLOCK_OPEN))
				result = KAUTH_RESULT_DENY;

			break;
		default:
			break;
		}

		break;
		}

	case KAUTH_PROCESS_PTRACE:
		if ((p == initproc) && (kstate != KEYLOCK_OPEN))
			result = KAUTH_RESULT_DENY;

		break;

	case KAUTH_PROCESS_CORENAME:
		if (kstate == KEYLOCK_CLOSE)
			result = KAUTH_RESULT_DENY;
		break;
	}
	return result;
}

/*
 * kauth(9) listener
 *
 * Security model: Multi-position keylock
 * Scope: Network
 * Responsibility: Keylock
 */
int
secmodel_keylock_network_cb(kauth_cred_t cred,
    kauth_action_t action, void *cookie, void *arg0,
    void *arg1, void *arg2, void *arg3)
{
	int result, kstate;
	enum kauth_network_req req;

	kstate = keylock_state();
	if (kstate == KEYLOCK_ABSENT)
		return KAUTH_RESULT_DEFER;
	else if (kstate == KEYLOCK_TAMPER)
		return KAUTH_RESULT_DENY;

	result = KAUTH_RESULT_DEFER;
	req = (enum kauth_network_req)arg0;

	switch (action) {
	case KAUTH_NETWORK_FIREWALL:
		switch (req) {
		case KAUTH_REQ_NETWORK_FIREWALL_FW:
		case KAUTH_REQ_NETWORK_FIREWALL_NAT:
			if (kstate == KEYLOCK_CLOSE)
				result = KAUTH_RESULT_DENY;
			break;

		default:
			break;
		}
		break;

	case KAUTH_NETWORK_FORWSRCRT:
		if (kstate != KEYLOCK_OPEN)
			result = KAUTH_RESULT_DENY;
		break;
	}

	return result;
}

/*              
 * kauth(9) listener
 *
 * Security model: Multi-position keylock
 * Scope: Machdep
 * Responsibility: Keylock
 */
int
secmodel_keylock_machdep_cb(kauth_cred_t cred,
    kauth_action_t action, void *cookie, void *arg0,
    void *arg1, void *arg2, void *arg3)
{
        int result, kstate;

	kstate = keylock_state();
	if (kstate == KEYLOCK_ABSENT)
		return KAUTH_RESULT_DEFER;
	else if (kstate == KEYLOCK_TAMPER)
		return KAUTH_RESULT_DENY;

        result = KAUTH_RESULT_DEFER;

        switch (action) {
	case KAUTH_MACHDEP_IOPERM_SET:
	case KAUTH_MACHDEP_IOPL:
		if (kstate != KEYLOCK_OPEN)
			result = KAUTH_RESULT_DENY;
		break;

	case KAUTH_MACHDEP_UNMANAGEDMEM:
		if (kstate != KEYLOCK_OPEN)
			result = KAUTH_RESULT_DENY;
		break;
	}

	return result;
}

/*
 * kauth(9) listener
 *
 * Security model: Multi-position keylock
 * Scope: Device 
 * Responsibility: Keylock
 */
int
secmodel_keylock_device_cb(kauth_cred_t cred,
    kauth_action_t action, void *cookie, void *arg0,
    void *arg1, void *arg2, void *arg3)
{
	int result, kstate, error;

	kstate = keylock_state();
	if (kstate == KEYLOCK_ABSENT)
		return KAUTH_RESULT_DEFER;
	else if (kstate == KEYLOCK_TAMPER)
		return KAUTH_RESULT_DENY;

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
				if (kstate != KEYLOCK_OPEN)
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
		case KAUTH_REQ_DEVICE_RAWIO_SPEC_RW:
			error = rawdev_mounted(vp, NULL);

			if (error == EINVAL)
				break;

			if (error && (kstate != KEYLOCK_OPEN))
				break;

			if (kstate == KEYLOCK_CLOSE)
				result = KAUTH_RESULT_DENY;

			break;
		default:
			break;
		}
		break;
		}

	case KAUTH_DEVICE_RAWIO_PASSTHRU:
		if (kstate != KEYLOCK_OPEN) {
			u_long bits;

			bits = (u_long)arg0;

			KASSERT(bits != 0);
			KASSERT((bits & ~KAUTH_REQ_DEVICE_RAWIO_PASSTHRU_ALL)
			    == 0);

			if (bits & ~KAUTH_REQ_DEVICE_RAWIO_PASSTHRU_READCONF)
				result = KAUTH_RESULT_DENY;
		}
		break;

	case KAUTH_DEVICE_GPIO_PINSET:
		if (kstate != KEYLOCK_OPEN)
			result = KAUTH_RESULT_DENY;
		break;
	default:
		break;
	}
	return result;
}
