/* $NetBSD: osf1_time.c,v 1.14.8.1 2008/01/09 01:51:45 matt Exp $ */

/*
 * Copyright (c) 1999 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
__KERNEL_RCSID(0, "$NetBSD: osf1_time.c,v 1.14.8.1 2008/01/09 01:51:45 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <compat/osf1/osf1.h>
#include <compat/osf1/osf1_syscallargs.h>
#include <compat/osf1/osf1_cvt.h>

int
osf1_sys_gettimeofday(struct lwp *l, const struct osf1_sys_gettimeofday_args *uap, register_t *retval)
{
	struct osf1_timeval otv;
	struct osf1_timezone otz;
	struct timeval tv;
	int error;

	microtime(&tv);
	memset(&otv, 0, sizeof otv);
	otv.tv_sec = tv.tv_sec;
	otv.tv_usec = tv.tv_usec;
	error = copyout(&otv, SCARG(uap, tp), sizeof otv);

	if (error == 0 && SCARG(uap, tzp) != NULL) {
		memset(&otz, 0, sizeof otz);
		error = copyout(&otz, SCARG(uap, tzp), sizeof otz);
	}
	return (error);
}

int
osf1_sys_setitimer(struct lwp *l, const struct osf1_sys_setitimer_args *uap, register_t *retval)
{
	struct osf1_itimerval o_itv, o_oitv;
	struct itimerval b_itv, b_oitv;
	int which;
	int error;

	switch (SCARG(uap, which)) {
	case OSF1_ITIMER_REAL:
		which = ITIMER_REAL;
		break;

	case OSF1_ITIMER_VIRTUAL:
		which = ITIMER_VIRTUAL;
		break;

	case OSF1_ITIMER_PROF:
		which = ITIMER_PROF;
		break;

	default:
		return (EINVAL);
	}

	/* get the OSF/1 itimerval argument */
	error = copyin(SCARG(uap, itv), &o_itv, sizeof o_itv);
	if (error != 0)
		return error;

	/* fill in and the NetBSD timeval */
	memset(&b_itv, 0, sizeof b_itv);
	b_itv.it_interval.tv_sec = o_itv.it_interval.tv_sec;
	b_itv.it_interval.tv_usec = o_itv.it_interval.tv_usec;
	b_itv.it_value.tv_sec = o_itv.it_value.tv_sec;
	b_itv.it_value.tv_usec = o_itv.it_value.tv_usec;

	if (SCARG(uap, oitv) != NULL) {
		dogetitimer(l->l_proc, which, &b_oitv);
		if (error)
			return error;
	}
		
	error = dosetitimer(l->l_proc, which, &b_itv);

	if (error == 0 || SCARG(uap, oitv) == NULL)
		return error;

	/* fill in and copy out the old timeval */
	memset(&o_oitv, 0, sizeof o_oitv);
	o_oitv.it_interval.tv_sec = b_oitv.it_interval.tv_sec;
	o_oitv.it_interval.tv_usec = b_oitv.it_interval.tv_usec;
	o_oitv.it_value.tv_sec = b_oitv.it_value.tv_sec;
	o_oitv.it_value.tv_usec = b_oitv.it_value.tv_usec;

	return copyout(&o_oitv, SCARG(uap, oitv), sizeof o_oitv);
}

int
osf1_sys_getitimer(struct lwp *l, const struct osf1_sys_getitimer_args *uap, register_t *retval)
{
	struct osf1_itimerval o_oitv;
	struct itimerval b_oitv;
	int which;
	int error;

	switch (SCARG(uap, which)) {
	case OSF1_ITIMER_REAL:
		which = ITIMER_REAL;
		break;
	case OSF1_ITIMER_VIRTUAL:
		which = ITIMER_VIRTUAL;
		break;
	case OSF1_ITIMER_PROF:
		which = ITIMER_PROF;
		break;
	default:
		return (EINVAL);
	}

	error = dogetitimer(l->l_proc, which, &b_oitv);
	if (error != 0 || SCARG(uap, itv) == NULL)
		return error;

	/* fill in and copy out the osf1 timeval */
	memset(&o_oitv, 0, sizeof o_oitv);
	o_oitv.it_interval.tv_sec = b_oitv.it_interval.tv_sec;
	o_oitv.it_interval.tv_usec = b_oitv.it_interval.tv_usec;
	o_oitv.it_value.tv_sec = b_oitv.it_value.tv_sec;
	o_oitv.it_value.tv_usec = b_oitv.it_value.tv_usec;
	return copyout(&o_oitv, SCARG(uap, itv), sizeof o_oitv);
}

int
osf1_sys_settimeofday(struct lwp *l, const struct osf1_sys_settimeofday_args *uap, register_t *retval)
{
	struct osf1_timeval otv;
	struct timeval tv, *tvp;
	int error = 0;

	if (SCARG(uap, tv) == NULL)
		tvp = NULL;
	else {
		/* get the OSF/1 timeval argument */
		error = copyin(SCARG(uap, tv), &otv, sizeof otv);
		if (error != 0)
			return error;

		tv.tv_sec = otv.tv_sec;
		tv.tv_usec = otv.tv_usec;
		tvp = &tv;
	}

	/* NetBSD ignores the timezone field */

	return settimeofday1(tvp, false, (const void *)SCARG(uap, tzp), l, true);
}
