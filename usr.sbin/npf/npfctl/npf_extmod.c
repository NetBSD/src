/*	$NetBSD: npf_extmod.c,v 1.4 2013/03/10 23:57:07 christos Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mindaugas Rasiukevicius.
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
 * npfctl(8) extension loading interface.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: npf_extmod.c,v 1.4 2013/03/10 23:57:07 christos Exp $");

#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <err.h>
#include <dlfcn.h>

#include "npfctl.h"

struct npf_extmod {
	char *			name;
	npfext_initfunc_t	init;
	npfext_consfunc_t	cons;
	npfext_paramfunc_t	param;
	struct npf_extmod *	next;
};

static npf_extmod_t *		npf_extmod_list;

static void *
npf_extmod_sym(void *handle, const char *name, const char *func)
{
	char buf[64];
	void *sym;

	snprintf(buf, sizeof(buf), "npfext_%s_%s", name, func);
	sym = dlsym(handle, buf);
	if (sym == NULL) {
		errx(EXIT_FAILURE, "dlsym: %s", dlerror());
	}
	return sym;
}

static npf_extmod_t *
npf_extmod_load(const char *name)
{
	npf_extmod_t *ext;
	void *handle;
	char extlib[PATH_MAX];

	snprintf(extlib, sizeof(extlib), "/lib/npf/ext_%s.so", name);
	handle = dlopen(extlib, RTLD_LAZY | RTLD_LOCAL);
	if (handle == NULL) {
		errx(EXIT_FAILURE, "dlopen: %s", dlerror());
	}

	ext = ecalloc(1, sizeof(npf_extmod_t));
	ext->name = estrdup(name);
	ext->init = npf_extmod_sym(handle, name, "init");
	ext->cons = npf_extmod_sym(handle, name, "construct");
	ext->param = npf_extmod_sym(handle, name, "param");

	/* Initialise the module. */
	if (ext->init() != 0) {
		free(ext);
		return NULL;
	}

	ext->next = npf_extmod_list;
	npf_extmod_list = ext;
	return ext;
}

npf_extmod_t *
npf_extmod_get(const char *name, nl_ext_t **extcall)
{
	npf_extmod_t *extmod = npf_extmod_list;

	while (extmod) {
		if ((strcmp(extmod->name, name) == 0) &&
		    (*extcall = extmod->cons(name)) != NULL) {
			return extmod;
		}
		extmod = extmod->next;
	}

	extmod = npf_extmod_load(name);
	if (extmod && (*extcall = extmod->cons(name)) != NULL) {
		return extmod;
	}

	return NULL;
}

int
npf_extmod_param(npf_extmod_t *extmod, nl_ext_t *ext,
    const char *param, const char *val)
{
	return extmod->param(ext, param, val);
}
