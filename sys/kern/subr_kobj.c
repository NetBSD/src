/*	$NetBSD: subr_kobj.c,v 1.76 2023/01/29 17:20:48 skrll Exp $	*/

/*
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software developed for The NetBSD Foundation
 * by Andrew Doran.
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
 * Copyright (c) 1998-2000 Doug Rabson
 * Copyright (c) 2004 Peter Wemm
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

/*
 * Kernel loader for ELF objects.
 *
 * TODO: adjust kmem_alloc() calls to avoid needless fragmentation.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_kobj.c,v 1.76 2023/01/29 17:20:48 skrll Exp $");

#ifdef _KERNEL_OPT
#include "opt_modular.h"
#endif

#include <sys/kobj_impl.h>

#ifdef MODULAR

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/proc.h>
#include <sys/ksyms.h>
#include <sys/module.h>

#include <uvm/uvm_extern.h>

#define kobj_error(_kobj, ...) \
	kobj_out(__func__, __LINE__, _kobj, __VA_ARGS__)

static int	kobj_relocate(kobj_t, bool);
static int	kobj_checksyms(kobj_t, bool);
static void	kobj_out(const char *, int, kobj_t, const char *, ...)
    __printflike(4, 5);
static void	kobj_jettison(kobj_t);
static void	kobj_free(kobj_t, void *, size_t);
static void	kobj_close(kobj_t);
static int	kobj_read_mem(kobj_t, void **, size_t, off_t, bool);
static void	kobj_close_mem(kobj_t);

/*
 * kobj_load_mem:
 *
 *	Load an object already resident in memory.  If size is not -1,
 *	the complete size of the object is known.
 */
int
kobj_load_mem(kobj_t *kop, const char *name, void *base, ssize_t size)
{
	kobj_t ko;

	ko = kmem_zalloc(sizeof(*ko), KM_SLEEP);
	ko->ko_type = KT_MEMORY;
	kobj_setname(ko, name);
	ko->ko_source = base;
	ko->ko_memsize = size;
	ko->ko_read = kobj_read_mem;
	ko->ko_close = kobj_close_mem;

	*kop = ko;
	return kobj_load(ko);
}

/*
 * kobj_close:
 *
 *	Close an open ELF object.
 */
static void
kobj_close(kobj_t ko)
{

	if (ko->ko_source == NULL) {
		return;
	}

	ko->ko_close(ko);
	ko->ko_source = NULL;
}

static void
kobj_close_mem(kobj_t ko)
{

	return;
}

/*
 * kobj_load:
 *
 *	Load an ELF object and prepare to link into the running kernel
 *	image.
 */
int
kobj_load(kobj_t ko)
{
	Elf_Ehdr *hdr;
	Elf_Shdr *shdr;
	Elf_Sym *es;
	vaddr_t map_text_base;
	vaddr_t map_data_base;
	vaddr_t map_rodata_base;
	size_t map_text_size;
	size_t map_data_size;
	size_t map_rodata_size;
	int error;
	int symtabindex;
	int symstrindex;
	int nsym;
	int pb, rl, ra;
	int alignmask;
	int i, j;
	void *addr;

	KASSERT(ko->ko_type != KT_UNSET);
	KASSERT(ko->ko_source != NULL);

	shdr = NULL;
	error = 0;
	hdr = NULL;

	/*
	 * Read the elf header from the file.
	 */
	error = ko->ko_read(ko, (void **)&hdr, sizeof(*hdr), 0, true);
	if (error != 0) {
		kobj_error(ko, "read failed %d", error);
		goto out;
	}
	if (memcmp(hdr->e_ident, ELFMAG, SELFMAG) != 0) {
		kobj_error(ko, "not an ELF object");
		error = ENOEXEC;
		goto out;
	}

	if (hdr->e_ident[EI_VERSION] != EV_CURRENT ||
	    hdr->e_version != EV_CURRENT) {
		kobj_error(ko, "unsupported file version %d",
		    hdr->e_ident[EI_VERSION]);
		error = ENOEXEC;
		goto out;
	}
	if (hdr->e_type != ET_REL) {
		kobj_error(ko, "unsupported file type %d", hdr->e_type);
		error = ENOEXEC;
		goto out;
	}
	switch (hdr->e_machine) {
#if ELFSIZE == 32
	ELF32_MACHDEP_ID_CASES
#elif ELFSIZE == 64
	ELF64_MACHDEP_ID_CASES
#else
#error not defined
#endif
	default:
		kobj_error(ko, "unsupported machine %d", hdr->e_machine);
		error = ENOEXEC;
		goto out;
	}

	ko->ko_nprogtab = 0;
	ko->ko_shdr = 0;
	ko->ko_nrel = 0;
	ko->ko_nrela = 0;

	/*
	 * Allocate and read in the section header.
	 */
	if (hdr->e_shnum == 0 || hdr->e_shnum > ELF_MAXSHNUM ||
	    hdr->e_shoff == 0 || hdr->e_shentsize != sizeof(Elf_Shdr)) {
		kobj_error(ko, "bad sizes");
		error = ENOEXEC;
		goto out;
	}
	ko->ko_shdrsz = hdr->e_shnum * sizeof(Elf_Shdr);
	error = ko->ko_read(ko, (void **)&shdr, ko->ko_shdrsz, hdr->e_shoff,
	    true);
	if (error != 0) {
		kobj_error(ko, "read failed %d", error);
		goto out;
	}
	ko->ko_shdr = shdr;

	/*
	 * Scan the section header for information and table sizing.
	 */
	nsym = 0;
	symtabindex = symstrindex = -1;
	for (i = 0; i < hdr->e_shnum; i++) {
		switch (shdr[i].sh_type) {
		case SHT_PROGBITS:
		case SHT_NOBITS:
			ko->ko_nprogtab++;
			break;
		case SHT_SYMTAB:
			nsym++;
			symtabindex = i;
			symstrindex = shdr[i].sh_link;
			break;
		case SHT_REL:
			if (shdr[shdr[i].sh_info].sh_type != SHT_PROGBITS)
				continue;
			ko->ko_nrel++;
			break;
		case SHT_RELA:
			if (shdr[shdr[i].sh_info].sh_type != SHT_PROGBITS)
				continue;
			ko->ko_nrela++;
			break;
		case SHT_STRTAB:
			break;
		}
	}
	if (ko->ko_nprogtab == 0) {
		kobj_error(ko, "file has no contents");
		error = ENOEXEC;
		goto out;
	}
	if (nsym != 1) {
		/* Only allow one symbol table for now */
		kobj_error(ko, "file has no valid symbol table");
		error = ENOEXEC;
		goto out;
	}
	KASSERT(symtabindex != -1);
	KASSERT(symstrindex != -1);

	if (symstrindex == SHN_UNDEF || symstrindex >= hdr->e_shnum ||
	    shdr[symstrindex].sh_type != SHT_STRTAB) {
		kobj_error(ko, "file has invalid symbol strings");
		error = ENOEXEC;
		goto out;
	}

	/*
	 * Allocate space for tracking the load chunks.
	 */
	if (ko->ko_nprogtab != 0) {
		ko->ko_progtab = kmem_zalloc(ko->ko_nprogtab *
		    sizeof(*ko->ko_progtab), KM_SLEEP);
		if (ko->ko_progtab == NULL) {
			error = ENOMEM;
			kobj_error(ko, "out of memory");
			goto out;
		}
	}
	if (ko->ko_nrel != 0) {
		ko->ko_reltab = kmem_zalloc(ko->ko_nrel *
		    sizeof(*ko->ko_reltab), KM_SLEEP);
		if (ko->ko_reltab == NULL) {
			error = ENOMEM;
			kobj_error(ko, "out of memory");
			goto out;
		}
	}
	if (ko->ko_nrela != 0) {
		ko->ko_relatab = kmem_zalloc(ko->ko_nrela *
		    sizeof(*ko->ko_relatab), KM_SLEEP);
		if (ko->ko_relatab == NULL) {
			error = ENOMEM;
			kobj_error(ko, "out of memory");
			goto out;
		}
	}

	/*
	 * Allocate space for and load the symbol table.
	 */
	ko->ko_symcnt = shdr[symtabindex].sh_size / sizeof(Elf_Sym);
	if (ko->ko_symcnt == 0) {
		kobj_error(ko, "no symbol table");
		error = ENOEXEC;
		goto out;
	}
	error = ko->ko_read(ko, (void **)&ko->ko_symtab,
	    ko->ko_symcnt * sizeof(Elf_Sym),
	    shdr[symtabindex].sh_offset, true);
	if (error != 0) {
		kobj_error(ko, "read failed %d", error);
		goto out;
	}

	/*
	 * Allocate space for and load the symbol strings.
	 */
	ko->ko_strtabsz = shdr[symstrindex].sh_size;
	if (ko->ko_strtabsz == 0) {
		kobj_error(ko, "no symbol strings");
		error = ENOEXEC;
		goto out;
	}
	error = ko->ko_read(ko, (void *)&ko->ko_strtab, ko->ko_strtabsz,
	    shdr[symstrindex].sh_offset, true);
	if (error != 0) {
		kobj_error(ko, "read failed %d", error);
		goto out;
	}

	/*
	 * Adjust module symbol namespace, if necessary (e.g. with rump)
	 */
	error = kobj_renamespace(ko->ko_symtab, ko->ko_symcnt,
	    &ko->ko_strtab, &ko->ko_strtabsz);
	if (error != 0) {
		kobj_error(ko, "renamespace failed %d", error);
		goto out;
	}

	/*
	 * Do we have a string table for the section names?
	 */
	if (hdr->e_shstrndx != SHN_UNDEF) {
		if (hdr->e_shstrndx >= hdr->e_shnum) {
			kobj_error(ko, "bad shstrndx");
			error = ENOEXEC;
			goto out;
		}
		if (shdr[hdr->e_shstrndx].sh_size != 0 &&
		    shdr[hdr->e_shstrndx].sh_type == SHT_STRTAB) {
			ko->ko_shstrtabsz = shdr[hdr->e_shstrndx].sh_size;
			error = ko->ko_read(ko, (void **)&ko->ko_shstrtab,
			    shdr[hdr->e_shstrndx].sh_size,
			    shdr[hdr->e_shstrndx].sh_offset, true);
			if (error != 0) {
				kobj_error(ko, "read failed %d", error);
				goto out;
			}
		}
	}

	/*
	 * Size up code/data(progbits) and bss(nobits).
	 */
	alignmask = 0;
	map_text_size = 0;
	map_data_size = 0;
	map_rodata_size = 0;
	for (i = 0; i < hdr->e_shnum; i++) {
		if (shdr[i].sh_type != SHT_PROGBITS &&
		    shdr[i].sh_type != SHT_NOBITS)
			continue;
		alignmask = shdr[i].sh_addralign - 1;
		if ((shdr[i].sh_flags & SHF_EXECINSTR)) {
			map_text_size += alignmask;
			map_text_size &= ~alignmask;
			map_text_size += shdr[i].sh_size;
		} else if (!(shdr[i].sh_flags & SHF_WRITE)) {
			map_rodata_size += alignmask;
			map_rodata_size &= ~alignmask;
			map_rodata_size += shdr[i].sh_size;
		} else {
			map_data_size += alignmask;
			map_data_size &= ~alignmask;
			map_data_size += shdr[i].sh_size;
		}
	}

	if (map_text_size == 0) {
		kobj_error(ko, "no text");
		error = ENOEXEC;
 		goto out;
 	}

	if (map_data_size != 0) {
		map_data_base = uvm_km_alloc(module_map, round_page(map_data_size),
			0, UVM_KMF_WIRED);
		if (map_data_base == 0) {
			kobj_error(ko, "out of memory");
			error = ENOMEM;
			goto out;
		}
		ko->ko_data_address = map_data_base;
		ko->ko_data_size = map_data_size;
 	} else {
		map_data_base = 0;
		ko->ko_data_address = 0;
		ko->ko_data_size = 0;
	}

	if (map_rodata_size != 0) {
		map_rodata_base = uvm_km_alloc(module_map, round_page(map_rodata_size),
			0, UVM_KMF_WIRED);
		if (map_rodata_base == 0) {
			kobj_error(ko, "out of memory");
			error = ENOMEM;
			goto out;
		}
		ko->ko_rodata_address = map_rodata_base;
		ko->ko_rodata_size = map_rodata_size;
 	} else {
		map_rodata_base = 0;
		ko->ko_rodata_address = 0;
		ko->ko_rodata_size = 0;
	}

	map_text_base = uvm_km_alloc(module_map, round_page(map_text_size),
	    0, UVM_KMF_WIRED | UVM_KMF_EXEC);
	if (map_text_base == 0) {
		kobj_error(ko, "out of memory");
		error = ENOMEM;
		goto out;
	}
	ko->ko_text_address = map_text_base;
	ko->ko_text_size = map_text_size;

	/*
	 * Now load code/data(progbits), zero bss(nobits), allocate space
	 * for and load relocs
	 */
	pb = 0;
	rl = 0;
	ra = 0;
	alignmask = 0;
	for (i = 0; i < hdr->e_shnum; i++) {
		switch (shdr[i].sh_type) {
		case SHT_PROGBITS:
		case SHT_NOBITS:
			alignmask = shdr[i].sh_addralign - 1;
			if ((shdr[i].sh_flags & SHF_EXECINSTR)) {
				map_text_base += alignmask;
				map_text_base &= ~alignmask;
				addr = (void *)map_text_base;
				map_text_base += shdr[i].sh_size;
			} else if (!(shdr[i].sh_flags & SHF_WRITE)) {
				map_rodata_base += alignmask;
				map_rodata_base &= ~alignmask;
				addr = (void *)map_rodata_base;
				map_rodata_base += shdr[i].sh_size;
 			} else {
				map_data_base += alignmask;
				map_data_base &= ~alignmask;
				addr = (void *)map_data_base;
				map_data_base += shdr[i].sh_size;
 			}

			ko->ko_progtab[pb].addr = addr;
			if (shdr[i].sh_type == SHT_PROGBITS) {
				ko->ko_progtab[pb].name = "<<PROGBITS>>";
				error = ko->ko_read(ko, &addr,
				    shdr[i].sh_size, shdr[i].sh_offset, false);
				if (error != 0) {
					kobj_error(ko, "read failed %d", error);
					goto out;
				}
			} else { /* SHT_NOBITS */
				ko->ko_progtab[pb].name = "<<NOBITS>>";
				memset(addr, 0, shdr[i].sh_size);
			}

			ko->ko_progtab[pb].size = shdr[i].sh_size;
			ko->ko_progtab[pb].sec = i;
			if (ko->ko_shstrtab != NULL && shdr[i].sh_name != 0) {
				ko->ko_progtab[pb].name =
				    ko->ko_shstrtab + shdr[i].sh_name;
			}

			/* Update all symbol values with the offset. */
			for (j = 0; j < ko->ko_symcnt; j++) {
				es = &ko->ko_symtab[j];
				if (es->st_shndx != i) {
					continue;
				}
				es->st_value += (Elf_Addr)addr;
			}
			pb++;
			break;
		case SHT_REL:
			if (shdr[shdr[i].sh_info].sh_type != SHT_PROGBITS)
				break;
			ko->ko_reltab[rl].size = shdr[i].sh_size;
			ko->ko_reltab[rl].size -=
			    shdr[i].sh_size % sizeof(Elf_Rel);
			if (ko->ko_reltab[rl].size != 0) {
				ko->ko_reltab[rl].nrel =
				    shdr[i].sh_size / sizeof(Elf_Rel);
				ko->ko_reltab[rl].sec = shdr[i].sh_info;
				error = ko->ko_read(ko,
				    (void **)&ko->ko_reltab[rl].rel,
				    ko->ko_reltab[rl].size,
				    shdr[i].sh_offset, true);
				if (error != 0) {
					kobj_error(ko, "read failed %d",
					    error);
					goto out;
				}
			}
			rl++;
			break;
		case SHT_RELA:
			if (shdr[shdr[i].sh_info].sh_type != SHT_PROGBITS)
				break;
			ko->ko_relatab[ra].size = shdr[i].sh_size;
			ko->ko_relatab[ra].size -=
			    shdr[i].sh_size % sizeof(Elf_Rela);
			if (ko->ko_relatab[ra].size != 0) {
				ko->ko_relatab[ra].nrela =
				    shdr[i].sh_size / sizeof(Elf_Rela);
				ko->ko_relatab[ra].sec = shdr[i].sh_info;
				error = ko->ko_read(ko,
				    (void **)&ko->ko_relatab[ra].rela,
				    shdr[i].sh_size,
				    shdr[i].sh_offset, true);
				if (error != 0) {
					kobj_error(ko, "read failed %d", error);
					goto out;
				}
			}
			ra++;
			break;
		default:
			break;
		}
	}
	if (pb != ko->ko_nprogtab) {
		panic("%s:%d: %s: lost progbits", __func__, __LINE__,
		   ko->ko_name);
	}
	if (rl != ko->ko_nrel) {
		panic("%s:%d: %s: lost rel", __func__, __LINE__,
		   ko->ko_name);
	}
	if (ra != ko->ko_nrela) {
		panic("%s:%d: %s: lost rela", __func__, __LINE__,
		   ko->ko_name);
	}
	if (map_text_base != ko->ko_text_address + map_text_size) {
		panic("%s:%d: %s: map_text_base 0x%lx != address %lx "
		    "+ map_text_size %ld (0x%lx)\n",
		    __func__, __LINE__, ko->ko_name, (long)map_text_base,
		    (long)ko->ko_text_address, (long)map_text_size,
		    (long)ko->ko_text_address + map_text_size);
	}
	if (map_data_base != ko->ko_data_address + map_data_size) {
		panic("%s:%d: %s: map_data_base 0x%lx != address %lx "
		    "+ map_data_size %ld (0x%lx)\n",
		    __func__, __LINE__, ko->ko_name, (long)map_data_base,
		    (long)ko->ko_data_address, (long)map_data_size,
		    (long)ko->ko_data_address + map_data_size);
	}
	if (map_rodata_base != ko->ko_rodata_address + map_rodata_size) {
		panic("%s:%d: %s: map_rodata_base 0x%lx != address %lx "
		    "+ map_rodata_size %ld (0x%lx)\n",
		    __func__, __LINE__, ko->ko_name, (long)map_rodata_base,
		    (long)ko->ko_rodata_address, (long)map_rodata_size,
		    (long)ko->ko_rodata_address + map_rodata_size);
	}

	/*
	 * Perform local relocations only.  Relocations relating to global
	 * symbols will be done by kobj_affix().
	 */
	error = kobj_checksyms(ko, false);
	if (error)
		goto out;

	error = kobj_relocate(ko, true);
	if (error)
		goto out;
out:
	if (hdr != NULL) {
		kobj_free(ko, hdr, sizeof(*hdr));
	}
	kobj_close(ko);
	if (error != 0) {
		kobj_unload(ko);
	}

	return error;
}

static void
kobj_unload_notify(kobj_t ko, vaddr_t addr, size_t size, const char *note)
{
	if (addr == 0)
		return;

	int error = kobj_machdep(ko, (void *)addr, size, false);
	if (error)
		kobj_error(ko, "machine dependent deinit failed (%s) %d",
		    note, error);
}

#define KOBJ_SEGMENT_NOTIFY(ko, what) \
    kobj_unload_notify(ko, (ko)->ko_ ## what ## _address, \
	(ko)->ko_ ## what ## _size, # what);

#define KOBJ_SEGMENT_FREE(ko, what) \
    do \
	if ((ko)->ko_ ## what ## _address != 0) \
		uvm_km_free(module_map, (ko)->ko_ ## what ## _address, \
		    round_page((ko)->ko_ ## what ## _size), UVM_KMF_WIRED); \
    while (/*CONSTCOND*/ 0)

/*
 * kobj_unload:
 *
 *	Unload an object previously loaded by kobj_load().
 */
void
kobj_unload(kobj_t ko)
{
	kobj_close(ko);
	kobj_jettison(ko);


	/*
	 * Notify MD code that a module has been unloaded.
	 */
	if (ko->ko_loaded) {
		KOBJ_SEGMENT_NOTIFY(ko, text);
		KOBJ_SEGMENT_NOTIFY(ko, data);
		KOBJ_SEGMENT_NOTIFY(ko, rodata);
	}

	KOBJ_SEGMENT_FREE(ko, text);
	KOBJ_SEGMENT_FREE(ko, data);
	KOBJ_SEGMENT_FREE(ko, rodata);

	if (ko->ko_ksyms == true) {
		ksyms_modunload(ko->ko_name);
	}
	if (ko->ko_symtab != NULL) {
		kobj_free(ko, ko->ko_symtab, ko->ko_symcnt * sizeof(Elf_Sym));
	}
	if (ko->ko_strtab != NULL) {
		kobj_free(ko, ko->ko_strtab, ko->ko_strtabsz);
	}
	if (ko->ko_progtab != NULL) {
		kobj_free(ko, ko->ko_progtab, ko->ko_nprogtab *
		    sizeof(*ko->ko_progtab));
		ko->ko_progtab = NULL;
	}
	if (ko->ko_shstrtab) {
		kobj_free(ko, ko->ko_shstrtab, ko->ko_shstrtabsz);
		ko->ko_shstrtab = NULL;
	}

	kmem_free(ko, sizeof(*ko));
}

/*
 * kobj_stat:
 *
 *	Return size and load address of an object.
 */
int
kobj_stat(kobj_t ko, vaddr_t *address, size_t *size)
{

	if (address != NULL) {
		*address = ko->ko_text_address;
	}
	if (size != NULL) {
		*size = ko->ko_text_size;
	}
	return 0;
}

/*
 * kobj_affix:
 *
 *	Set an object's name and perform global relocs.  May only be
 *	called after the module and any requisite modules are loaded.
 */
int
kobj_affix(kobj_t ko, const char *name)
{
	int error;

	KASSERT(ko->ko_ksyms == false);
	KASSERT(ko->ko_loaded == false);

	kobj_setname(ko, name);

	/* Cache addresses of undefined symbols. */
	error = kobj_checksyms(ko, true);
	if (error)
		goto out;

	/* Now do global relocations. */
	error = kobj_relocate(ko, false);
	if (error)
		goto out;

	/*
	 * Now that we know the name, register the symbol table.
	 * Do after global relocations because ksyms will pack
	 * the table.
	 */
	ksyms_modload(ko->ko_name, ko->ko_symtab,
	    ko->ko_symcnt * sizeof(Elf_Sym), ko->ko_strtab, ko->ko_strtabsz);
	ko->ko_ksyms = true;

	/* Jettison unneeded memory post-link. */
	kobj_jettison(ko);

	/*
	 * Notify MD code that a module has been loaded.
	 *
	 * Most architectures use this opportunity to flush their caches.
	 */
	if (ko->ko_text_address != 0) {
		error = kobj_machdep(ko, (void *)ko->ko_text_address,
		    ko->ko_text_size, true);
		if (error) {
			kobj_error(ko, "machine dependent init failed (text)"
			    " %d", error);
			goto out;
		}
	}

	if (ko->ko_data_address != 0) {
		error = kobj_machdep(ko, (void *)ko->ko_data_address,
		    ko->ko_data_size, true);
		if (error) {
			kobj_error(ko, "machine dependent init failed (data)"
			    " %d", error);
			goto out;
		}
	}

	if (ko->ko_rodata_address != 0) {
		error = kobj_machdep(ko, (void *)ko->ko_rodata_address,
		    ko->ko_rodata_size, true);
		if (error) {
			kobj_error(ko, "machine dependent init failed (rodata)"
			    " %d", error);
			goto out;
		}
	}

	ko->ko_loaded = true;

	/* Change the memory protections, when needed. */
	if (ko->ko_text_address != 0) {
		uvm_km_protect(module_map, ko->ko_text_address,
		    ko->ko_text_size, VM_PROT_READ|VM_PROT_EXECUTE);
	}
	if (ko->ko_rodata_address != 0) {
		uvm_km_protect(module_map, ko->ko_rodata_address,
		    ko->ko_rodata_size, VM_PROT_READ);
	}

	/* Success! */
	error = 0;

out:	if (error) {
		/* If there was an error, destroy the whole object. */
		kobj_unload(ko);
	}
	return error;
}

/*
 * kobj_find_section:
 *
 *	Given a section name, search the loaded object and return
 *	virtual address if present and loaded.
 */
int
kobj_find_section(kobj_t ko, const char *name, void **addr, size_t *size)
{
	int i;

	KASSERT(ko->ko_progtab != NULL);

	for (i = 0; i < ko->ko_nprogtab; i++) {
		if (strcmp(ko->ko_progtab[i].name, name) == 0) {
			if (addr != NULL) {
				*addr = ko->ko_progtab[i].addr;
			}
			if (size != NULL) {
				*size = ko->ko_progtab[i].size;
			}
			return 0;
		}
	}

	return ENOENT;
}

/*
 * kobj_jettison:
 *
 *	Release object data not needed after performing relocations.
 */
static void
kobj_jettison(kobj_t ko)
{
	int i;

	if (ko->ko_reltab != NULL) {
		for (i = 0; i < ko->ko_nrel; i++) {
			if (ko->ko_reltab[i].rel) {
				kobj_free(ko, ko->ko_reltab[i].rel,
				    ko->ko_reltab[i].size);
			}
		}
		kobj_free(ko, ko->ko_reltab, ko->ko_nrel *
		    sizeof(*ko->ko_reltab));
		ko->ko_reltab = NULL;
		ko->ko_nrel = 0;
	}
	if (ko->ko_relatab != NULL) {
		for (i = 0; i < ko->ko_nrela; i++) {
			if (ko->ko_relatab[i].rela) {
				kobj_free(ko, ko->ko_relatab[i].rela,
				    ko->ko_relatab[i].size);
			}
		}
		kobj_free(ko, ko->ko_relatab, ko->ko_nrela *
		    sizeof(*ko->ko_relatab));
		ko->ko_relatab = NULL;
		ko->ko_nrela = 0;
	}
	if (ko->ko_shdr != NULL) {
		kobj_free(ko, ko->ko_shdr, ko->ko_shdrsz);
		ko->ko_shdr = NULL;
	}
}

/*
 * kobj_sym_lookup:
 *
 *	Symbol lookup function to be used when the symbol index
 *	is known (ie during relocation).
 */
int
kobj_sym_lookup(kobj_t ko, uintptr_t symidx, Elf_Addr *val)
{
	const Elf_Sym *sym;
	const char *symbol;

	sym = ko->ko_symtab + symidx;

	if (symidx == SHN_ABS || symidx == 0) {
		*val = (uintptr_t)sym->st_value;
		return 0;
	} else if (symidx >= ko->ko_symcnt) {
		/*
		 * Don't even try to lookup the symbol if the index is
		 * bogus.
		 */
		kobj_error(ko, "symbol index %ju out of range",
		    (uintmax_t)symidx);
		return EINVAL;
	}

	/* Quick answer if there is a definition included. */
	if (sym->st_shndx != SHN_UNDEF) {
		*val = (uintptr_t)sym->st_value;
		return 0;
	}

	/* If we get here, then it is undefined and needs a lookup. */
	switch (ELF_ST_BIND(sym->st_info)) {
	case STB_LOCAL:
		/* Local, but undefined? huh? */
		kobj_error(ko, "local symbol @%ju undefined",
		    (uintmax_t)symidx);
		return EINVAL;

	case STB_GLOBAL:
		/* Relative to Data or Function name */
		symbol = ko->ko_strtab + sym->st_name;

		/* Force a lookup failure if the symbol name is bogus. */
		if (*symbol == 0) {
			kobj_error(ko, "bad symbol @%ju name",
			    (uintmax_t)symidx);
			return EINVAL;
		}
		if (sym->st_value == 0) {
			kobj_error(ko, "%s @%ju: bad value", symbol,
			    (uintmax_t)symidx);
			return EINVAL;
		}

		*val = (uintptr_t)sym->st_value;
		return 0;

	case STB_WEAK:
		kobj_error(ko, "weak symbol @%ju not supported",
		    (uintmax_t)symidx);
		return EINVAL;

	default:
		kobj_error(ko, "bad binding %#x for symbol @%ju",
		    ELF_ST_BIND(sym->st_info), (uintmax_t)symidx);
		return EINVAL;
	}
}

/*
 * kobj_findbase:
 *
 *	Return base address of the given section.
 */
static uintptr_t
kobj_findbase(kobj_t ko, int sec)
{
	int i;

	for (i = 0; i < ko->ko_nprogtab; i++) {
		if (sec == ko->ko_progtab[i].sec) {
			return (uintptr_t)ko->ko_progtab[i].addr;
		}
	}
	return 0;
}

/*
 * kobj_checksyms:
 *
 *	Scan symbol table for duplicates or resolve references to
 *	external symbols.
 */
static int
kobj_checksyms(kobj_t ko, bool undefined)
{
	unsigned long rval;
	Elf_Sym *sym, *ksym, *ms;
	const char *name;
	int error;

	error = 0;

	for (ms = (sym = ko->ko_symtab) + ko->ko_symcnt; sym < ms; sym++) {
		/* Check validity of the symbol. */
		if (ELF_ST_BIND(sym->st_info) != STB_GLOBAL ||
		    sym->st_name == 0)
			continue;
		if (undefined != (sym->st_shndx == SHN_UNDEF)) {
			continue;
		}

		/*
		 * Look it up.  Don't need to lock, as it is known that
		 * the symbol tables aren't going to change (we hold
		 * module_lock).
		 */
		name = ko->ko_strtab + sym->st_name;
		if (ksyms_getval_unlocked(NULL, name, &ksym, &rval,
		    KSYMS_EXTERN) != 0) {
			if (undefined) {
				kobj_error(ko, "symbol `%s' not found",
				    name);
				error = ENOEXEC;
			}
			continue;
		}

		/* Save values of undefined globals. */
		if (undefined) {
			if (ksym->st_shndx == SHN_ABS) {
				sym->st_shndx = SHN_ABS;
			}
			sym->st_value = (Elf_Addr)rval;
			continue;
		}

		/* Check (and complain) about differing values. */
		if (sym->st_value == rval) {
			continue;
		}
		if (strcmp(name, "_bss_start") == 0 ||
		    strcmp(name, "__bss_start") == 0 ||
		    strcmp(name, "_bss_end__") == 0 ||
		    strcmp(name, "__bss_end__") == 0 ||
		    strcmp(name, "_edata") == 0 ||
		    strcmp(name, "_end") == 0 ||
		    strcmp(name, "__end") == 0 ||
		    strcmp(name, "__end__") == 0 ||
		    strncmp(name, "__start_link_set_", 17) == 0 ||
		    strncmp(name, "__stop_link_set_", 16) == 0) {
		    	continue;
		}
		kobj_error(ko, "global symbol `%s' redefined",
		    name);
		error = ENOEXEC;
	}

	return error;
}

/*
 * kobj_relocate:
 *
 *	Resolve relocations for the loaded object.
 */
static int
kobj_relocate(kobj_t ko, bool local)
{
	const Elf_Rel *rellim;
	const Elf_Rel *rel;
	const Elf_Rela *relalim;
	const Elf_Rela *rela;
	const Elf_Sym *sym;
	uintptr_t base;
	int i, error;
	uintptr_t symidx;

	/*
	 * Perform relocations without addend if there are any.
	 */
	for (i = 0; i < ko->ko_nrel; i++) {
		rel = ko->ko_reltab[i].rel;
		if (rel == NULL) {
			continue;
		}
		rellim = rel + ko->ko_reltab[i].nrel;
		base = kobj_findbase(ko, ko->ko_reltab[i].sec);
		if (base == 0) {
			panic("%s:%d: %s: lost base for e_reltab[%d] sec %d",
			   __func__, __LINE__, ko->ko_name, i,
			   ko->ko_reltab[i].sec);
		}
		for (; rel < rellim; rel++) {
			symidx = ELF_R_SYM(rel->r_info);
			if (symidx >= ko->ko_symcnt) {
				continue;
			}
			sym = ko->ko_symtab + symidx;
			if (local != (ELF_ST_BIND(sym->st_info) == STB_LOCAL)) {
				continue;
			}
			error = kobj_reloc(ko, base, rel, false, local);
			if (error != 0) {
				kobj_error(ko, "unresolved rel relocation "
				    "@%#jx type=%d symidx=%d",
				    (intmax_t)rel->r_offset,
				    (int)ELF_R_TYPE(rel->r_info),
				    (int)ELF_R_SYM(rel->r_info));
				return ENOEXEC;
			}
		}
	}

	/*
	 * Perform relocations with addend if there are any.
	 */
	for (i = 0; i < ko->ko_nrela; i++) {
		rela = ko->ko_relatab[i].rela;
		if (rela == NULL) {
			continue;
		}
		relalim = rela + ko->ko_relatab[i].nrela;
		base = kobj_findbase(ko, ko->ko_relatab[i].sec);
		if (base == 0) {
			panic("%s:%d: %s: lost base for e_relatab[%d] sec %d",
			   __func__, __LINE__, ko->ko_name, i,
			   ko->ko_relatab[i].sec);
		}
		for (; rela < relalim; rela++) {
			symidx = ELF_R_SYM(rela->r_info);
			if (symidx >= ko->ko_symcnt) {
				continue;
			}
			sym = ko->ko_symtab + symidx;
			if (local != (ELF_ST_BIND(sym->st_info) == STB_LOCAL)) {
				continue;
			}
			error = kobj_reloc(ko, base, rela, true, local);
			if (error != 0) {
				kobj_error(ko, "unresolved rela relocation "
				    "@%#jx type=%d symidx=%d",
				    (intmax_t)rela->r_offset,
				    (int)ELF_R_TYPE(rela->r_info),
				    (int)ELF_R_SYM(rela->r_info));
				return ENOEXEC;
			}
		}
	}

	return 0;
}

/*
 * kobj_out:
 *
 *	Utility function: log an error.
 */
static void
kobj_out(const char *fname, int lnum, kobj_t ko, const char *fmt, ...)
{
	va_list ap;

	printf("%s, %d: [%s]: linker error: ", fname, lnum, ko->ko_name);
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");
}

static int
kobj_read_mem(kobj_t ko, void **basep, size_t size, off_t off,
    bool allocate)
{
	void *base = *basep;
	int error = 0;

	KASSERT(ko->ko_source != NULL);

	if (off < 0) {
		kobj_error(ko, "negative offset %lld",
		    (unsigned long long)off);
		error = EINVAL;
		base = NULL;
		goto out;
	} else if (ko->ko_memsize != -1 &&
	    (size > ko->ko_memsize || off > ko->ko_memsize - size)) {
		kobj_error(ko, "preloaded object short");
		error = EINVAL;
		base = NULL;
		goto out;
	}

	if (allocate)
		base = kmem_alloc(size, KM_SLEEP);

	/* Copy the section */
	memcpy(base, (uint8_t *)ko->ko_source + off, size);

out:	if (allocate)
		*basep = base;
	return error;
}

/*
 * kobj_free:
 *
 *	Utility function: free memory if it was allocated from the heap.
 */
static void
kobj_free(kobj_t ko, void *base, size_t size)
{

	kmem_free(base, size);
}

void
kobj_setname(kobj_t ko, const char *name)
{
	const char *d = name, *dots = "";
	size_t len, dlen;

	for (char *s = module_base; *d == *s; d++, s++)
		continue;

	if (d == name)
		name = "";
	else
		name = "%M";
	dlen = strlen(d);
	len = dlen + strlen(name);
	if (len >= sizeof(ko->ko_name)) {
		len = (len - sizeof(ko->ko_name)) + 5; /* dots + NUL */
		if (dlen >= len) {
			d += len;
			dots = "/...";
		}
	}
	snprintf(ko->ko_name, sizeof(ko->ko_name), "%s%s%s", name, dots, d);
}

#else	/* MODULAR */

int
kobj_load_mem(kobj_t *kop, const char *name, void *base, ssize_t size)
{

	return ENOSYS;
}

void
kobj_unload(kobj_t ko)
{

	panic("not modular");
}

int
kobj_stat(kobj_t ko, vaddr_t *base, size_t *size)
{

	return ENOSYS;
}

int
kobj_affix(kobj_t ko, const char *name)
{

	panic("not modular");
}

int
kobj_find_section(kobj_t ko, const char *name, void **addr, size_t *size)
{

	panic("not modular");
}

void
kobj_setname(kobj_t ko, const char *name)
{

	panic("not modular");
}

#endif	/* MODULAR */
