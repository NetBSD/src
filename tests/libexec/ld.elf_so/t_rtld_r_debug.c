/*	$NetBSD: t_rtld_r_debug.c,v 1.6 2024/09/08 09:36:52 rillig Exp $	*/

/*
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>

#include <atf-c.h>
#include <dlfcn.h>
#include <link_elf.h>
#include <stdbool.h>

#include "h_macros.h"

static long int
getauxval(unsigned int type)
{
	const AuxInfo *aux;

	for (aux = _dlauxinfo(); aux->a_type != AT_NULL; ++aux) {
		if (type == aux->a_type)
			return aux->a_v;
	}

	return 0;
}

static Elf_Dyn *
get_dynamic_section(void)
{
	uintptr_t relocbase = (uintptr_t)~0U;
	const Elf_Phdr *phdr;
	Elf_Half phnum;
	const Elf_Phdr *phlimit, *dynphdr;

	phdr = (void *)getauxval(AT_PHDR);
	phnum = (Elf_Half)getauxval(AT_PHNUM);

	ATF_CHECK(phdr != NULL);
	ATF_CHECK(phnum != (Elf_Half)~0);

	phlimit = phdr + phnum;
	dynphdr = NULL;

	for (; phdr < phlimit; ++phdr) {
		if (phdr->p_type == PT_DYNAMIC)
			dynphdr = phdr;
		if (phdr->p_type == PT_PHDR)
			relocbase = (uintptr_t)phdr - phdr->p_vaddr;
	}

	return (Elf_Dyn *)((uint8_t *)dynphdr->p_vaddr + relocbase);
}

static struct r_debug *
get_rtld_r_debug(void)
{
	struct r_debug *debug = NULL;
	Elf_Dyn *dynp;

	for (dynp = get_dynamic_section(); dynp->d_tag != DT_NULL; dynp++) {
		if (dynp->d_tag == DT_DEBUG) {
			debug = (void *)dynp->d_un.d_val;
			break;
		}
	}
	ATF_CHECK(debug != NULL);

	return debug;
}

static void
check_r_debug_return_link_map(const char *name, struct link_map **rmap)
{
	struct r_debug *debug;
	struct link_map *map;
	void *loader;
	bool found;

	loader = NULL;
	debug = get_rtld_r_debug();
	ATF_CHECK(debug != NULL);
	ATF_CHECK_EQ_MSG(debug->r_version, R_DEBUG_VERSION,
	    "debug->r_version=%d R_DEBUG_VERSION=%d",
	    debug->r_version, R_DEBUG_VERSION);
	map = debug->r_map;
	ATF_CHECK(map != NULL);

	for (found = false; map; map = map->l_next) {
		if (strstr(map->l_name, name) != NULL) {
			if (rmap)
				*rmap = map;
			found = true;
		} else if (strstr(map->l_name, "ld.elf_so") != NULL) {
			loader = (void *)map->l_addr;
		}
	}
	ATF_CHECK(found);
	ATF_CHECK(loader != NULL);
	ATF_CHECK(debug->r_brk != NULL);
	ATF_CHECK_EQ_MSG(debug->r_state, RT_CONSISTENT,
	    "debug->r_state=%d RT_CONSISTENT=%d",
	    debug->r_state, RT_CONSISTENT);
	ATF_CHECK_EQ_MSG(debug->r_ldbase, loader,
	    "debug->r_ldbase=%p loader=%p",
	    debug->r_ldbase, loader);
}

ATF_TC(self);
ATF_TC_HEAD(self, tc)
{
	atf_tc_set_md_var(tc, "descr", "check whether r_debug is well-formed");
}
ATF_TC_BODY(self, tc)
{
	check_r_debug_return_link_map("t_rtld_r_debug", NULL);
}

ATF_TC(dlopen);
ATF_TC_HEAD(dlopen, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "check whether r_debug is well-formed after a dlopen(3) call");
}
ATF_TC_BODY(dlopen, tc)
{
	void *handle;
	struct link_map *map, *r_map;

	handle = dlopen("libutil.so", RTLD_LAZY);
	ATF_REQUIRE_MSG(handle, "dlopen: %s", dlerror());

	check_r_debug_return_link_map("libutil.so", &r_map);

	ATF_REQUIRE_EQ_MSG(dlinfo(handle, RTLD_DI_LINKMAP, &map), 0,
	    "dlinfo: %s", dlerror());

	ATF_CHECK_EQ_MSG(map, r_map, "map=%p r_map=%p", map, r_map);
	ATF_CHECK_EQ_MSG(dlclose(handle), 0, "dlclose: %s", dlerror());
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, self);
	ATF_TP_ADD_TC(tp, dlopen);
	return atf_no_error();
}
