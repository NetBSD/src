/*	$NetBSD: reloc.c,v 1.1.2.1 1998/05/08 17:39:12 mycroft Exp $	*/

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

#ifdef __alpha__
static int
_rtld_do_copy_relocation(
    const Obj_Entry *dstobj,
    const Elf_RelA *rela)
{
    void *dstaddr = (void *) (dstobj->relocbase + rela->r_offset);
    const Elf_Sym *dstsym = dstobj->symtab + ELF_R_SYM(rela->r_info);
    const char *name = dstobj->strtab + dstsym->st_name;
    unsigned long hash = _rtld_elf_hash(name);
    size_t size = dstsym->st_size;
    const void *srcaddr;
    const Elf_Sym *srcsym;
    Obj_Entry *srcobj;

    for (srcobj = dstobj->next;  srcobj != NULL;  srcobj = srcobj->next)
	if ((srcsym = _rtld_symlook_obj(name, hash, srcobj, false)) != NULL)
	    break;

    if (srcobj == NULL) {
	_rtld_error("Undefined symbol \"%s\" referenced from COPY"
	      " relocation in %s", name, dstobj->path);
	return -1;
    }

    srcaddr = (const void *) (srcobj->relocbase + srcsym->st_value);
    memcpy(dstaddr, srcaddr, size);
    return 0;
}
#endif /* __alpha__ */

/*
 * Process the special R_xxx_COPY relocations in the main program.  These
 * copy data from a shared object into a region in the main program's BSS
 * segment.
 *
 * Returns 0 on success, -1 on failure.
 */
int
_rtld_do_copy_relocations(
    const Obj_Entry *dstobj)
{
    assert(dstobj->mainprog);	/* COPY relocations are invalid elsewhere */

#ifdef __alpha__ /* jrs */
    if (dstobj->rel != NULL) {
	const Elf_Rel *rel;
	for (rel = dstobj->rel;  rel < dstobj->rellim;  ++rel) {
	    if (ELF_R_TYPE(rel->r_info) == R_TYPE(COPY)) {
		Elf_RelA ourrela;
		ourrela.r_info = rel->r_info;
		ourrela.r_offset = rel->r_offset;
		ourrela.r_addend = 0;
		if (_rtld_do_copy_relocation(dstobj, &ourrela) < 0)
		    return -1;
	    }
	}
    }

    if (dstobj->rela != NULL) {
	const Elf_RelA *rela;
	for (rela = dstobj->rela;  rela < dstobj->relalim;  ++rela) {
	    if (ELF_R_TYPE(rela->r_info) == R_TYPE(COPY)) {
		if (_rtld_do_copy_relocation(dstobj, rela) < 0)
		    return -1;
	    }
	}
    }
#endif /* jrs */

    return 0;
}

static int
_rtld_relocate_nonplt_object(
    const Obj_Entry *obj,
    const Elf_RelA *rela)
{
    Elf_Addr *where = (Elf_Addr *) (obj->relocbase + rela->r_offset);

    switch (ELF_R_TYPE(rela->r_info)) {

    case R_TYPE(NONE):
	break;

#ifdef __i386__
    case R_386_GOT32: {
	const Elf_Sym *def;
	const Obj_Entry *defobj;

	def = _rtld_find_symdef(_rtld_objlist, rela->r_info, NULL, obj, &defobj, false);
	if (def == NULL)
	    return -1;

	if (*where != (Elf_Addr) (defobj->relocbase + def->st_value + rela->r_addend))
	    *where = (Elf_Addr) (defobj->relocbase + def->st_value + rela->r_addend);
	break;
    }

    case R_386_PC32:
	/*
	 * I don't think the dynamic linker should ever see this
	 * type of relocation.  But the binutils-2.6 tools sometimes
	 * generate it.
	 */
    {
	const Elf_Sym *def;
	const Obj_Entry *defobj;

	def = _rtld_find_symdef(_rtld_objlist, rela->r_info, NULL, obj, &defobj, false);
	if (def == NULL)
	    return -1;

	*where += (Elf_Addr) (defobj->relocbase + def->st_value)
	    - (Elf_Addr) where;
	break;
    }
#endif
#ifdef __alpha__
    case R_ALPHA_REFQUAD: {
	const Elf_Sym *def;
	const Obj_Entry *defobj;
	Elf_Addr tmp_value;

	def = _rtld_find_symdef(_rtld_objlist, rela->r_info, NULL, obj, &defobj, false);
	if (def == NULL)
	    return -1;

	tmp_value = (Elf_Addr) (defobj->relocbase + def->st_value)
	    + *where + rela->r_addend;
	if (*where != tmp_value)
	    *where = tmp_value;
	break;
    }
/*#endif*/

    case R_TYPE(GLOB_DAT):
    {
	const Elf_Sym *def;
	const Obj_Entry *defobj;

	def = _rtld_find_symdef(_rtld_objlist, rela->r_info, NULL, obj, &defobj, false);
	if (def == NULL)
	    return -1;

	if (*where != (Elf_Addr) (defobj->relocbase + def->st_value))
	    *where = (Elf_Addr) (defobj->relocbase + def->st_value);
	break;
    }

    case R_TYPE(RELATIVE): {
	extern Elf_Addr _GLOBAL_OFFSET_TABLE_[];
	extern Elf_Dyn _DYNAMIC;

	if (obj != &_rtld_objself ||
	    (caddr_t)where < (caddr_t)_GLOBAL_OFFSET_TABLE_ ||
	    (caddr_t)where >= (caddr_t)&_DYNAMIC)
	    *where += (Elf_Addr) obj->relocbase;
	break;
    }

    case R_TYPE(COPY): {
	/*
	 * These are deferred until all other relocations have
	 * been done.  All we do here is make sure that the COPY
	 * relocation is not in a shared library.  They are allowed
	 * only in executable files.
	 */
	if (!obj->mainprog) {
	    _rtld_error("%s: Unexpected R_COPY relocation in shared library",
		  obj->path);
	    return -1;
	}
	break;
    }
#endif /* __alpha__ */

#ifdef __mips__
    case R_TYPE(REL32): {
    		/* 32-bit PC-relative reference */

        const Elf_Sym *def;
        const Obj_Entry *defobj;

	def = obj->symtab + ELF_R_SYM(rela->r_info);

        if (ELF_SYM_BIND(def->st_info) == Elf_estb_local &&
          (ELF_SYM_TYPE(def->st_info) == Elf_estt_section ||
           ELF_SYM_TYPE(def->st_info) == Elf_estt_notype)) {
            *where += (Elf_Addr) obj->relocbase;
        } else {
/* XXX maybe do something re: bootstrapping? */
            def = _rtld_find_symdef(_rtld_objlist, rela->r_info, NULL, obj,
	        &defobj, false);
            if (def == NULL)
                return -1;
	    *where += (Elf_Addr)(defobj->relocbase + def->st_value);
        }
        break;
    }
 
#endif /* mips */

    default: {
	const Elf_Sym *def;
	const Obj_Entry *defobj;

	def = _rtld_find_symdef(_rtld_objlist, rela->r_info, NULL, obj, &defobj, true);
	dbg("sym = %d, type = %d, offset = %p, addend = %p, contents = %p, symbol = %s",
	    ELF_R_SYM(rela->r_info), ELF_R_TYPE(rela->r_info),
	    rela->r_offset, rela->r_addend, *where,
	    def ? defobj->strtab + def->st_name : "??");
	_rtld_error("%s: Unsupported relocation type %d in non-PLT relocations\n",
	      obj->path, ELF_R_TYPE(rela->r_info));
	return -1;
    }
    }
    return 0;
}

static int
_rtld_relocate_plt_object(
    const Obj_Entry *obj,
    const Elf_RelA *rela,
    bool bind_now)
{
    Elf_Addr *where = (Elf_Addr *) (obj->relocbase + rela->r_offset);
    Elf_Addr new_value;

    /* Fully resolve procedure addresses now */

#if defined(__alpha__)	/* (jrs) */
    if (bind_now || obj->pltgot == NULL) {
	const Elf_Sym *def;
	const Obj_Entry *defobj;

	assert(ELF_R_TYPE(rela->r_info) == R_TYPE(JMP_SLOT));

	def = _rtld_find_symdef(_rtld_objlist, rela->r_info, NULL, obj, &defobj, true);
	if (def == NULL)
	    return -1;

	new_value = (Elf_Addr) (defobj->relocbase + def->st_value);
#if 0
	dbg("fixup %s in %s --> %p in %s", 
	    defobj->strtab + def->st_name, obj->path,
	    new_value, defobj->path);
#endif
    } else
#endif	/* __alpha__ (jrs) */
     if (!obj->mainprog) {
	/* Just relocate the GOT slots pointing into the PLT */
	new_value = *where + (Elf_Addr) (obj->relocbase);
    } else {
	return 0;
    }
    /*
     * Since this page is probably copy-on-write, let's not write
     * it unless we really really have to.
     */
    if (*where != new_value)
	*where = new_value;
    return 0;
}

caddr_t
_rtld_bind(
    const Obj_Entry *obj,
    Elf_Word reloff)
{
    const Elf_RelA *rela;
    Elf_RelA ourrela;

    if (obj->pltrel != NULL) {
	ourrela.r_info =   ((const Elf_Rel *) ((caddr_t) obj->pltrel + reloff))->r_info;
	ourrela.r_offset = ((const Elf_Rel *) ((caddr_t) obj->pltrel + reloff))->r_offset;
	rela = &ourrela;
    } else {
	rela = (const Elf_RelA *) ((caddr_t) obj->pltrela + reloff);
    }


    if (_rtld_relocate_plt_object(obj, rela, true) < 0)
	_rtld_die();

    return *(caddr_t *)(obj->relocbase + rela->r_offset);
}

/*
 * Relocate newly-loaded shared objects.  The argument is a pointer to
 * the Obj_Entry for the first such object.  All objects from the first
 * to the end of the list of objects are relocated.  Returns 0 on success,
 * or -1 on failure.
 */
int
_rtld_relocate_objects(
    Obj_Entry *first,
    bool bind_now)
{
    Obj_Entry *obj;
    int ok = 1;

    for (obj = first;  obj != NULL;  obj = obj->next) {

	if (obj->nbuckets == 0 || obj->nchains == 0
	        || obj->buckets == NULL || obj->symtab == NULL
	        || obj->strtab == NULL) {
	    _rtld_error("%s: Shared object has no run-time symbol table",
			obj->path);
	    return -1;
	}

	dbg(" relocating %s (%d/%d rel/rela, %d/%d plt rel/rela)",
	    obj->path,
	    obj->rellim - obj->rel, obj->relalim - obj->rela,
	    obj->pltrellim - obj->pltrel, obj->pltrelalim - obj->pltrela);

	if (obj->textrel) {
	    /* There are relocations to the write-protected text segment. */
	    if (mprotect(obj->mapbase, obj->textsize,
			 PROT_READ|PROT_WRITE|PROT_EXEC) == -1) {
		_rtld_error("%s: Cannot write-enable text segment: %s",
			    obj->path, xstrerror(errno));
		return -1;
	    }
	}

	if (obj->rel != NULL) {
	    /* Process the non-PLT relocations. */
	    const Elf_Rel *rel;
	    for (rel = obj->rel;  rel < obj->rellim;  ++rel) {
		Elf_RelA ourrela;
		ourrela.r_info   = rel->r_info;
		ourrela.r_offset = rel->r_offset;
#if defined(__mips__)
		/* rel->r_offset is not valid on mips? */
		if (ELF_R_TYPE(ourrela.r_info) == R_TYPE(NONE))
		    ourrela.r_addend = 0;
		else
#endif
		ourrela.r_addend = *(Elf_Word *) (obj->relocbase + rel->r_offset);

		if (_rtld_relocate_nonplt_object(obj, &ourrela) < 0)
		    ok = 0;
	    }
	}

	if (obj->rela != NULL) {
	    /* Process the non-PLT relocations. */
	    const Elf_RelA *rela;
	    for (rela = obj->rela;  rela < obj->relalim;  ++rela) {
		if (_rtld_relocate_nonplt_object(obj, rela) < 0)
		    ok = 0;
	    }
	}

	if (obj->textrel) {	/* Re-protected the text segment. */
	    if (mprotect(obj->mapbase, obj->textsize,
			 PROT_READ|PROT_EXEC) == -1) {
		_rtld_error("%s: Cannot write-protect text segment: %s",
			    obj->path, xstrerror(errno));
		return -1;
	    }
	}

	/* Process the PLT relocations. */
	if (obj->pltrel != NULL) {
	    const Elf_Rel *rel;
	    for (rel = obj->pltrel; rel < obj->pltrellim;  ++rel) {
		Elf_RelA ourrela;
		ourrela.r_info   = rel->r_info;
		ourrela.r_offset = rel->r_offset;
		ourrela.r_addend = *(Elf_Word *) (obj->relocbase + rel->r_offset);
		if (_rtld_relocate_plt_object(obj, &ourrela, bind_now) < 0)
		    ok = 0;
	    }
	}

	if (obj->pltrela != NULL) {
	    const Elf_RelA *rela;
	    for (rela = obj->pltrela;  rela < obj->pltrelalim;  ++rela) {
		if (_rtld_relocate_plt_object(obj, rela, bind_now) < 0)
		    ok = 0;
	    }
	}

	if (!ok)
	    return -1;


	/* Set some sanity-checking numbers in the Obj_Entry. */
	obj->magic = RTLD_MAGIC;
	obj->version = RTLD_VERSION;

	/* Fill in the dynamic linker entry points. */
	obj->dlopen  = _rtld_dlopen;
	obj->dlsym   = _rtld_dlsym;
	obj->dlerror = _rtld_dlerror;
	obj->dlclose = _rtld_dlclose;

	/* Set the special PLTGOT entries. */
	if (obj->pltgot != NULL) {
#if defined(__i386__)
	    obj->pltgot[1] = (Elf_Addr) obj;
	    obj->pltgot[2] = (Elf_Addr) &_rtld_bind_start;
#endif
#if defined(__alpha__)
	    /* This function will be called to perform the relocation.  */
	    obj->pltgot[2] = (Elf_Addr) &_rtld_bind_start;
	    /* Identify this shared object */
	    obj->pltgot[3] = (Elf_Addr) obj;
#endif
#if defined(__mips__)
	    _rtld_relocate_mips_got(obj);

	    obj->pltgot[0] = (Elf_Addr) &_rtld_bind_start;
	    /* XXX only if obj->pltgot[1] & 0x80000000 ?? */
	    obj->pltgot[1] |= (Elf_Addr) obj;
#endif
	}
    }

    return 0;
}
