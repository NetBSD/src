/* $NetBSD: linux_time.c,v 1.2.2.7 2002/05/29 21:32:46 nathanw Exp $ */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: linux_time.c,v 1.2.2.7 2002/05/29 21:32:46 nathanw Exp $");

#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#include <sys/signal.h>
#include <sys/stdint.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>

#include <compat/linux/linux_syscallargs.h>

/*
 * This is not implemented for alpha yet
 */
#if defined (__i386__) || defined (__m68k__) || \
    defined (__powerpc__) || defined (__mips__) || defined(__arm__)

/* 
 * Linux keeps track of a system timezone in the kernel. It is readen
 * by gettimeofday and set by settimeofday. This emulates this behavior
 * See linux/kernel/time.c
 */ 
struct timezone linux_sys_tz;

int linux_sys_gettimeofday(l, v, retval)
   struct lwp *l;
   void *v;
   register_t *retval;
{
	struct linux_sys_gettimeofday_args /* {
		syscallarg(struct timeval *) tz;
		syscallarg(struct timezone *) tzp;
	} */ *uap = v;
	int error = 0;

	if (SCARG(uap, tp)) {
		error = sys_gettimeofday (l, v, retval);
		if (error) 
			return (error);
	}

	if (SCARG(uap, tzp)) {
		error = copyout(&linux_sys_tz, SCARG(uap, tzp), sizeof(linux_sys_tz));
		if (error) 
			return (error);
   }		

	return (0);
}

int linux_sys_settimeofday(l, v, retval)
   struct lwp *l;
   void *v;
   register_t *retval;
{
	struct linux_sys_settimeofday_args /* {
		syscallarg(struct timeval *) tz;
		syscallarg(struct timezone *) tzp;
	} */ *uap = v;
	int error = 0;

	if (SCARG(uap, tp)) {
		error = sys_settimeofday(l, v, retval);
		if (error) 
			return (error);
	}

	/* 
	 * If user is not the superuser, we returned
	 * after the sys_settimeofday() call. 
	 */
	if (SCARG(uap, tzp)) {
		error = copyin(SCARG(uap, tzp), &linux_sys_tz, sizeof(linux_sys_tz));
		if (error) 
			return (error);
   }		

	return (0);
}

#endif /* __i386__ || __m68k__ || __powerpc__ || __mips__ || __arm__ */
