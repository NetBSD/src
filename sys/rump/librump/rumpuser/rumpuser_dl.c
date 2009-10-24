/*      $NetBSD: rumpuser_dl.c,v 1.4 2009/10/24 11:36:59 pooka Exp $	*/

/*
 * Copyright (c) 2009 Antti Kantee.  All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Load all module link sets.  Called during rump bootstrap.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: rumpuser_dl.c,v 1.4 2009/10/24 11:36:59 pooka Exp $");

#include <sys/types.h>
#include <sys/time.h>

#include <assert.h>
#include <dlfcn.h>
#include <link_elf.h>
#include <stdio.h>
#include <string.h>

#include <rump/rumpuser.h>

#if defined(__NetBSD__) || defined(__FreeBSD__)				\
    || (defined(__sun__) && defined(__svr4__))
static void
process(const char *soname, rump_modinit_fn domodinit)
{
	void *handle;
	struct modinfo **mi, **mi_end;

	if (strstr(soname, "librump") == NULL)
		return;

	handle = dlopen(soname, RTLD_LAZY);
	if (handle == NULL)
		return;

	mi = dlsym(handle, "__start_link_set_modules");
	if (!mi)
		goto out;
	mi_end = dlsym(handle, "__stop_link_set_modules");
	if (!mi_end)
		goto out;

	for (; mi < mi_end; mi++)
		domodinit(*mi, NULL);
	assert(mi == mi_end);

 out:
	dlclose(handle);
}

/*
 * Get the linkmap from the dynlinker.  Try to load kernel modules
 * from all objects in the linkmap.
 */
void
rumpuser_dl_module_bootstrap(rump_modinit_fn domodinit)
{
	struct link_map *map;

	if (dlinfo(RTLD_SELF, RTLD_DI_LINKMAP, &map) == -1) {
		fprintf(stderr, "warning: rumpuser module bootstrap "
		    "failed: %s\n", dlerror());
		return;
	}

	/*
	 * Load starting from last object because of
	 * possible dependencies.
	 * XXX: not perfect.  this could retry the list until no (or all)
	 * modules were be loaded?
	 */
	for (; map->l_next; map = map->l_next)
		continue;
	for (; map; map = map->l_prev)
		process(map->l_name, domodinit);
}
#else
void
rumpuser_dl_module_bootstrap(void)
{

	fprintf(stderr, "Warning, dlinfo() unsupported on host?\n");
	fprintf(stderr, "module bootstrap unavailable\n");
}
#endif
