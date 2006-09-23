/*	$NetBSD: netbsd32_sysctl.c,v 1.22 2006/09/23 22:12:00 manu Exp $	*/

/*
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_sysctl.c,v 1.22 2006/09/23 22:12:00 manu Exp $");

#if defined(_KERNEL_OPT)
#include "opt_ddb.h"
#include "opt_ktrace.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>
#include <sys/proc.h>
#include <sys/sysctl.h>
#include <sys/dirent.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif

#include <uvm/uvm_extern.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscall.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

#if defined(DDB)
#include <ddb/ddbvar.h>
#endif

struct sysctlnode netbsd32_sysctl_root = {
	.sysctl_flags = SYSCTL_VERSION|CTLFLAG_ROOT|CTLTYPE_NODE,
	.sysctl_num = 0,
	.sysctl_name = "(netbsd32_root)",
	sysc_init_field(_sysctl_size, sizeof(struct sysctlnode)),
};

/*
 * sysctl helper routine for netbsd32's kern.boottime node
 */
static int
netbsd32_sysctl_kern_boottime(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct netbsd32_timeval bt32;

	netbsd32_from_timeval(&boottime, &bt32);

	node = *rnode;
	node.sysctl_data = &bt32;
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}

/*
 * sysctl helper routine for netbsd32's vm.loadavg node
 */
static int
netbsd32_sysctl_vm_loadavg(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct netbsd32_loadavg av32;

	netbsd32_from_loadavg(&av32, &averunnable);

	node = *rnode;
	node.sysctl_data = &av32;
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}

SYSCTL_SETUP(netbsd32_sysctl_emul_setup, "sysctl netbsd32 shadow tree setup")
{
	const struct sysctlnode *_root = &netbsd32_sysctl_root;
	extern const char machine_arch32[];
	extern const char machine32[];

	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "kern", NULL,
		       NULL, 0, NULL, 0,
		       CTL_KERN, CTL_EOL);
	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "boottime", NULL,
		       netbsd32_sysctl_kern_boottime, 0, NULL,
		       sizeof(struct netbsd32_timeval),
		       CTL_KERN, KERN_BOOTTIME, CTL_EOL);

	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "vm", NULL,
		       NULL, 0, NULL, 0,
		       CTL_VM, CTL_EOL);
	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "loadavg", NULL,
		       netbsd32_sysctl_vm_loadavg, 0, NULL,
		       sizeof(struct netbsd32_loadavg),
		       CTL_VM, VM_LOADAVG, CTL_EOL);

	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "hw", NULL,
		       NULL, 0, NULL, 0,
		       CTL_HW, CTL_EOL);
	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "machine", NULL,
		       NULL, 0, &machine32, 0,
		       CTL_HW, HW_MACHINE, CTL_EOL);
	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "machine_arch", NULL,
		       NULL, 0, &machine_arch32, 0,
		       CTL_HW, HW_MACHINE_ARCH, CTL_EOL);
}

int
netbsd32___sysctl(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32___sysctl_args /* {
		syscallarg(netbsd32_intp) name;
		syscallarg(u_int) namelen;
		syscallarg(netbsd32_voidp) old;
		syscallarg(netbsd32_size_tp) oldlenp;
		syscallarg(netbsd32_voidp) new;
		syscallarg(netbsd32_size_t) newlen;
	} */ *uap = v;
	const struct sysctlnode *pnode;
	netbsd32_size_t netbsd32_oldlen;
	size_t oldlen, *oldlenp, savelen;
	int name[CTL_MAXNAME], error, nerror, *namep;
	void *newp, *oldp;

	/*
	 * get and convert 32 bit size_t to native size_t
	 */
	namep = NETBSD32PTR64(SCARG(uap, name));
	oldp = NETBSD32PTR64(SCARG(uap, old));
	newp = NETBSD32PTR64(SCARG(uap, new));
	oldlenp = NETBSD32PTR64(SCARG(uap, oldlenp));
	oldlen = 0;
	if (oldlenp != NULL) {
		error = copyin(oldlenp, &netbsd32_oldlen,
			       sizeof(netbsd32_oldlen));
		if (error)
			return (error);
		oldlen = netbsd32_oldlen;
	}
	savelen = oldlen;

	/*
	 * retrieve name and see if we need to dispatch this query to
	 * the shadow tree.  if we find it in the shadow tree,
	 * dispatch to there, otherwise NULL means use the built-in
	 * default main tree.
	 */
	if (SCARG(uap, namelen) > CTL_MAXNAME || SCARG(uap, namelen) < 1)
		return (EINVAL);
	error = copyin(namep, &name[0], SCARG(uap, namelen) * sizeof(int));
        if (error)
                return (error);

#ifdef KTRACE
	if (KTRPOINT(l->l_proc, KTR_MIB))
		ktrmib(l, name, SCARG(uap, namelen));
#endif

	/*
	 * wire old so that copyout() is less likely to fail?
	 */
	error = sysctl_lock(l, oldp, savelen);
	if (error)
		return (error);

	pnode = &netbsd32_sysctl_root;
	error = sysctl_locate(l, &name[0], SCARG(uap, namelen), &pnode, NULL);
	pnode = (error == 0) ? &netbsd32_sysctl_root : NULL;
	error = sysctl_dispatch(&name[0], SCARG(uap, namelen),
				oldp, &oldlen,
				newp, SCARG(uap, newlen),
				&name[0], l, pnode);

	/*
	 * release the sysctl lock
	 */
	sysctl_unlock(l);

	/*
	 * reset caller's oldlen, even if we got an error
	 */
	if (oldlenp) {
		netbsd32_oldlen = oldlen;
                nerror = copyout(&netbsd32_oldlen, oldlenp,
				 sizeof(netbsd32_oldlen));
                if (error == 0)
                        error = nerror;
	}

	/*
	 * if the only problem is that we weren't given enough space,
	 * that's an ENOMEM error
	 */
	if (error == 0 && oldp != NULL && savelen < oldlen)
		error = ENOMEM;

	return (error);
}
