/*	$NetBSD: hppa_reloc.c,v 1.16 2003/07/24 10:12:27 skrll Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Fredette.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/queue.h>

#include "rtld.h"
#include "debug.h"

#ifdef RTLD_DEBUG_HPPA
#define	hdbg(x)		xprintf x
#else
#define	hdbg(x)		/* nothing */
#endif

void _rtld_bind_start(void);
void __rtld_setup_hppa_pltgot(const Obj_Entry *, Elf_Addr *);

/*
 * In the runtime architecture (ABI), PLABEL function 
 * pointers are distinguished from normal function 
 * pointers by having the next-least-significant bit
 * set.  (This bit is referred to as the L field in
 * HP documentation).  The $$dyncall millicode is
 * aware of this.
 */
#define	RTLD_MAKE_PLABEL(plabel)	(((Elf_Addr)(plabel)) | (1 << 1))
#define RTLD_IS_PLABEL(addr)		(((Elf_Addr)(addr)) & (1 << 1))
#define	RTLD_GET_PLABEL(addr)	((hppa_plabel *) (((Elf_Addr)addr) & ~3))

/*
 * This is the PLABEL structure.  The function PC and
 * shared linkage members must come first, as they are
 * the actual PLABEL.
 */
typedef struct _hppa_plabel {
	Elf_Addr	hppa_plabel_pc;
	Elf_Addr	hppa_plabel_sl;
	SLIST_ENTRY(_hppa_plabel)	hppa_plabel_next;
} hppa_plabel;

/*
 * For now allocated PLABEL structures are tracked on a 
 * singly linked list.  This maybe should be revisited.
 */
static SLIST_HEAD(hppa_plabel_head, _hppa_plabel) hppa_plabel_list
    = SLIST_HEAD_INITIALIZER(hppa_plabel_list);

/*
 * Because I'm hesitant to use NEW while relocating self,
 * this is a small pool of preallocated PLABELs.
 */
#define	HPPA_PLABEL_PRE	(10)
static hppa_plabel hppa_plabel_pre[HPPA_PLABEL_PRE];
static int hppa_plabel_pre_next = 0;

/*
 * The DT_PLTGOT _DYNAMIC entry always gives the linkage table 
 * pointer for an object.  This is often, but not always, the
 * same as the object's value for _GLOBAL_OFFSET_TABLE_.  We
 * cache one object's GOT value, otherwise we look it up.
 * XXX it would be nice to be able to keep this in the Obj_Entry.
 */
static const Obj_Entry *hppa_got_cache_obj = NULL;
static Elf_Addr *hppa_got_cache_got;
#define HPPA_OBJ_SL(obj)	((obj)->pltgot)
#define	HPPA_OBJ_GOT(obj)	((obj) == hppa_got_cache_obj ?		\
				  hppa_got_cache_got :			\
				  _rtld_fill_hppa_got_cache(obj))
static Elf_Addr *_rtld_fill_hppa_got_cache(const Obj_Entry *);

/*
 * This bootstraps the dynamic linker by relocating its GOT.
 * On the hppa, unlike on other architectures, static strings
 * are found through the GOT.  Static strings are essential
 * for RTLD_DEBUG, and I suspect they're used early even when 
 * !defined(RTLD_DEBUG), making relocating the GOT essential.
 *
 * It gets worse.  Relocating the GOT doesn't mean just walking
 * it and adding the relocbase to all of the entries.  You must
 * find and use the GOT relocations, since those RELA relocations 
 * have the necessary addends - the GOT comes initialized as 
 * zeroes.
 */
void
_rtld_bootstrap_hppa_got(Elf_Dyn *dynp, Elf_Addr relocbase,
    Elf_Addr got_begin, Elf_Addr got_end)
{
	const Elf_Rela	*relafirst, *rela, *relalim;
	Elf_Addr        relasz = 0;
	Elf_Addr	where;

	/* 
	 * Process the DYNAMIC section, looking for the non-PLT
	 * relocations.
	 */ 
	relafirst = NULL;
	for (; dynp->d_tag != DT_NULL; ++dynp) {
		switch (dynp->d_tag) {

		case DT_RELA:
			relafirst = (const Elf_Rela *)
			    (relocbase + dynp->d_un.d_ptr);
			break;

		case DT_RELASZ:
			relasz = dynp->d_un.d_val;
			break;
		}
	}
	relalim = (const Elf_Rela *)((caddr_t)relafirst + relasz);

	/* 
	 * Process all relocations that look like they're in 
	 * the GOT.
	 */
	for(rela = relafirst; rela < relalim; rela++) {
		where = (Elf_Addr)(relocbase + rela->r_offset);
		if (where >= got_begin && where < got_end)
			*((Elf_Addr *)where) = relocbase + rela->r_addend;
	}

#if defined(RTLD_DEBUG_HPPA)
	for(rela = relafirst; rela < relalim; rela++) {
		where = (Elf_Addr)(relocbase + rela->r_offset);
		if (where >= got_begin && where < got_end)
			xprintf("GOT rela @%p(%p) -> %p(%p)\n",
			    (void *)rela->r_offset,
			    (void *)where,
			    (void *)rela->r_addend,
			    (void *)*((Elf_Addr *)where));
	}
#endif /* RTLD_DEBUG_HPPA */
}

/*
 * This looks up the object's _GLOBAL_OFFSET_TABLE_
 * and caches the result.
 */
static Elf_Addr *
_rtld_fill_hppa_got_cache(const Obj_Entry *obj)
{
	const char *name = "_GLOBAL_OFFSET_TABLE_";
	unsigned long hash;
	const Elf_Sym *def;

	hash = _rtld_elf_hash(name);
	def = _rtld_symlook_obj(name, hash, obj, true);
	assert(def != NULL);
	hppa_got_cache_obj = obj;
	return hppa_got_cache_got = 
	    (Elf_Addr *)(obj->relocbase + def->st_value);
}

/*
 * This allocates a PLABEL.  If called with a non-NULL def, the 
 * plabel is for the function associated with that definition
 * in the defining object defobj, plus the given addend.  If 
 * called with a NULL def, the plabel is for the function at
 * the (unrelocated) address in addend in the object defobj.
 */
Elf_Addr
_rtld_function_descriptor_alloc(const Obj_Entry *defobj, const Elf_Sym *def,
    Elf_Addr addend)
{
	Elf_Addr	func_pc, func_sl;
	hppa_plabel	*plabel;

	if (def != NULL) {
	
		/*
		 * We assume that symbols of type STT_NOTYPE
		 * are undefined.  Return NULL for these.
		 */
		if (ELF_ST_TYPE(def->st_info) == STT_NOTYPE)
			return (Elf_Addr)NULL;

		/* Otherwise assert that this symbol must be a function. */
		assert(ELF_ST_TYPE(def->st_info) == STT_FUNC);

		func_pc = (Elf_Addr)(defobj->relocbase + def->st_value + 
		    addend);
	} else
		func_pc = (Elf_Addr)(defobj->relocbase + addend);

	/*
	 * Search the existing PLABELs for one matching
	 * this function.  If there is one, return it.
	 */
	func_sl = (Elf_Addr)HPPA_OBJ_SL(defobj);
	SLIST_FOREACH(plabel, &hppa_plabel_list, hppa_plabel_next)
		if (plabel->hppa_plabel_pc == func_pc &&
		    plabel->hppa_plabel_sl == func_sl)
			return RTLD_MAKE_PLABEL(plabel);

	/*
	 * XXX - this assumes that the dynamic linker doesn't
	 * have more than HPPA_PLABEL_PRE PLABEL relocations.
	 * Once we've used up the preallocated set, we start
	 * using NEW to allocate plabels.
	 */
	if (hppa_plabel_pre_next < HPPA_PLABEL_PRE)
		plabel = &hppa_plabel_pre[hppa_plabel_pre_next++];
	else {
		plabel = NEW(hppa_plabel);
		if (plabel == NULL)
			return (Elf_Addr)-1;
	}

	/* Fill the new entry and insert it on the list. */
	plabel->hppa_plabel_pc = func_pc;
	plabel->hppa_plabel_sl = func_sl;
	SLIST_INSERT_HEAD(&hppa_plabel_list, plabel, hppa_plabel_next);

	return RTLD_MAKE_PLABEL(plabel);
}

/*
 * If a pointer is a PLABEL, this unwraps it.
 */
const void *
_rtld_function_descriptor_function(const void *addr)
{
	return (RTLD_IS_PLABEL(addr) ? 
	    (const void *) RTLD_GET_PLABEL(addr)->hppa_plabel_pc :
	    addr);
}

/*
 * This handles an IPLT relocation, with or without a symbol.
 */
int
_rtld_relocate_plt_object(const Obj_Entry *obj, const Elf_Rela *rela, caddr_t *addrp)
{
	Elf_Addr	*where = (Elf_Addr *)(obj->relocbase + rela->r_offset);
	const Elf_Sym	*def;
	const Obj_Entry	*defobj;
	Elf_Addr	func_pc, func_sl;

	assert(ELF_R_TYPE(rela->r_info) == R_TYPE(IPLT));

	/*
	 * If this is an IPLT reloc for a static function,
	 * fully resolve the PLT entry now.
	 */
	if (ELF_R_SYM(rela->r_info) == 0) {
		func_pc = (Elf_Addr)(obj->relocbase + rela->r_addend);
		func_sl = (Elf_Addr)HPPA_OBJ_SL(obj);
	}

	/*
	 * If we must bind now, fully resolve the PLT entry.
	 */
	else {

		/*
		 * Look up the symbol.  While we're relocating self,
		 * _rtld_objlist is NULL, so just pass in self.
		 */
		def = _rtld_find_symdef(ELF_R_SYM(rela->r_info), obj, &defobj,
		    false);
		if (def == NULL)
			return -1;
		func_pc = (Elf_Addr)(defobj->relocbase + def->st_value +
		    rela->r_addend);
		func_sl = (Elf_Addr)HPPA_OBJ_SL(defobj);
	}

	/*
	 * Fill this PLT entry and return.
	 */
	where[0] = func_pc;
	where[1] = func_sl;

	*addrp = (caddr_t)where;
	return 0;
}

/* This sets up an object's GOT. */
void
_rtld_setup_pltgot(const Obj_Entry *obj)
{
	__rtld_setup_hppa_pltgot(obj, HPPA_OBJ_GOT(obj));
}

int
_rtld_relocate_nonplt_objects(const Obj_Entry *obj)
{
	const Elf_Rela *rela;

	for (rela = obj->rela; rela < obj->relalim; rela++) {
		Elf_Addr        *where;
		const Elf_Sym   *def;
		const Obj_Entry *defobj;
		Elf_Addr         tmp;
		unsigned long	 symnum;

		where = (Elf_Addr *)(obj->relocbase + rela->r_offset);
		symnum = ELF_R_SYM(rela->r_info);

		switch (ELF_R_TYPE(rela->r_info)) {
		case R_TYPE(NONE):
			break;

		case R_TYPE(DIR32):
			if (symnum) {
				/*
				 * This is either a DIR32 against a symbol
				 * (def->st_name != 0), or against a local
				 * section (def->st_name == 0).
				 */
				def = obj->symtab + symnum;
				defobj = obj;
				if (def->st_name != 0)
					/*
			 		 * While we're relocating self,
					 * _rtld_objlist is NULL, so we just
					 * pass in self.
					 */
					def = _rtld_find_symdef(symnum, obj,
					    &defobj, false);
				if (def == NULL)
					return -1;

				tmp = (Elf_Addr)(defobj->relocbase +
				    def->st_value + rela->r_addend);

				if (*where != tmp)
					*where = tmp;
				rdbg(("DIR32 %s in %s --> %p in %s",
				    obj->strtab + obj->symtab[symnum].st_name,
				    obj->path, (void *)*where, defobj->path));
			} else {
				extern Elf_Addr	_GLOBAL_OFFSET_TABLE_[];
				extern Elf_Addr	_GOT_END_[];

				tmp = (Elf_Addr)(obj->relocbase +
				    rela->r_addend);

				/* This is the ...iffy hueristic. */
				if (!self ||
				    (caddr_t)where < (caddr_t)_GLOBAL_OFFSET_TABLE_ ||
				    (caddr_t)where >= (caddr_t)_GOT_END_) {
					if (*where != tmp)
						*where = tmp;
					rdbg(("DIR32 in %s --> %p", obj->path,
					    (void *)*where));
				} else
					rdbg(("DIR32 in %s stays at %p",
					    obj->path, (void *)*where));
			}
			break;

		case R_TYPE(PLABEL32):
			if (symnum) {
				/*
		 		 * While we're relocating self, _rtld_objlist
				 * is NULL, so we just pass in self.
				 */
				def = _rtld_find_symdef(symnum, obj, &defobj,
				    false);
				if (def == NULL)
					return -1;

				tmp = _rtld_function_descriptor_alloc(defobj, def,
				    rela->r_addend);
				if (tmp == (Elf_Addr)-1)
					return -1;

				if (*where != tmp)
					*where = tmp;
				rdbg(("PLABEL32 %s in %s --> %p in %s",
				    obj->strtab + obj->symtab[symnum].st_name,
				    obj->path, (void *)*where, defobj->path));
			} else {
				/*
				 * This is a PLABEL for a static function, and
				 * the dynamic linker has both allocated a PLT
				 * entry for this function and told us where it
				 * is.  We can safely use the PLT entry as the
				 * PLABEL because there should be no other
				 * PLABEL reloc referencing this function.
				 * This object should also have an IPLT
				 * relocation to initialize the PLT entry.
				 *
				 * The dynamic linker should also have ensured
				 * that the addend has the
				 * next-least-significant bit set; the
				 * $$dyncall millicode uses this to distinguish
				 * a PLABEL pointer from a plain function
				 * pointer.
				 */
				tmp = (Elf_Addr)(obj->relocbase + rela->r_addend);

				if (*where != tmp)
					*where = tmp;
				rdbg(("PLABEL32 in %s --> %p", obj->path,
				    (void *)*where));
			}
			break;

		case R_TYPE(COPY):
			/*
			 * These are deferred until all other relocations have
			 * been done.  All we do here is make sure that the
			 * COPY relocation is not in a shared library.  They
			 * are allowed only in executable files.
			 */
			if (obj->isdynamic) {
				_rtld_error(
			"%s: Unexpected R_COPY relocation in shared library",
				    obj->path);
				return -1;
			}
			rdbg(("COPY (avoid in main)"));
			break;

		default:
			rdbg(("sym = %lu, type = %lu, offset = %p, "
			    "addend = %p, contents = %p, symbol = %s",
			    symnum, (u_long)ELF_R_TYPE(rela->r_info),
			    (void *)rela->r_offset, (void *)rela->r_addend,
			    (void *)*where,
			    obj->strtab + obj->symtab[symnum].st_name));
			_rtld_error("%s: Unsupported relocation type %ld "
			    "in non-PLT relocations\n",
			    obj->path, (u_long) ELF_R_TYPE(rela->r_info));
			return -1;
		}
	}
	return 0;
}

int
_rtld_relocate_plt_lazy(const Obj_Entry *obj)
{
	const Elf_Rela *rela;

	for (rela = obj->pltrela; rela < obj->pltrelalim; rela++) {
		Elf_Addr *where = (Elf_Addr *)(obj->relocbase + rela->r_offset);
		Elf_Addr func_pc, func_sl;

		assert(ELF_R_TYPE(rela->r_info) == R_TYPE(IPLT));

		/*
		 * If this is an IPLT reloc for a static function,
		 * fully resolve the PLT entry now.
		 */
		if (ELF_R_SYM(rela->r_info) == 0) {
			func_pc = (Elf_Addr)(obj->relocbase + rela->r_addend);
			func_sl = (Elf_Addr)HPPA_OBJ_SL(obj);
		}

		/*
		 * Otherwise set up for lazy binding.
		 */
		else {
			/*
			 * This function pointer points to the PLT
			 * stub added by the linker, and instead of
			 * a shared linkage value, we stash this
			 * relocation's offset.  The PLT stub has
			 * already been set up to transfer to
			 * _rtld_bind_start.
			 */
			func_pc = ((Elf_Addr)HPPA_OBJ_GOT(obj)) - 16;
			func_sl = (Elf_Addr)((caddr_t)rela - (caddr_t)obj->pltrela);
		}

		/*
		 * Fill this PLT entry and return.
		 */
		where[0] = func_pc;
		where[1] = func_sl;
	}
	return 0;
}
