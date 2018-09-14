/*	$NetBSD: kern_reboot.c,v 1.1 2018/09/14 01:55:19 mrg Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)kern_xxx.c	8.3 (Berkeley) 2/14/95
 *	from: NetBSD: kern_xxx.c,v 1.74 2017/10/28 00:37:11 pgoyette Exp
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_reboot.c,v 1.1 2018/09/14 01:55:19 mrg Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/syscall.h>
#include <sys/syscallargs.h>
#include <sys/kauth.h>

/* ARGSUSED */
int
sys_reboot(struct lwp *l, const struct sys_reboot_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) opt;
		syscallarg(char *) bootstr;
	} */
	int error;
	char *bootstr, bs[128];

	if ((error = kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_REBOOT,
	    0, NULL, NULL, NULL)) != 0)
		return (error);

	/*
	 * Only use the boot string if RB_STRING is set.
	 */
	if ((SCARG(uap, opt) & RB_STRING) &&
	    (error = copyinstr(SCARG(uap, bootstr), bs, sizeof(bs), 0)) == 0)
		bootstr = bs;
	else
		bootstr = NULL;
	/*
	 * Not all ports use the bootstr currently.
	 */
	KERNEL_LOCK(1, NULL);
	cpu_reboot(SCARG(uap, opt), bootstr);
	KERNEL_UNLOCK_ONE(NULL);
	return (0);
}
