/*	$NetBSD: reloc.c,v 1.60 2002/09/05 20:08:14 mycroft Exp $	 */

/*
 * Copyright 1996 John D. Polstra.
 * Copyright 1996 Matt Thomas <matt@3am-software.com>
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

/*
 * Dynamic linker for ELF.
 *
 * John Polstra <jdp@polstra.com>.
 */

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <dirent.h>

#include "debug.h"
#include "rtld.h"

#ifndef RTLD_INHIBIT_COPY_RELOCS
static int _rtld_do_copy_relocation __P((const Obj_Entry *, const Elf_Rela *,
    bool));

/*
 * XXX: These don't work for the alpha and i386; don't know about powerpc
 *	The alpha and the i386 avoid the problem by compiling everything PIC.
 *	These relocation are supposed to be writing the address of the
 *	function to be called on the bss.rel or bss.rela segment, but:
 *		- st_size == 0
 *		- on the i386 at least the call instruction is a direct call
 *		  not an indirect call.
 */
static int
_rtld_do_copy_relocation(dstobj, rela, dodebug)
	const Obj_Entry *dstobj;
	const Elf_Rela *rela;
	bool dodebug;
{
	void           *dstaddr = (void *)(dstobj->relocbase + rela->r_offset);
	const Elf_Sym  *dstsym = dstobj->symtab + ELF_R_SYM(rela->r_info);
	const char     *name = dstobj->strtab + dstsym->st_name;
	unsigned long   hash = _rtld_elf_hash(name);
	size_t          size = dstsym->st_size;
	const void     *srcaddr;
	const Elf_Sym  *srcsym = NULL;
	Obj_Entry      *srcobj;

	for (srcobj = dstobj->next; srcobj != NULL; srcobj = srcobj->next)
		if ((srcsym = _rtld_symlook_obj(name, hash, srcobj,
		    false)) != NULL)
			break;

	if (srcobj == NULL) {
		_rtld_error("Undefined symbol \"%s\" referenced from COPY"
		    " relocation in %s", name, dstobj->path);
		return (-1);
	}
	srcaddr = (const void *)(srcobj->relocbase + srcsym->st_value);
	(void)memcpy(dstaddr, srcaddr, size);
	rdbg(dodebug, ("COPY %s %s %s --> src=%p dst=%p *dst= %p size %ld",
	    dstobj->path, srcobj->path, name, (void *)srcaddr,
	    (void *)dstaddr, (void *)*(long *)dstaddr, (long)size));
	return (0);
}
#endif /* RTLD_INHIBIT_COPY_RELOCS */


/*
 * Process the special R_xxx_COPY relocations in the main program.  These
 * copy data from a shared object into a region in the main program's BSS
 * segment.
 *
 * Returns 0 on success, -1 on failure.
 */
int
_rtld_do_copy_relocations(dstobj, dodebug)
	const Obj_Entry *dstobj;
	bool dodebug;
{
#ifndef RTLD_INHIBIT_COPY_RELOCS

	/* COPY relocations are invalid elsewhere */
	assert(dstobj->mainprog);

	if (dstobj->rel != NULL) {
		const Elf_Rel  *rel;
		for (rel = dstobj->rel; rel < dstobj->rellim; ++rel) {
			if (ELF_R_TYPE(rel->r_info) == R_TYPE(COPY)) {
				Elf_Rela        ourrela;
				ourrela.r_info = rel->r_info;
				ourrela.r_offset = rel->r_offset;
				ourrela.r_addend = 0;
				if (_rtld_do_copy_relocation(dstobj,
				    &ourrela, dodebug) < 0)
					return (-1);
			}
		}
	}
	if (dstobj->rela != NULL) {
		const Elf_Rela *rela;
		for (rela = dstobj->rela; rela < dstobj->relalim; ++rela) {
			if (ELF_R_TYPE(rela->r_info) == R_TYPE(COPY)) {
				if (_rtld_do_copy_relocation(dstobj, rela,
				    dodebug) < 0)
					return (-1);
			}
		}
	}
#endif /* RTLD_INHIBIT_COPY_RELOCS */

	return (0);
}


#if !defined(__sparc__) && !defined(__x86_64__) && !defined(__mips__)

#if !defined(__powerpc__) && !defined(__hppa__)

int
_rtld_relocate_plt_object(obj, rela, addrp, bind_now, dodebug)
	Obj_Entry *obj;
	const Elf_Rela *rela;
	caddr_t *addrp;
	bool bind_now;
	bool dodebug;
{
	Elf_Addr *where = (Elf_Addr *)(obj->relocbase + rela->r_offset);
	Elf_Addr new_value;

	/* Fully resolve procedure addresses now */

	if (bind_now || obj->pltgot == NULL) {
		const Elf_Sym  *def;
		const Obj_Entry *defobj;

#if !defined(__arm__)
		assert(ELF_R_TYPE(rela->r_info) == R_TYPE(JMP_SLOT));
#else
		assert(ELF_R_TYPE(rela->r_info) == R_TYPE(JUMP_SLOT));
#endif

		def = _rtld_find_symdef(rela->r_info, obj, &defobj, true);
		if (def == NULL)
			return -1;

		new_value = (Elf_Addr)(defobj->relocbase + def->st_value);
#if defined(__sh__) || defined(__vax__)
		new_value += rela->r_addend;
#endif
		rdbg(dodebug, ("bind now %d/fixup in %s --> old=%p new=%p",
		    (int)bind_now,
		    defobj->strtab + def->st_name,
		    (void *)*where, (void *)new_value));
	} else if (!obj->mainprog) {
		/* Just relocate the GOT slots pointing into the PLT */
		new_value = *where + (Elf_Addr)(obj->relocbase);
		rdbg(dodebug, ("fixup !main in %s --> %p", obj->path,
		    (void *)*where));
	} else {
		return 0;
	}
	/*
         * Since this page is probably copy-on-write, let's not write
         * it unless we really really have to.
         */
	if (*where != new_value)
		*where = new_value;
	if (addrp != NULL) {
		*addrp = *(caddr_t *)(obj->relocbase + rela->r_offset);
#if defined(__vax__)
		*addrp -= rela->r_addend;
#endif
	}
	return 0;
}

#endif /* __powerpc__  || __hppa__ */

#endif /* __sparc__ || __x86_64__ || __mips__ */

caddr_t
_rtld_bind(obj, reloff)
	Obj_Entry *obj;
	Elf_Word reloff;
{
	const Elf_Rela *rela;
	Elf_Rela        ourrela;
	caddr_t		addr;

	if (obj->pltrel != NULL) {
		const Elf_Rel *rel;

		rel = (const Elf_Rel *)((caddr_t) obj->pltrel + reloff);
		ourrela.r_info = rel->r_info;
		ourrela.r_offset = rel->r_offset;
		ourrela.r_addend = 0;
		rela = &ourrela;
	} else {
		rela = (const Elf_Rela *)((caddr_t) obj->pltrela + reloff);
#ifdef __sparc64__
		if (ELF_R_TYPE(obj->pltrela->r_info) == R_TYPE(JMP_SLOT)) {
			/*
			 * XXXX
			 *
			 * The first four PLT entries are reserved.  There
			 * is some disagreement whether they should have
			 * associated relocation entries.  Both the SPARC
			 * 32-bit and 64-bit ELF specifications say that
			 * they should have relocation entries, but the 
			 * 32-bit SPARC binutils do not generate them,
			 * and now the 64-bit SPARC binutils have stopped
			 * generating them too.
			 * 
			 * So, to provide binary compatibility, we will
			 * check the first entry, if it is reserved it
			 * should not be of the type JMP_SLOT.  If it
			 * is JMP_SLOT, then the 4 reserved entries were
			 * not generated and our index is 4 entries too far.
			 */
			rela -= 4;
		}
#endif
	}

	if (_rtld_relocate_plt_object(obj, rela, &addr, true, true) < 0)
		_rtld_die();

	return addr;
}

/*
 * Relocate newly-loaded shared objects.  The argument is a pointer to
 * the Obj_Entry for the first such object.  All objects from the first
 * to the end of the list of objects are relocated.  Returns 0 on success,
 * or -1 on failure.
 */
int
_rtld_relocate_objects(first, bind_now, dodebug)
	Obj_Entry *first;
	bool bind_now;
	bool dodebug;
{
	Obj_Entry *obj;
	int ok = 1;

	for (obj = first; obj != NULL; obj = obj->next) {
		if (obj->nbuckets == 0 || obj->nchains == 0 ||
		    obj->buckets == NULL || obj->symtab == NULL ||
		    obj->strtab == NULL) {
			_rtld_error("%s: Shared object has no run-time"
			    " symbol table", obj->path);
			return -1;
		}
		rdbg(dodebug, (" relocating %s (%ld/%ld rel/rela, "
		    "%ld/%ld plt rel/rela)",
		    obj->path,
		    (long)(obj->rellim - obj->rel),
		    (long)(obj->relalim - obj->rela),
		    (long)(obj->pltrellim - obj->pltrel),
		    (long)(obj->pltrelalim - obj->pltrela)));

		if (obj->textrel) {
			/*
			 * There are relocations to the write-protected text
			 * segment.
			 */
			if (mprotect(obj->mapbase, obj->textsize,
				PROT_READ | PROT_WRITE | PROT_EXEC) == -1) {
				_rtld_error("%s: Cannot write-enable text "
				    "segment: %s", obj->path, xstrerror(errno));
				return -1;
			}
		}
		if (_rtld_relocate_nonplt_objects(obj, dodebug) < 0)
			ok = 0;
		if (obj->textrel) {	/* Re-protected the text segment. */
			if (mprotect(obj->mapbase, obj->textsize,
				     PROT_READ | PROT_EXEC) == -1) {
				_rtld_error("%s: Cannot write-protect text "
				    "segment: %s", obj->path, xstrerror(errno));
				return -1;
			}
		}
		/* Process the PLT relocations. */
		if (obj->pltrel != NULL) {
			const Elf_Rel  *rel;
			for (rel = obj->pltrel; rel < obj->pltrellim; ++rel) {
				Elf_Rela        ourrela;
				ourrela.r_info = rel->r_info;
				ourrela.r_offset = rel->r_offset;
				ourrela.r_addend =
				    *(Elf_Word *)(obj->relocbase +
				    rel->r_offset);
				if (_rtld_relocate_plt_object(obj, &ourrela,
				    NULL, bind_now, dodebug) < 0)
					ok = 0;
			}
		}
		if (obj->pltrela != NULL) {
			const Elf_Rela *rela;
			for (rela = obj->pltrela; rela < obj->pltrelalim;
			    ++rela) {
#ifdef __sparc64__
				if (ELF_R_TYPE(rela->r_info) !=
					R_TYPE(JMP_SLOT)) {
					/*
					 * XXXX
					 *
					 * The first four PLT entries are
					 * reserved.  There is some
					 * disagreement whether they should
					 * have associated relocation
					 * entries.  Both the SPARC 32-bit
					 * and 64-bit ELF specifications say
					 * that they should have relocation
					 * entries, but the 32-bit SPARC
					 * binutils do not generate them,
					 * and now the 64-bit SPARC binutils
					 * have stopped generating them too.
					 *
					 * To provide binary compatibility, we
					 * will skip any entries that are not
					 * of type JMP_SLOT.  
					 */
					continue;
				}
#endif
				if (_rtld_relocate_plt_object(obj, rela,
				    NULL, bind_now, dodebug) < 0)
					ok = 0;
			}
		}
		if (!ok)
			return -1;


		/* Set some sanity-checking numbers in the Obj_Entry. */
		obj->magic = RTLD_MAGIC;
		obj->version = RTLD_VERSION;

		/* Fill in the dynamic linker entry points. */
		obj->dlopen = _rtld_dlopen;
		obj->dlsym = _rtld_dlsym;
		obj->dlerror = _rtld_dlerror;
		obj->dlclose = _rtld_dlclose;
		obj->dladdr = _rtld_dladdr;

		/* Set the special PLTGOT entries. */
		if (obj->pltgot != NULL)
			_rtld_setup_pltgot(obj);
	}

	return 0;
}
