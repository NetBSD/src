/*	$NetBSD: mips_reloc.c,v 1.29 2002/09/13 17:07:12 mycroft Exp $	*/

/*
 * Copyright 1997 Michael L. Hitch <mhitch@montana.edu>
 * Portions copyright 2002 Charles M. Hannum.
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

void _rtld_bind_start(void);
void _rtld_relocate_nonplt_self(Elf_Dyn *, Elf_Addr);
caddr_t _rtld_bind_mips(Elf_Word, Elf_Addr, Elf_Addr, Elf_Addr);

caddr_t
_rtld_bind_mips(a0, a1, a2, a3)
	Elf_Word a0;
	Elf_Addr a1, a2, a3;
{
	Elf_Addr *u = (Elf_Addr *)(a2 - 0x7ff0);
	const Obj_Entry *obj = (Obj_Entry *)(u[1] & 0x7fffffff);
	const Elf_Sym *def;
	const Obj_Entry *defobj;
	Elf_Addr new_value;

	def = _rtld_find_symdef(a0, obj, &defobj, true);
	if (def == NULL)
		return(NULL);	/* XXX */

	new_value = (Elf_Addr)(defobj->relocbase + def->st_value);
	rdbg(("bind now/fixup in %s --> new=%p",
	    defobj->strtab + def->st_name, (void *)new_value));
	u[obj->local_gotno + a0 - obj->gotsym] = new_value;
	return ((caddr_t)new_value);
}

void
_rtld_setup_pltgot(obj)
	const Obj_Entry *obj;
{
	obj->pltgot[0] = (Elf_Addr) &_rtld_bind_start;
	/* XXX only if obj->pltgot[1] & 0x80000000 ?? */
	obj->pltgot[1] |= (Elf_Addr) obj;
}

void
_rtld_relocate_nonplt_self(dynp, relocbase)
	Elf_Dyn *dynp;
	Elf_Addr relocbase;
{
	const Elf_Rel *rel = 0, *rellim;
	Elf_Addr relsz = 0;
	Elf_Addr *where;
	const Elf_Sym *symtab, *sym;
	Elf_Addr *got;
	Elf_Word local_gotno, symtabno, gotsym;
	int i;

	for (; dynp->d_tag != DT_NULL; dynp++) {
		switch (dynp->d_tag) {
		case DT_REL:
			rel = (const Elf_Rel *)(relocbase + dynp->d_un.d_ptr);
			break;
		case DT_RELSZ:
			relsz = dynp->d_un.d_val;
			break;
		case DT_SYMTAB:
			symtab = (const Elf_Sym *)(relocbase + dynp->d_un.d_ptr);
			break;
		case DT_PLTGOT:
			got = (Elf_Addr *)(relocbase + dynp->d_un.d_ptr);
			break;
		case DT_MIPS_LOCAL_GOTNO:
			local_gotno = dynp->d_un.d_val;
			break;
		case DT_MIPS_SYMTABNO:
			symtabno = dynp->d_un.d_val;
			break;
		case DT_MIPS_GOTSYM:
			gotsym = dynp->d_un.d_val;
			break;
		}
	}
	rellim = (const Elf_Rel *)((caddr_t)rel + relsz);
	for (; rel < rellim; rel++) {
		where = (Elf_Addr *)(relocbase + rel->r_offset);

		switch (ELF_R_TYPE(rel->r_info)) {
		case R_TYPE(NONE):
			break;

		case R_TYPE(REL32):
			sym = symtab + ELF_R_SYM(rel->r_info);
			if (ELF_ST_BIND(sym->st_info) == STB_LOCAL &&
			    ELF_ST_TYPE(sym->st_info) == STT_SECTION)
				*where += (Elf_Addr)(sym->st_value + relocbase);
			else
				abort();
			break;

		default:
			abort();
		}
	}

	i = (got[1] & 0x80000000) ? 2 : 1;
	/* Relocate the local GOT entries */
	got += i;
	for (; i < local_gotno; i++)
		*got++ += relocbase;
	sym = symtab + gotsym;
	/* Now do the global GOT entries */
	for (i = gotsym; i < symtabno; i++) {
		*got = sym->st_value + relocbase;
		++sym;
		++got;
	}
}

int
_rtld_relocate_nonplt_objects(obj, self)
	const Obj_Entry *obj;
	bool self;
{
	const Elf_Rel *rel;
	Elf_Addr *got = obj->pltgot;
	const Elf_Sym *sym, *def;
	const Obj_Entry *defobj;
	int i;

	if (self)
		return 0;

	for (rel = obj->rel; rel < obj->rellim; rel++) {
		Elf_Addr        *where;
		unsigned long	 symnum;

		where = (Elf_Addr *)(obj->relocbase + rel->r_offset);
		symnum = ELF_R_SYM(rel->r_info);

		switch (ELF_R_TYPE(rel->r_info)) {
		case R_TYPE(NONE):
			break;

		case R_TYPE(REL32):
			/* 32-bit PC-relative reference */
			def = obj->symtab + symnum;

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
				    *where < def->st_value)
					*where += (Elf_Addr)def->st_value;

				*where += (Elf_Addr)obj->relocbase;

				rdbg(("REL32 %s in %s --> %p in %s",
				    obj->strtab + def->st_name, obj->path,
				    (void *)*where, obj->path));
			} else {
				def = _rtld_find_symdef(symnum, obj, &defobj,
				    false);
				if (def == NULL)
					return -1;
				*where += (Elf_Addr)(defobj->relocbase +
				    def->st_value);
				rdbg(("REL32 %s in %s --> %p in %s",
				    obj->strtab + obj->symtab[symnum].st_name,
				    obj->path, (void *)*where, defobj->path));
			}
			break;

		default:
			rdbg(("sym = %lu, type = %lu, offset = %p, "
			    "contents = %p, symbol = %s",
			    symnum, (u_long)ELF_R_TYPE(rel->r_info),
			    (void *)rel->r_offset, (void *)*where,
			    obj->strtab + obj->symtab[symnum].st_name));
			_rtld_error("%s: Unsupported relocation type %ld "
			    "in non-PLT relocations\n",
			    obj->path, (u_long) ELF_R_TYPE(rel->r_info));
			return -1;
		}
	}

	i = (got[1] & 0x80000000) ? 2 : 1;
	/* Relocate the local GOT entries */
	got += i;
	for (; i < obj->local_gotno; i++)
		*got++ += (Elf_Addr)obj->relocbase;
	sym = obj->symtab + obj->gotsym;
	/* Now do the global GOT entries */
	for (i = obj->gotsym; i < obj->symtabno; i++) {
		rdbg((" doing got %d sym %p (%s, %x)", i - obj->gotsym, sym,
		    sym->st_name + obj->strtab, *got));

		if (ELF_ST_TYPE(sym->st_info) == STT_FUNC &&
		    sym->st_value != 0)
			*got = sym->st_value + (Elf_Addr)obj->relocbase;
		else if (ELF_ST_TYPE(sym->st_info) == STT_SECTION &&
		    ELF_ST_BIND(sym->st_info) == STB_GLOBAL) {
			if (sym->st_shndx == SHN_ABS)
				*got = sym->st_value +
				    (Elf_Addr)obj->relocbase;
			/* else SGI stuff ignored */
		} else {
			def = _rtld_find_symdef(i, obj, &defobj, true);
			if (def == NULL)
				return -1;
			*got = def->st_value + (Elf_Addr)defobj->relocbase;
		}
		++sym;
		++got;
	}

	return 0;
}

int
_rtld_relocate_plt_lazy(obj)
	const Obj_Entry *obj;
{
	return 0;
}
