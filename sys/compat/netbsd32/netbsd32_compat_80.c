/*	$NetBSD: netbsd32_compat_80.c,v 1.1.2.4 2018/09/18 10:35:04 pgoyette Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software developed for The NetBSD Foundation.
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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_compat_80.c,v 1.1.2.4 2018/09/18 10:35:04 pgoyette Exp $");

#include <sys/param.h>
#include <sys/dirent.h>
#include <sys/kauth.h>
#include <sys/module.h>
#include <sys/kobj.h>

#include <compat/sys/siginfo.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscall.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

#ifdef COMPAT_80

int netbsd32_80_modctl(struct lwp *, const struct netbsd32_modctl_args *,
	register_t *);

static int
modctl32_handle_ostat(int cmd, struct netbsd32_iovec *iov, void *arg)
{
	omodstat_t *oms, *omso;
	modinfo_t *mi;
	module_t *mod;
	vaddr_t addr;
	size_t size;
	size_t omslen;
	size_t used;
	int error;
	int omscnt;
	bool stataddr;
	const char *suffix = "...";

	if (cmd != MODCTL_OSTAT)
		return EINVAL;

	/* If not privileged, don't expose kernel addresses. */
	error = kauth_authorize_system(kauth_cred_get(), KAUTH_SYSTEM_MODULE,
	    0, (void *)(uintptr_t)MODCTL_STAT, NULL, NULL);
	stataddr = (error == 0);

	kernconfig_lock();
	omscnt = 0;
	TAILQ_FOREACH(mod, &module_list, mod_chain) {
		omscnt++;
		mi = mod->mod_info;
	}
	TAILQ_FOREACH(mod, &module_builtins, mod_chain) {
		omscnt++;
		mi = mod->mod_info;
	}
	omslen = omscnt * sizeof(omodstat_t);
	omso = kmem_zalloc(omslen, KM_SLEEP);
	oms = omso;
	TAILQ_FOREACH(mod, &module_list, mod_chain) {
		mi = mod->mod_info;
		strlcpy(oms->oms_name, mi->mi_name, sizeof(oms->oms_name));
		if (mi->mi_required != NULL) {
			used = strlcpy(oms->oms_required, mi->mi_required,
			    sizeof(oms->oms_required));
			if (used >= sizeof(oms->oms_required)) { 
				oms->oms_required[sizeof(oms->oms_required) -
				    strlen(suffix) - 1] = '\0';
				strlcat(oms->oms_required, suffix,
				    sizeof(oms->oms_required));
                        }
		}
		if (mod->mod_kobj != NULL && stataddr) {
			kobj_stat(mod->mod_kobj, &addr, &size);
			oms->oms_addr = addr;
			oms->oms_size = size;
		}
		oms->oms_class = mi->mi_class;
		oms->oms_refcnt = mod->mod_refcnt;
		oms->oms_source = mod->mod_source;
		oms->oms_flags = mod->mod_flags;
		oms++;
	}
	TAILQ_FOREACH(mod, &module_builtins, mod_chain) {
		mi = mod->mod_info;
		strlcpy(oms->oms_name, mi->mi_name, sizeof(oms->oms_name));
		if (mi->mi_required != NULL) {
			used = strlcpy(oms->oms_required, mi->mi_required,
			    sizeof(oms->oms_required));
			if (used >= sizeof(oms->oms_required)) { 
				oms->oms_required[sizeof(oms->oms_required) -
				    strlen(suffix) - 1] = '\0';
				strlcat(oms->oms_required, suffix,
				    sizeof(oms->oms_required));
                        }
		}
		if (mod->mod_kobj != NULL && stataddr) {
			kobj_stat(mod->mod_kobj, &addr, &size);
			oms->oms_addr = addr;
			oms->oms_size = size;
		}
		oms->oms_class = mi->mi_class;
		oms->oms_refcnt = -1;
		KASSERT(mod->mod_source == MODULE_SOURCE_KERNEL);
		oms->oms_source = mod->mod_source;
		oms++;
	}
	kernconfig_unlock();
	error = copyout(omso, NETBSD32PTR64(iov->iov_base),
	    uimin(omslen - sizeof(modstat_t), iov->iov_len));
	kmem_free(omso, omslen);
	if (error == 0) {
		iov->iov_len = omslen - sizeof(modstat_t);
		error = copyout(iov, arg, sizeof(*iov));
	}

	return error;
}

int
netbsd32_80_modctl(struct lwp *lwp, const struct netbsd32_modctl_args *uap,
	register_t *result)
{
	/* {
		syscallarg(int) cmd;
		syscallarg(netbsd32_voidp) arg;
	} */
	struct netbsd32_iovec iov;
	int error;
	void *arg;

	arg = SCARG_P32(uap, arg);

	switch (SCARG(uap, cmd)) {
	case MODCTL_OSTAT:
		error = copyin(arg, &iov, sizeof(iov));
		if (error != 0) {
			break;
		}
		error = modctl32_handle_ostat(SCARG(uap, cmd), &iov, arg);
		break;
	default:
		error = EPASSTHROUGH;
		break;
	}

	return error;
}

COMPAT_SET_HOOK(compat32_80_modctl_hook, "nb32_modctl_80", netbsd32_80_modctl);
COMPAT_UNSET_HOOK(compat32_80_modctl_hook);

MODULE(MODULE_CLASS_EXEC, compat_netbsd32_80, "compat_netbsd32,compat_80");

static int
compat_netbsd32_80_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		compat32_80_modctl_hook_set();
		return 0;

	case MODULE_CMD_FINI:
		compat32_80_modctl_hook_unset();
		return 0;

	default:
		return ENOTTY;
	}
}
#endif	/* COMPAT_80 */
