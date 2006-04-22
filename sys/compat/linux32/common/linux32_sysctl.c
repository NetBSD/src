/*	$NetBSD: linux32_sysctl.c,v 1.1.10.2 2006/04/22 11:38:14 simonb Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: linux32_sysctl.c,v 1.1.10.2 2006/04/22 11:38:14 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/sysctl.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>

#include <compat/netbsd32/netbsd32.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>

#include <compat/linux/linux_syscallargs.h>

#include <compat/linux32/common/linux32_types.h>
#include <compat/linux32/common/linux32_signal.h>
#include <compat/linux32/common/linux32_sysctl.h>

#include <compat/linux32/linux32_syscallargs.h>

char linux32_sysname[128] = "Linux";
char linux32_release[128] = "2.4.18";
char linux32_version[128] = "#0 Wed Feb 20 20:00:02 CET 2002";


SYSCTL_SETUP(sysctl_emul_linux32_setup, "sysctl emul.linux32 subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "emul", NULL,
		       NULL, 0, NULL, 0,
		       CTL_EMUL, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "linux32",
		       SYSCTL_DESCR("Linux 32 bit emulation settings"),
		       NULL, 0, NULL, 0,
		       CTL_EMUL, EMUL_LINUX32, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "kern",
		       SYSCTL_DESCR("Linux 32 bit kernel emulation settings"),
		       NULL, 0, NULL, 0,
		       CTL_EMUL, EMUL_LINUX32, EMUL_LINUX32_KERN, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRING, "ostype",
		       SYSCTL_DESCR("Linux 32 bit operating system type"),
		       NULL, 0, linux32_sysname, sizeof(linux32_sysname),
		       CTL_EMUL, EMUL_LINUX32, EMUL_LINUX32_KERN,
		       EMUL_LINUX32_KERN_OSTYPE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRING, "osrelease",
		       SYSCTL_DESCR("Linux 32 bit operating system release"),
		       NULL, 0, linux32_release, sizeof(linux32_release),
		       CTL_EMUL, EMUL_LINUX32, EMUL_LINUX32_KERN,
		       EMUL_LINUX32_KERN_OSRELEASE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRING, "osversion",
		       SYSCTL_DESCR("Linux 32 bit operating system revision"),
		       NULL, 0, linux32_version, sizeof(linux32_version),
		       CTL_EMUL, EMUL_LINUX32, EMUL_LINUX32_KERN,
		       EMUL_LINUX32_KERN_VERSION, CTL_EOL);
}


int
linux32_sys___sysctl(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys___sysctl_args /* {
		syscallarg(linux32___sysctlp_t) lsp;
	} */ *uap = v;
	caddr_t sg = stackgap_init(l->l_proc, 0);
	struct linux32_sysctl ls32;
	struct linux___sysctl ls;
	struct linux_sys___sysctl_args ua;
	int error;

	if ((error = copyin(NETBSD32PTR64(SCARG(uap, lsp)), 
	    &ls32, sizeof(ls32))) != 0)
		return error;

	ls.name = NETBSD32PTR64(ls32.name);
	ls.nlen = ls32.name;
	ls.oldval = NETBSD32PTR64(ls32.oldval);
	ls.oldlenp = NETBSD32PTR64(ls32.oldlenp);
	ls.newval = NETBSD32PTR64(ls32.newval);
	ls.newlen = ls32.newlen;

#ifdef DEBUG_LINUX
	{
		int i = ls.nlen;
		int *cp = ls.name;
		int val;

		printf("linux32_sysctl(%p, %d, %p, %p, %p, %ld) [",
		    ls.name, ls.nlen, ls.oldval, 
		    ls.oldlenp, ls.newval, ls.newlen);
		while (i > 0) {
			if ((error = copyin(cp, &val, sizeof(val))) != 0)
				return error;
			printf("%d ", val);
			cp++;
			i--;
		}
		printf("]\n");
	}
#endif

	SCARG(&ua, lsp) = stackgap_alloc(l->l_proc, &sg, sizeof(ls));

	if ((error = copyout(&ls, SCARG(&ua, lsp), sizeof(ls))) != 0)
		return error;

	if ((error = linux_sys___sysctl(l, &ua, retval)) != 0)
		return error;

	if ((error = copyin(SCARG(&ua, lsp), &ls, sizeof(ls))) != 0)
		return error;

	ls32.name = (netbsd32_intp)(long)ls.name;
	ls32.nlen = ls.nlen;
	ls32.oldval = (netbsd32_voidp)(long)ls.oldval;
	ls32.oldlenp = (netbsd32_size_tp)(long)ls.oldlenp;
	ls32.newval = (netbsd32_voidp)(long)ls.newval;
	ls32.newlen = (netbsd32_size_t)(long)ls.newlen;

	if ((error = copyout(&ls32, 
	    NETBSD32PTR64(SCARG(uap, lsp)), sizeof(ls32))) != 0)
		return error;

	return 0;
}
