/*	$NetBSD: symbol.c,v 1.6 1999/11/10 18:34:50 thorpej Exp $	 */

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

/*
 * Hash function for symbol table lookup.  Don't even think about changing
 * this.  It is specified by the System V ABI.
 */
unsigned long
_rtld_elf_hash(name)
	const char *name;
{
	const unsigned char *p = (const unsigned char *) name;
	unsigned long   h = 0;
	unsigned long   g;

	while (*p != '\0') {
		h = (h << 4) + *p++;
		if ((g = h & 0xf0000000) != 0)
			h ^= g >> 24;
		h &= ~g;
	}
	return h;
}

const Elf_Sym *
_rtld_symlook_list(const char *name, unsigned long hash, Objlist *objlist,
  const Obj_Entry **defobj_out, bool in_plt)
{
	const Elf_Sym *symp;
	const Elf_Sym *def;
	const Obj_Entry *defobj;
	const Objlist_Entry *elm;
	
	def = NULL;
	defobj = NULL;
	for (elm = SIMPLEQ_FIRST(objlist); elm; elm = SIMPLEQ_NEXT(elm, link)) {
		if (elm->obj->mark == curmark)
			continue;
		elm->obj->mark = curmark;
		if ((symp = _rtld_symlook_obj(name, hash, elm->obj, in_plt))
		    != NULL) {
			if ((def == NULL) ||
			    (ELF_ST_BIND(symp->st_info) != STB_WEAK)) {
				def = symp;
				defobj = elm->obj;
				if (ELF_ST_BIND(def->st_info) != STB_WEAK)
					break;
			}
		}
	}
	if (def != NULL)
		*defobj_out = defobj;
	return def;
}

/*
 * Search the symbol table of a single shared object for a symbol of
 * the given name.  Returns a pointer to the symbol, or NULL if no
 * definition was found.
 *
 * The symbol's hash value is passed in for efficiency reasons; that
 * eliminates many recomputations of the hash value.
 */
const Elf_Sym *
_rtld_symlook_obj(name, hash, obj, in_plt)
	const char *name;
	unsigned long hash;
	const Obj_Entry *obj;
	bool in_plt;
{
	unsigned long symnum = obj->buckets[hash % obj->nbuckets];

	while (symnum != ELF_SYM_UNDEFINED) {
		const Elf_Sym  *symp;
		const char     *strp;

		assert(symnum < obj->nchains);
		symp = obj->symtab + symnum;
		strp = obj->strtab + symp->st_name;
#if 0
		assert(symp->st_name != 0);
#endif
		if (strcmp(name, strp) == 0) {
			if (symp->st_shndx != SHN_UNDEF
#if !defined(__mips__)		/* Following doesn't work on MIPS? mhitch */
			    || (!in_plt && symp->st_value != 0 &&
			        ELF_ST_TYPE(symp->st_info) == STT_FUNC)
#endif
				) {
				return symp;
			}
		}
		symnum = obj->chains[symnum];
	}

	return NULL;
}

/*
 * Given a symbol number in a referencing object, find the corresponding
 * definition of the symbol.  Returns a pointer to the symbol, or NULL if
 * no definition was found.  Returns a pointer to the Obj_Entry of the
 * defining object via the reference parameter DEFOBJ_OUT.
 */
const Elf_Sym *
_rtld_find_symdef(obj_list, r_info, name, refobj, defobj_out, in_plt)
	const Obj_Entry *obj_list;
	Elf_Word r_info;
	const char *name;
	Obj_Entry *refobj;
	const Obj_Entry **defobj_out;
	bool in_plt;
{
	Elf_Word symnum = ELF_R_SYM(r_info);
	const Elf_Sym  *ref;
	const Elf_Sym  *def;
	const Elf_Sym  *symp;
	const Obj_Entry *obj;
	const Obj_Entry *defobj;
	const Objlist_Entry *elm;
	unsigned long   hash;

	if (name == NULL) {
		ref = refobj->symtab + symnum;
		name = refobj->strtab + ref->st_name;
	}
	hash = _rtld_elf_hash(name);
	def = NULL;
	defobj = NULL;
	curmark++;
	
	if (refobj->symbolic) {	/* Look first in the referencing object */
		symp = _rtld_symlook_obj(name, hash, refobj, in_plt);
		refobj->mark = curmark;
		if (symp != NULL) {
			def = symp;
			defobj = refobj;
		}
	}
	
	/* Search all objects loaded at program start up. */
	if (def == NULL || ELF_ST_BIND(def->st_info) == STB_WEAK) {
		symp = _rtld_symlook_list(name, hash, &_rtld_list_main, &obj, in_plt);
		if (symp != NULL &&
		    (def == NULL || ELF_ST_BIND(symp->st_info) != STB_WEAK)) {
			def = symp;
			defobj = obj;
		}
	}
	
	/* Search all dlopened DAGs containing the referencing object. */
	for (elm = SIMPLEQ_FIRST(&refobj->dldags); elm; elm = SIMPLEQ_NEXT(elm, link)) {
		if (def != NULL && ELF_ST_BIND(def->st_info) != STB_WEAK)
			break;
		symp = _rtld_symlook_list(name, hash, &elm->obj->dagmembers, &obj,
					  in_plt);
		if (symp != NULL &&
		    (def == NULL || ELF_ST_BIND(symp->st_info) != STB_WEAK)) {
			def = symp;
			defobj = obj;
		}
	}
	
	/* Search all RTLD_GLOBAL objects. */
	if (def == NULL || ELF_ST_BIND(def->st_info) == STB_WEAK) {
		symp = _rtld_symlook_list(name, hash, &_rtld_list_global, &obj, in_plt);
		if (symp != NULL &&
		    (def == NULL || ELF_ST_BIND(symp->st_info) != STB_WEAK)) {
			def = symp;
			defobj = obj;
		}
	}
	
	/*
	 * If we found no definition and the reference is weak, treat the
	 * symbol as having the value zero.
	 */
	if (def == NULL && ELF_ST_BIND(ref->st_info) == STB_WEAK) {
		def = &_rtld_sym_zero;
		defobj = _rtld_objmain;
	}
	
	if (def != NULL)
		*defobj_out = defobj;
	else if (ELF_R_TYPE(r_info) != R_TYPE(NONE)) {
		_rtld_error(
	    "%s: Undefined %ssymbol \"%s\" (reloc type = %d, symnum = %d)",
			    refobj->path, in_plt ? "PLT " : "", name,
			    ELF_R_TYPE(r_info), symnum);
	}
	return def;
}
