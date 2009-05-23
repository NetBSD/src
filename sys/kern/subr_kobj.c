/*	$NetBSD: subr_kobj.c,v 1.35 2009/05/23 15:13:57 ad Exp $	*/

/*-
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

/*-
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
__KERNEL_RCSID(0, "$NetBSD: subr_kobj.c,v 1.35 2009/05/23 15:13:57 ad Exp $");

#include "opt_modular.h"

#define	ELFSIZE		ARCH_ELFSIZE

#include <sys/systm.h>
#include <sys/kobj.h>
#include <sys/errno.h>

#ifdef MODULAR

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/proc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/fcntl.h>
#include <sys/ksyms.h>
#include <sys/module.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>

#include <machine/stdarg.h>

#include <uvm/uvm_extern.h>

typedef struct {
	void		*addr;
	Elf_Off		size;
	int		flags;
	int		sec;		/* Original section */
	const char	*name;
} progent_t;

typedef struct {
	Elf_Rel		*rel;
	int 		nrel;
	int 		sec;
	size_t		size;
} relent_t;

typedef struct {
	Elf_Rela	*rela;
	int		nrela;
	int		sec;
	size_t		size;
} relaent_t;

typedef enum kobjtype {
	KT_UNSET,
	KT_VNODE,
	KT_MEMORY
} kobjtype_t;

struct kobj {
	char		ko_name[MAXMODNAME];
	kobjtype_t	ko_type;
	void		*ko_source;
	ssize_t		ko_memsize;
	vaddr_t		ko_address;	/* Relocation address */
	Elf_Shdr	*ko_shdr;
	progent_t	*ko_progtab;
	relaent_t	*ko_relatab;
	relent_t	*ko_reltab;
	Elf_Sym		*ko_symtab;	/* Symbol table */
	char		*ko_strtab;	/* String table */
	char		*ko_shstrtab;	/* Section name string table */
	size_t		ko_size;	/* Size of text/data/bss */
	size_t		ko_symcnt;	/* Number of symbols */
	size_t		ko_strtabsz;	/* Number of bytes in string table */
	size_t		ko_shstrtabsz;	/* Number of bytes in scn str table */
	size_t		ko_shdrsz;
	int		ko_nrel;
	int		ko_nrela;
	int		ko_nprogtab;
	bool		ko_ksyms;
	bool		ko_loaded;
};

static int	kobj_relocate(kobj_t, bool);
static int	kobj_checksyms(kobj_t, bool);
static void	kobj_error(const char *, ...);
static int	kobj_read(kobj_t, void **, size_t, off_t);
static int	kobj_read_bits(kobj_t, void *, size_t, off_t);
static void	kobj_jettison(kobj_t);
static void	kobj_free(kobj_t, void *, size_t);
static void	kobj_close(kobj_t);
static int	kobj_load(kobj_t);

extern struct vm_map *module_map;

/*
 * kobj_load_file:
 *
 *	Load an object located in the file system.
 */
int
kobj_load_file(kobj_t *kop, const char *filename, const char *base,
	       bool autoload)
{
	struct nameidata nd;
	kauth_cred_t cred;
	char *path;
	int error;
	kobj_t ko;

	cred = kauth_cred_get();

	ko = kmem_zalloc(sizeof(*ko), KM_SLEEP);
	if (ko == NULL) {
		return ENOMEM;
	}

	if (autoload) {
		error = ENOENT;
	} else {
		NDINIT(&nd, LOOKUP, FOLLOW, UIO_SYSSPACE, filename);
		error = vn_open(&nd, FREAD, 0);
	}
	if (error != 0) {
		if (error != ENOENT) {
			goto out;
		}
		path = PNBUF_GET();
		snprintf(path, MAXPATHLEN - 1, "%s/%s/%s.kmod", base,
		    filename, filename);
		NDINIT(&nd, LOOKUP, FOLLOW | NOCHROOT, UIO_SYSSPACE, path);
		error = vn_open(&nd, FREAD, 0);
		PNBUF_PUT(path);
	}

 out:
 	if (error != 0) {
	 	kmem_free(ko, sizeof(*ko));
	 	return error;
	}

	ko->ko_type = KT_VNODE;
	ko->ko_source = nd.ni_vp;
	*kop = ko;
	return kobj_load(ko);
}

/*
 * kobj_load_mem:
 *
 *	Load an object already resident in memory.  If size is not -1,
 *	the complete size of the object is known.
 */
int
kobj_load_mem(kobj_t *kop, void *base, ssize_t size)
{
	kobj_t ko;

	ko = kmem_zalloc(sizeof(*ko), KM_SLEEP);
	if (ko == NULL) {
		return ENOMEM;
	}

	ko->ko_type = KT_MEMORY;
	ko->ko_source = base;
	ko->ko_memsize = size;
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

	switch (ko->ko_type) {
	case KT_VNODE:
		VOP_UNLOCK(ko->ko_source, 0);
		vn_close(ko->ko_source, FREAD, kauth_cred_get());
		break;
	case KT_MEMORY:
		/* nothing */
		break;
	default:
		panic("kobj_close: unknown type");
		break;
	}

	ko->ko_source = NULL;
}

/*
 * kobj_load:
 *
 *	Load an ELF object and prepare to link into the running kernel
 *	image.
 */
static int
kobj_load(kobj_t ko)
{
	Elf_Ehdr *hdr;
	Elf_Shdr *shdr;
	Elf_Sym *es;
	vaddr_t mapbase;
	size_t mapsize;
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
	mapsize = 0;
	error = 0;
	hdr = NULL;

	/*
	 * Read the elf header from the file.
	 */
	error = kobj_read(ko, (void **)&hdr, sizeof(*hdr), 0);
	if (error != 0)
		goto out;
	if (memcmp(hdr->e_ident, ELFMAG, SELFMAG) != 0) {
		kobj_error("not an ELF object");
		error = ENOEXEC;
		goto out;
	}

	if (hdr->e_ident[EI_VERSION] != EV_CURRENT ||
	    hdr->e_version != EV_CURRENT) {
		kobj_error("unsupported file version");
		error = ENOEXEC;
		goto out;
	}
	if (hdr->e_type != ET_REL) {
		kobj_error("unsupported file type");
		error = ENOEXEC;
		goto out;
	}
	switch (hdr->e_machine) {
#if ELFSIZE == 32
	ELF32_MACHDEP_ID_CASES
#else
	ELF64_MACHDEP_ID_CASES
#endif
	default:
		kobj_error("unsupported machine");
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
	ko->ko_shdrsz = hdr->e_shnum * hdr->e_shentsize;
	if (ko->ko_shdrsz == 0 || hdr->e_shoff == 0 ||
	    hdr->e_shentsize != sizeof(Elf_Shdr)) {
		error = ENOEXEC;
		goto out;
	}
	error = kobj_read(ko, (void **)&shdr, ko->ko_shdrsz, hdr->e_shoff);
	if (error != 0) {
		goto out;
	}
	ko->ko_shdr = shdr;

	/*
	 * Scan the section header for information and table sizing.
	 */
	nsym = 0;
	symtabindex = -1;
	symstrindex = -1;
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
			ko->ko_nrel++;
			break;
		case SHT_RELA:
			ko->ko_nrela++;
			break;
		case SHT_STRTAB:
			break;
		}
	}
	if (ko->ko_nprogtab == 0) {
		kobj_error("file has no contents");
		error = ENOEXEC;
		goto out;
	}
	if (nsym != 1) {
		/* Only allow one symbol table for now */
		kobj_error("file has no valid symbol table");
		error = ENOEXEC;
		goto out;
	}
	if (symstrindex < 0 || symstrindex > hdr->e_shnum ||
	    shdr[symstrindex].sh_type != SHT_STRTAB) {
		kobj_error("file has invalid symbol strings");
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
			goto out;
		}
	}
	if (ko->ko_nrel != 0) {
		ko->ko_reltab = kmem_zalloc(ko->ko_nrel *
		    sizeof(*ko->ko_reltab), KM_SLEEP);
		if (ko->ko_reltab == NULL) {
			error = ENOMEM;
			goto out;
		}
	}
	if (ko->ko_nrela != 0) {
		ko->ko_relatab = kmem_zalloc(ko->ko_nrela *
		    sizeof(*ko->ko_relatab), KM_SLEEP);
		if (ko->ko_relatab == NULL) {
			error = ENOMEM;
			goto out;
		}
	}
	if (symtabindex == -1) {
		kobj_error("lost symbol table index");
		goto out;
	}

	/*
	 * Allocate space for and load the symbol table.
	 */
	ko->ko_symcnt = shdr[symtabindex].sh_size / sizeof(Elf_Sym);
	if (ko->ko_symcnt == 0) {
		kobj_error("no symbol table");
		goto out;
	}
	error = kobj_read(ko, (void **)&ko->ko_symtab,
	    ko->ko_symcnt * sizeof(Elf_Sym),
	    shdr[symtabindex].sh_offset);
	if (error != 0) {
		goto out;
	}

	/*
	 * Allocate space for and load the symbol strings.
	 */
	ko->ko_strtabsz = shdr[symstrindex].sh_size;
	if (ko->ko_strtabsz == 0) {
		kobj_error("no symbol strings");
		goto out;
	}
	error = kobj_read(ko, (void *)&ko->ko_strtab, ko->ko_strtabsz,
	    shdr[symstrindex].sh_offset);
	if (error != 0) {
		goto out;
	}

	/*
	 * Do we have a string table for the section names?
	 */
	if (hdr->e_shstrndx != 0 && shdr[hdr->e_shstrndx].sh_size != 0 &&
	    shdr[hdr->e_shstrndx].sh_type == SHT_STRTAB) {
		ko->ko_shstrtabsz = shdr[hdr->e_shstrndx].sh_size;
		error = kobj_read(ko, (void **)&ko->ko_shstrtab,
		    shdr[hdr->e_shstrndx].sh_size,
		    shdr[hdr->e_shstrndx].sh_offset);
		if (error != 0) {
			goto out;
		}
	}

	/*
	 * Size up code/data(progbits) and bss(nobits).
	 */
	alignmask = 0;
	mapbase = 0;
	for (i = 0; i < hdr->e_shnum; i++) {
		switch (shdr[i].sh_type) {
		case SHT_PROGBITS:
		case SHT_NOBITS:
			if (mapbase == 0)
				mapbase = shdr[i].sh_offset;
			alignmask = shdr[i].sh_addralign - 1;
			mapsize += alignmask;
			mapsize &= ~alignmask;
			mapsize += shdr[i].sh_size;
			break;
		}
	}

	/*
	 * We know how much space we need for the text/data/bss/etc.
	 * This stuff needs to be in a single chunk so that profiling etc
	 * can get the bounds and gdb can associate offsets with modules.
	 */
	if (mapsize == 0) {
		kobj_error("no text/data/bss");
		goto out;
	}
	if (ko->ko_type == KT_MEMORY) {
		mapbase += (vaddr_t)ko->ko_source;
	} else {
		mapbase = uvm_km_alloc(module_map, round_page(mapsize),
		    0, UVM_KMF_WIRED | UVM_KMF_EXEC);
		if (mapbase == 0) {
			error = ENOMEM;
			goto out;
		}
	}
	ko->ko_address = mapbase;
	ko->ko_size = mapsize;

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
			if (ko->ko_type == KT_MEMORY) {
				addr = (void *)(shdr[i].sh_offset +
				    (vaddr_t)ko->ko_source);
				if (((vaddr_t)addr & alignmask) != 0) {
					kobj_error("section %d not aligned\n",
					    i);
					goto out;
				}
			} else {
				mapbase += alignmask;
				mapbase &= ~alignmask;
				addr = (void *)mapbase;
				mapbase += shdr[i].sh_size;
			}
			ko->ko_progtab[pb].addr = addr;
			if (shdr[i].sh_type == SHT_PROGBITS) {
				ko->ko_progtab[pb].name = "<<PROGBITS>>";
				error = kobj_read_bits(ko, addr,
				    shdr[i].sh_size, shdr[i].sh_offset);
				if (error != 0) {
					goto out;
				}
			} else if (ko->ko_type == KT_MEMORY &&
			    shdr[i].sh_size != 0) {
			    	kobj_error("non-loadable BSS section in "
			    	    "pre-loaded module");
				error = EINVAL;
			    	goto out;
			} else {
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
			ko->ko_reltab[rl].size = shdr[i].sh_size;
			ko->ko_reltab[rl].size -=
			    shdr[i].sh_size % sizeof(Elf_Rel);
			if (ko->ko_reltab[rl].size != 0) {
				ko->ko_reltab[rl].nrel =
				    shdr[i].sh_size / sizeof(Elf_Rel);
				ko->ko_reltab[rl].sec = shdr[i].sh_info;
				error = kobj_read(ko,
				    (void **)&ko->ko_reltab[rl].rel,
				    ko->ko_reltab[rl].size,
				    shdr[i].sh_offset);
				if (error != 0) {
					goto out;
				}
			}
			rl++;
			break;
		case SHT_RELA:
			ko->ko_relatab[ra].size = shdr[i].sh_size;
			ko->ko_relatab[ra].size -=
			    shdr[i].sh_size % sizeof(Elf_Rela);
			if (ko->ko_relatab[ra].size != 0) {
				ko->ko_relatab[ra].nrela =
				    shdr[i].sh_size / sizeof(Elf_Rela);
				ko->ko_relatab[ra].sec = shdr[i].sh_info;
				error = kobj_read(ko,
				    (void **)&ko->ko_relatab[ra].rela,
				    shdr[i].sh_size,
				    shdr[i].sh_offset);
				if (error != 0) {
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
		panic("lost progbits");
	}
	if (rl != ko->ko_nrel) {
		panic("lost rel");
	}
	if (ra != ko->ko_nrela) {
		panic("lost rela");
	}
	if (ko->ko_type != KT_MEMORY && mapbase != ko->ko_address + mapsize) {
		panic("mapbase 0x%lx != address %lx + mapsize %ld (0x%lx)\n",
		    (long)mapbase, (long)ko->ko_address, (long)mapsize,
		    (long)ko->ko_address + mapsize);
	}

	/*
	 * Perform local relocations only.  Relocations relating to global
	 * symbols will be done by kobj_affix().
	 */
	error = kobj_checksyms(ko, false);
	if (error == 0) {
		error = kobj_relocate(ko, true);
	}
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

/*
 * kobj_unload:
 *
 *	Unload an object previously loaded by kobj_load().
 */
void
kobj_unload(kobj_t ko)
{
	int error;

	kobj_close(ko);
	kobj_jettison(ko);

	/*
	 * Notify MD code that a module has been unloaded.
	 */
	if (ko->ko_loaded) {
		error = kobj_machdep(ko, (void *)ko->ko_address, ko->ko_size,
		    false);
		if (error != 0) {
			kobj_error("machine dependent deinit failed");
		}
	}
	if (ko->ko_address != 0 && ko->ko_type != KT_MEMORY) {
		uvm_km_free(module_map, ko->ko_address, round_page(ko->ko_size),
		    UVM_KMF_WIRED);
	}
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
void
kobj_stat(kobj_t ko, vaddr_t *address, size_t *size)
{

	if (address != NULL) {
		*address = ko->ko_address;
	}
	if (size != NULL) {
		*size = ko->ko_size;
	}
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

	strlcpy(ko->ko_name, name, sizeof(ko->ko_name));

	/* Cache addresses of undefined symbols. */
	error = kobj_checksyms(ko, true);

	/* Now do global relocations. */
	if (error == 0)
		error = kobj_relocate(ko, false);

	/*
	 * Now that we know the name, register the symbol table.
	 * Do after global relocations because ksyms will pack
	 * the table.
	 */
	if (error == 0) {
		ksyms_modload(ko->ko_name, ko->ko_symtab, ko->ko_symcnt *
		    sizeof(Elf_Sym), ko->ko_strtab, ko->ko_strtabsz);
		ko->ko_ksyms = true;
	}

	/* Jettison unneeded memory post-link. */
	kobj_jettison(ko);

	/*
	 * Notify MD code that a module has been loaded.
	 *
	 * Most architectures use this opportunity to flush their caches.
	 */
	if (error == 0) {
		error = kobj_machdep(ko, (void *)ko->ko_address, ko->ko_size,
		    true);
		if (error != 0) {
			kobj_error("machine dependent init failed");
		}
		ko->ko_loaded = true;
	}

	/* If there was an error, destroy the whole object. */
	if (error != 0) {
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
uintptr_t
kobj_sym_lookup(kobj_t ko, uintptr_t symidx)
{
	const Elf_Sym *sym;
	const char *symbol;

	/* Don't even try to lookup the symbol if the index is bogus. */
	if (symidx >= ko->ko_symcnt)
		return 0;

	sym = ko->ko_symtab + symidx;

	/* Quick answer if there is a definition included. */
	if (sym->st_shndx != SHN_UNDEF) {
		return (uintptr_t)sym->st_value;
	}

	/* If we get here, then it is undefined and needs a lookup. */
	switch (ELF_ST_BIND(sym->st_info)) {
	case STB_LOCAL:
		/* Local, but undefined? huh? */
		kobj_error("local symbol undefined");
		return 0;

	case STB_GLOBAL:
		/* Relative to Data or Function name */
		symbol = ko->ko_strtab + sym->st_name;

		/* Force a lookup failure if the symbol name is bogus. */
		if (*symbol == 0) {
			kobj_error("bad symbol name");
			return 0;
		}

		return (uintptr_t)sym->st_value;

	case STB_WEAK:
		kobj_error("weak symbols not supported\n");
		return 0;

	default:
		return 0;
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
 *	exernal symbols.
 */
static int
kobj_checksyms(kobj_t ko, bool undefined)
{
	unsigned long rval;
	Elf_Sym *sym, *ms;
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
		if (ksyms_getval_unlocked(NULL, name, &rval,
		    KSYMS_EXTERN) != 0) {
			if (undefined) {
				kobj_error("symbol `%s' not found", name);
				error = ENOEXEC;
			}
			continue;
		}

		/* Save values of undefined globals. */
		if (undefined) {
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
		    strncmp(name, "__stop_link_set_", 16)) {
		    	continue;
		}
		kobj_error("global symbol `%s' redefined\n", name);
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
			panic("lost base for e_reltab");
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
				return ENOENT;
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
			panic("lost base for e_relatab");
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
				return ENOENT;
			}
		}
	}

	return 0;
}

/*
 * kobj_error:
 *
 *	Utility function: log an error.
 */
static void
kobj_error(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	printf("WARNING: linker error: ");
	vprintf(fmt, ap);
	printf("\n");
	va_end(ap);
}

/*
 * kobj_read:
 *
 *	Utility function: read from the object.
 */
static int
kobj_read(kobj_t ko, void **basep, size_t size, off_t off)
{
	size_t resid;
	void *base;
	int error;

	KASSERT(ko->ko_source != NULL);

	switch (ko->ko_type) {
	case KT_VNODE:
		base = kmem_alloc(size, KM_SLEEP);
		if (base == NULL) {
			error = ENOMEM;
			break;
		}
		error = vn_rdwr(UIO_READ, ko->ko_source, base, size, off,
		    UIO_SYSSPACE, IO_NODELOCKED, curlwp->l_cred, &resid,
		    curlwp);
		if (error == 0 && resid != 0) {
			error = EINVAL;
		}
		if (error != 0) {
			kmem_free(base, size);
			base = NULL;
		}
		break;
	case KT_MEMORY:
		if (ko->ko_memsize != -1 && off + size > ko->ko_memsize) {
			kobj_error("kobj_read: preloaded object short");
			error = EINVAL;
			base = NULL;
		} else {
			base = (uint8_t *)ko->ko_source + off;
			error = 0;
		}
		break;
	default:
		panic("kobj_read: invalid type");
	}

	*basep = base;
	return error;
}

/*
 * kobj_read_bits:
 *
 *	Utility function: load a section from the object.
 */
static int
kobj_read_bits(kobj_t ko, void *base, size_t size, off_t off)
{
	size_t resid;
	int error;

	KASSERT(ko->ko_source != NULL);

	switch (ko->ko_type) {
	case KT_VNODE:
		KASSERT((uintptr_t)base >= (uintptr_t)ko->ko_address);
		KASSERT((uintptr_t)base + size <=
		    (uintptr_t)ko->ko_address + ko->ko_size);
		error = vn_rdwr(UIO_READ, ko->ko_source, base, size, off,
		    UIO_SYSSPACE, IO_NODELOCKED, curlwp->l_cred, &resid,
		    curlwp);
		if (error == 0 && resid != 0) {
			error = EINVAL;
		}
		break;
	case KT_MEMORY:
		if (ko->ko_memsize != -1 && off + size > ko->ko_memsize) {
			kobj_error("kobj_read_bits: preloaded object short");
			error = EINVAL;
		} else if ((uint8_t *)base != (uint8_t *)ko->ko_source + off) {
			kobj_error("kobj_read_bits: object not aligned");
			kobj_error("source=%p base=%p off=%d size=%zd",
			    ko->ko_source, base, (int)off, size);
			error = EINVAL;
		} else {
			/* Nothing to do.  Loading in-situ. */
			error = 0;
		}
		break;
	default:
		panic("kobj_read: invalid type");
	}

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

	if (ko->ko_type != KT_MEMORY)
		kmem_free(base, size);
}

#else	/* MODULAR */

int
kobj_load_file(kobj_t *kop, const char *name, const char *base, bool autoload)
{

	return ENOSYS;
}

int
kobj_load_mem(kobj_t *kop, void *base, ssize_t size)
{

	return ENOSYS;
}

void
kobj_unload(kobj_t ko)
{

	panic("not modular");
}

void
kobj_stat(kobj_t ko, vaddr_t *base, size_t *size)
{

	panic("not modular");
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

#endif	/* MODULAR */
