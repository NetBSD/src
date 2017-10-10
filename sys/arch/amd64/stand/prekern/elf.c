/*	$NetBSD: elf.c,v 1.1 2017/10/10 09:29:14 maxv Exp $	*/

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
	struct {
		vaddr_t va;
		size_t sz;
	} text;
	struct {
		vaddr_t va;
		size_t sz;
	} rodata;
	struct {
		vaddr_t va;
		size_t sz;
	} data;
};

static struct elfinfo eif;
static const char entrypoint[] = "start_prekern";

/* XXX */
static int
memcmp(const char *a, const char *b, size_t c)
{
	size_t i;
	for (i = 0; i < c; i++) {
		if (a[i] != b[i])
			return 1;
	}
	return 0;
}
static int
strcmp(char *a, char *b)
{
	size_t i;
	for (i = 0; a[i] != '\0'; i++) {
		if (a[i] != b[i])
			return 1;
	}
	return 0;
}


static int
elf_check_header()
{
	if (memcmp((char *)eif.ehdr->e_ident, ELFMAG, SELFMAG) != 0 ||
	    eif.ehdr->e_ident[EI_CLASS] != ELFCLASS) {
		return -1;
	}
	return 0;
}

static vaddr_t
elf_get_entrypoint()
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

static void
elf_build_info(vaddr_t baseva)
{
	vaddr_t secva, minva, maxva;
	size_t secsz;
	size_t i, j;

	memset(&eif, 0, sizeof(struct elfinfo));

	eif.ehdr = (Elf_Ehdr *)baseva;
	eif.shdr = (Elf_Shdr *)((uint8_t *)eif.ehdr + eif.ehdr->e_shoff);

	if (elf_check_header(&eif) == -1) {
		fatal("elf_build_info: wrong kernel ELF header");
	}

	/* Locate the section names */
	j = eif.ehdr->e_shstrndx;
	if (j == SHN_UNDEF) {
		fatal("elf_build_info: shstrtab not found");
	}
	if (j >= eif.ehdr->e_shnum) {
		fatal("elf_build_info: wrong shstrtab index");
	}
	eif.shstrtab = (char *)((uint8_t *)eif.ehdr + eif.shdr[j].sh_offset);
	eif.shstrsz = eif.shdr[j].sh_size;

	/* Locate the symbol table */
	for (i = 0; i < eif.ehdr->e_shnum; i++) {
		if (eif.shdr[i].sh_type == SHT_SYMTAB)
			break;
	}
	if (i == eif.ehdr->e_shnum) {
		fatal("elf_build_info: symtab not found");
	}
	eif.symtab = (Elf_Sym *)((uint8_t *)eif.ehdr + eif.shdr[i].sh_offset);
	eif.symcnt = eif.shdr[i].sh_size / sizeof(Elf_Sym);

	/* Also locate the string table */
	j = eif.shdr[i].sh_link;
	if (j == SHN_UNDEF || j >= eif.ehdr->e_shnum) {
		fatal("elf_build_info: wrong strtab index");
	}
	if (eif.shdr[j].sh_type != SHT_STRTAB) {
		fatal("elf_build_info: wrong strtab type");
	}
	eif.strtab = (char *)((uint8_t *)eif.ehdr + eif.shdr[j].sh_offset);
	eif.strsz = eif.shdr[j].sh_size;

	/*
	 * Save the locations of the kernel segments. Attention: there is a
	 * difference between "segment" and "section". A segment can contain
	 * several sections.
	 */

	/* text */
	minva = 0xFFFFFFFFFFFFFFFF, maxva = 0;
	for (i = 0; i < eif.ehdr->e_shnum; i++) {
		if (eif.shdr[i].sh_type != SHT_NOBITS &&
		    eif.shdr[i].sh_type != SHT_PROGBITS) {
			continue;
		}
		if (!(eif.shdr[i].sh_flags & SHF_EXECINSTR)) {
			continue;
		}
		secva = baseva + eif.shdr[i].sh_offset;
		secsz = eif.shdr[i].sh_size;
		if (secva < minva) {
			minva = secva;
		}
		if (secva + secsz > maxva) {
			maxva = secva + secsz;
		}
	}
	eif.text.va = minva;
	eif.text.sz = roundup(maxva - minva, PAGE_SIZE);
	ASSERT(eif.text.va % PAGE_SIZE == 0);

	/* rodata */
	minva = 0xFFFFFFFFFFFFFFFF, maxva = 0;
	for (i = 0; i < eif.ehdr->e_shnum; i++) {
		if (eif.shdr[i].sh_type != SHT_NOBITS &&
		    eif.shdr[i].sh_type != SHT_PROGBITS) {
			continue;
		}
		if ((eif.shdr[i].sh_flags & (SHF_EXECINSTR|SHF_WRITE))) {
			continue;
		}
		secva = baseva + eif.shdr[i].sh_offset;
		secsz = eif.shdr[i].sh_size;
		if (secva < minva) {
			minva = secva;
		}
		if (secva + secsz > maxva) {
			maxva = secva + secsz;
		}
	}
	eif.rodata.va = minva;
	eif.rodata.sz = roundup(maxva - minva, PAGE_SIZE);
	ASSERT(eif.rodata.va % PAGE_SIZE == 0);

	/* data */
	minva = 0xFFFFFFFFFFFFFFFF, maxva = 0;
	for (i = 0; i < eif.ehdr->e_shnum; i++) {
		if (eif.shdr[i].sh_type != SHT_NOBITS &&
		    eif.shdr[i].sh_type != SHT_PROGBITS) {
			continue;
		}
		if (!(eif.shdr[i].sh_flags & SHF_WRITE) ||
		    (eif.shdr[i].sh_flags & SHF_EXECINSTR)) {
			continue;
		}
		secva = baseva + eif.shdr[i].sh_offset;
		secsz = eif.shdr[i].sh_size;
		if (secva < minva) {
			minva = secva;
		}
		if (secva + secsz > maxva) {
			maxva = secva + secsz;
		}
	}
	eif.data.va = minva;
	eif.data.sz = roundup(maxva - minva, PAGE_SIZE);
	ASSERT(eif.data.va % PAGE_SIZE == 0);
}

vaddr_t
elf_kernel_reloc(vaddr_t baseva)
{
	vaddr_t secva, ent;
	Elf_Sym *sym;
	size_t i, j;

	elf_build_info(baseva);

	print_state(true, "ELF info created");

	/*
	 * The loaded sections are: SHT_PROGBITS, SHT_NOBITS, SHT_STRTAB,
	 * SHT_SYMTAB.
	 */

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
	ent = elf_get_entrypoint(&eif);
	if (ent == 0) {
		fatal("elf_kernel_reloc: entry point not found");
	}

	print_state(true, "Entry point found");

	/*
	 * Remap the code segments with proper permissions.
	 */
	mm_mprotect(eif.text.va, eif.text.sz, MM_PROT_READ|MM_PROT_EXECUTE);
	mm_mprotect(eif.rodata.va, eif.rodata.sz, MM_PROT_READ);
	mm_mprotect(eif.data.va, eif.data.sz, MM_PROT_READ|MM_PROT_WRITE);

	print_state(true, "Segments protection updated");

	return ent;
}

void
elf_get_text(vaddr_t *va, paddr_t *pa, size_t *sz)
{
	*va = eif.text.va;
	*pa = mm_vatopa(eif.text.va);
	*sz = eif.text.sz;
}

void
elf_get_rodata(vaddr_t *va, paddr_t *pa, size_t *sz)
{
	*va = eif.rodata.va;
	*pa = mm_vatopa(eif.rodata.va);
	*sz = eif.rodata.sz;
}

void
elf_get_data(vaddr_t *va, paddr_t *pa, size_t *sz)
{
	*va = eif.data.va;
	*pa = mm_vatopa(eif.data.va);
	*sz = eif.data.sz;
}
