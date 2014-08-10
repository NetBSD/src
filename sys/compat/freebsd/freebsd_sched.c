/*	$NetBSD: freebsd_sched.c,v 1.19.60.1 2014/08/10 06:54:32 tls Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center; by Matthias Scheler.
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

/*
 * FreeBSD compatibility module. Try to deal with scheduler related syscalls.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: freebsd_sched.c,v 1.19.60.1 2014/08/10 06:54:32 tls Exp $");

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/syscallargs.h>
#include <sys/kauth.h>

#include <sys/cpu.h>

#include <compat/freebsd/freebsd_syscallargs.h>
#include <compat/freebsd/freebsd_sched.h>

int
freebsd_sys_yield(struct lwp *l, const void *v, register_t *retval)
{

	yield();
	return 0;
}

/*
 * XXX: Needs adjustment to do a proper conversion.
 */
static int
sched_freebsd2native(int freebsd_policy,
    const struct freebsd_sched_param *freebsd_params, int *native_policy,
    struct sched_param *native_params)
{
	int p;

	switch (freebsd_policy) {
	case FREEBSD_SCHED_OTHER:
		p = SCHED_OTHER;
		break;

	case FREEBSD_SCHED_FIFO:
		p = SCHED_FIFO;
		break;
        
	case FREEBSD_SCHED_RR:
		p = SCHED_RR;
		break;

	default:
		return EINVAL;
	}
 
	if (native_policy != NULL)
		*native_policy = p;

	if (freebsd_params != NULL && native_params != NULL) {
		/* XXX: Needs adjustment to do a proper conversion. */
		native_params->sched_priority = freebsd_params->sched_priority;
	}
	return 0;
}

static int
sched_native2freebsd(int native_policy, const struct sched_param *native_params,
    int *freebsd_policy, struct freebsd_sched_param *freebsd_params)
{
	int p;

	switch (native_policy) {
	case SCHED_OTHER:
		p = FREEBSD_SCHED_OTHER;
		break;

	case SCHED_FIFO:
		p = FREEBSD_SCHED_FIFO;
		break;
        
	case SCHED_RR:
		p = FREEBSD_SCHED_RR;
		break;

	default:
		return EINVAL;
	}
 
	if (freebsd_policy != NULL)
		*freebsd_policy = p;

	if (native_params != NULL && freebsd_params != NULL) {
		/* XXX: Needs adjustment to do a proper conversion. */
		freebsd_params->sched_priority = native_params->sched_priority;
	}

	return 0;
}

int
freebsd_sys_sched_setparam(struct lwp *l, const struct freebsd_sys_sched_setparam_args *uap, register_t *retval)
{
	/* {
		syscallarg(pid_t) pid;
		syscallarg(const struct freebsd_sched_param *) sp;
	} */
	int error, policy;
	struct freebsd_sched_param lp;
	struct sched_param sp;

	if (SCARG(uap, pid) < 0 || SCARG(uap, sp) == NULL) {
		error = EINVAL;
		goto out;
	}

	error = copyin(SCARG(uap, sp), &lp, sizeof(lp));
	if (error)
		goto out;

	/* We need the current policy in FreeBSD terms. */
	error = do_sched_getparam(SCARG(uap, pid), 0, &policy, NULL);
	if (error)
		goto out;
	error = sched_native2freebsd(policy, NULL, &policy, NULL);
	if (error)
		goto out;

	error = sched_freebsd2native(policy, &lp, &policy, &sp);
	if (error)
		goto out;

	error = do_sched_setparam(SCARG(uap, pid), 0, policy, &sp);
	if (error)
		goto out;

 out:
	return error;
}

int
freebsd_sys_sched_getparam(struct lwp *l, const struct freebsd_sys_sched_getparam_args *uap, register_t *retval)
{
	/* {
		syscallarg(pid_t) pid;
		syscallarg(struct freebsd_sched_param *) sp;
	} */
	struct freebsd_sched_param lp;
	struct sched_param sp;
	int error;

	if (SCARG(uap, pid) < 0 || SCARG(uap, sp) == NULL) {
		error = EINVAL;
		goto out;
	}

	error = do_sched_getparam(SCARG(uap, pid), 0, NULL, &sp);
	if (error)
		goto out;

	error = sched_native2freebsd(0, &sp, NULL, &lp);
	if (error)
		goto out;

	error = copyout(&lp, SCARG(uap, sp), sizeof(lp));
	if (error)
		goto out;

 out:
	return error;
}

int
freebsd_sys_sched_setscheduler(struct lwp *l, const struct freebsd_sys_sched_setscheduler_args *uap, register_t *retval)
{
	/* {
		syscallarg(pid_t) pid;
		syscallarg(int) policy;
		syscallarg(cont struct freebsd_sched_scheduler *) sp;
	} */
	int error, policy;
	struct freebsd_sched_param lp;
	struct sched_param sp;

	if (SCARG(uap, pid) < 0 || SCARG(uap, sp) == NULL) {
 		error = EINVAL;
		goto out;
	}

	error = copyin(SCARG(uap, sp), &lp, sizeof(lp));
	if (error)
		goto out;

	error = sched_freebsd2native(SCARG(uap, policy), &lp, &policy, &sp);
	if (error)
		goto out;

	error = do_sched_setparam(SCARG(uap, pid), 0, policy, &sp);
	if (error)
		goto out;

 out:
	return error;
}

int
freebsd_sys_sched_getscheduler(struct lwp *l, const struct freebsd_sys_sched_getscheduler_args *uap, register_t *retval)
{
	/* {
		syscallarg(pid_t) pid;
	} */
	int error, policy;

	*retval = -1;

	error = do_sched_getparam(SCARG(uap, pid), 0, &policy, NULL);
	if (error)
		goto out;

	error = sched_native2freebsd(policy, NULL, &policy, NULL);
	if (error)
		goto out;

	*retval = policy;

 out:
	return error;
}

int
freebsd_sys_sched_yield(struct lwp *l, const void *v, register_t *retval)
{

	yield();
	return 0;
}

int
freebsd_sys_sched_get_priority_max(struct lwp *l, const struct freebsd_sys_sched_get_priority_max_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) policy;
	} */

	/*
	 * We can't emulate anything put the default scheduling policy.
	 */
	if (SCARG(uap, policy) != FREEBSD_SCHED_OTHER) {
		*retval = -1;
		return EINVAL;
	}

	*retval = 0;
	return 0;
}

int
freebsd_sys_sched_get_priority_min(struct lwp *l, const struct freebsd_sys_sched_get_priority_min_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) policy;
	} */

	/*
	 * We can't emulate anything put the default scheduling policy.
	 */
	if (SCARG(uap, policy) != FREEBSD_SCHED_OTHER) {
		*retval = -1;
		return EINVAL;
	}

	*retval = 0;
	return 0;
}
