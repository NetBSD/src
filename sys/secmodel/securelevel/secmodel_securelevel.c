/* $NetBSD: secmodel_securelevel.c,v 1.4.10.1 2008/01/02 21:57:57 bouyer Exp $ */
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
__KERNEL_RCSID(0, "$NetBSD: secmodel_securelevel.c,v 1.4.10.1 2008/01/02 21:57:57 bouyer Exp $");

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

#include <miscfs/specfs/specdev.h>

#include <secmodel/securelevel/securelevel.h>

static int securelevel;

static kauth_listener_t l_system, l_process, l_network, l_machdep, l_device;

/*
 * sysctl helper routine for securelevel. ensures that the value
 * only rises unless the caller has pid 1 (assumed to be init).
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
        
	if (newsecurelevel < securelevel && l && l->l_proc->p_pid != 1)
		return (EPERM);

	securelevel = newsecurelevel;

	return (error);
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

SYSCTL_SETUP(sysctl_security_securelevel_setup,
    "sysctl security securelevel setup")
{
	/*
	 * For compatibility, we create a kern.securelevel variable.
	 */
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
}

#if defined(_LKM)
void
secmodel_securelevel_stop(void)
{
	kauth_unlisten_scope(l_system);
	kauth_unlisten_scope(l_process);
	kauth_unlisten_scope(l_network);
	kauth_unlisten_scope(l_machdep);
	kauth_unlisten_scope(l_device);
}
#endif /* _LKM */

/*
 * kauth(9) listener
 *
 * Security model: Traditional NetBSD
 * Scope: System
 * Responsibility: Securelevel
 */
int
secmodel_securelevel_system_cb(kauth_cred_t cred,
    kauth_action_t action, void *cookie, void *arg0, void *arg1,
    void *arg2, void *arg3)
{
	int result;
	enum kauth_system_req req;

	result = KAUTH_RESULT_DEFER;
	req = (enum kauth_system_req)arg0;

	switch (action) {
	case KAUTH_SYSTEM_CHSYSFLAGS:
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
			struct timeval *delta = arg2;

			/*
			 * Don't allow the time to be set forward so far it will wrap
			 * and become negative, thus allowing an attacker to bypass
			 * the next check below.  The cutoff is 1 year before rollover
			 * occurs, so even if the attacker uses adjtime(2) to move
			 * the time past the cutoff, it will take a very long time
			 * to get to the wrap point.
			 *
			 * XXX: we check against INT_MAX since on 64-bit
			 *      platforms, sizeof(int) != sizeof(long) and
			 *      time_t is 32 bits even when atv.tv_sec is 64 bits.
			 */
			if (securelevel > 1 &&
			    ((ts->tv_sec > INT_MAX - 365*24*60*60) ||
			     (delta->tv_sec < 0 || delta->tv_usec < 0)))
				result = KAUTH_RESULT_DENY;

			break;
		}

		default:
			break;
		}
		break;

	case KAUTH_SYSTEM_LKM:
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
secmodel_securelevel_process_cb(kauth_cred_t cred,
    kauth_action_t action, void *cookie, void *arg0,
    void *arg1, void *arg2, void *arg3)
{
	struct proc *p;
	int result;

	result = KAUTH_RESULT_DEFER;
	p = arg0;

	switch (action) {
	case KAUTH_PROCESS_CANPROCFS: {
		enum kauth_process_req req;

		req = (enum kauth_process_req)arg2;
		switch (req) {
		case KAUTH_REQ_PROCESS_CANPROCFS_READ:
			break;

		case KAUTH_REQ_PROCESS_CANPROCFS_RW:
		case KAUTH_REQ_PROCESS_CANPROCFS_WRITE:
			if ((p == initproc) && (securelevel > -1))
				result = KAUTH_RESULT_DENY;

			break;

		default:
			break;
		}

		break;
		}

	case KAUTH_PROCESS_CANPTRACE:
		if ((p == initproc) && (securelevel >= 0))
			result = KAUTH_RESULT_DENY;

		break;

	case KAUTH_PROCESS_CORENAME:
		if (securelevel > 1)
			result = KAUTH_RESULT_DENY;
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
secmodel_securelevel_network_cb(kauth_cred_t cred,
    kauth_action_t action, void *cookie, void *arg0,
    void *arg1, void *arg2, void *arg3)
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
secmodel_securelevel_machdep_cb(kauth_cred_t cred,
    kauth_action_t action, void *cookie, void *arg0,
    void *arg1, void *arg2, void *arg3)
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
secmodel_securelevel_device_cb(kauth_cred_t cred,
    kauth_action_t action, void *cookie, void *arg0,
    void *arg1, void *arg2, void *arg3)
{
	int result;

	result = KAUTH_RESULT_DEFER;

	switch (action) {
	case KAUTH_DEVICE_RAWIO_SPEC: {
		struct vnode *vp, *bvp;
		enum kauth_device_req req;
		dev_t dev;
		int d_type;

		req = (enum kauth_device_req)arg0;
		vp = arg1;

		KASSERT(vp != NULL);

		dev = vp->v_un.vu_specinfo->si_rdev;
		d_type = D_OTHER;
		bvp = NULL;

		/* Handle /dev/mem and /dev/kmem. */
		if ((vp->v_type == VCHR) && iskmemdev(dev)) {
			switch (req) {
			case KAUTH_REQ_DEVICE_RAWIO_SPEC_READ:
				break;

			case KAUTH_REQ_DEVICE_RAWIO_SPEC_WRITE:
			case KAUTH_REQ_DEVICE_RAWIO_SPEC_RW:
				if (securelevel > 0)
					result = KAUTH_RESULT_DENY;
				break;
			}

			break;
		}

		switch (req) {
		case KAUTH_REQ_DEVICE_RAWIO_SPEC_READ:
			break;

		case KAUTH_REQ_DEVICE_RAWIO_SPEC_WRITE:
		case KAUTH_REQ_DEVICE_RAWIO_SPEC_RW:
			switch (vp->v_type) {
			case VCHR: {
				const struct cdevsw *cdev;

				cdev = cdevsw_lookup(dev);
				if (cdev != NULL) {
					dev_t blkdev;

					blkdev = devsw_chr2blk(dev);
					if (blkdev != NODEV) {
						vfinddev(blkdev, VBLK, &bvp);
						if (bvp != NULL)
							d_type = (cdev->d_flag
							    & D_TYPEMASK);
					}
				}

				break;
				}
			case VBLK: {
				const struct bdevsw *bdev;

				bdev = bdevsw_lookup(dev);
				if (bdev != NULL)
					d_type = (bdev->d_flag & D_TYPEMASK);

				bvp = vp;

				break;
				}

			default:
				break;
			}

			if (d_type != D_DISK)
				break;

			/*
			 * XXX: This is bogus. We should be failing the request
			 * XXX: not only if this specific slice is mounted, but
			 * XXX: if it's on a disk with any other mounted slice.
			 */
			if (vfs_mountedon(bvp) && (securelevel > 0))
				break;

			if (securelevel > 1)
				result = KAUTH_RESULT_DENY;

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
	}

	return (result);
}
