/*	$NetBSD: mips_reloc.c,v 1.9 2002/09/05 20:08:17 mycroft Exp $	*/

/*
 * Copyright 1997 Michael L. Hitch <mhitch@montana.edu>
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
 * 3. The name of the author may not be used to endorse or promote products
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


#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "debug.h"
#include "rtld.h"

/*
 * _rtld_bind_mips(symbol_index, return_address, old_gp, stub_return_addr)
 */

caddr_t
_rtld_bind_mips(a0, a1, a2, a3)
	Elf_Word a0;
	Elf_Addr a1, a2, a3;
{
	Elf_Addr *u = (Elf_Addr *)(a2 - 0x7ff0);
	Obj_Entry *obj = (Obj_Entry *)(u[1] & 0x7fffffff);
	const Elf_Sym *def;
	const Obj_Entry *defobj;

	def = _rtld_find_symdef(ELF_R_INFO(a0, 0), obj, &defobj, true);
	if (def) {
		u[obj->local_gotno + a0 - obj->gotsym] = (Elf_Addr)
		    (def->st_value + defobj->relocbase);
		return((caddr_t)(def->st_value + defobj->relocbase));
	}

	return(NULL);	/* XXX */
}

void
_rtld_setup_pltgot(obj)
	Obj_Entry *obj;
{
	Elf_Addr *got = obj->pltgot;
	const Elf_Sym *sym = obj->symtab;
	const Elf_Sym *def;
	const Obj_Entry *defobj;
	int i;

	i = (got[1] & 0x80000000) ? 2 : 1;
	/* Relocate the local GOT entries */
	while (i < obj->local_gotno)
		got[i++] += (Elf_Word)obj->relocbase;
	got += obj->local_gotno;
	i = obj->symtabno - obj->gotsym;
	sym += obj->gotsym;
	/* Now do the global GOT entries */
	while (i--) {
		rdbg(1, (" doing got %d sym %p (%s, %x)",  
				obj->symtabno - obj->gotsym - i - 1, 
				sym, sym->st_name + obj->strtab, *got));

		def = _rtld_find_symdef(ELF_R_INFO(obj->symtabno - i - 1, 0),
		    obj, &defobj, true);
		if (def == NULL)
			_rtld_error(
	    "%s: Undefined PLT symbol \"%s\" (section type = %ld, symnum = %ld)",
			    obj->path, sym->st_name + obj->strtab,
			    (u_long) ELF_ST_TYPE(sym->st_info), 
			    (u_long) obj->symtabno - i - 1);
		else {

			if (sym->st_shndx == SHN_UNDEF) {
#if 0	/* These don't seem to work? */

				if (ELF_ST_TYPE(sym->st_info) == STT_FUNC) {
					if (sym->st_value)
						*got = sym->st_value +
						    (Elf_Word)obj->relocbase;
					else
						*got = def->st_value +
						    (Elf_Word)defobj->relocbase;
				} else
#endif
					*got = def->st_value +
					    (Elf_Word)defobj->relocbase;
			} else if (sym->st_shndx == SHN_COMMON) {
				*got = def->st_value +
				    (Elf_Word)defobj->relocbase;
			} else if (ELF_ST_TYPE(sym->st_info) == STT_FUNC &&
			    *got != sym->st_value) {
				*got += (Elf_Word)obj->relocbase;
			} else if (ELF_ST_TYPE(sym->st_info) == STT_SECTION &&
			    ELF_ST_BIND(sym->st_info) == STB_GLOBAL) {
				if (sym->st_shndx == SHN_ABS)
					*got = sym->st_value +
					    (Elf_Word)obj->relocbase;
				/* else SGI stuff ignored */
			} else
				*got = def->st_value +
				    (Elf_Word)defobj->relocbase;
		}

		++sym;
		++got;
	}

	obj->pltgot[0] = (Elf_Addr) &_rtld_bind_start;
	/* XXX only if obj->pltgot[1] & 0x80000000 ?? */
	obj->pltgot[1] |= (Elf_Addr) obj;
}

int
_rtld_relocate_nonplt_objects(obj, dodebug)
	Obj_Entry *obj;
	bool dodebug;
{
	const Elf_Rel *rel;

	for (rel = obj->rel; rel < obj->rellim; rel++) {
		Elf_Addr        *where;
		const Elf_Sym   *def;
		const Obj_Entry *defobj;

		where = (Elf_Addr *)(obj->relocbase + rel->r_offset);

		switch (ELF_R_TYPE(rel->r_info)) {
		case R_TYPE(NONE):
			break;

		case R_TYPE(REL32):
			/* 32-bit PC-relative reference */
			def = obj->symtab + ELF_R_SYM(rel->r_info);

			if (ELF_ST_BIND(def->st_info) == STB_LOCAL &&
			  (ELF_ST_TYPE(def->st_info) == STT_SECTION ||
			   ELF_ST_TYPE(def->st_info) == STT_NOTYPE)) {
				/*
				 * XXX: ABI DIFFERENCE!
				 *
				 * Old NetBSD binutils would generate shared
				 * libs with section-relative relocations being
				 * already adjusted for the start address of
				 * the section.
				 *
				 * New binutils, OTOH, generate shared libs
				 * with the same relocations being based at
				 * zero, so we need to add in the start address
				 * of the section.
				 *
				 * We assume that all section-relative relocs
				 * with contents less than the start of the
				 * section need to be adjusted; this should
				 * work with both old and new shlibs.
				 *
				 * --rkb, Oct 6, 2001
				 */
				if (def->st_info == STT_SECTION &&
					    (*where < def->st_value))
				    *where += (Elf_Addr) def->st_value;

				*where += (Elf_Addr)obj->relocbase;

				rdbg(dodebug, ("REL32 %s in %s --> %p in %s",
				    obj->strtab + def->st_name, obj->path,
				    (void *)*where, obj->path));
			} else {
				/* XXX maybe do something re: bootstrapping? */
				def = _rtld_find_symdef(rel->r_info, obj,
				    &defobj, false);
				if (def == NULL)
					return -1;
				*where += (Elf_Addr)(defobj->relocbase +
				    def->st_value);
				rdbg(dodebug, ("REL32 %s in %s --> %p in %s",
				    defobj->strtab + def->st_name, obj->path,
				    (void *)*where, defobj->path));
			}
			break;

		default:
			def = _rtld_find_symdef(rel->r_info, obj, &defobj,
			    true);
			rdbg(dodebug, ("sym = %lu, type = %lu, offset = %p, "
			    "contents = %p, symbol = %s",
			    (u_long)ELF_R_SYM(rel->r_info),
			    (u_long)ELF_R_TYPE(rel->r_info),
			    (void *)rel->r_offset, (void *)*where,
			    def ? defobj->strtab + def->st_name : "??"));
			_rtld_error("%s: Unsupported relocation type %ld "
			    "in non-PLT relocations\n",
			    obj->path, (u_long) ELF_R_TYPE(rel->r_info));
			return -1;
		}
	}
	return 0;
}

int
_rtld_relocate_plt_objects(obj, rela, addrp, bind_now, dodebug)
	Obj_Entry *obj;
	const Elf_Rela *rela;
	caddr_t *addrp;
	bool bind_now;
	bool dodebug;
{
	Elf_Addr *where = (Elf_Addr *)(obj->relocbase + rela->r_offset);
	Elf_Addr new_value;

	/* Fully resolve procedure addresses now */

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
	if (addrp != NULL) {
		*addrp = *(caddr_t *)(obj->relocbase + rela->r_offset);
#if defined(__vax__)
		*addrp -= rela->r_addend;
#endif
	}
	return 0;
}
