/*	$NetBSD: mips_reloc.c,v 1.41 2003/09/24 09:55:35 mycroft Exp $	*/

/*
 * Copyright 1997 Michael L. Hitch <mhitch@montana.edu>
 * Portions copyright 2002 Charles M. Hannum <root@ihack.net>
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

#include <sys/types.h>
#include <sys/stat.h>

#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "rtld.h"

#define SUPPORT_OLD_BROKEN_LD

void _rtld_bind_start(void);
void _rtld_relocate_nonplt_self(Elf_Dyn *, Elf_Addr);
caddr_t _rtld_bind(Elf_Word, Elf_Addr, Elf_Addr, Elf_Addr);

void
_rtld_setup_pltgot(const Obj_Entry *obj)
{
	obj->pltgot[0] = (Elf_Addr) &_rtld_bind_start;
	/* XXX only if obj->pltgot[1] & 0x80000000 ?? */
	obj->pltgot[1] |= (Elf_Addr) obj;
}

void
_rtld_relocate_nonplt_self(Elf_Dyn *dynp, Elf_Addr relocbase)
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

	rellim = (const Elf_Rel *)((caddr_t)rel + relsz);
	for (; rel < rellim; rel++) {
		where = (Elf_Addr *)(relocbase + rel->r_offset);

		switch (ELF_R_TYPE(rel->r_info)) {
		case R_TYPE(NONE):
			break;

		case R_TYPE(REL32):
			assert(ELF_R_SYM(rel->r_info) < gotsym);
			sym = symtab + ELF_R_SYM(rel->r_info);
			assert(sym->st_info ==
			    ELF_ST_INFO(STB_LOCAL, STT_SECTION));
			*where += (Elf_Addr)(sym->st_value + relocbase);
			break;

		default:
			abort();
		}
	}
}

/*
 * It is possible for the compiler to emit relocations for unaligned data.
 * We handle this situation with these inlines.
 */
#define	RELOC_ALIGNED_P(x) \
	(((uintptr_t)(x) & (sizeof(void *) - 1)) == 0)

static __inline Elf_Addr
load_ptr(void *where)
{
	Elf_Addr res;

	memcpy(&res, where, sizeof(res));

	return (res);
}

static __inline void
store_ptr(void *where, Elf_Addr val)
{

	memcpy(where, &val, sizeof(val));
}

int
_rtld_relocate_nonplt_objects(const Obj_Entry *obj)
{
	const Elf_Rel *rel;
	Elf_Addr *got = obj->pltgot;
	const Elf_Sym *sym, *def;
	const Obj_Entry *defobj;
	int i;
#ifdef SUPPORT_OLD_BROKEN_LD
	int broken;
#endif

#ifdef SUPPORT_OLD_BROKEN_LD
	broken = 0;
	sym = obj->symtab;
	for (i = 1; i < 12; i++)
		if (sym[i].st_info == ELF_ST_INFO(STB_LOCAL, STT_NOTYPE))
			broken = 1;
	dbg(("%s: broken=%d", obj->path, broken));
#endif

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

#ifdef SUPPORT_OLD_BROKEN_LD
		if (ELF_ST_TYPE(sym->st_info) == STT_FUNC &&
		    broken && sym->st_shndx == SHN_UNDEF) {
			/*
			 * XXX DANGER WILL ROBINSON!
			 * You might think this is stupid, as it intentionally
			 * defeats lazy binding -- and you'd be right.
			 * Unfortunately, for lazy binding to work right, we
			 * need to a way to force the GOT slots used for
			 * function pointers to be resolved immediately.  This
			 * is supposed to be done automatically by the linker,
			 * by not outputting a PLT slot and setting st_value
			 * to 0 if there are non-PLT references, but older
			 * versions of GNU ld do not do this.
			 */
			def = _rtld_find_symdef(i, obj, &defobj, false);
			if (def == NULL)
				return -1;
			*got = def->st_value + (Elf_Addr)defobj->relocbase;
		} else
#endif
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
			 * library.  This is a bug.  For now, if there is a
			 * definition, we fail the test above and do not do
			 * lazy binding for this GOT slot.  Unfortunately, a
			 * similar problem also seems to happen with references
			 * to data items, and we can't fix that here.
			 * - mycroft, 2003/09/24
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

		rdbg(("  --> now %x", *got));
		++sym;
		++got;
	}

	got = obj->pltgot;
	for (rel = obj->rel; rel < obj->rellim; rel++) {
		Elf_Addr        *where, tmp;
		unsigned long	 symnum;

		where = (Elf_Addr *)(obj->relocbase + rel->r_offset);
		symnum = ELF_R_SYM(rel->r_info);

		switch (ELF_R_TYPE(rel->r_info)) {
		case R_TYPE(NONE):
			break;

		case R_TYPE(REL32):
			/* 32-bit PC-relative reference */
			def = obj->symtab + symnum;

			if (symnum >= obj->gotsym) {
				tmp = *where;
				tmp += got[obj->local_gotno + symnum - obj->gotsym];
				*where = tmp;

				rdbg(("REL32/G %s in %s --> %p in %s",
				    obj->strtab + def->st_name, obj->path,
				    (void *)tmp, obj->path));
				break;
			} else {
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
				 * --rkb, Oct 6, 2001
				 */
				tmp = *where;

				if (def->st_info ==
				    ELF_ST_INFO(STB_LOCAL, STT_SECTION)
#ifdef SUPPORT_OLD_BROKEN_LD
				    && !broken
#endif
				    )
					tmp += (Elf_Addr)def->st_value;

				tmp += (Elf_Addr)obj->relocbase;
				*where = tmp;

				rdbg(("REL32/L %s in %s --> %p in %s",
				    obj->strtab + def->st_name, obj->path,
				    (void *)tmp, obj->path));
			}
			break;

		default:
			rdbg(("sym = %lu, type = %lu, offset = %p, "
			    "contents = %p, symbol = %s",
			    symnum, (u_long)ELF_R_TYPE(rel->r_info),
			    (void *)rel->r_offset, (void *)load_ptr(where),
			    obj->strtab + obj->symtab[symnum].st_name));
			_rtld_error("%s: Unsupported relocation type %ld "
			    "in non-PLT relocations\n",
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

caddr_t
_rtld_bind(Elf_Word a0, Elf_Addr a1, Elf_Addr a2, Elf_Addr a3)
{
	Elf_Addr *got = (Elf_Addr *)(a2 - 0x7ff0);
	const Obj_Entry *obj = (Obj_Entry *)(got[1] & 0x7fffffff);
	const Elf_Sym *def;
	const Obj_Entry *defobj;
	Elf_Addr new_value;

	def = _rtld_find_symdef(a0, obj, &defobj, true);
	if (def == NULL)
		_rtld_die();

	new_value = (Elf_Addr)(defobj->relocbase + def->st_value);
	rdbg(("bind now/fixup in %s --> new=%p",
	    defobj->strtab + def->st_name, (void *)new_value));
	got[obj->local_gotno + a0 - obj->gotsym] = new_value;
	return ((caddr_t)new_value);
}
