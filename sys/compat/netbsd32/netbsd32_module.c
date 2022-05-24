/*	$NetBSD: netbsd32_module.c,v 1.11 2022/05/24 06:20:04 andvar Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_module.c,v 1.11 2022/05/24 06:20:04 andvar Exp $");

#include <sys/param.h>
#include <sys/dirent.h>
#include <sys/kauth.h>
#include <sys/module.h>
#include <sys/kobj.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscall.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

static int
modctl32_handle_stat(struct netbsd32_iovec *iov, void *arg)
{
	int ms_cnt;
	modstat_t *ms, *mso;
	size_t ms_len;
	char *req, *reqo;
	size_t req_len;
	char *out_p;
	size_t out_s;

	modinfo_t *mi;
	module_t *mod;
	vaddr_t addr;
	size_t size;
	size_t used;
	int off;
	int error;
	bool stataddr;

	/* If not privileged, don't expose kernel addresses. */
	error = kauth_authorize_system(kauth_cred_get(), KAUTH_SYSTEM_MODULE,
	    0, (void *)(uintptr_t)MODCTL_STAT, NULL, NULL);
	stataddr = (error == 0);

	kernconfig_lock();
	ms_cnt = 0;
	req_len = 1;

	/*
	 * Count up the number of modstat_t needed, and total size of
	 * require_module lists on both active and built-in lists
	 */
	TAILQ_FOREACH(mod, &module_list, mod_chain) {
		ms_cnt++;
		mi = mod->mod_info;
		if (mi->mi_required != NULL) {
			req_len += strlen(mi->mi_required) + 1;
		}
	}
	TAILQ_FOREACH(mod, &module_builtins, mod_chain) {
		ms_cnt++;
		mi = mod->mod_info;
		if (mi->mi_required != NULL) {
			req_len += strlen(mi->mi_required) + 1;
		}
	}

	/* Allocate internal buffers to hold all the output data */
	ms_len = ms_cnt * sizeof(modstat_t);
	ms = kmem_zalloc(ms_len, KM_SLEEP);
	req = kmem_zalloc(req_len, KM_SLEEP);

	mso = ms;
	reqo = req++;
	off = 1;

	/*
	 * Load data into our internal buffers for both active and
	 * built-in module lists
	 */
	TAILQ_FOREACH(mod, &module_list, mod_chain) {
		mi = mod->mod_info;
		strlcpy(ms->ms_name, mi->mi_name, sizeof(ms->ms_name));
		if (mi->mi_required != NULL) {
			ms->ms_reqoffset = off;
			used = strlcpy(req,  mi->mi_required, req_len - off);
			KASSERTMSG(used < req_len - off, "reqlist grew!");
			off = used + 1;
			req += used + 1;
		} else
			ms->ms_reqoffset = 0;
		if (mod->mod_kobj != NULL && stataddr) {
			kobj_stat(mod->mod_kobj, &addr, &size);
			ms->ms_addr = addr;
			ms->ms_size = size;
		}
		ms->ms_class = mi->mi_class;
		ms->ms_refcnt = mod->mod_refcnt;
		ms->ms_source = mod->mod_source;
		ms->ms_flags = mod->mod_flags;
		ms++;
	}
	TAILQ_FOREACH(mod, &module_builtins, mod_chain) {
		mi = mod->mod_info;
		strlcpy(ms->ms_name, mi->mi_name, sizeof(ms->ms_name));
		if (mi->mi_required != NULL) {
			ms->ms_reqoffset = off;
			used = strlcpy(req,  mi->mi_required, req_len - off);
			KASSERTMSG(used < req_len - off, "reqlist grew!");
			off += used + 1;
			req += used + 1;
		} else
			ms->ms_reqoffset = 0;
		if (mod->mod_kobj != NULL && stataddr) {
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

	/*
	 * Now copyout our internal buffers back to userland
	 */
	out_p = NETBSD32PTR64(iov->iov_base);
	out_s = iov->iov_len;
	size = sizeof(ms_cnt);

	/* Copy out the count of modstat_t */
	if (out_s) {
		size = uimin(sizeof(ms_cnt), out_s);
		error = copyout(&ms_cnt, out_p, size);
		out_p += size;
		out_s -= size;
	}
	/* Copy out the modstat_t array */
	if (out_s && error == 0) {
		size = uimin(ms_len, out_s);
		error = copyout(mso, out_p, size);
		out_p += size;
		out_s -= size;
	}
	/* Copy out the "required" strings */
	if (out_s && error == 0) {
		size = uimin(req_len, out_s);
		error = copyout(reqo, out_p, size);
		out_p += size;
		out_s -= size;
	}
	kmem_free(mso, ms_len);
	kmem_free(reqo, req_len);

	/* Finally, update the userland copy of the iovec's length */
	if (error == 0) {
		iov->iov_len = ms_len + req_len + sizeof(ms_cnt);
		error = copyout(iov, arg, sizeof(*iov));
	}

	return error;
}

int
compat32_80_modctl_compat_stub(struct lwp *lwp,
    const struct netbsd32_modctl_args *uap, register_t *result)
{

	return EPASSTHROUGH;
}

int
netbsd32_modctl(struct lwp *lwp, const struct netbsd32_modctl_args *uap,
	register_t *result)
{
	/* {
		syscallarg(int) cmd;
		syscallarg(netbsd32_voidp) arg;
	} */
	char buf[MAXMODNAME];
	struct netbsd32_iovec iov;
	struct netbsd32_modctl_load ml;
	int error;
	void *arg;
#ifdef MODULAR
	uintptr_t loadtype;
#endif

	arg = SCARG_P32(uap, arg);

	MODULE_HOOK_CALL(compat32_80_modctl_hook, (lwp, uap, result),
	    enosys(), error);
	if (error != EPASSTHROUGH && error != ENOSYS)
		return error;

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
		error = modctl32_handle_stat(&iov, arg);
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
