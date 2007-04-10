/*	$NetBSD: netbsd32_compat_09.c,v 1.14.6.1 2007/04/10 13:26:26 ad Exp $	*/

/*
 * Copyright (c) 1998 Matthew R. Green
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_compat_09.c,v 1.14.6.1 2007/04/10 13:26:26 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <sys/time.h>
#include <uvm/uvm_extern.h>
#include <sys/sysctl.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

int
compat_09_netbsd32_ogetdomainname(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_09_netbsd32_ogetdomainname_args /* {
		syscallarg(netbsd32_charp) domainname;
		syscallarg(int) len;
	} */ *uap = v;
	int name[2];
	size_t sz;

	name[0] = CTL_KERN;
	name[1] = KERN_DOMAINNAME;
	sz = SCARG(uap, len);
	return (old_sysctl(&name[0], 2,
	    (char *)SCARG_P32(uap, domainname), &sz, 0, 0, l));
}

int
compat_09_netbsd32_osetdomainname(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_09_netbsd32_osetdomainname_args /* {
		syscallarg(netbsd32_charp) domainname;
		syscallarg(int) len;
	} */ *uap = v;
	int name[2];

	name[0] = CTL_KERN;
	name[1] = KERN_DOMAINNAME;
	return (old_sysctl(&name[0], 2, 0, 0,
	    (char *)SCARG_P32(uap, domainname), SCARG(uap, len), l));
}

int
compat_09_netbsd32_uname(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_09_netbsd32_uname_args /* {
		syscallarg(netbsd32_outsnamep_t) name;
	} */ *uap = v;
	struct compat_09_sys_uname_args ua;

	NETBSD32TOP_UAP(name, struct outsname);
	return (compat_09_sys_uname(l, &ua, retval));
}
