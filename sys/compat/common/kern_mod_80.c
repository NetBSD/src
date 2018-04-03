/*	$NetBSD: kern_mod_80.c,v 1.1.2.1 2018/04/03 08:29:44 pgoyette Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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
 * System calls relating to loadable modules.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_mod_80.c,v 1.1.2.1 2018/04/03 08:29:44 pgoyette Exp $");

#ifdef _KERNEL_OPT
#include "opt_modular.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/namei.h>
#include <sys/kauth.h>
#include <sys/kmem.h>
#include <sys/kobj.h>
#include <sys/module.h>
#include <sys/syscall.h>
#include <sys/syscallargs.h>
#include <sys/compat_stub.h>

#include <compat/common/compat_mod.h>

static void
copy_oalias(omodstat_t *oms, const char * const *aliasp, modinfo_t *mi,
    module_t *mod)
{

	strlcpy(oms->oms_name, *aliasp, sizeof(oms->oms_name));
	strlcpy(oms->oms_required, mi->mi_name, sizeof(oms->oms_required));
	oms->oms_class = mi->mi_class;
	oms->oms_source = mod->mod_source;
	oms->oms_flags = mod->mod_flags | MODFLG_IS_ALIAS;
}

static int
compat_80_modstat(int cmd, struct iovec *iov, void *arg)
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
	const char * const *aliasp;
	const char *suffix = "...";

	if (cmd != MODCTL_OSTAT)
		return EINVAL;

	error = copyin(arg, iov, sizeof(*iov));
	if (error != 0) {
		return error;
	}

	/* If not privileged, don't expose kernel addresses. */
	error = kauth_authorize_system(kauth_cred_get(), KAUTH_SYSTEM_MODULE,
	    0, (void *)(uintptr_t)MODCTL_STAT, NULL, NULL);
	stataddr = (error == 0);

	kernconfig_lock();
	omscnt = 0;
	TAILQ_FOREACH(mod, &module_list, mod_chain) {
		omscnt++;
		mi = mod->mod_info;
		if ((aliasp = *mi->mi_aliases) != NULL) {
			while (*aliasp++ != NULL)
				omscnt++;
		}
	}
	TAILQ_FOREACH(mod, &module_builtins, mod_chain) {
		omscnt++;
		mi = mod->mod_info;
		if ((aliasp = *mi->mi_aliases) != NULL) {
			while (*aliasp++ != NULL)
				omscnt++;
		}
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
		aliasp = *mi->mi_aliases;
		if (aliasp == NULL)
			continue;
		while (*aliasp) {
			copy_oalias(oms, aliasp, mi, mod);
			aliasp++;
			oms++;
		}
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
		aliasp = *mi->mi_aliases;
		if (aliasp == NULL)
			continue;
		while (*aliasp) {
			copy_oalias(oms, aliasp, mi, mod);
			aliasp++;
			oms++;
		}
	}
	kernconfig_unlock();
	error = copyout(omso, iov->iov_base, min(omslen, iov->iov_len));
	kmem_free(omso, omslen);
	if (error == 0) {
		iov->iov_len = omslen;
		error = copyout(iov, arg, sizeof(*iov));
	}

	return error;
}

void
kern_mod_80_init(void)
{

	compat_modstat_80 = compat_80_modstat;
}

void
kern_mod_80_fini(void)
{

	compat_modstat_80 = (void *)enosys;
}
