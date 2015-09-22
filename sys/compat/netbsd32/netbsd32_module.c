/*	$NetBSD: netbsd32_module.c,v 1.2.2.2 2015/09/22 12:05:55 skrll Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_module.c,v 1.2.2.2 2015/09/22 12:05:55 skrll Exp $");

#include <sys/param.h>
#include <sys/dirent.h>
#include <sys/kauth.h>
#include <sys/module.h>
#include <sys/kobj.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscall.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

int
netbsd32_modctl(struct lwp *lwp, const struct netbsd32_modctl_args *uap,
	register_t *result)
{
	/* {
		syscallarg(int) cmd;
		syscallarg(netbsd32_voidp) arg;
	} */
	char buf[MAXMODNAME];
	size_t mslen;
	module_t *mod;
	modinfo_t *mi;
	modstat_t *ms, *mso;
	vaddr_t addr;
	size_t size;
	struct netbsd32_iovec iov;
	struct netbsd32_modctl_load ml;
	int error;
	void *arg;
#ifdef MODULAR
	uintptr_t loadtype;
#endif

	arg = SCARG_P32(uap, arg);

	switch (SCARG(uap, cmd)) {
	case MODCTL_LOAD:
		error = copyin(arg, &ml, sizeof(ml));
		if (error != 0)
			break;
		error = handle_modctl_load(NETBSD32PTR64(ml.ml_filename),
		     ml.ml_flags, NETBSD32PTR64(ml.ml_props), ml.ml_propslen);
		break;

	case MODCTL_UNLOAD:
		error = copyinstr(arg, buf, sizeof(buf), NULL);
		if (error == 0) {
			error = module_unload(buf);
		}
		break;

	case MODCTL_STAT:
		error = copyin(arg, &iov, sizeof(iov));
		if (error != 0) {
			break;
		}
		kernconfig_lock();
		mslen = (module_count+module_builtinlist+1) * sizeof(modstat_t);
		mso = kmem_zalloc(mslen, KM_SLEEP);
		if (mso == NULL) {
			kernconfig_unlock();
			return ENOMEM;
		}
		ms = mso;
		TAILQ_FOREACH(mod, &module_list, mod_chain) {
			mi = mod->mod_info;
			strlcpy(ms->ms_name, mi->mi_name, sizeof(ms->ms_name));
			if (mi->mi_required != NULL) {
				strlcpy(ms->ms_required, mi->mi_required,
				    sizeof(ms->ms_required));
			}
			if (mod->mod_kobj != NULL) {
				kobj_stat(mod->mod_kobj, &addr, &size);
				ms->ms_addr = addr;
				ms->ms_size = size;
			}
			ms->ms_class = mi->mi_class;
			ms->ms_refcnt = mod->mod_refcnt;
			ms->ms_source = mod->mod_source;
			ms++;
		}
		TAILQ_FOREACH(mod, &module_builtins, mod_chain) {
			mi = mod->mod_info;
			strlcpy(ms->ms_name, mi->mi_name, sizeof(ms->ms_name));
			if (mi->mi_required != NULL) {
				strlcpy(ms->ms_required, mi->mi_required,
				    sizeof(ms->ms_required));
			}
			if (mod->mod_kobj != NULL) {
				kobj_stat(mod->mod_kobj, &addr, &size);
				ms->ms_addr = addr;
				ms->ms_size = size;
			}
			ms->ms_class = mi->mi_class;
			ms->ms_refcnt = -1;
			KASSERT(mod->mod_source == MODULE_SOURCE_KERNEL);
			ms->ms_source = mod->mod_source;
			ms++;
		}
		kernconfig_unlock();
		error = copyout(mso, NETBSD32PTR64(iov.iov_base),
		    min(mslen - sizeof(modstat_t), iov.iov_len));
		kmem_free(mso, mslen);
		if (error == 0) {
			iov.iov_len = mslen - sizeof(modstat_t);
			error = copyout(&iov, arg, sizeof(iov));
		}
		break;

	case MODCTL_EXISTS:
#ifndef MODULAR
		error = ENOSYS;
#else
		loadtype = (uintptr_t)arg;
		switch (loadtype) {	/* 0 = modload, 1 = autoload */
		case 0:			/* FALLTHROUGH */
		case 1:
			error = kauth_authorize_system(kauth_cred_get(),
			     KAUTH_SYSTEM_MODULE, 0,
			     (void *)(uintptr_t)MODCTL_LOAD,
			     (void *)loadtype, NULL);
			break;

		default:
			error = EINVAL;
			break;
		}
#endif
		break;

	default:
		error = EINVAL;
		break;
	}

	return error;
}


