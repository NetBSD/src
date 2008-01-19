/* $NetBSD: secmodel_bsd44_suser.c,v 1.42.6.3 2008/01/19 12:15:39 bouyer Exp $ */
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
 * NetBSD superuser access restrictions.
 *
 * There are two main resources a request can be issued to: user-owned and
 * system owned. For the first, traditional Unix access checks are done, as
 * well as superuser checks. If needed, the request context is examined before
 * a decision is made. For the latter, usually only superuser checks are done
 * as normal users are not allowed to access system resources.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: secmodel_bsd44_suser.c,v 1.42.6.3 2008/01/19 12:15:39 bouyer Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/kauth.h>

#include <sys/acct.h>
#include <sys/mutex.h>
#include <sys/ktrace.h>
#include <sys/mount.h>
#include <sys/socketvar.h>
#include <sys/sysctl.h>
#include <sys/tty.h>
#include <net/route.h>
#include <sys/ptrace.h>
#include <sys/vnode.h>
#include <sys/proc.h>

#include <miscfs/procfs/procfs.h>

#include <secmodel/bsd44/suser.h>

extern int dovfsusermount;

static kauth_listener_t l_generic, l_system, l_process, l_network, l_machdep,
    l_device;

void
secmodel_bsd44_suser_start(void)
{
	l_generic = kauth_listen_scope(KAUTH_SCOPE_GENERIC,
	    secmodel_bsd44_suser_generic_cb, NULL);
	l_system = kauth_listen_scope(KAUTH_SCOPE_SYSTEM,
	    secmodel_bsd44_suser_system_cb, NULL);
	l_process = kauth_listen_scope(KAUTH_SCOPE_PROCESS,
	    secmodel_bsd44_suser_process_cb, NULL);
	l_network = kauth_listen_scope(KAUTH_SCOPE_NETWORK,
	    secmodel_bsd44_suser_network_cb, NULL);
	l_machdep = kauth_listen_scope(KAUTH_SCOPE_MACHDEP,
	    secmodel_bsd44_suser_machdep_cb, NULL);
	l_device = kauth_listen_scope(KAUTH_SCOPE_DEVICE,
	    secmodel_bsd44_suser_device_cb, NULL);
}

#if defined(_LKM)
void
secmodel_bsd44_suser_stop(void)
{
	kauth_unlisten_scope(l_generic);
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
 * Scope: Generic
 * Responsibility: Superuser access
 */
int
secmodel_bsd44_suser_generic_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1,
    void *arg2, void *arg3)
{
	bool isroot;
	int result;

	isroot = (kauth_cred_geteuid(cred) == 0);
	result = KAUTH_RESULT_DENY;

	switch (action) {
	case KAUTH_GENERIC_ISSUSER:
		if (isroot)
			result = KAUTH_RESULT_ALLOW;
		break;

	case KAUTH_GENERIC_CANSEE:     
		if (!secmodel_bsd44_curtain)
			result = KAUTH_RESULT_ALLOW;
		else if (isroot || kauth_cred_uidmatch(cred, arg0))
			result = KAUTH_RESULT_ALLOW;

		break;

	default:
		result = KAUTH_RESULT_DEFER;
		break;
	}

	return (result);
}

/*
 * kauth(9) listener
 *
 * Security model: Traditional NetBSD
 * Scope: System
 * Responsibility: Superuser access
 */
int
secmodel_bsd44_suser_system_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1,
    void *arg2, void *arg3)
{
	bool isroot;
	int result;
	enum kauth_system_req req;

	isroot = (kauth_cred_geteuid(cred) == 0);
	result = KAUTH_RESULT_DENY;
	req = (enum kauth_system_req)arg0;

	switch (action) {
	case KAUTH_SYSTEM_MOUNT:
		switch (req) {
		case KAUTH_REQ_SYSTEM_MOUNT_GET:
			result = KAUTH_RESULT_ALLOW;
			break;

		case KAUTH_REQ_SYSTEM_MOUNT_NEW:
			if (isroot)
				result = KAUTH_RESULT_ALLOW;
			else if (dovfsusermount) {
				struct vnode *vp = arg1;
				u_long flags = (u_long)arg2;

				if (!(flags & MNT_NODEV) ||
				    !(flags & MNT_NOSUID))
					break;

				if ((vp->v_mount->mnt_flag & MNT_NOEXEC) &&
				    !(flags & MNT_NOEXEC))
					break;

				result = KAUTH_RESULT_ALLOW;
			}

			break;

		case KAUTH_REQ_SYSTEM_MOUNT_UNMOUNT:
			if (isroot)
				result = KAUTH_RESULT_ALLOW;
			else {
				struct mount *mp = arg1;

				if (mp->mnt_stat.f_owner ==
				    kauth_cred_geteuid(cred))
					result = KAUTH_RESULT_ALLOW;
			}

			break;

		case KAUTH_REQ_SYSTEM_MOUNT_UPDATE:
			if (isroot)
				result = KAUTH_RESULT_ALLOW;
			else if (dovfsusermount) {
				struct mount *mp = arg1;
				u_long flags = (u_long)arg2;

				/* No exporting for non-root. */
				if (flags & MNT_EXPORTED)
					break;

				if (!(flags & MNT_NODEV) ||
				    !(flags & MNT_NOSUID))
					break;

				/*
				 * Only super-user, or user that did the mount,
				 * can update.
				 */
				if (mp->mnt_stat.f_owner !=
				    kauth_cred_geteuid(cred))
					break;

				/* Retain 'noexec'. */
				if ((mp->mnt_flag & MNT_NOEXEC) &&
				    !(flags & MNT_NOEXEC))
					break;

				result = KAUTH_RESULT_ALLOW;
			}

			break;

		default:
			result = KAUTH_RESULT_DEFER;
			break;
		}

		break;

	case KAUTH_SYSTEM_TIME:
		switch (req) {
		case KAUTH_REQ_SYSTEM_TIME_ADJTIME:
		case KAUTH_REQ_SYSTEM_TIME_NTPADJTIME:
		case KAUTH_REQ_SYSTEM_TIME_TIMECOUNTERS:
			if (isroot)
				result = KAUTH_RESULT_ALLOW;
			break;

		case KAUTH_REQ_SYSTEM_TIME_SYSTEM: {
			bool device_context = (bool)arg3;

			if (device_context || isroot)
				result = KAUTH_RESULT_ALLOW;

			break;
		}

		case KAUTH_REQ_SYSTEM_TIME_RTCOFFSET:
			/*
			 * Decisions here are root-agnostic.
			 *
			 * KAUTH_REQ_SYSTEM_TIME_RTCOFFSET - Should be used
			 *  only after the caller was determined as someone
			 *  who can modify sysctl. For us, this means root.
			 */
			result = KAUTH_RESULT_ALLOW;
			break;

		default:
			result = KAUTH_RESULT_DEFER;
			break;
		}
		break;

	case KAUTH_SYSTEM_SYSCTL:
		switch (req) {
		case KAUTH_REQ_SYSTEM_SYSCTL_ADD:
		case KAUTH_REQ_SYSTEM_SYSCTL_DELETE:
		case KAUTH_REQ_SYSTEM_SYSCTL_DESC:
		case KAUTH_REQ_SYSTEM_SYSCTL_PRVT:
			if (isroot)
				result = KAUTH_RESULT_ALLOW;
			break;

		default:
			break;
		}

		break;

	case KAUTH_SYSTEM_SWAPCTL:
	case KAUTH_SYSTEM_ACCOUNTING:
	case KAUTH_SYSTEM_REBOOT:
	case KAUTH_SYSTEM_CHROOT:
	case KAUTH_SYSTEM_FILEHANDLE:
	case KAUTH_SYSTEM_MKNOD:
		if (isroot)
			result = KAUTH_RESULT_ALLOW;
		break;

	case KAUTH_SYSTEM_DEBUG:
		switch (req) {
		case KAUTH_REQ_SYSTEM_DEBUG_IPKDB:
		default:
			/* Decisions are root-agnostic. */
			result = KAUTH_RESULT_ALLOW;
			break;
		}

		break;

	case KAUTH_SYSTEM_CHSYSFLAGS:
	case KAUTH_SYSTEM_LKM:
	case KAUTH_SYSTEM_SETIDCORE:
		/*
		 * Decisions here are root-agnostic.
		 *
		 * CHSYSFLAGS - Should be used only after the caller was
		 *              determined as root. Needs to be re-factored
		 *              anyway. Infects ufs, ext2fs, tmpfs, and rump.
		 *
		 * LKM - Subject to permissions on /dev/lkm for now.
		 *
		 * SETIDCORE - Should be used only after the caller was
		 *             determined as someone who can modify sysctl
		 *             data. For us, this means root.
		 */
		result = KAUTH_RESULT_ALLOW;
		break;

	case KAUTH_SYSTEM_MODULE:
		if (isroot)
			result = KAUTH_RESULT_ALLOW;
		break;

	default:
		result = KAUTH_RESULT_DEFER;
		break;
	}

	return (result);
}

/*
 * common code for corename, rlimit, and stopflag.
 */
static int
proc_uidmatch(kauth_cred_t cred, kauth_cred_t target)
{
	int r = 0;

	if (kauth_cred_getuid(cred) != kauth_cred_getuid(target) ||
	    kauth_cred_getuid(cred) != kauth_cred_getsvuid(target)) {
		/*
		 * suid proc of ours or proc not ours
		 */
		r = EPERM;
	} else if (kauth_cred_getgid(target) != kauth_cred_getsvgid(target)) {
		/*
		 * sgid proc has sgid back to us temporarily
		 */
		r = EPERM;
	} else {
		/*
		 * our rgid must be in target's group list (ie,
		 * sub-processes started by a sgid process)
		 */
		int ismember = 0;

		if (kauth_cred_ismember_gid(cred,
		    kauth_cred_getgid(target), &ismember) != 0 ||
		    !ismember)
			r = EPERM;
	}

	return (r);
}

/*
 * kauth(9) listener
 *
 * Security model: Traditional NetBSD
 * Scope: Process
 * Responsibility: Superuser access
 */
int
secmodel_bsd44_suser_process_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1, void *arg2, void *arg3)
{
	struct proc *p;
	bool isroot;
	int result;

	isroot = (kauth_cred_geteuid(cred) == 0);
	result = KAUTH_RESULT_DENY;
	p = arg0;

	switch (action) {
	case KAUTH_PROCESS_CANSIGNAL: {
		int signum;

		signum = (int)(unsigned long)arg1;

		if (isroot || kauth_cred_uidmatch(cred, p->p_cred) ||
		    (signum == SIGCONT && (curproc->p_session == p->p_session)))
			result = KAUTH_RESULT_ALLOW;
		break;
		}

	case KAUTH_PROCESS_CANSEE:
		if (!secmodel_bsd44_curtain)
			result = KAUTH_RESULT_ALLOW;
		else if (isroot || kauth_cred_uidmatch(cred, p->p_cred))
			result = KAUTH_RESULT_ALLOW;
		break;

	case KAUTH_PROCESS_CANKTRACE:
		if (isroot) {
			result = KAUTH_RESULT_ALLOW;
			break;
		}

		if ((p->p_traceflag & KTRFAC_ROOT) || (p->p_flag & PK_SUGID)) {
			result = KAUTH_RESULT_DENY;
			break;
		}

		if (kauth_cred_geteuid(cred) == kauth_cred_getuid(p->p_cred) &&
		    kauth_cred_getuid(cred) == kauth_cred_getsvuid(p->p_cred) &&
		    kauth_cred_getgid(cred) == kauth_cred_getgid(p->p_cred) &&
		    kauth_cred_getgid(cred) == kauth_cred_getsvgid(p->p_cred)) {
			result = KAUTH_RESULT_ALLOW;
			break;
		}

		result = KAUTH_RESULT_DENY;
		break;

	case KAUTH_PROCESS_CANPROCFS: {
		enum kauth_process_req req = (enum kauth_process_req)arg2;
		struct pfsnode *pfs = arg1;

		if (isroot) {
			result = KAUTH_RESULT_ALLOW;
			break;
		}

		if (req == KAUTH_REQ_PROCESS_CANPROCFS_CTL) {
			result = KAUTH_RESULT_DENY;
			break;
		}

		switch (pfs->pfs_type) {
		case PFSregs:
		case PFSfpregs:
		case PFSmem:
			if (kauth_cred_getuid(cred) !=
			    kauth_cred_getuid(p->p_cred) ||
			    ISSET(p->p_flag, PK_SUGID)) {
				result = KAUTH_RESULT_DENY;
				break;
			}
			/*FALLTHROUGH*/
		default:
			result = KAUTH_RESULT_ALLOW;
			break;
		}

		break;
		}

	case KAUTH_PROCESS_CANPTRACE: {
		switch ((u_long)arg1) {
		case PT_TRACE_ME:
		case PT_ATTACH:
		case PT_WRITE_I:
		case PT_WRITE_D:
		case PT_READ_I:
		case PT_READ_D:
		case PT_IO:
#ifdef PT_GETREGS
		case PT_GETREGS:
#endif
#ifdef PT_SETREGS
		case PT_SETREGS:
#endif
#ifdef PT_GETFPREGS
		case PT_GETFPREGS:
#endif
#ifdef PT_SETFPREGS
		case PT_SETFPREGS:
#endif
#ifdef __HAVE_PTRACE_MACHDEP
		PTRACE_MACHDEP_REQUEST_CASES
#endif
			if (isroot) {
				result = KAUTH_RESULT_ALLOW;
				break;
			}

			if (kauth_cred_getuid(cred) !=
			    kauth_cred_getuid(p->p_cred) ||
			    ISSET(p->p_flag, PK_SUGID)) {
				result = KAUTH_RESULT_DENY;
				break;
			}

			result = KAUTH_RESULT_ALLOW;
			break;

#ifdef PT_STEP
		case PT_STEP:
#endif
		case PT_CONTINUE:
		case PT_KILL:
		case PT_DETACH:
		case PT_LWPINFO:
		case PT_SYSCALL:
		case PT_DUMPCORE:
			result = KAUTH_RESULT_ALLOW;
			break;

		default:
	        	result = KAUTH_RESULT_DEFER;
		        break;
		}

		break;
		}

	case KAUTH_PROCESS_CORENAME:
		result = KAUTH_RESULT_ALLOW;

		if (isroot)
			break;

		if (proc_uidmatch(cred, p->p_cred) != 0) {
			result = KAUTH_RESULT_DENY;
			break;
		}

		break;

	case KAUTH_PROCESS_FORK: {
		int lnprocs = (int)(unsigned long)arg2;

		/*
		 * Don't allow a nonprivileged user to use the last few
		 * processes. The variable lnprocs is the current number of
		 * processes, maxproc is the limit.
		 */
		if (__predict_false((lnprocs >= maxproc - 5) && !isroot))
			result = KAUTH_RESULT_DENY;
		else
			result = KAUTH_RESULT_ALLOW;

		break;
		}

	case KAUTH_PROCESS_NICE:
		if (isroot) {
			result = KAUTH_RESULT_ALLOW;
			break;
		}

		if (kauth_cred_geteuid(cred) !=
		    kauth_cred_geteuid(p->p_cred) &&
		    kauth_cred_getuid(cred) !=
		    kauth_cred_geteuid(p->p_cred)) {
			result = KAUTH_RESULT_DENY;
			break;
		}

		if ((u_long)arg1 >= p->p_nice)
			result = KAUTH_RESULT_ALLOW;

		break;

	case KAUTH_PROCESS_RLIMIT: {
		struct rlimit *new_rlimit;
		u_long which;

		if (isroot) {
			result = KAUTH_RESULT_ALLOW;
			break;
		}

		if ((p != curlwp->l_proc) &&
		    (proc_uidmatch(cred, p->p_cred) != 0)) {
			result = KAUTH_RESULT_DENY;
			break;
		}

		new_rlimit = arg1;
		which = (u_long)arg2;

		if (new_rlimit->rlim_max <=
		    p->p_rlimit[which].rlim_max)
			result = KAUTH_RESULT_ALLOW;

		break;
		}

	case KAUTH_PROCESS_SETID:
		if (isroot)
			result = KAUTH_RESULT_ALLOW;
		break;

	case KAUTH_PROCESS_STOPFLAG:
		result = KAUTH_RESULT_ALLOW;

		if (isroot)
			break;

		if (proc_uidmatch(cred, p->p_cred) != 0) {
			result = KAUTH_RESULT_DENY;
			break;
		}
		break;

	default:
		result = KAUTH_RESULT_DEFER;
		break;
	}

	return (result);
}

/*
 * kauth(9) listener
 *
 * Security model: Traditional NetBSD
 * Scope: Network
 * Responsibility: Superuser access
 */
int
secmodel_bsd44_suser_network_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1, void *arg2,
    void *arg3)
{
	bool isroot;
	int result;
	enum kauth_network_req req;

	isroot = (kauth_cred_geteuid(cred) == 0);
	result = KAUTH_RESULT_DENY;
	req = (enum kauth_network_req)arg0;

	switch (action) {
	case KAUTH_NETWORK_ALTQ:
		switch (req) {
		case KAUTH_REQ_NETWORK_ALTQ_AFMAP:
		case KAUTH_REQ_NETWORK_ALTQ_BLUE:
		case KAUTH_REQ_NETWORK_ALTQ_CBQ:
		case KAUTH_REQ_NETWORK_ALTQ_CDNR:
		case KAUTH_REQ_NETWORK_ALTQ_CONF:
		case KAUTH_REQ_NETWORK_ALTQ_FIFOQ:
		case KAUTH_REQ_NETWORK_ALTQ_HFSC:
		case KAUTH_REQ_NETWORK_ALTQ_JOBS:
		case KAUTH_REQ_NETWORK_ALTQ_PRIQ:
		case KAUTH_REQ_NETWORK_ALTQ_RED:
		case KAUTH_REQ_NETWORK_ALTQ_RIO:
		case KAUTH_REQ_NETWORK_ALTQ_WFQ:
			if (isroot)
				result = KAUTH_RESULT_ALLOW;
			break;

		default:
			result = KAUTH_RESULT_DEFER;
			break;
		}

		break;

	case KAUTH_NETWORK_BIND:
		switch (req) {
		case KAUTH_REQ_NETWORK_BIND_PRIVPORT:
			if (isroot)
				result = KAUTH_RESULT_ALLOW;
			break;
		default:
			result = KAUTH_RESULT_ALLOW;
			break;
		}
		break;

	case KAUTH_NETWORK_FIREWALL:
		switch (req) {
		case KAUTH_REQ_NETWORK_FIREWALL_FW:
		case KAUTH_REQ_NETWORK_FIREWALL_NAT:
			/*
			 * Decisions are root-agnostic.
			 *
			 * Both requests are issued from the context of a
			 * device with permission bits acting as access
			 * control.
			 */
			result = KAUTH_RESULT_ALLOW;
			break;

		default:
			break;
		}
		break;

	case KAUTH_NETWORK_FORWSRCRT:
		/*
		 * Decision is root-agnostic.
		 *
		 * Can only be issued from sysctl context, in our case, only
		 * root can get here.
		 */
		result = KAUTH_RESULT_ALLOW;
		break;

	case KAUTH_NETWORK_INTERFACE:
		switch (req) {
		case KAUTH_REQ_NETWORK_INTERFACE_GET:
		case KAUTH_REQ_NETWORK_INTERFACE_SET:
			result = KAUTH_RESULT_ALLOW;
			break;

		case KAUTH_REQ_NETWORK_INTERFACE_GETPRIV:
		case KAUTH_REQ_NETWORK_INTERFACE_SETPRIV:
			if (isroot)
				result = KAUTH_RESULT_ALLOW;
			break;

		default:
			result = KAUTH_RESULT_DEFER;
			break;
		}
		break;

	case KAUTH_NETWORK_ROUTE:
		switch (((struct rt_msghdr *)arg1)->rtm_type) {
		case RTM_GET:
			result = KAUTH_RESULT_ALLOW;
			break;

		default:
			if (isroot)
				result = KAUTH_RESULT_ALLOW;
			break;
		}
		break;

	case KAUTH_NETWORK_SOCKET:
		switch (req) {
		case KAUTH_REQ_NETWORK_SOCKET_OPEN:
			if ((u_long)arg1 == PF_ROUTE || (u_long)arg1 == PF_BLUETOOTH)
				result = KAUTH_RESULT_ALLOW;
			else if ((u_long)arg2 == SOCK_RAW) {
				if (isroot)
					result = KAUTH_RESULT_ALLOW;
			} else
				result = KAUTH_RESULT_ALLOW;
			break;

		case KAUTH_REQ_NETWORK_SOCKET_RAWSOCK:
			if (isroot)
				result = KAUTH_RESULT_ALLOW;
			break;

		case KAUTH_REQ_NETWORK_SOCKET_CANSEE:
			if (secmodel_bsd44_curtain) {
				uid_t so_uid;

				so_uid =
				    ((struct socket *)arg1)->so_uidinfo->ui_uid;
				if (isroot ||
				    kauth_cred_geteuid(cred) == so_uid)
					result = KAUTH_RESULT_ALLOW;
			} else
				result = KAUTH_RESULT_ALLOW;
			break;

		default:
			result = KAUTH_RESULT_ALLOW;
			break;
		}

		break;

	default:
		result = KAUTH_RESULT_DEFER;
		break;
	}

	return (result);
}

/*
 * kauth(9) listener
 *
 * Security model: Traditional NetBSD
 * Scope: Machdep
 * Responsibility: Superuser access
 */
int
secmodel_bsd44_suser_machdep_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1, void *arg2,
    void *arg3)
{
        bool isroot;
        int result;

        isroot = (kauth_cred_geteuid(cred) == 0);
        result = KAUTH_RESULT_DENY;

        switch (action) {
	case KAUTH_MACHDEP_IOPERM_GET:
	case KAUTH_MACHDEP_LDT_GET:
	case KAUTH_MACHDEP_LDT_SET:
	case KAUTH_MACHDEP_MTRR_GET:
		result = KAUTH_RESULT_ALLOW;
		break;

	case KAUTH_MACHDEP_IOPERM_SET:
	case KAUTH_MACHDEP_IOPL:
	case KAUTH_MACHDEP_MTRR_SET:
	case KAUTH_MACHDEP_UNMANAGEDMEM:
		if (isroot)
			result = KAUTH_RESULT_ALLOW;
		break;

	default:
		result = KAUTH_RESULT_DEFER;
		break;
	}

	return (result);
}

/*
 * kauth(9) listener
 *
 * Security model: Traditional NetBSD
 * Scope: Device
 * Responsibility: Superuser access
 */
int
secmodel_bsd44_suser_device_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1, void *arg2,
    void *arg3)
{
	struct tty *tty;
        bool isroot;
        int result;

        isroot = (kauth_cred_geteuid(cred) == 0);
        result = KAUTH_RESULT_DENY;

	switch (action) {
	case KAUTH_DEVICE_RAWIO_SPEC:
	case KAUTH_DEVICE_RAWIO_PASSTHRU:
		/*
		 * Decision is root-agnostic.
		 *
		 * Both requests can be issued on devices subject to their
		 * permission bits.
		 */
		result = KAUTH_RESULT_ALLOW;
		break;

	case KAUTH_DEVICE_TTY_OPEN:
		tty = arg0;

		if (!(tty->t_state & TS_ISOPEN))
			result = KAUTH_RESULT_ALLOW;
		else if (tty->t_state & TS_XCLUDE) {
			if (isroot)
				result = KAUTH_RESULT_ALLOW;
		} else
			result = KAUTH_RESULT_ALLOW;

		break;

	case KAUTH_DEVICE_TTY_PRIVSET:
		if (isroot)
			result = KAUTH_RESULT_ALLOW;

		break;

	default:
		result = KAUTH_RESULT_DEFER;
		break;
	}

	return (result);
}
