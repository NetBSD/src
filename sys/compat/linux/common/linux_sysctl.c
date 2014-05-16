/*	$NetBSD: linux_sysctl.c,v 1.43 2014/05/16 12:22:32 martin Exp $	*/

/*-
 * Copyright (c) 2003, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Brown.
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
 * sysctl system call.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_sysctl.c,v 1.43 2014/05/16 12:22:32 martin Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/sysctl.h>
#include <sys/sched.h>
#include <sys/syscallargs.h>
#include <sys/ktrace.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_ipc.h>
#include <compat/linux/common/linux_sem.h>

#include <compat/linux/linux_syscallargs.h>
#include <compat/linux/common/linux_sysctl.h>
#include <compat/linux/common/linux_exec.h>
#include <compat/linux/common/linux_machdep.h>

char linux_sysname[128] = "Linux";
char linux_release[128] = "3.11.6";
char linux_version[128] = "#1 SMP PREEMPT Thu Oct 24 16:23:02 UTC 2013";

struct sysctlnode linux_sysctl_root = {
	.sysctl_flags = SYSCTL_VERSION|
	    CTLFLAG_ROOT|CTLTYPE_NODE|CTLFLAG_READWRITE,
	.sysctl_num = 0,
	.sysctl_name = "(linux_root)",
	.sysctl_size = sizeof(struct sysctlnode),
};

static struct sysctllog *linux_clog1;
static struct sysctllog *linux_clog2;

void
linux_sysctl_fini(void)
{

	sysctl_teardown(&linux_clog2);
	sysctl_teardown(&linux_clog1);
	sysctl_free(&linux_sysctl_root);
}

void
linux_sysctl_init(void)
{
	const struct sysctlnode *node = &linux_sysctl_root;

	sysctl_createv(&linux_clog1, 0, &node, &node,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "kern", NULL,
		       NULL, 0, NULL, 0,
		       LINUX_CTL_KERN, CTL_EOL);
	sysctl_createv(&linux_clog1, 0, &node, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "ostype", NULL,
		       NULL, 0, linux_sysname, sizeof(linux_sysname),
		       LINUX_KERN_OSTYPE, CTL_EOL);
	sysctl_createv(&linux_clog1, 0, &node, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "osrelease", NULL,
		       NULL, 0, linux_release, sizeof(linux_release),
		       LINUX_KERN_OSRELEASE, CTL_EOL);
	sysctl_createv(&linux_clog1, 0, &node, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "version", NULL,
		       NULL, 0, linux_version, sizeof(linux_version),
		       LINUX_KERN_VERSION, CTL_EOL);

	sysctl_createv(&linux_clog2, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "linux",
		       SYSCTL_DESCR("Linux emulation settings"),
		       NULL, 0, NULL, 0,
		       CTL_EMUL, EMUL_LINUX, CTL_EOL);
	sysctl_createv(&linux_clog2, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "kern",
		       SYSCTL_DESCR("Linux kernel emulation settings"),
		       NULL, 0, NULL, 0,
		       CTL_EMUL, EMUL_LINUX, EMUL_LINUX_KERN, CTL_EOL);
	sysctl_createv(&linux_clog2, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRING, "ostype",
		       SYSCTL_DESCR("Linux operating system type"),
		       NULL, 0, linux_sysname, sizeof(linux_sysname),
		       CTL_EMUL, EMUL_LINUX, EMUL_LINUX_KERN,
		       EMUL_LINUX_KERN_OSTYPE, CTL_EOL);
	sysctl_createv(&linux_clog2, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRING, "osrelease",
		       SYSCTL_DESCR("Linux operating system release"),
		       NULL, 0, linux_release, sizeof(linux_release),
		       CTL_EMUL, EMUL_LINUX, EMUL_LINUX_KERN,
		       EMUL_LINUX_KERN_OSRELEASE, CTL_EOL);
	sysctl_createv(&linux_clog2, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRING, "osversion",
		       SYSCTL_DESCR("Linux operating system revision"),
		       NULL, 0, linux_version, sizeof(linux_version),
		       CTL_EMUL, EMUL_LINUX, EMUL_LINUX_KERN,
		       EMUL_LINUX_KERN_VERSION, CTL_EOL);

	linux_sysctl_root.sysctl_flags &= ~CTLFLAG_READWRITE;
}

/*
 * linux sysctl system call
 */
int
linux_sys___sysctl(struct lwp *l, const struct linux_sys___sysctl_args *uap, register_t *retval)
{
	struct linux___sysctl ls;
	int error, nerror, name[CTL_MAXNAME];
	size_t savelen = 0, oldlen = 0;

	/*
	 * get linux args structure
	 */
	if ((error = copyin(SCARG(uap, lsp), &ls, sizeof(ls))))
		return error;

	/*
	 * get oldlen
	 */
	oldlen = 0;
	if (ls.oldlenp != NULL) {
		error = copyin(ls.oldlenp, &oldlen, sizeof(oldlen));
		if (error)
			return (error);
	}
	savelen = oldlen;

	/*
	 * top-level sysctl names may or may not be non-terminal, but
	 * we don't care
	 */
	if (ls.nlen > CTL_MAXNAME || ls.nlen < 1)
		return (ENOTDIR);
	error = copyin(ls.name, &name, ls.nlen * sizeof(int));
	if (error)
		return (error);

	ktrmib(name, ls.nlen);

	/*
	 * dispatch request into linux sysctl tree
	 */
	sysctl_lock(ls.newval != NULL);
	error = sysctl_dispatch(&name[0], ls.nlen,
				ls.oldval, &oldlen,
				ls.newval, ls.newlen,
				&name[0], l, &linux_sysctl_root);
	sysctl_unlock();

	/*
	 * reset caller's oldlen, even if we got an error
	 */
	if (ls.oldlenp) {
		nerror = copyout(&oldlen, ls.oldlenp, sizeof(oldlen));
		if (error == 0)
			error = nerror;
	}

	/*
	 * if the only problem is that we weren't given enough space,
	 * that's an ENOMEM error
	 */
	if (error == 0 && ls.oldval != NULL && savelen < oldlen)
		error = ENOMEM;

	return (error);
}
