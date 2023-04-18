/*	$NetBSD: symbol.c,v 1.75 2023/04/18 22:42:52 christos Exp $	 */

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

/*
 * Dynamic linker for ELF.
 *
 * John Polstra <jdp@polstra.com>.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: symbol.c,v 1.75 2023/04/18 22:42:52 christos Exp $");
#endif /* not lint */

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
#include <sys/bitops.h>
#include <dirent.h>

#include "debug.h"
#include "rtld.h"

/*
 * If the given object is already in the donelist, return true.  Otherwise
 * add the object to the list and return false.
 */
static bool
_rtld_donelist_check(DoneList *dlp, const Obj_Entry *obj)
{
	unsigned int i;

	for (i = 0;  i < dlp->num_used;  i++)
		if (dlp->objs[i] == obj)
			return true;
	/*
	 * Our donelist allocation may not always be sufficient as we're not
	 * thread safe. We'll handle it properly anyway.
	 */
	if (dlp->num_used < dlp->num_alloc)
		dlp->objs[dlp->num_used++] = obj;
	return false;
}

/*
 * SysV hash function for symbol table lookup.  It is a slightly optimized
 * version of the hash specified by the System V ABI.
 */
Elf32_Word
_rtld_sysv_hash(const char *name)
{
	const unsigned char *p = (const unsigned char *) name;
	Elf32_Word h = 0;

	while (__predict_true(*p != '\0')) {
		h = (h << 4) + *p++;
		h ^= (h >> 24) & 0xf0;
	}
	return (h & 0x0fffffff);
}

/*
 * Hash function for symbol table lookup.  Don't even think about changing
 * this.  It is specified by the GNU toolchain ABI.
 */
Elf32_Word
_rtld_gnu_hash(const char *name)
{
	const unsigned char *p = (const unsigned char *) name;
	uint_fast32_t h = 5381;
	unsigned char c;

	for (c = *p; c != '\0'; c = *++p)
		h = h * 33 + c;
	return (h & 0xffffffff);
}

const Elf_Sym *
_rtld_symlook_list(const char *name, Elf_Hash *hash, const Objlist *objlist,
    const Obj_Entry **defobj_out, u_int flags, const Ver_Entry *ventry,
    DoneList *dlp)
{
	const Elf_Sym *symp;
	const Elf_Sym *def;
	const Obj_Entry *defobj;
	const Objlist_Entry *elm;

	def = NULL;
	defobj = NULL;
	SIMPLEQ_FOREACH(elm, objlist, link) {
		if (_rtld_donelist_check(dlp, elm->obj))
			continue;
		rdbg(("search object %p (%s) for %s", elm->obj, elm->obj->path,
		    name));
		symp = _rtld_symlook_obj(name, hash, elm->obj, flags, ventry);
		if (symp != NULL) {
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
 * Search the symbol table of a shared object and all objects needed by it for
 * a symbol of the given name. Search order is breadth-first. Returns a pointer
 * to the symbol, or NULL if no definition was found.
 */
const Elf_Sym *
_rtld_symlook_needed(const char *name, Elf_Hash *hash,
    const Needed_Entry *needed, const Obj_Entry **defobj_out, u_int flags,
    const Ver_Entry *ventry, DoneList *breadth, DoneList *depth)
{
	const Elf_Sym *def, *def_w;
	const Needed_Entry *n;
	const Obj_Entry *obj, *defobj, *defobj1;

	def = def_w = NULL;
	defobj = NULL;
	for (n = needed; n != NULL; n = n->next) {
		if ((obj = n->obj) == NULL)
			continue;
		if (_rtld_donelist_check(breadth, obj))
			continue;
		def = _rtld_symlook_obj(name, hash, obj, flags, ventry);
		if (def == NULL)
			continue;
		defobj = obj;
		if (ELF_ST_BIND(def->st_info) != STB_WEAK) {
			*defobj_out = defobj;

			return (def);
		}
	}
	/*
	 * Either the symbol definition has not been found in directly needed
	 * objects, or the found symbol is weak.
	 */
	for (n = needed; n != NULL; n = n->next) {
		if ((obj = n->obj) == NULL)
			continue;
		if (_rtld_donelist_check(depth, obj))
			continue;
		def_w = _rtld_symlook_needed(name, hash, obj->needed, &defobj1,
		    flags, ventry, breadth, depth);
		if (def_w == NULL)
			continue;
		if (def == NULL || ELF_ST_BIND(def_w->st_info) != STB_WEAK) {
			def = def_w;
			defobj = defobj1;
			if (ELF_ST_BIND(def_w->st_info) != STB_WEAK)
				break;
		}
	}
	if (def != NULL)
		*defobj_out = defobj;

	return def;
}

static bool
_rtld_symlook_obj_matched_symbol(const char *name,
    const Obj_Entry *obj, u_int flags, const Ver_Entry *ventry,
    unsigned long symnum, const Elf_Sym **vsymp, int *vcount)
{
	const Elf_Sym  *symp;
	const char     *strp;
	Elf_Half verndx;

	symp = obj->symtab + symnum;
	strp = obj->strtab + symp->st_name;
	rdbg(("check \"%s\" vs \"%s\" in %s", name, strp, obj->path));
	if (name[1] != strp[1] || strcmp(name, strp))
		return false;
#if defined(__mips__) || defined(__vax__)
	if (symp->st_shndx == SHN_UNDEF)
		return false;
#else
	/*
	 * XXX DANGER WILL ROBINSON!
	 * If we have a function pointer in the executable's
	 * data section, it points to the executable's PLT
	 * slot, and there is NO relocation emitted.  To make
	 * the function pointer comparable to function pointers
	 * in shared libraries, we must resolve data references
	 * in the libraries to point to PLT slots in the
	 * executable, if they exist.
	 */
	if (symp->st_shndx == SHN_UNDEF &&
	    ((flags & SYMLOOK_IN_PLT) ||
	    symp->st_value == 0 ||
	    ELF_ST_TYPE(symp->st_info) != STT_FUNC))
		return false;
#endif

	if (ventry == NULL) {
		if (obj->versyms != NULL) {
			verndx = VER_NDX(obj->versyms[symnum].vs_vers);
			if (verndx > obj->vertabnum) {
				_rtld_error("%s: symbol %s references "
				    "wrong version %d", obj->path,
				    &obj->strtab[symnum], verndx);
				return false;
			}

			/*
			 * If we are not called from dlsym (i.e. this
			 * is a normal relocation from unversioned
			 * binary), accept the symbol immediately
			 * if it happens to have first version after
			 * this shared object became versioned.
			 * Otherwise, if symbol is versioned and not
			 * hidden, remember it. If it is the only
			 * symbol with this name exported by the shared
			 * object, it will be returned as a match at the
			 * end of the function. If symbol is global
			 * (verndx < 2) accept it unconditionally.
			 */
			if (!(flags & SYMLOOK_DLSYM) &&
			    verndx == VER_NDX_GIVEN) {
				*vsymp = symp;
				return true;
			} else if (verndx >= VER_NDX_GIVEN) {
				if (!(obj->versyms[symnum].vs_vers & VER_NDX_HIDDEN)) {
					if (*vsymp == NULL)
						*vsymp = symp;
					(*vcount)++;
				}
				return false;
			}
		}
		*vsymp = symp;
		return true;
	} else {
		if (obj->versyms == NULL) {
			if (_rtld_object_match_name(obj, ventry->name)){
				_rtld_error("%s: object %s should "
				    "provide version %s for symbol %s",
				    _rtld_objself.path, obj->path,
				    ventry->name, &obj->strtab[symnum]);
				return false;
			}
		} else {
			verndx = VER_NDX(obj->versyms[symnum].vs_vers);
			if (verndx > obj->vertabnum) {
				_rtld_error("%s: symbol %s references "
				    "wrong version %d", obj->path,
				    &obj->strtab[symnum], verndx);
				return false;
			}
			if (obj->vertab[verndx].hash != ventry->hash ||
			    strcmp(obj->vertab[verndx].name, ventry->name)) {
				/*
				* Version does not match. Look if this
				* is a global symbol and if it is not
				* hidden. If global symbol (verndx < 2)
				* is available, use it. Do not return
				* symbol if we are called by dlvsym,
				* because dlvsym looks for a specific
				* version and default one is not what
				* dlvsym wants.
				*/
				if ((flags & SYMLOOK_DLSYM) ||
				    (obj->versyms[symnum].vs_vers & VER_NDX_HIDDEN) ||
				    (verndx >= VER_NDX_GIVEN))
					return false;
			}
		}
		*vsymp = symp;
		return true;
	}
}

/*
 * Search the symbol table of a single shared object for a symbol of
 * the given name.  Returns a pointer to the symbol, or NULL if no
 * definition was found.
 *
 * SysV Hash version.
 */
static const Elf_Sym *
_rtld_symlook_obj_sysv(const char *name, unsigned long hash,
    const Obj_Entry *obj, u_int flags, const Ver_Entry *ventry)
{
	unsigned long symnum;
	const Elf_Sym *vsymp = NULL;
	int vcount = 0;

	for (symnum = obj->buckets[fast_remainder32(hash, obj->nbuckets,
	     obj->nbuckets_m, obj->nbuckets_s1, obj->nbuckets_s2)];
	     symnum != ELF_SYM_UNDEFINED;
	     symnum = obj->chains[symnum]) {
		assert(symnum < obj->nchains);

		if (_rtld_symlook_obj_matched_symbol(name, obj, flags,
		    ventry, symnum, &vsymp, &vcount)) {
			return vsymp;
		}
	}
	if (vcount == 1)
		return vsymp;
	return NULL;
}

/*
 * Search the symbol table of a single shared object for a symbol of
 * the given name.  Returns a pointer to the symbol, or NULL if no
 * definition was found.
 *
 * GNU Hash version.
 */
static const Elf_Sym *
_rtld_symlook_obj_gnu(const char *name, unsigned long hash,
    const Obj_Entry *obj, u_int flags, const Ver_Entry *ventry)
{
	unsigned long symnum;
	const Elf_Sym *vsymp = NULL;
	const Elf32_Word *hashval;
	Elf_Addr bloom_word;
	Elf32_Word bucket;
	int vcount = 0;
	unsigned int h1, h2;

	/* Pick right bitmask word from Bloom filter array */
	bloom_word = obj->bloom_gnu[(hash / ELFSIZE) & obj->mask_bm_gnu];

	/* Calculate modulus word size of gnu hash and its derivative */
	h1 = hash & (ELFSIZE - 1);
	h2 = ((hash >> obj->shift2_gnu) & (ELFSIZE - 1));

	/* Filter out the "definitely not in set" queries */
	if (((bloom_word >> h1) & (bloom_word >> h2) & 1) == 0)
		return NULL;

	/* Locate hash chain and corresponding value element*/
	bucket = obj->buckets_gnu[fast_remainder32(hash, obj->nbuckets_gnu,
	     obj->nbuckets_m_gnu, obj->nbuckets_s1_gnu, obj->nbuckets_s2_gnu)];
	if (bucket == 0)
		return NULL;

	hashval = &obj->chains_gnu[bucket];
	do {
		if (((*hashval ^ hash) >> 1) == 0) {
			symnum = hashval - obj->chains_gnu;

			if (_rtld_symlook_obj_matched_symbol(name, obj, flags,
			    ventry, symnum, &vsymp, &vcount)) {
				return vsymp;
			}
		}
	} while ((*hashval++ & 1) == 0);
	if (vcount == 1)
		return vsymp;
	return NULL;
}

/*
 * Search the symbol table of a single shared object for a symbol of
 * the given name.  Returns a pointer to the symbol, or NULL if no
 * definition was found.
 *
 * The symbol's hash value is passed in for efficiency reasons; that
 * eliminates many recomputations of the hash value.
 *
 * Redirect to either GNU Hash (whenever available) or ELF Hash.
 */
const Elf_Sym *
_rtld_symlook_obj(const char *name, Elf_Hash *hash,
    const Obj_Entry *obj, u_int flags, const Ver_Entry *ventry)
{

	assert(obj->sysv_hash || obj->gnu_hash);

	/* Always prefer the GNU Hash as it is faster. */
	if (obj->gnu_hash)
		return _rtld_symlook_obj_gnu(name, hash->gnu, obj, flags, ventry);
	else
		return _rtld_symlook_obj_sysv(name, hash->sysv, obj, flags, ventry);
}

/*
 * Given a symbol number in a referencing object, find the corresponding
 * definition of the symbol.  Returns a pointer to the symbol, or NULL if
 * no definition was found.  Returns a pointer to the Obj_Entry of the
 * defining object via the reference parameter DEFOBJ_OUT.
 */
const Elf_Sym *
_rtld_find_symdef(unsigned long symnum, const Obj_Entry *refobj,
    const Obj_Entry **defobj_out, u_int flags)
{
	const Elf_Sym  *ref;
	const Elf_Sym  *def;
	const Obj_Entry *defobj;
	const char     *name;
	Elf_Hash        hash;

	ref = refobj->symtab + symnum;
	name = refobj->strtab + ref->st_name;

	/*
	 * We don't have to do a full scale lookup if the symbol is local.
	 * We know it will bind to the instance in this load module; to
	 * which we already have a pointer (ie ref).
	 */
	if (ELF_ST_BIND(ref->st_info) != STB_LOCAL) {
		if (ELF_ST_TYPE(ref->st_info) == STT_SECTION) {
			_rtld_error("%s: Bogus symbol table entry %lu",
			    refobj->path, symnum);
        	}

		hash.sysv = _rtld_sysv_hash(name);
		hash.gnu = _rtld_gnu_hash(name);
		defobj = NULL;
		def = _rtld_symlook_default(name, &hash, refobj, &defobj, flags,
		    _rtld_fetch_ventry(refobj, symnum));
	} else {
		rdbg(("STB_LOCAL symbol %s in %s", name, refobj->path));
		def = ref;
		defobj = refobj;
	}

	/*
	 * If we found no definition and the reference is weak, treat the
	 * symbol as having the value zero.
	 */
	if (def == NULL && ELF_ST_BIND(ref->st_info) == STB_WEAK) {
		rdbg(("  returning _rtld_sym_zero@_rtld_objself"));
		def = &_rtld_sym_zero;
		defobj = &_rtld_objself;
	}

	if (def != NULL) {
		*defobj_out = defobj;
	} else {
		rdbg(("lookup failed"));
		_rtld_error("%s: Undefined %ssymbol \"%s\" (symnum = %ld)",
		    refobj->path, (flags & SYMLOOK_IN_PLT) ? "PLT " : "",
		    name, symnum);
	}
	return def;
}

const Elf_Sym *
_rtld_find_plt_symdef(unsigned long symnum, const Obj_Entry *obj,
    const Obj_Entry **defobj, bool imm)
{
 	const Elf_Sym  *def = _rtld_find_symdef(symnum, obj, defobj,
	    SYMLOOK_IN_PLT);
	if (__predict_false(def == NULL))
 		return NULL;

	if (__predict_false(def == &_rtld_sym_zero)) {
		/* tp is set during lazy binding. */
		if (imm) {
			const Elf_Sym	*ref = obj->symtab + symnum;
			const char	*name = obj->strtab + ref->st_name;

			_rtld_error(
			    "%s: Trying to call undefined weak symbol `%s'",
			    obj->path, name);
			return NULL;
		}
	}
	return def;
}

/*
 * Given a symbol name in a referencing object, find the corresponding
 * definition of the symbol.  Returns a pointer to the symbol, or NULL if
 * no definition was found.  Returns a pointer to the Obj_Entry of the
 * defining object via the reference parameter DEFOBJ_OUT.
 */
const Elf_Sym *
_rtld_symlook_default(const char *name, Elf_Hash *hash,
    const Obj_Entry *refobj, const Obj_Entry **defobj_out, u_int flags,
    const Ver_Entry *ventry)
{
	const Elf_Sym *def;
	const Elf_Sym *symp;
	const Obj_Entry *obj;
	const Obj_Entry *defobj;
	const Objlist_Entry *elm;
	def = NULL;
	defobj = NULL;
	DoneList donelist;

	_rtld_donelist_init(&donelist);

	/* Look first in the referencing object if linked symbolically. */
	if (refobj->symbolic && !_rtld_donelist_check(&donelist, refobj)) {
		rdbg(("search referencing object for %s", name));
		symp = _rtld_symlook_obj(name, hash, refobj, flags, ventry);
		if (symp != NULL) {
			def = symp;
			defobj = refobj;
		}
	}

	/* Search all objects loaded at program start up. */
	if (def == NULL || ELF_ST_BIND(def->st_info) == STB_WEAK) {
		rdbg(("search _rtld_list_main for %s", name));
		symp = _rtld_symlook_list(name, hash, &_rtld_list_main, &obj,
		    flags, ventry, &donelist);
		if (symp != NULL &&
		    (def == NULL || ELF_ST_BIND(symp->st_info) != STB_WEAK)) {
			def = symp;
			defobj = obj;
		}
	}

	/* Search all RTLD_GLOBAL objects. */
	if (def == NULL || ELF_ST_BIND(def->st_info) == STB_WEAK) {
		rdbg(("search _rtld_list_global for %s", name));
		symp = _rtld_symlook_list(name, hash, &_rtld_list_global,
		    &obj, flags, ventry, &donelist);
		if (symp != NULL &&
		    (def == NULL || ELF_ST_BIND(symp->st_info) != STB_WEAK)) {
			def = symp;
			defobj = obj;
		}
	}

	/* Search all dlopened DAGs containing the referencing object. */
	SIMPLEQ_FOREACH(elm, &refobj->dldags, link) {
		if (def != NULL && ELF_ST_BIND(def->st_info) != STB_WEAK)
			break;
		rdbg(("search DAG with root %p (%s) for %s", elm->obj,
		    elm->obj->path, name));
		symp = _rtld_symlook_list(name, hash, &elm->obj->dagmembers,
		    &obj, flags, ventry, &donelist);
		if (symp != NULL &&
		    (def == NULL || ELF_ST_BIND(symp->st_info) != STB_WEAK)) {
			def = symp;
			defobj = obj;
		}
	}

	/*
	 * Finally, look in the referencing object if not linked symbolically.
	 * This is necessary for DF_1_NODELETE objects where the containing DAG
	 * has been unlinked, so local references are resolved properly.
	 */
	if ((def == NULL || ELF_ST_BIND(def->st_info) == STB_WEAK) &&
	    !refobj->symbolic && !_rtld_donelist_check(&donelist, refobj)) {
		rdbg(("search referencing object for %s", name));
		symp = _rtld_symlook_obj(name, hash, refobj, flags, ventry);
		if (symp != NULL) {
			def = symp;
			defobj = refobj;
		}
	}

	/*
	 * Search the dynamic linker itself, and possibly resolve the
	 * symbol from there.  This is how the application links to
	 * dynamic linker services such as dlopen.
	 */
	if (def == NULL || ELF_ST_BIND(def->st_info) == STB_WEAK) {
		rdbg(("Search the dynamic linker itself."));
		symp = _rtld_symlook_obj(name, hash, &_rtld_objself, flags,
		    ventry);
		if (symp != NULL) {
			def = symp;
			defobj = &_rtld_objself;
		}
	}

	if (def != NULL)
		*defobj_out = defobj;
	return def;
}
