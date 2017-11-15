/*	$NetBSD: elf.c,v 1.15 2017/11/15 20:45:16 maxv Exp $	*/

/*
 * Copyright (c) 2017 The NetBSD Foundation, Inc. All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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

#define	ELFSIZE	64

#include "prekern.h"
#include <sys/exec_elf.h>

struct elfinfo {
	Elf_Ehdr *ehdr;
	Elf_Shdr *shdr;
	char *shstrtab;
	size_t shstrsz;
	Elf_Sym *symtab;
	size_t symcnt;
	char *strtab;
	size_t strsz;
};

extern paddr_t kernpa_start, kernpa_end;

static struct elfinfo eif;
static const char entrypoint[] = "start_prekern";

static int
elf_check_header(void)
{
	if (memcmp((char *)eif.ehdr->e_ident, ELFMAG, SELFMAG) != 0 ||
	    eif.ehdr->e_ident[EI_CLASS] != ELFCLASS ||
	    eif.ehdr->e_type != ET_REL) {
		return -1;
	}
	return 0;
}

static vaddr_t
elf_get_entrypoint(void)
{
	Elf_Sym *sym;
	size_t i;
	char *buf;

	for (i = 0; i < eif.symcnt; i++) {
		sym = &eif.symtab[i];

		if (ELF_ST_TYPE(sym->st_info) != STT_FUNC)
			continue;
		if (sym->st_name == 0)
			continue;
		if (sym->st_shndx == SHN_UNDEF)
			continue; /* Skip external references */
		buf = eif.strtab + sym->st_name;

		if (!memcmp(buf, entrypoint, sizeof(entrypoint))) {
			return (vaddr_t)sym->st_value;
		}
	}

	return 0;
}

static Elf_Shdr *
elf_find_section(char *name)
{
	char *buf;
	size_t i;

	for (i = 0; i < eif.ehdr->e_shnum; i++) {
		if (eif.shdr[i].sh_name == 0) {
			continue;
		}
		buf = eif.shstrtab + eif.shdr[i].sh_name;
		if (!strcmp(name, buf)) {
			return &eif.shdr[i];
		}
	}

	return NULL;
}

static uintptr_t
elf_sym_lookup(size_t symidx)
{
	const Elf_Sym *sym;
	char *buf, *secname;
	Elf_Shdr *sec;

	if (symidx == STN_UNDEF) {
		return 0;
	}

	if (symidx >= eif.symcnt) {
		fatal("elf_sym_lookup: symbol beyond table");
	}
	sym = &eif.symtab[symidx];
	buf = eif.strtab + sym->st_name;

	if (sym->st_shndx == SHN_UNDEF) {
		if (!memcmp(buf, "__start_link_set", 16)) {
			secname = buf + 8;
			sec = elf_find_section(secname);
			if (sec == NULL) {
				fatal("elf_sym_lookup: unknown start link set");
			}
			return (uintptr_t)((uint8_t *)eif.ehdr +
			    sec->sh_offset);
		}
		if (!memcmp(buf, "__stop_link_set", 15)) {
			secname = buf + 7;
			sec = elf_find_section(secname);
			if (sec == NULL) {
				fatal("elf_sym_lookup: unknown stop link set");
			}
			return (uintptr_t)((uint8_t *)eif.ehdr +
			    sec->sh_offset + sec->sh_size);
		}

		fatal("elf_sym_lookup: external symbol");
	}
	if (sym->st_value == 0) {
		fatal("elf_sym_lookup: zero value");
	}
	return (uintptr_t)sym->st_value;
}

static void
elf_apply_reloc(uintptr_t relocbase, const void *data, bool isrela)
{
	Elf64_Addr *where, val;
	Elf32_Addr *where32, val32;
	Elf64_Addr addr;
	Elf64_Addr addend;
	uintptr_t rtype, symidx;
	const Elf_Rel *rel;
	const Elf_Rela *rela;

	if (isrela) {
		rela = (const Elf_Rela *)data;
		where = (Elf64_Addr *)(relocbase + rela->r_offset);
		addend = rela->r_addend;
		rtype = ELF_R_TYPE(rela->r_info);
		symidx = ELF_R_SYM(rela->r_info);
	} else {
		rel = (const Elf_Rel *)data;
		where = (Elf64_Addr *)(relocbase + rel->r_offset);
		rtype = ELF_R_TYPE(rel->r_info);
		symidx = ELF_R_SYM(rel->r_info);
		/* Addend is 32 bit on 32 bit relocs */
		switch (rtype) {
		case R_X86_64_PC32:
		case R_X86_64_32:
		case R_X86_64_32S:
			addend = *(Elf32_Addr *)where;
			break;
		default:
			addend = *where;
			break;
		}
	}

	switch (rtype) {
	case R_X86_64_NONE:	/* none */
		break;

	case R_X86_64_64:		/* S + A */
		addr = elf_sym_lookup(symidx);
		val = addr + addend;
		*where = val;
		break;

	case R_X86_64_PC32:	/* S + A - P */
		addr = elf_sym_lookup(symidx);
		where32 = (Elf32_Addr *)where;
		val32 = (Elf32_Addr)(addr + addend - (Elf64_Addr)where);
		*where32 = val32;
		break;

	case R_X86_64_32:	/* S + A */
	case R_X86_64_32S:	/* S + A sign extend */
		addr = elf_sym_lookup(symidx);
		val32 = (Elf32_Addr)(addr + addend);
		where32 = (Elf32_Addr *)where;
		*where32 = val32;
		break;

	case R_X86_64_GLOB_DAT:	/* S */
	case R_X86_64_JUMP_SLOT:/* XXX need addend + offset */
		addr = elf_sym_lookup(symidx);
		*where = addr;
		break;

	case R_X86_64_RELATIVE:	/* B + A */
		addr = relocbase + addend;
		val = addr;
		*where = val;
		break;

	default:
		fatal("elf_apply_reloc: unexpected relocation type");
	}
}

/* -------------------------------------------------------------------------- */

size_t
elf_get_head_size(vaddr_t headva)
{
	Elf_Ehdr *ehdr;
	Elf_Shdr *shdr;
	size_t size;

	ehdr = (Elf_Ehdr *)headva;
	shdr = (Elf_Shdr *)((uint8_t *)ehdr + ehdr->e_shoff);

	size = (vaddr_t)shdr + (vaddr_t)(ehdr->e_shnum * sizeof(Elf_Shdr)) -
	    (vaddr_t)ehdr;

	return roundup(size, PAGE_SIZE);
}

void
elf_build_head(vaddr_t headva)
{
	memset(&eif, 0, sizeof(struct elfinfo));

	eif.ehdr = (Elf_Ehdr *)headva;
	eif.shdr = (Elf_Shdr *)((uint8_t *)eif.ehdr + eif.ehdr->e_shoff);

	if (elf_check_header() == -1) {
		fatal("elf_build_head: wrong kernel ELF header");
	}
}

void
elf_map_sections(void)
{
	const paddr_t basepa = kernpa_start;
	const vaddr_t headva = (vaddr_t)eif.ehdr;
	Elf_Shdr *shdr;
	int segtype;
	vaddr_t secva;
	paddr_t secpa;
	size_t i, secsz, secalign;

	for (i = 0; i < eif.ehdr->e_shnum; i++) {
		shdr = &eif.shdr[i];

		if (!(shdr->sh_flags & SHF_ALLOC)) {
			continue;
		}
		if (shdr->sh_type != SHT_NOBITS &&
		    shdr->sh_type != SHT_PROGBITS) {
			continue;
		}

		if (shdr->sh_flags & SHF_EXECINSTR) {
			segtype = BTSEG_TEXT;
		} else if (shdr->sh_flags & SHF_WRITE) {
			segtype = BTSEG_DATA;
		} else {
			segtype = BTSEG_RODATA;
		}
		secpa = basepa + shdr->sh_offset;
		secsz = shdr->sh_size;
		secalign = shdr->sh_addralign;
		ASSERT(shdr->sh_offset != 0);
		ASSERT(secpa % PAGE_SIZE == 0);

		secva = mm_map_segment(segtype, secpa, secsz, secalign);

		/* We want (headva + sh_offset) to be the VA of the section. */
		ASSERT(secva > headva);
		shdr->sh_offset = secva - headva;
	}
}

void
elf_build_boot(vaddr_t bootva, paddr_t bootpa)
{
	const paddr_t basepa = kernpa_start;
	const vaddr_t headva = (vaddr_t)eif.ehdr;
	size_t i, j, offboot;

	for (i = 0; i < eif.ehdr->e_shnum; i++) {
		if (eif.shdr[i].sh_type != SHT_STRTAB &&
		    eif.shdr[i].sh_type != SHT_REL &&
		    eif.shdr[i].sh_type != SHT_RELA &&
		    eif.shdr[i].sh_type != SHT_SYMTAB) {
			continue;
		}
		if (eif.shdr[i].sh_offset == 0) {
			/* hasn't been loaded */
			continue;
		}

		/* Offset of the section within the boot region. */
		offboot = basepa + eif.shdr[i].sh_offset - bootpa;

		/* We want (headva + sh_offset) to be the VA of the region. */
		eif.shdr[i].sh_offset = (bootva + offboot - headva);
	}

	/* Locate the section names */
	j = eif.ehdr->e_shstrndx;
	if (j == SHN_UNDEF) {
		fatal("elf_build_boot: shstrtab not found");
	}
	if (j >= eif.ehdr->e_shnum) {
		fatal("elf_build_boot: wrong shstrtab index");
	}
	eif.shstrtab = (char *)((uint8_t *)eif.ehdr + eif.shdr[j].sh_offset);
	eif.shstrsz = eif.shdr[j].sh_size;

	/* Locate the symbol table */
	for (i = 0; i < eif.ehdr->e_shnum; i++) {
		if (eif.shdr[i].sh_type == SHT_SYMTAB)
			break;
	}
	if (i == eif.ehdr->e_shnum) {
		fatal("elf_build_boot: symtab not found");
	}
	eif.symtab = (Elf_Sym *)((uint8_t *)eif.ehdr + eif.shdr[i].sh_offset);
	eif.symcnt = eif.shdr[i].sh_size / sizeof(Elf_Sym);

	/* Also locate the string table */
	j = eif.shdr[i].sh_link;
	if (j == SHN_UNDEF || j >= eif.ehdr->e_shnum) {
		fatal("elf_build_boot: wrong strtab index");
	}
	if (eif.shdr[j].sh_type != SHT_STRTAB) {
		fatal("elf_build_boot: wrong strtab type");
	}
	eif.strtab = (char *)((uint8_t *)eif.ehdr + eif.shdr[j].sh_offset);
	eif.strsz = eif.shdr[j].sh_size;
}

vaddr_t
elf_kernel_reloc(void)
{
	const vaddr_t baseva = (vaddr_t)eif.ehdr;
	vaddr_t secva, ent;
	Elf_Sym *sym;
	size_t i, j;

	print_state(true, "ELF info created");

	/*
	 * Update all symbol values with the appropriate offset.
	 */
	for (i = 0; i < eif.ehdr->e_shnum; i++) {
		if (eif.shdr[i].sh_type != SHT_NOBITS &&
		    eif.shdr[i].sh_type != SHT_PROGBITS) {
			continue;
		}
		secva = baseva + eif.shdr[i].sh_offset;
		for (j = 0; j < eif.symcnt; j++) {
			sym = &eif.symtab[j];
			if (sym->st_shndx != i) {
				continue;
			}
			sym->st_value += (Elf_Addr)secva;
		}
	}

	print_state(true, "Symbol values updated");

	/*
	 * Perform relocations without addend if there are any.
	 */
	for (i = 0; i < eif.ehdr->e_shnum; i++) {
		Elf_Rel *reltab, *rel;
		size_t secidx, nrel;
		uintptr_t base;

		if (eif.shdr[i].sh_type != SHT_REL)
			continue;

		reltab = (Elf_Rel *)((uint8_t *)eif.ehdr + eif.shdr[i].sh_offset);
		nrel = eif.shdr[i].sh_size / sizeof(Elf_Rel);

		secidx = eif.shdr[i].sh_info;
		if (secidx >= eif.ehdr->e_shnum) {
			fatal("elf_kernel_reloc: wrong REL relocation");
		}
		base = (uintptr_t)eif.ehdr + eif.shdr[secidx].sh_offset;

		for (j = 0; j < nrel; j++) {
			rel = &reltab[j];
			elf_apply_reloc(base, rel, false);
		}
	}

	print_state(true, "REL relocations applied");

	/*
	 * Perform relocations with addend if there are any.
	 */
	for (i = 0; i < eif.ehdr->e_shnum; i++) {
		Elf_Rela *relatab, *rela;
		size_t secidx, nrela;
		uintptr_t base;

		if (eif.shdr[i].sh_type != SHT_RELA)
			continue;

		relatab = (Elf_Rela *)((uint8_t *)eif.ehdr + eif.shdr[i].sh_offset);
		nrela = eif.shdr[i].sh_size / sizeof(Elf_Rela);

		secidx = eif.shdr[i].sh_info;
		if (secidx >= eif.ehdr->e_shnum) {
			fatal("elf_kernel_reloc: wrong RELA relocation");
		}
		base = (uintptr_t)eif.ehdr + eif.shdr[secidx].sh_offset;

		for (j = 0; j < nrela; j++) {
			rela = &relatab[j];
			elf_apply_reloc(base, rela, true);
		}
	}

	print_state(true, "RELA relocations applied");

	/*
	 * Get the entry point.
	 */
	ent = elf_get_entrypoint();
	if (ent == 0) {
		fatal("elf_kernel_reloc: entry point not found");
	}

	print_state(true, "Entry point found");

	return ent;
}
