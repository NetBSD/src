/*	$NetBSD: libkmod.c,v 1.1 2009/06/21 21:11:16 agc Exp $	*/

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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: libkmod.c,v 1.1 2009/06/21 21:11:16 agc Exp $");
#endif /* !lint */

#include <sys/module.h>

#include <prop/proplib.h>

#include <assert.h>
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libkmod.h"

#ifdef USE_LIBKMOD

static const char *classes[] = {
	"any",
	"misc",
	"vfs",
	"driver",
	"exec",
};

static const char *sources[] = {
	"builtin",
	"boot",
	"filesys",
};

/* comparison routine for qsort */
static int
modcmp(const void *a, const void *b)
{
	const modstat_t *msa, *msb;

	msa = a;
	msb = b;
	return strcmp(msa->ms_name, msb->ms_name);
}

/* function which "opens" a module */
int
openkmod(kernel_t *kernel)
{
	kernel->size = 4096;
	for (;;) {
		kernel->iov.iov_base = malloc(kernel->size);
		kernel->iov.iov_len = kernel->size;
		if (modctl(MODCTL_STAT, &kernel->iov)) {
			warn("modctl(MODCTL_STAT)");
			return 0;
		}
		if (kernel->size >= kernel->iov.iov_len) {
			break;
		}
		free(kernel->iov.iov_base);
		kernel->size = kernel->iov.iov_len;
	}
	kernel->size = kernel->iov.iov_len / sizeof(modstat_t);
	qsort(kernel->iov.iov_base, kernel->size, sizeof(modstat_t), modcmp);
	return 1;
}

/* return details on the module */
int
readkmod(kernel_t *kernel, kmod_t *module)
{
	modstat_t *ms;

	if (kernel->c * sizeof(*ms) >= kernel->iov.iov_len) {
		return 0;
	}
	ms = kernel->iov.iov_base;
	ms += kernel->c++;
	(void) memset(module, 0x0, sizeof(*module));
	module->name = strdup(ms->ms_name);
	module->class = strdup(classes[ms->ms_class]);
	module->source = strdup(sources[ms->ms_source]);
	module->refcnt = ms->ms_refcnt;
	module->size = ms->ms_size;
	if (ms->ms_required[0]) {
		module->required = strdup(ms->ms_required);
	}
	return 1;
}

/* free up all resources allocated in a module read */
void
freekmod(kmod_t *module)
{
	(void) free(module->name);
	(void) free(module->class);
	(void) free(module->source);
	if (module->required) {
		(void) free(module->required);
	}
}

/* "close" the module */
int
closekmod(kernel_t *kernel)
{
	(void) free(kernel->iov.iov_base);
	return 1;
}

/* do the modstat operation */
int
kmodstat(const char *modname, FILE *fp)
{
	kernel_t	kernel;
	kmod_t		module;
	int		modc;

	(void) memset(&kernel, 0x0, sizeof(kernel));
	(void) memset(&module, 0x0, sizeof(module));
	if (!openkmod(&kernel)) {
		(void) fprintf(stderr, "can't read kernel modules\n");
		return 0;
	}
	for (modc = 0 ; readkmod(&kernel, &module) ; ) {
		if (modname == NULL || strcmp(modname, module.name) == 0) {
			if (modc++ == 0) {
				if (fp) {
					(void) fprintf(fp,
						"NAME\t\tCLASS\tSOURCE\tREFS"
						"\tSIZE\tREQUIRES\n");
				}
			}
			if (fp) {
				(void) fprintf(fp, "%-16s%s\t%s\t%d\t%u\t%s\n",
				module.name,
				module.class,
				module.source,
				module.refcnt,
				module.size,
				(module.required) ? module.required : "-");
			}
			freekmod(&module);
		}
	}
	(void) closekmod(&kernel);
	return modc;
}

/* load the named module into the kernel */
int
kmodload(const char *module)
{
	prop_dictionary_t	 props;
	modctl_load_t		 cmdargs;
	char			*propsstr;

	props = prop_dictionary_create();

	propsstr = prop_dictionary_externalize(props);
	if (propsstr == NULL) {
		(void) fprintf(stderr, "Failed to process properties");
		return 0;
	}

	cmdargs.ml_filename = module;
	cmdargs.ml_flags = 0;
	cmdargs.ml_props = propsstr;
	cmdargs.ml_propslen = strlen(propsstr);

	if (modctl(MODCTL_LOAD, &cmdargs)) {
		(void) fprintf(stderr, "modctl failure\n");
		return 0;
	}

	(void) free(propsstr);
	prop_object_release(props);

	return 1;
}

/* and unload the module from the kernel */
int
kmodunload(const char *name)
{
	return modctl(MODCTL_UNLOAD, __UNCONST(name)) == 0;
}

#endif /* USE_LIBKMOD */
