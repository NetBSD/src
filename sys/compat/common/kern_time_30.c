/*	$NetBSD: kern_time_30.c,v 1.5.14.1 2018/03/28 04:18:24 pgoyette Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_time_30.c,v 1.5.14.1 2018/03/28 04:18:24 pgoyette Exp $");

#ifdef _KERNEL_OPT
#include "opt_ntp.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/timex.h>
#include <sys/syscall.h>
#include <sys/syscallvar.h>
#include <sys/syscallargs.h>
#include <sys/compat_stub.h>

#include <compat/common/compat_mod.h>
#include <compat/common/compat_util.h>
#include <compat/sys/time.h>
#include <compat/sys/timex.h>

static const struct syscall_package kern_time_30_syscalls[] = {
        { SYS_compat_30_ntp_gettime, 0,
	    (sy_call_t *)compat_30_sys_ntp_gettime },
	{ 0, 0, NULL }
};

int
compat_30_sys_ntp_gettime(struct lwp *l,
    const struct compat_30_sys_ntp_gettime_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct ntptimeval30 *) ontvp;
	} */
	struct ntptimeval ntv;
	struct ntptimeval30 ntv30;
	struct timeval tv;
	int error;

	if (vec_ntp_gettime == NULL)
		return ENOSYS;

	if (SCARG(uap, ntvp)) {
		(*vec_ntp_gettime)(&ntv);
		TIMESPEC_TO_TIMEVAL(&tv, &ntv.time);
		timeval_to_timeval50(&tv, &ntv30.time);
		ntv30.maxerror = ntv.maxerror;
		ntv30.esterror = ntv.esterror;

		error = copyout(&ntv30, SCARG(uap, ntvp), sizeof(ntv30));
		if (error)
			return error;
 	}
	*retval = (*vec_ntp_timestatus)();
	return 0;
}

int
kern_time_30_init(void)
{

	return syscall_establish(NULL, kern_time_30_syscalls);
}

int
kern_time_30_fini(void)
{

	return syscall_disestablish(NULL, kern_time_30_syscalls);
}
