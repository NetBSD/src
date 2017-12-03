/*	$NetBSD: self_reloc.c,v 1.1.18.2 2017/12/03 11:36:18 jdolecek Exp $	*/

/*-
 * Copyright (c) 2008-2010 Rui Paulo <rpaulo@FreeBSD.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/exec_elf.h>

#include <efi.h>

void self_reloc(Elf_Addr, Elf_Dyn *);

#if defined(__aarch64__)
#define	USE_RELA
#endif

/*
 * A simple elf relocator.
 */
void
self_reloc(Elf_Addr baseaddr, Elf_Dyn *dynamic)
{
	Elf_Word relsz, relent;
	Elf_Addr *newaddr;
	Elf_Rel *rel = NULL;
	Elf_Dyn *dynp;

	/*
	 * Find the relocation address, its size and the relocation entry.
	 */
	relsz = 0;
	relent = 0;
	for (dynp = dynamic; dynp->d_tag != DT_NULL; dynp++) {
		switch (dynp->d_tag) {
		case DT_REL:
		case DT_RELA:
			rel = (Elf_Rel *)(dynp->d_un.d_ptr + baseaddr);
			break;
		case DT_RELSZ:
		case DT_RELASZ:
			relsz = dynp->d_un.d_val;
			break;
		case DT_RELENT:
		case DT_RELAENT:
			relent = dynp->d_un.d_val;
			break;
		default:
			break;
		}
	}

	/*
	 * Perform the actual relocation.
	 */
	for (; relsz > 0; relsz -= relent) {
		switch (ELF_R_TYPE(rel->r_info)) {
		case R_TYPE(NONE):
			/* No relocation needs be performed. */
			break;

		case R_TYPE(RELATIVE):
			/* Address relative to the base address. */
			newaddr = (Elf_Addr *)(rel->r_offset + baseaddr);
			*newaddr += baseaddr;
#ifdef USE_RELA
			/* Add the addend when the ABI uses them */
			*newaddr += ((Elf_Rela *)rel)->r_addend;
#endif
			break;
		default:
			/* XXX: do we need other relocations ? */
			break;
		}
		rel = (Elf_Rel *) ((char *) rel + relent);
	}
}
