/*	$NetBSD: irix_misc.c,v 1.6.4.3 2008/01/21 09:41:03 yamt Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: irix_misc.c,v 1.6.4.3 2008/01/21 09:41:03 yamt Exp $");

#include <sys/types.h>
#include <sys/signal.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <compat/svr4/svr4_types.h>
#include <compat/svr4/svr4_lwp.h>
#include <compat/svr4/svr4_signal.h>
#include <compat/svr4/svr4_ucontext.h>
#include <compat/svr4/svr4_utsname.h>
#include <compat/svr4/svr4_syscallargs.h>

#include <compat/irix/irix_types.h>
#include <compat/irix/irix_signal.h>
#include <compat/irix/irix_exec.h>
#include <compat/irix/irix_sysctl.h>
#include <compat/irix/irix_syscallargs.h>

/*
 * This is copied from sys/compat/sunos/sunos_misc.c:sunos_sys_setpgrp()
 * Maybe consider moving this to sys/compat/common/compat_util.c?
 */
int
irix_sys_setpgrp(struct lwp *l, const struct irix_sys_setpgrp_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) pid;
		syscallarg(int) pgid;
	} */
	struct proc *p = l->l_proc;

	/*
	 * difference to our setpgid call is to include backwards
	 * compatibility to pre-setsid() binaries. Do setsid()
	 * instead of setpgid() in those cases where the process
	 * tries to create a new session the old way.
	 */
	if (!SCARG(uap, pgid) &&
	    (!SCARG(uap, pid) || SCARG(uap, pid) == p->p_pid))
		return sys_setsid(l, uap, retval);
	else
		return sys_setpgid(l, (const void *)uap, retval);
}

#define BUF_SIZE 16

int
irix_sys_uname(struct lwp *l, const struct irix_sys_uname_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct irix_utsname *) name;
	} */
	struct irix_utsname sut;
	char irix_release[BUF_SIZE + 1];

	snprintf(irix_release, sizeof(irix_release), "%s.%s",
	    irix_si_osrel_maj, irix_si_osrel_min);
	memset(&sut, 0, sizeof(sut));

	strncpy(sut.sysname, irix_si_os_name, sizeof(sut.sysname));
	sut.sysname[sizeof(sut.sysname) - 1] = '\0';

	strncpy(sut.nodename, hostname, sizeof(sut.nodename));
	sut.nodename[sizeof(sut.nodename) - 1] = '\0';

	strncpy(sut.release, irix_release, sizeof(sut.release));
	sut.release[sizeof(sut.release) - 1] = '\0';

	strncpy(sut.version, irix_si_version, sizeof(sut.version));
	sut.version[sizeof(sut.version) - 1] = '\0';

	strncpy(sut.machine, irix_si_hw_name, sizeof(sut.machine));
	sut.machine[sizeof(sut.machine) - 1] = '\0';

	return copyout((void *) &sut, (void *) SCARG(uap, name),
	    sizeof(struct irix_utsname));
}

int
irix_sys_utssys(struct lwp *l, const struct irix_sys_utssys_args *uap, register_t *retval)
{
	/* {
		syscallarg(void *) a1;
		syscallarg(void *) a2;
		syscallarg(int) sel;
		syscallarg(void) a3;
	} */

	switch (SCARG(uap, sel)) {
	case 0: {	/* uname(2)  */
		struct irix_sys_uname_args ua;
		SCARG(&ua, name) = SCARG(uap, a1);
		return irix_sys_uname(l, &ua, retval);
	}
	break;

	default:
		return(svr4_sys_utssys(l, (const void *)uap, retval));
	break;
	}

	return 0;
}
