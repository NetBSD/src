/*	$NetBSD: reloc.c,v 1.22 1999/10/28 23:58:21 simonb Exp $	 */

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
static int _rtld_do_copy_relocation __P((const Obj_Entry *, const Elf_RelA *,
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
	const Elf_RelA *rela;
	bool dodebug;
{
	void           *dstaddr = (void *)(dstobj->relocbase + rela->r_offset);
	const Elf_Sym  *dstsym = dstobj->symtab + ELF_R_SYM(rela->r_info);
	const char     *name = dstobj->strtab + dstsym->st_name;
	unsigned long   hash = _rtld_elf_hash(name);
	size_t          size = dstsym->st_size;
	const void     *srcaddr;
	const Elf_Sym  *srcsym;
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
				Elf_RelA        ourrela;
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
		const Elf_RelA *rela;
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


#ifndef __sparc__
int
_rtld_relocate_nonplt_object(obj, rela, dodebug)
	const Obj_Entry *obj;
	const Elf_RelA *rela;
	bool dodebug;
{
	Elf_Addr        *where = (Elf_Addr *)(obj->relocbase + rela->r_offset);
	const Elf_Sym   *def;
	const Obj_Entry *defobj;
#if defined(__alpha__) || defined(__i386__) || defined(__m68k__)
	extern Elf_Addr  _GLOBAL_OFFSET_TABLE_[];
	extern Elf_Dyn   _DYNAMIC;
#endif
#if defined(__alpha__) || defined(__i386__) || defined(__m68k__) || \
    defined(__powerpc__) || defined(__vax__)
	Elf_Addr         tmp;
#endif

	switch (ELF_R_TYPE(rela->r_info)) {

	case R_TYPE(NONE):
		break;

#if defined(__i386__) || defined(__m68k__)
	case R_TYPE(GOT32):

		def = _rtld_find_symdef(_rtld_objlist, rela->r_info, NULL, obj,
		    &defobj, false);
		if (def == NULL)
			return -1;

		tmp = (Elf_Addr)(defobj->relocbase + def->st_value);
		if (*where != tmp)
			*where = tmp;
		rdbg(dodebug, ("GOT32 %s in %s --> %p in %s",
		    defobj->strtab + def->st_name, obj->path,
		    (void *)*where, defobj->path));
		break;

	case R_TYPE(PC32):
		/*
		 * I don't think the dynamic linker should ever see this
		 * type of relocation.  But the binutils-2.6 tools sometimes
		 * generate it.
		 */

		def = _rtld_find_symdef(_rtld_objlist, rela->r_info, NULL, obj,
		    &defobj, false);
		if (def == NULL)
			return -1;

		*where += (Elf_Addr)(defobj->relocbase + def->st_value) -
		    (Elf_Addr)where;
		rdbg(dodebug, ("PC32 %s in %s --> %p in %s",
		    defobj->strtab + def->st_name, obj->path,
		    (void *)*where, defobj->path));
		break;

	case R_TYPE(32):
		def = _rtld_find_symdef(_rtld_objlist, rela->r_info, NULL, obj,
		    &defobj, false);
		if (def == NULL)
			return -1;

		*where += (Elf_Addr)(defobj->relocbase + def->st_value);
		rdbg(dodebug, ("32 %s in %s --> %p in %s",
		    defobj->strtab + def->st_name, obj->path,
		    (void *)*where, defobj->path));
		break;
#endif /* __i386__ || __m68k__ */

#if defined(__alpha__)
	case R_TYPE(REFQUAD):
		def = _rtld_find_symdef(_rtld_objlist, rela->r_info, NULL, obj,
		    &defobj, false);
		if (def == NULL)
			return -1;

		tmp = (Elf_Addr)(defobj->relocbase + def->st_value) +
		    *where + rela->r_addend;
		if (*where != tmp)
			*where = tmp;
		rdbg(dodebug, ("REFQUAD %s in %s --> %p in %s",
		    defobj->strtab + def->st_name, obj->path,
		    (void *)*where, defobj->path));
		break;
#endif /* __alpha__ */

#if defined(__alpha__) || defined(__i386__) || defined(__m68k__)
	case R_TYPE(GLOB_DAT):
		def = _rtld_find_symdef(_rtld_objlist, rela->r_info, NULL, obj,
		    &defobj, false);
		if (def == NULL)
			return -1;

		if (*where != (Elf_Addr)(defobj->relocbase + def->st_value))
			*where = (Elf_Addr)(defobj->relocbase + def->st_value);
		rdbg(dodebug, ("GLOB_DAT %s in %s --> %p in %s",
		    defobj->strtab + def->st_name, obj->path,
		    (void *)*where, defobj->path));
		break;

	case R_TYPE(RELATIVE):
		if (!dodebug ||
		    (caddr_t)where < (caddr_t)_GLOBAL_OFFSET_TABLE_ ||
		    (caddr_t)where >= (caddr_t)&_DYNAMIC) {
			*where += (Elf_Addr)obj->relocbase;
			rdbg(dodebug, ("RELATIVE in %s --> %p", obj->path,
			    (void *)*where));
		}
		else
			rdbg(dodebug, ("RELATIVE in %s stays at %p",
			    obj->path, (void *)*where));
		break;

	case R_TYPE(COPY):
		/*
		 * These are deferred until all other relocations have
		 * been done.  All we do here is make sure that the COPY
		 * relocation is not in a shared library.  They are allowed
		 * only in executable files.
		 */
		if (!obj->mainprog) {
			_rtld_error(
			"%s: Unexpected R_COPY relocation in shared library",
			    obj->path);
			return -1;
		}
		rdbg(dodebug, ("COPY (avoid in main)"));
		break;
#endif /* __i386__ || __alpha__ */

#if defined(__mips__)
	case R_TYPE(REL32):
		/* 32-bit PC-relative reference */
		def = obj->symtab + ELF_R_SYM(rela->r_info);

		if (ELFDEFNNAME(ST_BIND)(def->st_info) == STB_LOCAL &&
		  (ELFDEFNNAME(ST_TYPE)(def->st_info) == STT_SECTION ||
		   ELFDEFNNAME(ST_TYPE)(def->st_info) == STT_NOTYPE)) {
			*where += (Elf_Addr)obj->relocbase;
			rdbg(dodebug, ("REL32 in %s --> %p", obj->path,
			    (void *)*where));
		} else {
			/* XXX maybe do something re: bootstrapping? */
			def = _rtld_find_symdef(_rtld_objlist, rela->r_info,
			    NULL, obj, &defobj, false);
			if (def == NULL)
				return -1;
			*where += (Elf_Addr)(defobj->relocbase + def->st_value);
			rdbg(dodebug, ("REL32 %s in %s --> %p in %s",
			    defobj->strtab + def->st_name, obj->path,
			    (void *)*where, defobj->path));
		}
		break;

#endif /* __mips__ */

#if defined(__powerpc__) || defined(__vax__)
	case R_TYPE(32):	/* word32 S + A */
	case R_TYPE(GLOB_DAT):	/* word32 S + A */
		def = _rtld_find_symdef(_rtld_objlist, rela->r_info, NULL, obj,
		    &defobj, false);
		if (def == NULL)
			return -1;

		tmp = (Elf_Addr)(defobj->relocbase + def->st_value +
		    rela->r_addend);

		if (*where != tmp)
			*where = tmp;
		rdbg(dodebug, ("32/GLOB_DAT %s in %s --> %p in %s",
		    defobj->strtab + def->st_name, obj->path,
		    (void *)*where, defobj->path));
		break;

	case R_TYPE(COPY):
		rdbg(dodebug, ("COPY"));
		break;

	case R_TYPE(JMP_SLOT):
		rdbg(dodebug, ("JMP_SLOT"));
		break;

	case R_TYPE(RELATIVE):	/* word32 B + A */
		tmp = (Elf_Addr)(obj->relocbase + rela->r_addend);
		if (*where != tmp)
			*where = tmp;
		rdbg(dodebug, ("RELATIVE in %s --> %p", obj->path,
		    (void *)*where));
		break;
#endif /* __powerpc__ || __vax__ */

#if defined(__vax__)
	case R_TYPE(REL32):
		def = _rtld_find_symdef(_rtld_objlist, rela->r_info, NULL, obj,
		    &defobj, false);
		if (def == NULL)
			return -1;

		*where += (Elf_Addr)(defobj->relocbase + def->st_value
			+ rela->r_addend);
		rdbg(dodebug, ("32 %s in %s --> %p in %s",
		    defobj->strtab + def->st_name, obj->path,
		    (void *)*where, defobj->path));
		break;
#endif /* __vax__ */

	default:
		def = _rtld_find_symdef(_rtld_objlist, rela->r_info, NULL, obj,
		    &defobj, true);
		rdbg(dodebug, ("sym = %lu, type = %lu, offset = %p, "
		    "addend = %p, contents = %p, symbol = %s",
		    (u_long)ELF_R_SYM(rela->r_info),
		    (u_long)ELF_R_TYPE(rela->r_info),
		    (void *)rela->r_offset, (void *)rela->r_addend,
		    (void *)*where,
		    def ? defobj->strtab + def->st_name : "??"));
		_rtld_error("%s: Unsupported relocation type %d"
		    "in non-PLT relocations\n",
		    obj->path, ELF_R_TYPE(rela->r_info));
		return -1;
	}
	return 0;
}



int
_rtld_relocate_plt_object(obj, rela, addrp, bind_now, dodebug)
	const Obj_Entry *obj;
	const Elf_RelA *rela;
	caddr_t *addrp;
	bool bind_now;
	bool dodebug;
{
	Elf_Addr *where = (Elf_Addr *)(obj->relocbase + rela->r_offset);
	Elf_Addr new_value;

	/* Fully resolve procedure addresses now */

#if defined(__powerpc__)
	return _rtld_reloc_powerpc_plt(obj, rela, bind_now);
#endif

#if defined(__alpha__) || defined(__i386__) || defined(__m68k__) || \
    defined(__vax__)
	if (bind_now || obj->pltgot == NULL) {
		const Elf_Sym  *def;
		const Obj_Entry *defobj;

		assert(ELF_R_TYPE(rela->r_info) == R_TYPE(JMP_SLOT));

		def = _rtld_find_symdef(_rtld_objlist, rela->r_info, NULL, obj,
		    &defobj, true);
		if (def == NULL)
			return -1;

		new_value = (Elf_Addr)(defobj->relocbase + def->st_value);
		rdbg(dodebug, ("bind now %d/fixup in %s --> old=%p new=%p",
		    (int)bind_now,
		    defobj->strtab + def->st_name,
		    (void *)*where, (void *)new_value));
	} else
#endif /* __alpha__ || __i386__ || __m68k__ || __vax__ */
	if (!obj->mainprog) {
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
	if (addrp != NULL)
		*addrp = *(caddr_t *)(obj->relocbase + rela->r_offset);
	return 0;
}
#endif /* __sparc__ */

caddr_t
_rtld_bind(obj, reloff)
	const Obj_Entry *obj;
	Elf_Word reloff;
{
	const Elf_RelA *rela;
	Elf_RelA        ourrela;
	caddr_t		addr;

	if (obj->pltrel != NULL) {
		const Elf_Rel *rel;

		rel = (const Elf_Rel *)((caddr_t) obj->pltrel + reloff);
		ourrela.r_info = rel->r_info;
		ourrela.r_offset = rel->r_offset;
		rela = &ourrela;
	} else {
		rela = (const Elf_RelA *)((caddr_t) obj->pltrela + reloff);
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
		if (obj->rel != NULL) {
			/* Process the non-PLT relocations. */
			const Elf_Rel  *rel;
			for (rel = obj->rel; rel < obj->rellim; ++rel) {
				Elf_RelA        ourrela;
				ourrela.r_info = rel->r_info;
				ourrela.r_offset = rel->r_offset;
#if defined(__mips__)
				/* rel->r_offset is not valid on mips? */
				if (ELF_R_TYPE(ourrela.r_info) == R_TYPE(NONE))
					ourrela.r_addend = 0;
				else
#endif
					ourrela.r_addend =
					    *(Elf_Word *)(obj->relocbase +
					    rel->r_offset);

				if (_rtld_relocate_nonplt_object(obj, &ourrela,
				    dodebug) < 0)
					ok = 0;
			}
		}
		if (obj->rela != NULL) {
			/* Process the non-PLT relocations. */
			const Elf_RelA *rela;
			for (rela = obj->rela; rela < obj->relalim; ++rela) {
				if (_rtld_relocate_nonplt_object(obj, rela,
				    dodebug) < 0)
					ok = 0;
			}
		}
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
				Elf_RelA        ourrela;
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
			const Elf_RelA *rela;
			for (rela = obj->pltrela; rela < obj->pltrelalim;
			    ++rela) {
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

		/* Set the special PLTGOT entries. */
		if (obj->pltgot != NULL) {
#if defined(__i386__) || defined(__m68k__)
			obj->pltgot[1] = (Elf_Addr) obj;
			obj->pltgot[2] = (Elf_Addr) & _rtld_bind_start;
#endif
#if defined(__alpha__)
			/*
			 * This function will be called to perform the
			 * relocation.
			 */
			obj->pltgot[2] = (Elf_Addr) & _rtld_bind_start;
			/* Identify this shared object */
			obj->pltgot[3] = (Elf_Addr) obj;
#endif
#if defined(__mips__)
			_rtld_relocate_mips_got(obj);

			obj->pltgot[0] = (Elf_Addr) & _rtld_bind_start;
			/* XXX only if obj->pltgot[1] & 0x80000000 ?? */
			obj->pltgot[1] |= (Elf_Addr) obj;
#endif
#if defined(__powerpc__)
			_rtld_setup_powerpc_plt(obj);
#endif
#if defined(__sparc__)
			/*
			 * PLTGOT is the PLT on the sparc.
			 * The first entry holds the call the dynamic linker.
			 * We construct a `call' sequence that transfers
			 * to `_rtld_bind_start()'.
			 * The second entry holds the object identification.
			 * Note: each PLT entry is three words long.
			 */
#define SAVE	0x9de3bfc0	/* i.e. `save %sp,-64,%sp' */
#define CALL	0x40000000
#define NOP	0x01000000
			obj->pltgot[0] = SAVE;
			obj->pltgot[1] = CALL |
			    ((Elf_Addr)&_rtld_bind_start -
			     (Elf_Addr)&obj->pltgot[1]) >> 2;
			obj->pltgot[2] = NOP;

			obj->pltgot[3] = (Elf_Addr) obj;
#endif
#if defined(__vax__)
			obj->pltgot[0] = (Elf_Addr) & _rtld_bind_start;
			obj->pltgot[1] = (Elf_Addr) obj;
#endif
		}
	}

	return 0;
}
