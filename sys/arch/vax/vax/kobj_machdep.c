/*      $NetBSD: kobj_machdep.c,v 1.1 2018/04/30 06:46:12 ragge Exp $   */

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Anders Magnusson.
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

#include <sys/param.h>
#include <sys/kobj.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>

/*
 * VAX kernel uses two relocations;
 *	- R_VAX_PC32 which is PC relative offset
 *	- R_VAX_32 which is an absolute symbol offset
 */
int
kobj_reloc(kobj_t ko, uintptr_t relocbase, const void *data,
	bool isrela, bool local)
{
	const Elf_Rela *rela = (const Elf_Rela *)data;
	Elf_Word rtype = ELF_R_TYPE(rela->r_info);
	Elf_Word symidx = ELF_R_SYM(rela->r_info);
	Elf_Addr *where = (Elf_Addr *)(relocbase + rela->r_offset);
	Elf_Addr addr, addend = rela->r_addend;
	int error;

	if (!isrela) {
		printf("Elf_Rel not supported");
		return -1;
	}
	if (rtype != R_VAX_PC32 && rtype != R_VAX_32) {
		printf("Bad relocation %d", rtype);
		return -1;
	}

	if ((error = kobj_sym_lookup(ko, symidx, &addr)))
		return -1;
	addr += addend;
	if (rtype == R_VAX_PC32)
		addr -= (Elf_Addr)where + 4;

	*where = addr;
	return 0;
}

int
kobj_machdep(kobj_t ko, void *base, size_t size, bool load)
{
	return 0;
}
