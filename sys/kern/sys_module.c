/*	$NetBSD: sys_module.c,v 1.23.2.7 2018/07/08 07:33:14 pgoyette Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: sys_module.c,v 1.23.2.7 2018/07/08 07:33:14 pgoyette Exp $");

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

/*
 * Arbitrary limit to avoid DoS for excessive memory allocation.
 */
#define MAXPROPSLEN	4096

int
handle_modctl_load(const char *ml_filename, int ml_flags, const char *ml_props,
    size_t ml_propslen)
{
	char *path;
	char *props;
	int error;
	prop_dictionary_t dict;
	size_t propslen = 0;

	if ((ml_props != NULL && ml_propslen == 0) ||
	    (ml_props == NULL && ml_propslen > 0)) {
		return EINVAL;
	}

	path = PNBUF_GET();
	error = copyinstr(ml_filename, path, MAXPATHLEN, NULL);
	if (error != 0)
		goto out1;

	if (ml_props != NULL) {
		if (ml_propslen > MAXPROPSLEN) {
			error = ENOMEM;
			goto out1;
		}
		propslen = ml_propslen + 1;

		props = kmem_alloc(propslen, KM_SLEEP);
		error = copyinstr(ml_props, props, propslen, NULL);
		if (error != 0)
			goto out2;

		dict = prop_dictionary_internalize(props);
		if (dict == NULL) {
			error = EINVAL;
			goto out2;
		}
	} else {
		dict = NULL;
		props = NULL;
	}

	error = module_load(path, ml_flags, dict, MODULE_CLASS_ANY);

	if (dict != NULL) {
		prop_object_release(dict);
	}

out2:
	if (props != NULL) {
		kmem_free(props, propslen);
	}
out1:
	PNBUF_PUT(path);
	return error;
}

static void
copy_alias(modstat_t *ms, const char * const *aliasp, modinfo_t *mi,
    module_t *mod)
{

	strlcpy(ms->ms_name, *aliasp, sizeof(ms->ms_name));
	ms->ms_class = mi->mi_class;
	ms->ms_source = mod->mod_source;
	ms->ms_flags = mod->mod_flags;
	SET(ms->ms_flags, MODFLG_IS_ALIAS);
	ms->ms_reqoffset = 0;
}

static int
handle_modctl_stat(struct iovec *iov, void *arg)
{
	int ms_cnt;
	modstat_t *ms, *mso;
	size_t ms_len;
	int req_cnt;
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
	const char * const *aliasp;

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
		if ((aliasp = *mi->mi_aliases) != NULL) {
			while (*aliasp++ != NULL)
				ms_cnt++;
		}
		if (mi->mi_required != NULL) {
			req_cnt++;
			req_len += strlen(mi->mi_required) + 1;
		}
	}
	TAILQ_FOREACH(mod, &module_builtins, mod_chain) {
		ms_cnt++;
		mi = mod->mod_info;
		if ((aliasp = *mi->mi_aliases) != NULL) {
			while (*aliasp++ != NULL)
				ms_cnt++;
		}
		if (mi->mi_required != NULL) {
			req_cnt++;
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
	 * build-in module lists
	 */
	TAILQ_FOREACH(mod, &module_list, mod_chain) {
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
		ms->ms_refcnt = mod->mod_refcnt;
		ms->ms_source = mod->mod_source;
		ms->ms_flags = mod->mod_flags;
		ms++;
		aliasp = *mi->mi_aliases;
		if (aliasp == NULL)
			continue;
		while (*aliasp) {
			copy_alias(ms, aliasp, mi, mod);
			aliasp++;
			ms++;
		}
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
		aliasp = *mi->mi_aliases;
		if (aliasp == NULL)
			continue;
		while (*aliasp) {
			copy_alias(ms, aliasp, mi, mod);
			aliasp++;
			ms++;
		}
	}
	kernconfig_unlock();

	/*
	 * Now copyout our internal buffers back to userland
	 */
	out_p = iov->iov_base;
	out_s = iov->iov_len;
	size = sizeof(ms_cnt);

	/* Copy out the count of modstat_t */
	if (out_s) {
		size = min(sizeof(ms_cnt), out_s);
		error = copyout(&ms_cnt, out_p, size);
		out_p += size;
		out_s -= size;
	}
	/* Copy out the modstat_t array */
	if (out_s && error == 0) {
		size = min(ms_len, out_s);
		error = copyout(mso, out_p, size);
		out_p += size;
		out_s -= size;
	}
	/* Copy out the "required" strings */
	if (out_s && error == 0) {
		size = min(req_len, out_s);
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
sys_modctl(struct lwp *l, const struct sys_modctl_args *uap,
	   register_t *retval)
{
	/* {
		syscallarg(int)		cmd;
		syscallarg(void *)	arg;
	} */
	char buf[MAXMODNAME];
	struct iovec iov;
	modctl_load_t ml;
	int error;
	void *arg;
#ifdef MODULAR
	uintptr_t loadtype;
#endif

	arg = SCARG(uap, arg);

	switch (SCARG(uap, cmd)) {
	case MODCTL_LOAD:
		error = copyin(arg, &ml, sizeof(ml));
		if (error != 0)
			break;
		error = handle_modctl_load(ml.ml_filename, ml.ml_flags,
		    ml.ml_props, ml.ml_propslen);
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
		error = handle_modctl_stat(&iov, arg);
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
		error = (*compat_modstat_80)(SCARG(uap, cmd), &iov, arg);
		if (error == ENOSYS)
			error = EINVAL;
		break;
	}

	return error;
}
