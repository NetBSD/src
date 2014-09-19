/*	$NetBSD: mdreloc.c,v 1.1 2014/09/19 17:36:25 matt Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: mdreloc.c,v 1.1 2014/09/19 17:36:25 matt Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/endian.h>
#include <sys/tls.h>

#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "rtld.h"

void _rtld_bind_start(void);
void _rtld_relocate_nonplt_self(Elf_Dyn *, Elf_Addr);
void *_rtld_bind(const Obj_Entry *, Elf_Word);

#if ELFSIZE == 64
#define	Elf_Sxword			Elf64_Sxword
#else
#define	Elf_Sxword			Elf32_Sword
#endif

void
_rtld_setup_pltgot(const Obj_Entry *obj)
{
	obj->pltgot[0] = (Elf_Addr) &_rtld_bind_start;
	obj->pltgot[1] = (Elf_Addr) obj;
}

void
_rtld_relocate_nonplt_self(Elf_Dyn *dynp, Elf_Addr relocbase)
{
	const Elf_Rel *rel = 0, *rellim;
	Elf_Addr relsz = 0;
	Elf_Sxword *where;
	const Elf_Sym *symtab = NULL, *sym;
	Elf_Addr *got = NULL;
	Elf_Word local_gotno = 0, symtabno = 0, gotsym = 0;
	size_t i;

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
		case DT_RISCV_LOCAL_GOTNO:
			local_gotno = dynp->d_un.d_val;
			break;
		case DT_RISCV_SYMTABNO:
			symtabno = dynp->d_un.d_val;
			break;
		case DT_RISCV_GOTSYM:
			gotsym = dynp->d_un.d_val;
			break;
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

	rellim = (const Elf_Rel *)((uintptr_t)rel + relsz);
	for (; rel < rellim; rel++) {
		Elf_Word r_symndx, r_type;

		where = (Elf_Sxword *)(relocbase + rel->r_offset);

		r_symndx = ELF_R_SYM(rel->r_info);
		r_type = ELF_R_TYPE(rel->r_info);

		switch (r_type & 0xff) {
		case R_TYPE(REL32): {
			Elf_Sxword old = *where;
			Elf_Sxword val = old;
#if ELFSIZE == 64
			assert(r_type == R_TYPE(REL32)
			    || r_type == (R_TYPE(REL32)|(R_TYPE(64) << 8)));
#endif
			assert(r_symndx < gotsym);
			sym = symtab + r_symndx;
			assert(ELF_ST_BIND(sym->st_info) == STB_LOCAL);
			val += relocbase;
			*(Elf_Sword *)where = val;
			rdbg(("REL32/L(%p) %p -> %p in <self>",
			    where, (void *)old, (void *)val));
			break;
		}

		case R_TYPE(NONE):
			break;

		default:
			abort();
		}
	}
}

int
_rtld_relocate_nonplt_objects(Obj_Entry *obj)
{
	const Elf_Rel *rel;
	Elf_Addr *got = obj->pltgot;
	const Elf_Sym *sym, *def;
	const Obj_Entry *defobj;
	Elf_Word i;

	i = 2;
	/* Relocate the local GOT entries */
	got += i;
	for (; i < obj->local_gotno; i++)
		*got++ += (Elf_Addr)obj->relocbase;

	sym = obj->symtab + obj->gotsym;
	/* Now do the global GOT entries */
	for (i = obj->gotsym; i < obj->symtabno; i++) {
		rdbg((" doing got %d sym %p (%s, %lx)", i - obj->gotsym, sym,
		    sym->st_name + obj->strtab, (u_long) *got));

		if (ELF_ST_TYPE(sym->st_info) == STT_FUNC &&
		    sym->st_value != 0 && sym->st_shndx == SHN_UNDEF) {
			/*
			 * If there are non-PLT references to the function,
			 * st_value should be 0, forcing us to resolve the
			 * address immediately.
			 *
			 * XXX DANGER WILL ROBINSON!
			 * The linker is not outputting PLT slots for calls to
			 * functions that are defined in the same shared
			 * library.  This is a bug, because it can screw up
			 * link ordering rules if the symbol is defined in
			 * more than one module.  For now, if there is a
			 * definition, we fail the test above and force a full
			 * symbol lookup.  This means that all intra-module
			 * calls are bound immediately.  - mycroft, 2003/09/24
			 */
			*got = sym->st_value + (Elf_Addr)obj->relocbase;
		} else if (sym->st_info == ELF_ST_INFO(STB_GLOBAL, STT_SECTION)) {
			/* Symbols with index SHN_ABS are not relocated. */
			if (sym->st_shndx != SHN_ABS)
				*got = sym->st_value +
				    (Elf_Addr)obj->relocbase;
		} else {
			def = _rtld_find_symdef(i, obj, &defobj, false);
			if (def == NULL)
				return -1;
			*got = def->st_value + (Elf_Addr)defobj->relocbase;
		}

		rdbg(("  --> now %lx", (u_long) *got));
		++sym;
		++got;
	}

	got = obj->pltgot;
	for (rel = obj->rel; rel < obj->rellim; rel++) {
		Elf_Addr * const where =
		    (Elf_Addr *)(obj->relocbase + rel->r_offset);
		const Elf_Word r_symndx = ELF_R_SYM(rel->r_info);
		const Elf_Word r_type = ELF_R_TYPE(rel->r_info);

		switch (r_type) {
		case R_TYPE(NONE):
			break;

		case R_TYPE(REL32): {
			/* 32-bit PC-relative reference */
			Elf_Sxword old = *where;
			Elf_Sxword val = old;

			def = obj->symtab + r_symndx;

			if (r_symndx < obj->gotsym) {
				val += (Elf_Addr)obj->relocbase;

				rdbg(("REL32/L(%p) %p -> %p (%s) in %s",
				    where, (void *)old, (void *)val,
				    obj->strtab + def->st_name, obj->path));
			} else {
				val += got[obj->local_gotno + r_symndx - obj->gotsym];
				rdbg(("REL32/G(%p) %p --> %p (%s) in %s",
				    where, (void *)old, (void *)val,
				    obj->strtab + def->st_name,
				    obj->path));
			}
			*where = val;
			break;
		}

#if ELFSIZE == 64
		case R_TYPE(TLS_DTPMOD64):
#else
		case R_TYPE(TLS_DTPMOD32): 
#endif
		{
			Elf_Addr old = *where;
			Elf_Addr val = old;

			def = _rtld_find_symdef(r_symndx, obj, &defobj, false);
			if (def == NULL)
				return -1;

			val += (Elf_Addr)defobj->tlsindex;

			*where = val;
			rdbg(("DTPMOD %s in %s --> %p in %s",
			    obj->strtab + obj->symtab[r_symndx].st_name,
			    obj->path, (void *)old, defobj->path));
			break;
		}

#if ELFSIZE == 64
		case R_TYPE(TLS_DTPREL64):
#else
		case R_TYPE(TLS_DTPREL32):
#endif
		{
			Elf_Addr old = *where;
			Elf_Addr val = old;

			def = _rtld_find_symdef(r_symndx, obj, &defobj, false);
			if (def == NULL)
				return -1;

			if (!defobj->tls_done && _rtld_tls_offset_allocate(obj))
				return -1;

			val += (Elf_Addr)def->st_value - TLS_DTV_OFFSET;
			*(Elf_Word *) where = val;

			rdbg(("DTPREL %s in %s --> %p in %s",
			    obj->strtab + obj->symtab[r_symndx].st_name,
			    obj->path, (void *)old, defobj->path));
			break;
		}

		default:
			rdbg(("sym = %lu, type = %lu, offset = %p, "
			    "contents = %p, symbol = %s",
			    (u_long)r_symndx, (u_long)ELF_R_TYPE(rel->r_info),
			    (void *)rel->r_offset,
			    (void *)load_ptr(where, sizeof(Elf_Sword)),
			    obj->strtab + obj->symtab[r_symndx].st_name));
			_rtld_error("%s: Unsupported relocation type %ld "
			    "in non-PLT relocations",
			    obj->path, (u_long) ELF_R_TYPE(rel->r_info));
			return -1;
		}
	}

	return 0;
}

int
_rtld_relocate_plt_lazy(const Obj_Entry *obj)
{
	/* PLT fixups were done above in the GOT relocation. */
	return 0;
}

static int
_rtld_relocate_plt_object(const Obj_Entry *obj, const Elf_Rel *rel,
    Elf_Addr *tp)
{
	const Obj_Entry *defobj;
	Elf_Addr new_value;

        assert(ELF_R_TYPE(rel->r_info) == R_TYPE(JMP_SLOT));

	const Elf_Sym *def = _rtld_find_plt_symdef(ELF_R_SYM(rel->r_info),
	    obj, &defobj, tp != NULL);
	if (__predict_false(def == NULL))
		return -1;
	if (__predict_false(def == &_rtld_sym_zero))
		return -1;

	if (ELF_ST_TYPE(def->st_info) == STT_GNU_IFUNC) {
		if (tp == NULL)
			return 0;
		new_value = _rtld_resolve_ifunc(defobj, def);
	} else {
		new_value = (Elf_Addr)(defobj->relocbase + def->st_value);
	}
	rdbg(("bind now/fixup in %s --> new=%p",
	    defobj->strtab + def->st_name, (void *)new_value));
	*(Elf_Addr *)(obj->relocbase + rel->r_offset) = new_value;

	if (tp)
		*tp = new_value;
	return 0;
}

void *
_rtld_bind(const Obj_Entry *obj, Elf_Word reloff)
{
	const Elf_Rel *pltrel = (const Elf_Rel *)(obj->pltrel + reloff);
	Elf_Addr new_value;
	int err;

	_rtld_shared_enter();
	err = _rtld_relocate_plt_object(obj, pltrel, &new_value);
	if (err)
		_rtld_die();
	_rtld_shared_exit();

	return (caddr_t)new_value;
}

int
_rtld_relocate_plt_objects(const Obj_Entry *obj)
{
	
	for (const Elf_Rel *rel = obj->pltrel; rel < obj->pltrellim; rel++) {
		if (_rtld_relocate_plt_object(obj, rel, NULL) < 0)
			return -1;
	}

	return 0;
}
