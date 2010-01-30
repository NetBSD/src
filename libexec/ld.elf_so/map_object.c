/*	$NetBSD: map_object.c,v 1.36.4.2 2010/01/30 18:53:47 snj Exp $	 */

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
__RCSID("$NetBSD: map_object.c,v 1.36.4.2 2010/01/30 18:53:47 snj Exp $");
#endif /* not lint */

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "rtld.h"

static int protflags(int);	/* Elf flags -> mmap protection */

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
	Elf_Phdr	*phlimit;
	Elf_Phdr	*segs[2];
	int		 nsegs;
	caddr_t		 mapbase = MAP_FAILED;
	size_t		 mapsize = 0;
	int		 mapflags;
	Elf_Off		 base_offset;
#ifdef MAP_ALIGNED
	Elf_Addr	 base_alignment;
#endif
	Elf_Addr	 base_vaddr;
	Elf_Addr	 base_vlimit;
	Elf_Addr	 text_vlimit;
	int		 text_flags;
	caddr_t		 base_addr;
	Elf_Off		 data_offset;
	Elf_Addr	 data_vaddr;
	Elf_Addr	 data_vlimit;
	int		 data_flags;
	caddr_t		 data_addr;
	caddr_t		 gap_addr;
	size_t		 gap_size;
#ifdef RTLD_LOADER
	Elf_Addr	 clear_vaddr;
	caddr_t		 clear_addr;
	size_t		 nclear;
#endif

	if (sb != NULL && sb->st_size < sizeof (Elf_Ehdr)) {
		_rtld_error("%s: unrecognized file format1", path);
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
		goto bad;
	}
	/* Make sure the file is valid */
	if (memcmp(ELFMAG, ehdr->e_ident, SELFMAG) != 0 ||
	    ehdr->e_ident[EI_CLASS] != ELFCLASS) {
		_rtld_error("%s: unrecognized file format2 [%x != %x]", path, ehdr->e_ident[EI_CLASS], ELFCLASS);
		goto bad;
	}
	/* Elf_e_ident includes class */
	if (ehdr->e_ident[EI_VERSION] != EV_CURRENT ||
	    ehdr->e_version != EV_CURRENT ||
	    ehdr->e_ident[EI_DATA] != ELFDEFNNAME(MACHDEP_ENDIANNESS)) {
		_rtld_error("%s: unsupported file version", path);
		goto bad;
	}
	if (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN) {
		_rtld_error("%s: unsupported file type", path);
		goto bad;
	}
	switch (ehdr->e_machine) {
		ELFDEFNNAME(MACHDEP_ID_CASES)
	default:
		_rtld_error("%s: unsupported machine", path);
		goto bad;
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
	phlimit = phdr + ehdr->e_phnum;
	nsegs = 0;
	while (phdr < phlimit) {
		switch (phdr->p_type) {
		case PT_INTERP:
			obj->interp = (void *)(uintptr_t)phdr->p_vaddr;
			break;

		case PT_LOAD:
			if (nsegs < 2)
				segs[nsegs] = phdr;
			++nsegs;
			break;

		case PT_DYNAMIC:
			obj->dynamic = (void *)(uintptr_t)phdr->p_vaddr;
			break;
		}

		++phdr;
	}
	obj->entry = (void *)(uintptr_t)ehdr->e_entry;
	if (!obj->dynamic) {
		_rtld_error("%s: not dynamically linked", path);
		goto bad;
	}
	if (nsegs != 2) {
		_rtld_error("%s: wrong number of segments (%d != 2)", path,
		    nsegs);
		goto bad;
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

#ifdef MAP_ALIGNED
	base_alignment = segs[0]->p_align;
#endif
	base_offset = round_down(segs[0]->p_offset);
	base_vaddr = round_down(segs[0]->p_vaddr);
	base_vlimit = round_up(segs[1]->p_vaddr + segs[1]->p_memsz);
	text_vlimit = round_up(segs[0]->p_vaddr + segs[0]->p_memsz);
	text_flags = protflags(segs[0]->p_flags);
	data_offset = round_down(segs[1]->p_offset);
	data_vaddr = round_down(segs[1]->p_vaddr);
	data_vlimit = round_up(segs[1]->p_vaddr + segs[1]->p_filesz);
	data_flags = protflags(segs[1]->p_flags);
#ifdef RTLD_LOADER
	clear_vaddr = segs[1]->p_vaddr + segs[1]->p_filesz;
#endif

	obj->textsize = text_vlimit - base_vaddr;
	obj->vaddrbase = base_vaddr;
	obj->isdynamic = ehdr->e_type == ET_DYN;

	/* Unmap header if it overlaps the first load section. */
	if (base_offset < _rtld_pagesz) {
		munmap(ehdr, _rtld_pagesz);
		obj->ehdr = MAP_FAILED;
	}

	/*
	 * Calculate log2 of the base section alignment.
	 */
	mapflags = 0;
#ifdef MAP_ALIGNED
	if (base_alignment > _rtld_pagesz) {
		unsigned int log2 = 0;
		for (; base_alignment > 1; base_alignment >>= 1)
			log2++;
		mapflags = MAP_ALIGNED(log2);
	}
#endif

#ifdef RTLD_LOADER
	base_addr = obj->isdynamic ? NULL : (caddr_t)base_vaddr;
#else
	base_addr = NULL;
#endif
	mapsize = base_vlimit - base_vaddr;
	mapbase = mmap(base_addr, mapsize, text_flags,
	    mapflags | MAP_FILE | MAP_PRIVATE, fd, base_offset);
	if (mapbase == MAP_FAILED) {
		_rtld_error("mmap of entire address space failed: %s",
		    xstrerror(errno));
		goto bad;
	}

	/* Overlay the data segment onto the proper region. */
	data_addr = mapbase + (data_vaddr - base_vaddr);
	if (mmap(data_addr, data_vlimit - data_vaddr, data_flags,
	    MAP_FILE | MAP_PRIVATE | MAP_FIXED, fd, data_offset) ==
	    MAP_FAILED) {
		_rtld_error("mmap of data failed: %s", xstrerror(errno));
		goto bad;
	}

	/* Overlay the bss segment onto the proper region. */
	if (mmap(mapbase + data_vlimit - base_vaddr, base_vlimit - data_vlimit,
	    data_flags, MAP_ANON | MAP_PRIVATE | MAP_FIXED, -1, 0) ==
	    MAP_FAILED) {
		_rtld_error("mmap of bss failed: %s", xstrerror(errno));
		goto bad;
	}

	/* Unmap the gap between the text and data. */
	gap_addr = mapbase + round_up(text_vlimit - base_vaddr);
	gap_size = data_addr - gap_addr;
	if (gap_size != 0 && mprotect(gap_addr, gap_size, PROT_NONE) == -1) {
		_rtld_error("mprotect of text -> data gap failed: %s",
		    xstrerror(errno));
		goto bad;
	}

#ifdef RTLD_LOADER
	/* Clear any BSS in the last page of the data segment. */
	clear_addr = mapbase + (clear_vaddr - base_vaddr);
	if ((nclear = data_vlimit - clear_vaddr) > 0)
		memset(clear_addr, 0, nclear);

	/* Non-file portion of BSS mapped above. */
#endif

	obj->mapbase = mapbase;
	obj->mapsize = mapsize;
	obj->relocbase = mapbase - base_vaddr;

	if (obj->dynamic)
		obj->dynamic = (void *)(obj->relocbase + (Elf_Addr)(uintptr_t)obj->dynamic);
	if (obj->entry)
		obj->entry = (void *)(obj->relocbase + (Elf_Addr)(uintptr_t)obj->entry);
	if (obj->interp)
		obj->interp = (void *)(obj->relocbase + (Elf_Addr)(uintptr_t)obj->interp);

	return obj;

bad:
	if (obj->ehdr != MAP_FAILED)
		munmap(obj->ehdr, _rtld_pagesz);
	if (mapbase != MAP_FAILED)
		munmap(mapbase, mapsize);
	_rtld_obj_free(obj);
	return NULL;
}

void
_rtld_obj_free(Obj_Entry *obj)
{
	Objlist_Entry *elm;

	xfree(obj->path);
	while (obj->needed != NULL) {
		Needed_Entry *needed = obj->needed;
		obj->needed = needed->next;
		xfree(needed);
	}
	while ((elm = SIMPLEQ_FIRST(&obj->dldags)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&obj->dldags, link);
		xfree(elm);
	}
	while ((elm = SIMPLEQ_FIRST(&obj->dagmembers)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&obj->dagmembers, link);
		xfree(elm);
	}
	xfree(obj);
#ifdef COMBRELOC
	_rtld_combreloc_reset(obj);
#endif
}

Obj_Entry *
_rtld_obj_new(void)
{
	Obj_Entry *obj;

	obj = CNEW(Obj_Entry);
	SIMPLEQ_INIT(&obj->dldags);
	SIMPLEQ_INIT(&obj->dagmembers);
	return obj;
}

/*
 * Given a set of ELF protection flags, return the corresponding protection
 * flags for MMAP.
 */
static int
protflags(int elfflags)
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
