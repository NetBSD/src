/*	$NetBSD: map_object.c,v 1.65 2023/01/12 19:17:11 christos Exp $	 */

/*
 * Copyright 1996 John D. Polstra.
 * Copyright 1996 Matt Thomas <matt@3am-software.com>
 * Copyright 2002 Charles M. Hannum <root@ihack.net>
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
 *      This product includes software developed by John Polstra.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: map_object.c,v 1.65 2023/01/12 19:17:11 christos Exp $");
#endif /* not lint */

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "debug.h"
#include "rtld.h"

static int convert_prot(int);	/* Elf flags -> mmap protection */
static int convert_flags(int);  /* Elf flags -> mmap flags */

#define EA_UNDEF		(~(Elf_Addr)0)

/*
 * Map a shared object into memory.  The argument is a file descriptor,
 * which must be open on the object and positioned at its beginning.
 *
 * The return value is a pointer to a newly-allocated Obj_Entry structure
 * for the shared object.  Returns NULL on failure.
 */
Obj_Entry *
_rtld_map_object(const char *path, int fd, const struct stat *sb)
{
	Obj_Entry	*obj;
	Elf_Ehdr	*ehdr;
	Elf_Phdr	*phdr;
#if defined(__HAVE_TLS_VARIANT_I) || defined(__HAVE_TLS_VARIANT_II)
	Elf_Phdr	*phtls;
#endif
	Elf_Phdr	*phlimit;
	Elf_Phdr       **segs = NULL;
	int		 nsegs;
	caddr_t		 mapbase = MAP_FAILED;
	size_t		 mapsize = 0;
	int		 mapflags;
	Elf_Addr	 base_alignment;
	Elf_Addr	 base_vaddr;
	Elf_Addr	 base_vlimit;
	Elf_Addr	 text_vlimit;
	Elf_Addr	 text_end;
	void		*base_addr;
	Elf_Off		 data_offset;
	Elf_Addr	 data_vaddr;
	Elf_Addr	 data_vlimit;
	int		 data_flags;
	int		 data_prot;
	caddr_t		 data_addr;
	Elf_Addr	 bss_vaddr;
	Elf_Addr	 bss_vlimit;
	caddr_t		 bss_addr;
#if defined(__HAVE_TLS_VARIANT_I) || defined(__HAVE_TLS_VARIANT_II)
	Elf_Addr	 tls_vaddr = 0; /* Noise GCC */
#endif
	Elf_Addr	 phdr_vaddr;
	size_t		 phdr_memsz, phsize;
	int i;
#ifdef RTLD_LOADER
	Elf_Addr	 clear_vaddr;
	caddr_t	 	 clear_page;
	caddr_t		 clear_addr;
	size_t		 nclear;
#endif
#ifdef GNU_RELRO
	Elf_Addr 	 relro_page;
	size_t		 relro_size;
#endif
#ifdef notyet
	int		 stack_flags;
#endif

	if (sb != NULL && sb->st_size < (off_t)sizeof (Elf_Ehdr)) {
		_rtld_error("%s: not ELF file (too short)", path);
		return NULL;
	}

	obj = _rtld_obj_new();
	obj->path = xstrdup(path);
	obj->pathlen = strlen(path);
	if (sb != NULL) {
		obj->dev = sb->st_dev;
		obj->ino = sb->st_ino;
	}

	ehdr = mmap(NULL, _rtld_pagesz, PROT_READ, MAP_FILE | MAP_SHARED, fd,
	    (off_t)0);
	obj->ehdr = ehdr;
	if (ehdr == MAP_FAILED) {
		_rtld_error("%s: read error: %s", path, xstrerror(errno));
		goto error;
	}
	/* Make sure the file is valid */
	if (memcmp(ELFMAG, ehdr->e_ident, SELFMAG) != 0) {
		_rtld_error("%s: not ELF file (magic number bad)", path);
		goto error;
	}
	if (ehdr->e_ident[EI_CLASS] != ELFCLASS) {
		_rtld_error("%s: invalid ELF class %x; expected %x", path,
		    ehdr->e_ident[EI_CLASS], ELFCLASS);
		goto error;
	}
	/* Elf_e_ident includes class */
	if (ehdr->e_ident[EI_VERSION] != EV_CURRENT ||
	    ehdr->e_version != EV_CURRENT ||
	    ehdr->e_ident[EI_DATA] != ELFDEFNNAME(MACHDEP_ENDIANNESS)) {
		_rtld_error("%s: unsupported file version", path);
		goto error;
	}
	if (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN) {
		_rtld_error("%s: unsupported file type", path);
		goto error;
	}
	switch (ehdr->e_machine) {
		ELFDEFNNAME(MACHDEP_ID_CASES)
	default:
		_rtld_error("%s: unsupported machine", path);
		goto error;
	}

	/*
         * We rely on the program header being in the first page.  This is
         * not strictly required by the ABI specification, but it seems to
         * always true in practice.  And, it simplifies things considerably.
         */
	assert(ehdr->e_phentsize == sizeof(Elf_Phdr));
	assert(ehdr->e_phoff + ehdr->e_phnum * sizeof(Elf_Phdr) <=
	    _rtld_pagesz);

	/*
         * Scan the program header entries, and save key information.
         *
         * We rely on there being exactly two load segments, text and data,
         * in that order.
         */
	phdr = (Elf_Phdr *) ((caddr_t)ehdr + ehdr->e_phoff);
#if defined(__HAVE_TLS_VARIANT_I) || defined(__HAVE_TLS_VARIANT_II)
	phtls = NULL;
#endif
	phsize = ehdr->e_phnum * sizeof(phdr[0]);
	obj->phdr = NULL;
#ifdef GNU_RELRO
	relro_page = 0;
	relro_size = 0;
#endif
	phdr_vaddr = EA_UNDEF;
	phdr_memsz = 0;
	phlimit = phdr + ehdr->e_phnum;
	segs = xmalloc(sizeof(segs[0]) * ehdr->e_phnum);
	if (segs == NULL) {
		_rtld_error("No memory for segs");
		goto error;
	}
#ifdef notyet
	stack_flags = PF_R | PF_W;
#endif
	nsegs = -1;
	while (phdr < phlimit) {
		switch (phdr->p_type) {
		case PT_INTERP:
			obj->interp = (void *)(uintptr_t)phdr->p_vaddr;
 			dbg(("%s: PT_INTERP %p", obj->path, obj->interp));
			break;

		case PT_LOAD:
			segs[++nsegs] = phdr;
			if ((segs[nsegs]->p_align & (_rtld_pagesz - 1)) != 0) {
				_rtld_error(
				    "%s: PT_LOAD segment %d not page-aligned",
				    path, nsegs);
				goto error;
			}
			if ((segs[nsegs]->p_flags & PF_X) == PF_X) {
				text_end = MAX(text_end,
				    round_up(segs[nsegs]->p_vaddr +
				    segs[nsegs]->p_memsz));
			}

			dbg(("%s: %s %p phsize %" PRImemsz, obj->path,
			    "PT_LOAD",
			    (void *)(uintptr_t)phdr->p_vaddr, phdr->p_memsz));
			break;

		case PT_PHDR:
			phdr_vaddr = phdr->p_vaddr;
			phdr_memsz = phdr->p_memsz;
			dbg(("%s: %s %p phsize %" PRImemsz, obj->path,
			    "PT_PHDR",
			    (void *)(uintptr_t)phdr->p_vaddr, phdr->p_memsz));
			break;

#ifdef notyet
		case PT_GNU_STACK:
			stack_flags = phdr->p_flags;
			break;
#endif

#ifdef GNU_RELRO
		case PT_GNU_RELRO:
			relro_page = phdr->p_vaddr;
			relro_size = phdr->p_memsz;
			break;
#endif

		case PT_DYNAMIC:
			obj->dynamic = (void *)(uintptr_t)phdr->p_vaddr;
			dbg(("%s: %s %p phsize %" PRImemsz, obj->path,
			    "PT_DYNAMIC",
			    (void *)(uintptr_t)phdr->p_vaddr, phdr->p_memsz));
			break;

#if defined(__HAVE_TLS_VARIANT_I) || defined(__HAVE_TLS_VARIANT_II)
		case PT_TLS:
			phtls = phdr;
			dbg(("%s: %s %p phsize %" PRImemsz, obj->path, "PT_TLS",
			    (void *)(uintptr_t)phdr->p_vaddr, phdr->p_memsz));
			break;
#endif
#ifdef __ARM_EABI__
		case PT_ARM_EXIDX:
			obj->exidx_start = (void *)(uintptr_t)phdr->p_vaddr;
			obj->exidx_sz = phdr->p_memsz;
			break;
#endif
		}

		++phdr;
	}
	phdr = (Elf_Phdr *) ((caddr_t)ehdr + ehdr->e_phoff);
	obj->entry = (void *)(uintptr_t)ehdr->e_entry;
	if (!obj->dynamic) {
		_rtld_error("%s: not dynamically linked", path);
		goto error;
	}

	/*
	 * Map the entire address space of the object as a file
	 * region to stake out our contiguous region and establish a
	 * base for relocation.  We use a file mapping so that
	 * the kernel will give us whatever alignment is appropriate
	 * for the platform we're running on.
	 *
	 * We map it using the text protection, map the data segment
	 * into the right place, then map an anon segment for the bss
	 * and unmap the gaps left by padding to alignment.
	 */

	base_alignment = segs[0]->p_align;
	base_vaddr = round_down(segs[0]->p_vaddr);
	base_vlimit = round_up(segs[nsegs]->p_vaddr + segs[nsegs]->p_memsz);
	text_vlimit = round_up(segs[0]->p_vaddr + segs[0]->p_memsz);
	data_offset = round_down(segs[nsegs]->p_offset);
	data_vaddr = round_down(segs[nsegs]->p_vaddr);
	data_vlimit = round_up(segs[nsegs]->p_vaddr + segs[nsegs]->p_filesz);
	data_flags = convert_prot(segs[nsegs]->p_flags);
#ifdef RTLD_LOADER
	clear_vaddr = segs[nsegs]->p_vaddr + segs[nsegs]->p_filesz;
#endif

	obj->textsize = text_vlimit - base_vaddr;
	obj->vaddrbase = base_vaddr;
	obj->isdynamic = ehdr->e_type == ET_DYN;

#if defined(__HAVE_TLS_VARIANT_I) || defined(__HAVE_TLS_VARIANT_II)
	if (phtls != NULL) {
		++_rtld_tls_dtv_generation;
		obj->tlsindex = ++_rtld_tls_max_index;
		obj->tlssize = phtls->p_memsz;
		obj->tlsalign = phtls->p_align;
		obj->tlsinitsize = phtls->p_filesz;
		tls_vaddr = phtls->p_vaddr;
	}
#endif

	/*
	 * Calculate log2 of the base section alignment.
	 */
	mapflags = MAP_PRIVATE | MAP_ANON;
	if (base_alignment > _rtld_pagesz) {
		unsigned int log2 = 0;
		for (; base_alignment > 1; base_alignment >>= 1)
			log2++;
		mapflags |= MAP_ALIGNED(log2);
	}

	base_addr = NULL;
#ifdef RTLD_LOADER
	if (!obj->isdynamic) {
		mapflags |= MAP_TRYFIXED;
		base_addr = (void *)(uintptr_t)base_vaddr;
	}
#endif
	mapsize = base_vlimit - base_vaddr;
	mapbase = mmap(base_addr, mapsize, PROT_NONE, mapflags, -1, 0);
	if (mapbase == MAP_FAILED) {
		_rtld_error("mmap of entire address space failed: %s",
		    xstrerror(errno));
		goto error;
	}
#ifdef RTLD_LOADER
	if (!obj->isdynamic && mapbase != base_addr) {
		_rtld_error("mmap of executable at correct address failed");
		goto error;
	}
#endif

	obj->phdr_loaded = false;
	for (i = 0; i <= nsegs; i++) {
		/* Overlay the segment onto the proper region. */
		data_offset = round_down(segs[i]->p_offset);
		data_vaddr = round_down(segs[i]->p_vaddr);
		data_vlimit = round_up(segs[i]->p_vaddr
		    + segs[i]->p_filesz);
		data_addr = mapbase + (data_vaddr - base_vaddr);
		data_prot = convert_prot(segs[i]->p_flags);
		data_flags = convert_flags(segs[i]->p_flags) | MAP_FIXED;
		if (data_vlimit != data_vaddr &&
		    mmap(data_addr, data_vlimit - data_vaddr, data_prot, 
		    data_flags, fd, data_offset) == MAP_FAILED) {
			_rtld_error("%s: mmap of data failed: %s", path,
			    xstrerror(errno));
			goto error;
		}

		/* Do BSS setup */
		if (segs[i]->p_filesz != segs[i]->p_memsz) {
#ifdef RTLD_LOADER
			/* Clear any BSS in the last page of the segment. */
			clear_vaddr = segs[i]->p_vaddr + segs[i]->p_filesz;
			clear_addr = mapbase + (clear_vaddr - base_vaddr);
			clear_page = mapbase + (round_down(clear_vaddr)
			    - base_vaddr);

			if ((nclear = data_vlimit - clear_vaddr) > 0) {
				/* Make sure the end of the segment is writable
				 */
				if ((data_prot & PROT_WRITE) == 0 && -1 ==
				     mprotect(clear_page, _rtld_pagesz,
				     data_prot|PROT_WRITE)) {
					_rtld_error("%s: mprotect failed: %s",
					    path, xstrerror(errno));
					goto error;
				}

				memset(clear_addr, 0, nclear);

				/* Reset the data protection back */
				if ((data_prot & PROT_WRITE) == 0)
					mprotect(clear_page, _rtld_pagesz,
					    data_prot);
			}
#endif

			/* Overlay the BSS segment onto the proper region. */
			bss_vaddr = data_vlimit;
			bss_vlimit = round_up(segs[i]->p_vaddr +
			    segs[i]->p_memsz);
			bss_addr = mapbase + (bss_vaddr - base_vaddr);
			if (bss_vlimit > bss_vaddr) {
				/* There is something to do */
				if (mmap(bss_addr, bss_vlimit - bss_vaddr,
				    data_prot, data_flags | MAP_ANON, -1, 0)
				    == MAP_FAILED) {
					_rtld_error(
					    "%s: mmap of bss failed: %s",
					    path, xstrerror(errno));
					goto error;
				}
			}
		}

		if (phdr_vaddr != EA_UNDEF &&
		    segs[i]->p_vaddr <= phdr_vaddr &&
		    segs[i]->p_memsz >= phdr_memsz) {
			obj->phdr_loaded = true;
		}
		if (segs[i]->p_offset <= ehdr->e_phoff &&
		    segs[i]->p_memsz >= phsize) {
			phdr_vaddr = segs[i]->p_vaddr + ehdr->e_phoff;
			phdr_memsz = phsize;
			obj->phdr_loaded = true;
		}
	}
	if (obj->phdr_loaded) {
		obj->phdr = (void *)(uintptr_t)phdr_vaddr;
		obj->phsize = phdr_memsz;
	} else {
		Elf_Phdr *buf = xmalloc(phsize);
		if (buf == NULL) {
			_rtld_error("%s: cannot allocate program header", path);
			goto error;
		}
		memcpy(buf, phdr, phsize);
		obj->phdr = buf;
		obj->phsize = phsize;
	}

#if defined(__HAVE_TLS_VARIANT_I) || defined(__HAVE_TLS_VARIANT_II)
	if (phtls != NULL)
		obj->tlsinit = mapbase + tls_vaddr;
#endif

	obj->mapbase = mapbase;
	obj->mapsize = mapsize;
	obj->relocbase = mapbase - base_vaddr;

#ifdef GNU_RELRO
	/* rounding happens later. */
	obj->relro_page = obj->relocbase + relro_page;
	obj->relro_size = relro_size;
#endif

	if (obj->dynamic)
		obj->dynamic = (void *)(obj->relocbase + (Elf_Addr)(uintptr_t)obj->dynamic);
	if (obj->entry)
		obj->entry = (void *)(obj->relocbase + (Elf_Addr)(uintptr_t)obj->entry);
	if (obj->interp)
		obj->interp = (void *)(obj->relocbase + (Elf_Addr)(uintptr_t)obj->interp);
	if (obj->phdr_loaded)
		obj->phdr =  (void *)(obj->relocbase + (Elf_Addr)(uintptr_t)obj->phdr);
#ifdef __ARM_EABI__
	if (obj->exidx_start)
		obj->exidx_start = (void *)(obj->relocbase + (Elf_Addr)(uintptr_t)obj->exidx_start);
#endif
	xfree(segs);

	return obj;

error:
	if (mapbase != MAP_FAILED)
		munmap(mapbase, mapsize);
	if (obj->ehdr != MAP_FAILED)
		munmap(obj->ehdr, _rtld_pagesz);
	_rtld_obj_free(obj);
	xfree(segs);
	return NULL;
}

void
_rtld_obj_free(Obj_Entry *obj)
{
	Objlist_Entry *elm;
	Name_Entry *entry;

#if defined(__HAVE_TLS_VARIANT_I) || defined(__HAVE_TLS_VARIANT_II)
	if (obj->tls_done)
		_rtld_tls_offset_free(obj);
#endif
	xfree(obj->path);
	while (obj->needed != NULL) {
		Needed_Entry *needed = obj->needed;
		obj->needed = needed->next;
		xfree(needed);
	}
	while ((entry = SIMPLEQ_FIRST(&obj->names)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&obj->names, link);
		xfree(entry);
	}
	while ((elm = SIMPLEQ_FIRST(&obj->dldags)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&obj->dldags, link);
		xfree(elm);
	}
	while ((elm = SIMPLEQ_FIRST(&obj->dagmembers)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&obj->dagmembers, link);
		xfree(elm);
	}
	if (!obj->phdr_loaded)
		xfree((void *)(uintptr_t)obj->phdr);
	xfree(obj);
}

Obj_Entry *
_rtld_obj_new(void)
{
	Obj_Entry *obj;

	obj = CNEW(Obj_Entry);
	SIMPLEQ_INIT(&obj->names);
	SIMPLEQ_INIT(&obj->dldags);
	SIMPLEQ_INIT(&obj->dagmembers);
	return obj;
}

/*
 * Given a set of ELF protection flags, return the corresponding protection
 * flags for MMAP.
 */
static int
convert_prot(int elfflags)
{
	int prot = 0;

	if (elfflags & PF_R)
		prot |= PROT_READ;
#ifdef RTLD_LOADER
	if (elfflags & PF_W)
		prot |= PROT_WRITE;
#endif
	if (elfflags & PF_X)
		prot |= PROT_EXEC;
	return prot;
}

static int
convert_flags(int elfflags __unused)
{
	int flags = MAP_PRIVATE; /* All mappings are private */

#ifdef MAP_NOCORE
	/*
	 * Readonly mappings are marked "MAP_NOCORE", because they can be
	 * reconstructed by a debugger.
	 */
	if (!(elfflags & PF_W))
		flags |= MAP_NOCORE;
#endif
	return flags;
}
