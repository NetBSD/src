/*	$NetBSD: kobj_machdep.c,v 1.6 2019/04/09 00:16:30 uwe Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: kobj_machdep.c,v 1.6 2019/04/09 00:16:30 uwe Exp $");

#define	ELFSIZE		ARCH_ELFSIZE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kobj.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>

#include <sh3/cache.h>

int
kobj_reloc(kobj_t ko, uintptr_t relocbase, const void *data,
	   bool isrela, bool local)
{
	const Elf_Rela *rela;
	Elf_Addr *where;
	uintptr_t addr, tmp;
	int symidx, rtype;
	int error;

	if (!isrela) {
		printf("kobj_reloc: support only RELA relocations\n");
		return -1;
	}

	rela = data;
	where = (Elf_Addr *)(relocbase + rela->r_offset);
	symidx = ELF_R_SYM(rela->r_info);
	rtype = ELF_R_TYPE(rela->r_info);

	switch (rtype) {
	case R_TYPE(NONE):
		break;

	case R_TYPE(DIR32):
	case R_TYPE(REL32):
		error = kobj_sym_lookup(ko, symidx, &addr);
		if (error)
			return -1;

		tmp = (Elf_Addr)(addr + *where + rela->r_addend);
		if (rtype == R_TYPE(REL32))
			tmp -= rela->r_offset;
		*where = tmp;
		break;

	default:
		printf("kobj_reloc: unexpected reloc type %d\n", (int)rtype);
		return -1;
	}

	return 0;
}

int
kobj_machdep(kobj_t ko, void *base, size_t size, bool load)
{

	if (load)
		sh_icache_sync_range((vaddr_t)base, size);

	return 0;
}
