/*	$NetBSD: subr_kobj.c,v 1.4 2008/01/06 15:13:07 jmcneill Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: subr_kobj.c,v 1.4 2008/01/06 15:13:07 jmcneill Exp $");

#define	ELFSIZE		ARCH_ELFSIZE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/proc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/fcntl.h>
#include <sys/kobj.h>
#include <sys/ksyms.h>
#include <sys/lkm.h>
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
	char		ko_name[MAXLKMNAME];
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
	uintptr_t	ko_entry;	/* Entry point */
	size_t		ko_size;	/* Size of text/data/bss */
	size_t		ko_symcnt;	/* Number of symbols */
	size_t		ko_strtabsz;	/* Number of bytes in string table */
	size_t		ko_shdrsz;
	int		ko_nrel;
	int		ko_nrela;
	int		ko_nprogtab;
	bool		ko_ksyms;
	bool		ko_loaded;
};

static int	kobj_relocate(kobj_t);
static void	kobj_error(const char *, ...);
static int	kobj_read(kobj_t, void *, size_t, off_t);
static void	kobj_release_mem(kobj_t);

extern struct vm_map *lkm_map;
static const char	*kobj_path = "/modules";	/* XXX ??? */

/*
 * kobj_open_file:
 *
 *	Open an object located in the file system.  'name' may not
 *	be known in advance and so is preliminary.
 */
int
kobj_open_file(kobj_t *kop, const char *name, const char *filename)
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

	strlcpy(ko->ko_name, name, sizeof(ko->ko_name));

	/* XXX where to look? */
	NDINIT(&nd, LOOKUP, FOLLOW, UIO_SYSSPACE, filename);
	error = vn_open(&nd, FREAD, 0);
	if (error != 0) {
		if (error != ENOENT) {
			goto out;
		}
		path = PNBUF_GET();
		snprintf(path, MAXPATHLEN - 1, "%s/%s", kobj_path,
		    filename);
		NDINIT(&nd, LOOKUP, FOLLOW, UIO_SYSSPACE, path);
		error = vn_open(&nd, FREAD, 0);
		PNBUF_PUT(path);
		if (error != 0) {
			goto out;
		}
	}

 out:
 	if (error != 0) {
	 	kmem_free(ko, sizeof(*ko));
	} else {
		ko->ko_type = KT_VNODE;
		ko->ko_source = nd.ni_vp;
		*kop = ko;
	}
	return error;
}

/*
 * kobj_open_mem:
 *
 *	Open a pre-loaded object already resident in memory.  If size
 *	is not -1, the complete size of the object is known.  'name' may
 *	not be known in advance and so is preliminary.
 */
int
kobj_open_mem(kobj_t *kop, const char *name, void *base, ssize_t size)
{
	kobj_t ko;

	ko = kmem_zalloc(sizeof(*ko), KM_SLEEP);
	if (ko == NULL) {
		return ENOMEM;
	}

	strlcpy(ko->ko_name, name, sizeof(ko->ko_name));
	ko->ko_type = KT_MEMORY;
	ko->ko_source = base;
	ko->ko_memsize = size;
	*kop = ko;

	return 0;
}

/*
 * kobj_close:
 *
 *	Close an open ELF object.  If the object was not successfully
 *	loaded, it will be destroyed.
 */
void
kobj_close(kobj_t ko)
{

	KASSERT(ko->ko_source != NULL);

	switch (ko->ko_type) {
	case KT_VNODE:
		VOP_UNLOCK(ko->ko_source, 0);
		vn_close(ko->ko_source, FREAD, kauth_cred_get(), curlwp);
		break;
	case KT_MEMORY:
		/* nothing */
		break;
	default:
		panic("kobj_close: unknown type");
		break;
	}

	ko->ko_source = NULL;
	ko->ko_type = KT_UNSET;

	/* If the object hasn't been loaded, then destroy it. */
	if (!ko->ko_loaded) {
		kobj_unload(ko);
	}
}

/*
 * kobj_load:
 *
 *	Load an ELF object from the file system and link into the
 *	running	kernel image.
 */
int
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

	KASSERT(ko->ko_type != KT_UNSET);
	KASSERT(ko->ko_source != NULL);

	shdr = NULL;
	mapsize = 0;
	error = 0;
	hdr = NULL;

	/*
	 * Read the elf header from the file.
	 */
	hdr = kmem_alloc(sizeof(*hdr), KM_SLEEP);
	if (hdr == NULL) {
		error = ENOMEM;
		goto out;
	}
	error = kobj_read(ko, hdr, sizeof(*hdr), 0);
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
	shdr = kmem_alloc(ko->ko_shdrsz, KM_SLEEP);
	if (shdr == NULL) {
		error = ENOMEM;
		goto out;
	}
	ko->ko_shdr = shdr;
	error = kobj_read(ko, shdr, ko->ko_shdrsz, hdr->e_shoff);
	if (error != 0) {
		goto out;
	}

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
	ko->ko_symtab = kmem_alloc(ko->ko_symcnt * sizeof(Elf_Sym), KM_SLEEP);
	if (ko->ko_symtab == NULL) {
		error = ENOMEM;
		goto out;
	}
	error = kobj_read(ko, ko->ko_symtab, shdr[symtabindex].sh_size,
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
	ko->ko_strtab = kmem_alloc(ko->ko_strtabsz, KM_SLEEP);
	if (ko->ko_strtab == NULL) {
		error = ENOMEM;
		goto out;
	}
	error = kobj_read(ko, ko->ko_strtab, shdr[symstrindex].sh_size,
	    shdr[symstrindex].sh_offset);
	if (error != 0) {
		goto out;
	}

	/*
	 * Size up code/data(progbits) and bss(nobits).
	 */
	alignmask = 0;
	for (i = 0; i < hdr->e_shnum; i++) {
		switch (shdr[i].sh_type) {
		case SHT_PROGBITS:
		case SHT_NOBITS:
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
	mapbase = uvm_km_alloc(lkm_map, round_page(mapsize), 0,
	    UVM_KMF_WIRED | UVM_KMF_EXEC);
	if (mapbase == 0) {
		error = ENOMEM;
		goto out;
	}
	ko->ko_address = mapbase;
	ko->ko_size = mapsize;
	ko->ko_entry = mapbase + hdr->e_entry;

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
			mapbase += alignmask;
			mapbase &= ~alignmask;
			ko->ko_progtab[pb].addr = (void *)mapbase;
			if (shdr[i].sh_type == SHT_PROGBITS) {
				ko->ko_progtab[pb].name = "<<PROGBITS>>";
				error = kobj_read(ko,
				    ko->ko_progtab[pb].addr, shdr[i].sh_size,
				    shdr[i].sh_offset);
				if (error != 0) {
					goto out;
				}
			} else {
				ko->ko_progtab[pb].name = "<<NOBITS>>";
				memset(ko->ko_progtab[pb].addr, 0,
				    shdr[i].sh_size);
			}
			ko->ko_progtab[pb].size = shdr[i].sh_size;
			ko->ko_progtab[pb].sec = i;

			/* Update all symbol values with the offset. */
			for (j = 0; j < ko->ko_symcnt; j++) {
				es = &ko->ko_symtab[j];
				if (es->st_shndx != i) {
					continue;
				}
				es->st_value +=
				    (Elf_Addr)ko->ko_progtab[pb].addr;
			}
			mapbase += shdr[i].sh_size;
			pb++;
			break;
		case SHT_REL:
			ko->ko_reltab[rl].size = shdr[i].sh_size;
			ko->ko_reltab[rl].size -=
			    shdr[i].sh_size % sizeof(Elf_Rel);
			if (ko->ko_reltab[rl].size != 0) {
				ko->ko_reltab[rl].rel =
				    kmem_alloc(ko->ko_reltab[rl].size,
				    KM_SLEEP);
				ko->ko_reltab[rl].nrel =
				    shdr[i].sh_size / sizeof(Elf_Rel);
				ko->ko_reltab[rl].sec = shdr[i].sh_info;
				error = kobj_read(ko,
				    ko->ko_reltab[rl].rel,
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
				ko->ko_relatab[ra].rela =
				    kmem_alloc(ko->ko_relatab[ra].size,
				    KM_SLEEP);
				ko->ko_relatab[ra].nrela =
				    shdr[i].sh_size / sizeof(Elf_Rela);
				ko->ko_relatab[ra].sec = shdr[i].sh_info;
				error = kobj_read(ko,
				    ko->ko_relatab[ra].rela,
				    shdr[i].sh_size,
				    shdr[i].sh_offset);
				if (error != 0) {
					goto out;
				}
			}
			ra++;
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
	if (mapbase != ko->ko_address + mapsize) {
		panic("mapbase 0x%lx != address %lx + mapsize 0x%lx (0x%lx)\n",
		    (long)mapbase, (long)ko->ko_address, (long)mapsize,
		    (long)ko->ko_address + mapsize);
	}

	/*
	 * Perform relocations.  Done before registering with ksyms,
	 * which will pack our symbol table.
	 */
	error = kobj_relocate(ko);
	if (error != 0) {
		goto out;
	}

	/*
	 * Register symbol table with ksyms.
	 */
	error = ksyms_addsymtab(ko->ko_name, ko->ko_symtab, ko->ko_symcnt *
	    sizeof(Elf_Sym), ko->ko_strtab, ko->ko_strtabsz);
	if (error != 0) {
		kobj_error("unable to register module symbol table");
		goto out;
	}
	ko->ko_ksyms = true;

	/*
	 * Notify MD code that a module has been loaded.
	 */
	error = kobj_machdep(ko, (void *)ko->ko_address, ko->ko_size, true);
	if (error != 0) {
		kobj_error("machine dependent init failed");
		goto out;
	}
	ko->ko_loaded = true;
 out:
	kobj_release_mem(ko);
	if (hdr != NULL) {
		kmem_free(hdr, sizeof(*hdr));
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

	if (ko->ko_address != 0) {
		uvm_km_free(lkm_map, ko->ko_address, round_page(ko->ko_size),
		    UVM_KMF_WIRED);
	}
	if (ko->ko_ksyms == true) {
		ksyms_delsymtab(ko->ko_name);
	}
	if (ko->ko_symtab != NULL) {
		kmem_free(ko->ko_symtab, ko->ko_symcnt * sizeof(Elf_Sym));
	}
	if (ko->ko_strtab != NULL) {
		kmem_free(ko->ko_strtab, ko->ko_strtabsz);
	}

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

	kmem_free(ko, sizeof(*ko));
}

/*
 * kobj_stat:
 *
 *	Return size and load address of an object.
 */
void
kobj_stat(kobj_t ko, vaddr_t *address, size_t *size, uintptr_t *entry)
{

	if (address != NULL) {
		*address = ko->ko_address;
	}
	if (size != NULL) {
		*size = ko->ko_size;
	}
	if (entry != NULL) {
		*entry = ko->ko_entry;
	}
}

/*
 * kobj_set_name:
 *
 *	Set an object's name.  Used only for symbol table lookups.
 *	May only be called after the module is loaded.
 */
void
kobj_set_name(kobj_t ko, const char *name)
{

	KASSERT(ko->ko_loaded);

	strlcpy(ko->ko_name, name, sizeof(ko->ko_name));
	/* XXX propagate name change to ksyms. */
}

/*
 * kobj_release_mem: 
 *
 *	Release object data not needed after loading.
 */
static void
kobj_release_mem(kobj_t ko)
{
	int i;

	for (i = 0; i < ko->ko_nrel; i++) {
		if (ko->ko_reltab[i].rel) {
			kmem_free(ko->ko_reltab[i].rel,
			    ko->ko_reltab[i].size);
		}
	}
	for (i = 0; i < ko->ko_nrela; i++) {
		if (ko->ko_relatab[i].rela) {
			kmem_free(ko->ko_relatab[i].rela,
			    ko->ko_relatab[i].size);
		}
	}
	if (ko->ko_reltab != NULL) {
		kmem_free(ko->ko_reltab, ko->ko_nrel *
		    sizeof(*ko->ko_reltab));
		ko->ko_reltab = NULL;
		ko->ko_nrel = 0;
	}
	if (ko->ko_relatab != NULL) {
		kmem_free(ko->ko_relatab, ko->ko_nrela *
		    sizeof(*ko->ko_relatab));
		ko->ko_relatab = NULL;
		ko->ko_nrela = 0;
	}
	if (ko->ko_progtab != NULL) {
		kmem_free(ko->ko_progtab, ko->ko_nprogtab *
		    sizeof(*ko->ko_progtab));
		ko->ko_progtab = NULL;
	}
	if (ko->ko_shdr != NULL) {
		kmem_free(ko->ko_shdr, ko->ko_shdrsz);
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
	int error;
	u_long addr;

	/* Don't even try to lookup the symbol if the index is bogus. */
	if (symidx >= ko->ko_symcnt)
		return 0;

	sym = ko->ko_symtab + symidx;

	/* Quick answer if there is a definition included. */
	if (sym->st_shndx != SHN_UNDEF) {
		return sym->st_value;
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

		error = ksyms_getval(NULL, symbol, &addr, KSYMS_ANY);
		if (error != 0) {
			kobj_error("symbol %s undefined", symbol);
			return (uintptr_t)0;
		}
		return (uintptr_t)addr;

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
 * kobj_relocate:
 *
 *	Resolve all relocations for the loaded object.
 */
static int
kobj_relocate(kobj_t ko)
{
	const Elf_Rel *rellim;
	const Elf_Rel *rel;
	const Elf_Rela *relalim;
	const Elf_Rela *rela;
	const Elf_Sym *sym;
	uintptr_t base;
	int i;
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
			if (ELF_ST_BIND(sym->st_info) == STB_LOCAL) {
				kobj_reloc(ko, base, rel, false, true);
				continue;
			}
			if (kobj_reloc(ko, base, rel, false, false)) {
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
			if (ELF_ST_BIND(sym->st_info) == STB_LOCAL) {
				kobj_reloc(ko, base, rela, true, true);
				continue;
			}
			if (kobj_reloc(ko, base, rela, true, false)) {
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
kobj_read(kobj_t ko, void *base, size_t size, off_t off)
{
	size_t resid;
	int error;

	KASSERT(ko->ko_source != NULL);

	switch (ko->ko_type) {
	case KT_VNODE:
		error = vn_rdwr(UIO_READ, ko->ko_source, base, size, off,
		    UIO_SYSSPACE, IO_NODELOCKED, curlwp->l_cred, &resid,
		    curlwp);
		if (error == 0 && resid != 0) {
			error = EINVAL;
		}
		break;
	case KT_MEMORY:
		if (ko->ko_memsize != -1 && off + size > ko->ko_memsize) {
			kobj_error("kobj_read: preloaded object short");
			error = EINVAL;
		} else {
			memcpy(base, (uint8_t *)ko->ko_source + off, size);
			error = 0;
		}
		break;
	default:
		panic("kobj_read: invalid type");
	}

	return error;
}
