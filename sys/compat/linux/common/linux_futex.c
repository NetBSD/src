/*	$NetBSD: linux_futex.c,v 1.39 2020/04/26 18:53:33 thorpej Exp $ */

/*-
 * Copyright (c) 2019 The NetBSD Foundation.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell and Jason R. Thorpe.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: linux_futex.c,v 1.39 2020/04/26 18:53:33 thorpej Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/lwp.h>
#include <sys/futex.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_emuldata.h>
#include <compat/linux/common/linux_exec.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_sched.h>
#include <compat/linux/common/linux_machdep.h>
#include <compat/linux/linux_syscallargs.h>

int
linux_sys_futex(struct lwp *l, const struct linux_sys_futex_args *uap,
	register_t *retval)
{
	/* {
		syscallarg(int *) uaddr;
		syscallarg(int) op;
		syscallarg(int) val;
		syscallarg(const struct linux_timespec *) timeout;
		syscallarg(int *) uaddr2;
		syscallarg(int) val3;
	} */
	struct linux_timespec lts;
	struct timespec ts, *tsp = NULL;
	int val2 = 0;
	int error;

	/*
	 * Linux overlays the "timeout" field and the "val2" field.
	 * "timeout" is only valid for FUTEX_WAIT on Linux.
	 */
	if ((SCARG(uap, op) & FUTEX_CMD_MASK) == FUTEX_WAIT &&
	    SCARG(uap, timeout) != NULL) {
		if ((error = copyin(SCARG(uap, timeout), 
		    &lts, sizeof(lts))) != 0) {
			return error;
		}
		linux_to_native_timespec(&ts, &lts);
		tsp = &ts;
	} else {
		val2 = (int)(uintptr_t)SCARG(uap, timeout);
	}

	return do_futex(SCARG(uap, uaddr), SCARG(uap, op), SCARG(uap, val),
	    tsp, SCARG(uap, uaddr2), val2, SCARG(uap, val3), retval);
}
