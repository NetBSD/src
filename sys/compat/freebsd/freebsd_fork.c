/*	$NetBSD: freebsd_fork.c,v 1.5.56.1 2007/12/26 19:48:53 ad Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: freebsd_fork.c,v 1.5.56.1 2007/12/26 19:48:53 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/uio.h>
#include <sys/signal.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <compat/sys/signal.h>
#include <compat/sys/signalvar.h>

#include <compat/freebsd/freebsd_syscallargs.h>
#include <compat/freebsd/freebsd_fork.h>

#include <machine/freebsd_machdep.h>

/*
 * rfork()
 */
int
freebsd_sys_rfork(struct lwp *l, const struct freebsd_sys_rfork_args *uap, register_t *retval)
{
	/* {
		syscallargs(int) flags;
	} */
	int flags;

	flags = 0;

	if ((SCARG(uap, flags)
	    & (FREEBSD_RFFDG | FREEBSD_RFCFDG))
	    == (FREEBSD_RFFDG | FREEBSD_RFCFDG))
		return (EINVAL);

	if ((SCARG(uap, flags) & FREEBSD_RFPROC) == 0)
		return (EINVAL);

	if (SCARG(uap, flags) & FREEBSD_RFNOWAIT)
		flags |= FORK_NOWAIT;
	if (SCARG(uap, flags) & FREEBSD_RFMEM)
		flags |= FORK_SHAREVM;
	if (SCARG(uap, flags) & FREEBSD_RFSIGSHARE)
		flags |= FORK_SHARESIGS;

	if (SCARG(uap, flags) & FREEBSD_RFCFDG)
		flags |= FORK_CLEANFILES;
	else if ((SCARG(uap, flags) & FREEBSD_RFFDG) == 0)
		flags |= FORK_SHAREFILES;

	return (fork1(l, flags,
	    SCARG(uap, flags) & FREEBSD_RFLINUXTHPN ? SIGUSR1 : SIGCHLD,
	    NULL, 0, NULL, NULL, retval, NULL));
}
