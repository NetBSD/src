/*	$NetBSD: linux32_sysctl.c,v 1.13.18.1 2014/08/20 00:03:33 tls Exp $ */

/*-
 * Copyright (c) 2006 Emmanuel Dreyfus, all rights reserved.
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
 *	This product includes software developed by Emmanuel Dreyfus
 * 4. The name of the author may not be used to endorse or promote 
 *    products derived from this software without specific prior written 
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE THE AUTHOR AND CONTRIBUTORS ``AS IS'' 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS 
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux32_sysctl.c,v 1.13.18.1 2014/08/20 00:03:33 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/sysctl.h>
#include <sys/syscallargs.h>
#include <sys/ktrace.h>

#include <compat/netbsd32/netbsd32.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_sysctl.h>
#include <compat/linux/common/linux_ipc.h>
#include <compat/linux/common/linux_sem.h>
#include <compat/linux/linux_syscallargs.h>

#include <compat/linux32/common/linux32_types.h>
#include <compat/linux32/common/linux32_signal.h>
#include <compat/linux32/common/linux32_sysctl.h>
#include <compat/linux32/linux32_syscallargs.h>

char linux32_sysname[128] = "Linux";
char linux32_release[128] = "3.11.6";
char linux32_version[128] = "#1 SMP PREEMPT Thu Oct 24 16:23:02 UTC 2013";

struct sysctlnode linux32_sysctl_root = {
	.sysctl_flags = SYSCTL_VERSION|
	    CTLFLAG_ROOT|CTLTYPE_NODE|CTLFLAG_READWRITE,
	.sysctl_num = 0,
	.sysctl_name = "(linux32_root)",
	.sysctl_size = sizeof(struct sysctlnode),
};

static struct sysctllog *linux32_clog1;
static struct sysctllog *linux32_clog2;

void
linux32_sysctl_fini(void)
{

	sysctl_teardown(&linux32_clog2);
	sysctl_teardown(&linux32_clog1);
	sysctl_free(&linux32_sysctl_root);
}

void
linux32_sysctl_init(void)
{
	const struct sysctlnode *node = &linux32_sysctl_root;

	sysctl_createv(&linux32_clog1, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "linux32",
		       SYSCTL_DESCR("Linux 32 bit emulation settings"),
		       NULL, 0, NULL, 0,
		       CTL_EMUL, EMUL_LINUX32, CTL_EOL);
	sysctl_createv(&linux32_clog1, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "kern",
		       SYSCTL_DESCR("Linux 32 bit kernel emulation settings"),
		       NULL, 0, NULL, 0,
		       CTL_EMUL, EMUL_LINUX32, EMUL_LINUX32_KERN, CTL_EOL);

	sysctl_createv(&linux32_clog1, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRING, "ostype",
		       SYSCTL_DESCR("Linux 32 bit operating system type"),
		       NULL, 0, linux32_sysname, sizeof(linux32_sysname),
		       CTL_EMUL, EMUL_LINUX32, EMUL_LINUX32_KERN,
		       EMUL_LINUX32_KERN_OSTYPE, CTL_EOL);
	sysctl_createv(&linux32_clog1, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRING, "osrelease",
		       SYSCTL_DESCR("Linux 32 bit operating system release"),
		       NULL, 0, linux32_release, sizeof(linux32_release),
		       CTL_EMUL, EMUL_LINUX32, EMUL_LINUX32_KERN,
		       EMUL_LINUX32_KERN_OSRELEASE, CTL_EOL);
	sysctl_createv(&linux32_clog1, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRING, "osversion",
		       SYSCTL_DESCR("Linux 32 bit operating system revision"),
		       NULL, 0, linux32_version, sizeof(linux32_version),
		       CTL_EMUL, EMUL_LINUX32, EMUL_LINUX32_KERN,
		       EMUL_LINUX32_KERN_VERSION, CTL_EOL);

	sysctl_createv(&linux32_clog2, 0, &node, &node,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "kern", NULL,
		       NULL, 0, NULL, 0,
		       LINUX_CTL_KERN, CTL_EOL);

	sysctl_createv(&linux32_clog2, 0, &node, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "ostype", NULL,
		       NULL, 0, linux32_sysname, sizeof(linux32_sysname),
		       LINUX_KERN_OSTYPE, CTL_EOL);
	sysctl_createv(&linux32_clog2, 0, &node, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "osrelease", NULL,
		       NULL, 0, linux32_release, sizeof(linux32_release),
		       LINUX_KERN_OSRELEASE, CTL_EOL);
	sysctl_createv(&linux32_clog2, 0, &node, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "version", NULL,
		       NULL, 0, linux32_version, sizeof(linux32_version),
		       LINUX_KERN_VERSION, CTL_EOL);

	linux32_sysctl_root.sysctl_flags &= ~CTLFLAG_READWRITE;
}

int
linux32_sys___sysctl(struct lwp *l, const struct linux32_sys___sysctl_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux32___sysctlp_t) lsp;
	} */
	struct linux32_sysctl ls32;
	int name[CTL_MAXNAME];
	size_t savelen;
	netbsd32_size_t oldlen32;
	size_t oldlen;
	int error;

	/*
	 * Read sysctl arguments 
	 */
	if ((error = copyin(SCARG_P32(uap, lsp), &ls32, sizeof(ls32))) != 0)
		return error;

	/*
	 * Read oldlen
	 */
	if (NETBSD32PTR64(ls32.oldlenp) != NULL) {
		if ((error = copyin(NETBSD32PTR64(ls32.oldlenp), 
		    &oldlen32, sizeof(oldlen32))) != 0)
			return error;
	} else {
		oldlen32 = 0;
	}

	savelen = (size_t)oldlen32;

	/* 
	 * Sanity check nlen
	 */
	if ((ls32.nlen > CTL_MAXNAME) || (ls32.nlen < 1))
		return ENOTDIR;

	/*
	 * Read the sysctl name
	 */
	if ((error = copyin(NETBSD32PTR64(ls32.name), &name, 
	   ls32.nlen * sizeof(int))) != 0)
		return error;

	ktrmib(name, ls32.nlen);
	/*
	 * First try linux32 tree, then linux tree
	 */
	oldlen = (size_t)oldlen32;
	sysctl_lock(NETBSD32PTR64(ls32.newval) != NULL);
	error = sysctl_dispatch(name, ls32.nlen,
				NETBSD32PTR64(ls32.oldval), &oldlen,
				NETBSD32PTR64(ls32.newval), ls32.newlen,
				name, l, &linux32_sysctl_root);
	oldlen32 = (netbsd32_size_t)oldlen;
	sysctl_unlock();

	/*
	 * Check for oldlen overflow (not likely, but who knows...)
	 */
	if (oldlen != oldlen32) {
#ifdef DEBUG_LINUX
		printf("%s: oldlen32 = %d, oldlen = %ld\n", 
		    __func__, oldlen32, oldlen);
#endif
		return EINVAL;
	}

	/*
	 * set caller's oldlen, even if we got an error
	 */
	if (NETBSD32PTR64(ls32.oldlenp)) {
		int nerror;

		nerror = copyout(&oldlen32, 
		    NETBSD32PTR64(ls32.oldlenp), sizeof(oldlen32));

		if (error == 0)
			error = nerror;
	}

	/*
	 * oldlen was too short
	 */
	if ((error == 0) && 
	    (NETBSD32PTR64(ls32.oldval) != NULL) &&
	    (savelen < oldlen32))
		error = ENOMEM;

	return error;
}
