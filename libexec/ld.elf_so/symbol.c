/*	$NetBSD: symbol.c,v 1.3 1999/03/01 16:40:08 christos Exp $	 */

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
			if (symp->st_shndx != Elf_eshn_undefined
#if !defined(__mips__)		/* Following doesn't work on MIPS? mhitch */
			    || (!in_plt && symp->st_value != 0 &&
			    ELF_SYM_TYPE(symp->st_info) == Elf_estt_func)) {
#else
				) {
#endif
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
	const Obj_Entry *refobj;
	const Obj_Entry **defobj_out;
	bool in_plt;
{
	Elf_Word symnum = ELF_R_SYM(r_info);
	const Elf_Sym  *ref;
	const Obj_Entry *obj;
	unsigned long   hash;

	if (name == NULL) {
		ref = refobj->symtab + symnum;
		name = refobj->strtab + ref->st_name;
	}
	hash = _rtld_elf_hash(name);

	if (refobj->symbolic) {	/* Look first in the referencing object */
		const Elf_Sym *def;

		def = _rtld_symlook_obj(name, hash, refobj, in_plt);
		if (def != NULL) {
			*defobj_out = refobj;
			return def;
		}
	}
	/*
         * Look in all loaded objects.  Skip the referencing object, if
         * we have already searched it.
         */
	for (obj = obj_list; obj != NULL; obj = obj->next) {
		if (obj != refobj || !refobj->symbolic) {
			const Elf_Sym *def;

			def = _rtld_symlook_obj(name, hash, obj, in_plt);
			if (def != NULL) {
				*defobj_out = obj;
				return def;
			}
		}
	}

	if (ELF_R_TYPE(r_info) != R_TYPE(NONE)) {
		_rtld_error(
	    "%s: Undefined %ssymbol \"%s\" (reloc type = %d, symnum = %d)",
		    refobj->path, in_plt ? "PLT " : "", name,
		    ELF_R_TYPE(r_info), symnum);
	}
	return NULL;
}
