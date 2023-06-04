/*	$NetBSD: mdreloc.c,v 1.9 2023/06/04 01:24:58 joerg Exp $	*/

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
__RCSID("$NetBSD: mdreloc.c,v 1.9 2023/06/04 01:24:58 joerg Exp $");
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

void
_rtld_setup_pltgot(const Obj_Entry *obj)
{
	obj->pltgot[0] = (Elf_Addr) &_rtld_bind_start;
	obj->pltgot[1] = (Elf_Addr) obj;
}

void
_rtld_relocate_nonplt_self(Elf_Dyn *dynp, Elf_Addr relocbase)
{
	const Elf_Rela *rela = NULL, *relalim;
	Elf_Addr relasz = 0;

	for (; dynp->d_tag != DT_NULL; dynp++) {
		switch (dynp->d_tag) {
		case DT_RELA:
			rela = (const Elf_Rela *)(relocbase + dynp->d_un.d_ptr);
			break;
		case DT_RELASZ:
			relasz = dynp->d_un.d_val;
			break;
		}
	}

	relalim = (const Elf_Rela *)((uintptr_t)rela + relasz);
	for (; rela < relalim; rela++) {
		Elf_Word r_type = ELF_R_TYPE(rela->r_info);
		Elf_Addr *where = (Elf_Addr *)(relocbase + rela->r_offset);

		switch (r_type) {
		case R_TYPE(RELATIVE): {
			Elf_Addr val = relocbase + rela->r_addend;
			*where = val;
			rdbg(("RELATIVE/L(%p) -> %p in <self>",
			    where, (void *)val));
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
	const Elf_Rela *rela;
	const Elf_Sym *def = NULL;
	const Obj_Entry *defobj = NULL;
	unsigned long last_symnum = ULONG_MAX;

	for (rela = obj->rela; rela < obj->relalim; rela++) {
		Elf_Addr * const where =
		    (Elf_Addr *)(obj->relocbase + rela->r_offset);
		const Elf_Word r_type = ELF_R_TYPE(rela->r_info);
		unsigned long symnum;

		switch (r_type) {
		case R_TYPESZ(ADDR):
		case R_TYPESZ(TLS_DTPMOD):
		case R_TYPESZ(TLS_DTPREL):
		case R_TYPESZ(TLS_TPREL):
			symnum = ELF_R_SYM(rela->r_info);
			if (last_symnum != symnum) {
				last_symnum = symnum;
				def = _rtld_find_symdef(symnum, obj, &defobj,
				    false);
				if (def == NULL)
					return -1;
			}
			break;
		default:
			break;
		}

		switch (r_type) {
		case R_TYPE(NONE):
			break;

		case R_TYPE(RELATIVE): {
			const Elf_Addr val = (Elf_Addr)obj->relocbase +
			    rela->r_addend;

			rdbg(("RELATIVE(%p) -> %p (%s) in %s",
			    where, (void *)val,
			    obj->strtab +
				obj->symtab[ELF_R_SYM(rela->r_info)].st_name,
			    obj->path));

			*where = val;
			break;
		    }

		case R_TYPESZ(ADDR): {
			const Elf_Addr val = (Elf_Addr)defobj->relocbase +
			    def->st_value + rela->r_addend;

			rdbg(("ADDR(%p) -> %p (%s) in %s%s",
			    where, (void *)val,
			    obj->strtab +
				obj->symtab[ELF_R_SYM(rela->r_info)].st_name,
			    obj->path,
			    def == &_rtld_sym_zero ? " (symzero)" : ""));

			*where = val;
			break;
		    }

		case R_TYPE(COPY):
			/*
			 * These are deferred until all other relocations have
			 * been done.  All we do here is make sure that the
			 * COPY relocation is not in a shared library.  They
			 * are allowed only in executable files.
			 */
			if (obj->isdynamic) {
				_rtld_error("%s: Unexpected R_COPY relocation"
				    " in shared library", obj->path);
				return -1;
			}
			rdbg(("COPY (avoid in main)"));
			break;

		case R_TYPESZ(TLS_DTPMOD): {
			const Elf_Addr val = (Elf_Addr)defobj->tlsindex;

			rdbg(("TLS_DTPMOD(%p) -> %p (%s) in %s",
			    where, (void *)val,
			    obj->strtab +
				obj->symtab[ELF_R_SYM(rela->r_info)].st_name,
			    obj->path));

			*where = val;
			break;
		    }

		case R_TYPESZ(TLS_DTPREL): {
			const Elf_Addr val = (Elf_Addr)(def->st_value +
			    rela->r_addend - TLS_DTV_OFFSET);

			rdbg(("TLS_DTPREL(%p) -> %p (%s) in %s",
			    where, (void *)val,
			    obj->strtab +
				obj->symtab[ELF_R_SYM(rela->r_info)].st_name,
			    defobj->path));

			*where = val;
			break;
		    }

		case R_TYPESZ(TLS_TPREL):
			if (!defobj->tls_static &&
			    _rtld_tls_offset_allocate(__UNCONST(defobj)))
				return -1;

			*where = (Elf_Addr)(def->st_value + defobj->tlsoffset +
			    rela->r_addend);

			rdbg(("TLS_TPREL %s in %s --> %p in %s",
			    obj->strtab +
				obj->symtab[ELF_R_SYM(rela->r_info)].st_name,
			    obj->path, (void *)*where, defobj->path));
			break;

		default:
			rdbg(("sym = %lu, type = %lu, offset = %p, "
			    "addend = %p, contents = %p",
			    (u_long)ELF_R_SYM(rela->r_info),
			    (u_long)ELF_R_TYPE(rela->r_info),
			    (void *)rela->r_offset, (void *)rela->r_addend,
			    (void *)*where));
			_rtld_error("%s: Unsupported relocation type %ld "
			    "in non-PLT relocations",
			    obj->path, (u_long)r_type);
			return -1;
		}
	}

	return 0;
}

int
_rtld_relocate_plt_lazy(Obj_Entry *obj)
{

	if (!obj->relocbase)
		return 0;

	for (const Elf_Rela *rela = obj->pltrela; rela < obj->pltrelalim; rela++) {
		Elf_Addr *where = (Elf_Addr *)(obj->relocbase + rela->r_offset);
		switch (ELF_R_TYPE(rela->r_info)) {
		case R_TYPE(JMP_SLOT):
			/* Just relocate the GOT slots pointing into the PLT */
			*where += (Elf_Addr)obj->relocbase;
			rdbg(("fixup !main in %s --> %p", obj->path, (void *)*where));
			break;
		default:
			rdbg(("not yet... %d", (int)ELF_R_TYPE(rela->r_info) ));
		}
	}

	return 0;
}

static int
_rtld_relocate_plt_object(const Obj_Entry *obj, const Elf_Rela *rela,
    Elf_Addr *tp)
{
	Elf_Addr * const where = (Elf_Addr *)(obj->relocbase + rela->r_offset);
	const Obj_Entry *defobj;
	Elf_Addr new_value;

        assert(ELF_R_TYPE(rela->r_info) == R_TYPE(JMP_SLOT));

	const Elf_Sym *def = _rtld_find_plt_symdef(ELF_R_SYM(rela->r_info),
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
	rdbg(("bind now/fixup in %s --> old=%p new=%p",
	    defobj->strtab + def->st_name, (void *)*where,
	    (void *)new_value));
	if (*where != new_value)
		*where = new_value;
	if (tp)
		*tp = new_value;

	return 0;
}

void *
_rtld_bind(const Obj_Entry *obj, Elf_Word gotoff)
{
	const Elf_Addr relidx = (gotoff / sizeof(Elf_Addr));
	const Elf_Rela *pltrela = obj->pltrela + relidx;

	Elf_Addr new_value = 0;

	_rtld_shared_enter();
	const int err = _rtld_relocate_plt_object(obj, pltrela, &new_value);
	if (err)
		_rtld_die();
	_rtld_shared_exit();

	return (void *)new_value;
}

int
_rtld_relocate_plt_objects(const Obj_Entry *obj)
{

	for (const Elf_Rela *rela = obj->pltrela; rela < obj->pltrelalim; rela++) {
		if (_rtld_relocate_plt_object(obj, rela, NULL) < 0)
			return -1;
	}

	return 0;
}
