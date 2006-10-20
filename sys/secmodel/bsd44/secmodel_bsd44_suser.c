/* $NetBSD: secmodel_bsd44_suser.c,v 1.10 2006/10/20 22:02:54 elad Exp $ */
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Elad Efrat.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 5. Use of the code by Wasabi Systems Inc. is hereby prohibited without
 *    written approval from the author.
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
__KERNEL_RCSID(0, "$NetBSD: secmodel_bsd44_suser.c,v 1.10 2006/10/20 22:02:54 elad Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/kauth.h>

#include <sys/acct.h>
#include <sys/ktrace.h>
#include <sys/mount.h>
#include <sys/socketvar.h>
#include <sys/sysctl.h>
#include <sys/tty.h>
#include <net/route.h>

#include <secmodel/bsd44/suser.h>

void
secmodel_bsd44_suser_start(void)
{
	kauth_listen_scope(KAUTH_SCOPE_GENERIC,
	    secmodel_bsd44_suser_generic_cb, NULL);
	kauth_listen_scope(KAUTH_SCOPE_SYSTEM,
	    secmodel_bsd44_suser_system_cb, NULL);
	kauth_listen_scope(KAUTH_SCOPE_PROCESS,
	    secmodel_bsd44_suser_process_cb, NULL);
	kauth_listen_scope(KAUTH_SCOPE_NETWORK,
	    secmodel_bsd44_suser_network_cb, NULL);
	kauth_listen_scope(KAUTH_SCOPE_MACHDEP,
	    secmodel_bsd44_suser_machdep_cb, NULL);
	kauth_listen_scope(KAUTH_SCOPE_DEVICE,
	    secmodel_bsd44_suser_device_cb, NULL);
}

/*
 * kauth(9) listener
 *
 * Security model: Traditional NetBSD
 * Scope: Generic
 * Responsibility: Superuser access
 */
int
secmodel_bsd44_suser_generic_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie __unused, void *arg0 __unused, void *arg1 __unused,
    void *arg2 __unused, void *arg3 __unused)
{
	boolean_t isroot;
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
    void *cookie __unused, void *arg0 __unused, void *arg1 __unused,
    void *arg2 __unused, void *arg3 __unused)
{
	boolean_t isroot;
	int result;
	enum kauth_system_req req;

	isroot = (kauth_cred_geteuid(cred) == 0);
	result = KAUTH_RESULT_DENY;
	req = (enum kauth_system_req)arg0;

	switch (action) {
	case KAUTH_SYSTEM_TIME:
		switch (req) {
		case KAUTH_REQ_SYSTEM_TIME_ADJTIME:
		case KAUTH_REQ_SYSTEM_TIME_NTPADJTIME:
		case KAUTH_REQ_SYSTEM_TIME_SYSTEM:
			if (isroot)
				result = KAUTH_RESULT_ALLOW;
			break;

		default:
			result = KAUTH_RESULT_DEFER;
			break;
		}
		break;

	case KAUTH_SYSTEM_SYSCTL:
		if (isroot)
			result = KAUTH_RESULT_ALLOW;
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
 * Scope: Process
 * Responsibility: Superuser access
 */
int
secmodel_bsd44_suser_process_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie __unused, void *arg0, void *arg1, void *arg2, void *arg3)
{
	struct proc *p;
	boolean_t isroot;
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

	case KAUTH_PROCESS_RESOURCE:
		switch ((u_long)arg1) {
		case KAUTH_REQ_PROCESS_RESOURCE_NICE:
			if (isroot)
				result = KAUTH_RESULT_ALLOW;
			else if ((u_long)arg2 >= p->p_nice)
				result = KAUTH_RESULT_ALLOW; 
			break;

		case KAUTH_REQ_PROCESS_RESOURCE_RLIMIT:
			if (isroot)
				result = KAUTH_RESULT_ALLOW;
			else {
				struct rlimit *new_rlimit;
				u_long which;

				new_rlimit = arg2;
				which = (u_long)arg3;

				if (new_rlimit->rlim_max <=
				    p->p_rlimit[which].rlim_max)
					result = KAUTH_RESULT_ALLOW;
			}
			break;

		default:
			result = KAUTH_RESULT_DEFER;
			break;
		}
		break;

	case KAUTH_PROCESS_SETID:
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
 * Scope: Network
 * Responsibility: Superuser access
 */
int
secmodel_bsd44_suser_network_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie __unused, void *arg0, void *arg1, void *arg2 __unused,
    void *arg3 __unused)
{
	boolean_t isroot;
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

	case KAUTH_NETWORK_SOCKET:
		switch (req) {
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
    void *cookie __unused, void *arg0, void *arg1 __unused, void *arg2 __unused,
    void *arg3 __unused)
{
        boolean_t isroot;
        int result;
	enum kauth_machdep_req req;

        isroot = (kauth_cred_geteuid(cred) == 0);
        result = KAUTH_RESULT_DENY;
	req = (enum kauth_machdep_req)arg0;

        switch (action) {
	case KAUTH_MACHDEP_X86:
		switch (req) {
		case KAUTH_REQ_MACHDEP_X86_IOPL:
		case KAUTH_REQ_MACHDEP_X86_IOPERM:
		case KAUTH_REQ_MACHDEP_X86_MTRR_SET:
			if (isroot)
				result = KAUTH_RESULT_ALLOW;
			break;

		default:
			result = KAUTH_RESULT_DEFER;
			break;
		}
		break;

	case KAUTH_MACHDEP_X86_64:
		switch (req) {
		case KAUTH_REQ_MACHDEP_X86_64_MTRR_GET:
			if (isroot)
				result = KAUTH_RESULT_ALLOW;
			break;

		default:
			result = KAUTH_RESULT_DEFER;
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
 * Scope: Device
 * Responsibility: Superuser access
 */
int
secmodel_bsd44_suser_device_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie __unused, void *arg0, void *arg1 __unused, void *arg2 __unused,
    void *arg3 __unused)
{
	struct tty *tty;
        boolean_t isroot;
        int result;

        isroot = (kauth_cred_geteuid(cred) == 0);
        result = KAUTH_RESULT_DENY;

	switch (action) {
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
