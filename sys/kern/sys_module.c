/*	$NetBSD: sys_module.c,v 1.8.4.1.2.1 2014/07/14 09:10:18 msaitoh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: sys_module.c,v 1.8.4.1.2.1 2014/07/14 09:10:18 msaitoh Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/namei.h>
#include <sys/kmem.h>
#include <sys/kobj.h>
#include <sys/module.h>
#include <sys/syscall.h>
#include <sys/syscallargs.h>

/*
 * Arbitrary limit to avoid DoS for excessive memory allocation.
 */
#define MAXPROPSLEN	4096

static int
handle_modctl_load(modctl_load_t *ml)
{
	char *path;
	char *props;
	int error;
	prop_dictionary_t dict;
	size_t propslen;

	if ((ml->ml_props != NULL && ml->ml_propslen == 0) ||
	    (ml->ml_props == NULL && ml->ml_propslen > 0)) {
		error = EINVAL;
		goto out1;
	}

	path = PNBUF_GET();
	error = copyinstr(ml->ml_filename, path, MAXPATHLEN, NULL);
	if (error != 0)
		goto out2;

	if (ml->ml_propslen > MAXPROPSLEN) {
		error = ENOMEM;
		goto out2;
	}

	propslen = ml->ml_propslen + 1;
	props = (char *)kmem_alloc(propslen, KM_SLEEP);
	if (props == NULL) {
		error = ENOMEM;
		goto out2;
	}

	error = copyinstr(ml->ml_props, props, propslen, NULL);
	if (error != 0)
		goto out3;

	dict = prop_dictionary_internalize(props);
	if (dict == NULL) {
		error = EINVAL;
		goto out3;
	}

	error = module_load(path, ml->ml_flags, dict, MODULE_CLASS_ANY);

	prop_object_release(dict);

out3:
	kmem_free(props, propslen);
out2:
	PNBUF_PUT(path);
out1:

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
	size_t mslen;
	module_t *mod;
	modinfo_t *mi;
	modstat_t *ms, *mso;
	vaddr_t addr;
	size_t size;
	struct iovec iov;
	modctl_load_t ml;
	int error;
	void *arg;

	arg = SCARG(uap, arg);

	switch (SCARG(uap, cmd)) {
	case MODCTL_LOAD:
		error = copyin(arg, &ml, sizeof(ml));
		if (error != 0)
			break;
		error = handle_modctl_load(&ml);
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
		mutex_enter(&module_lock);
		mslen = (module_count + 1) * sizeof(modstat_t);
		mso = kmem_zalloc(mslen, KM_SLEEP);
		if (mso == NULL) {
			mutex_exit(&module_lock);
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
		mutex_exit(&module_lock);
		error = copyout(mso, iov.iov_base,
		    min(mslen - sizeof(modstat_t), iov.iov_len));
		kmem_free(mso, mslen);
		if (error == 0) {
			iov.iov_len = mslen - sizeof(modstat_t);
			error = copyout(&iov, arg, sizeof(iov));
		}
		break;

	default:
		error = EINVAL;
		break;
	}

	return error;
}
