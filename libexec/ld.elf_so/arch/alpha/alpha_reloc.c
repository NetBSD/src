/*	$NetBSD: alpha_reloc.c,v 1.10 2002/09/06 03:05:35 mycroft Exp $	*/

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/stat.h>

#include "rtld.h"
#include "debug.h"

#ifdef RTLD_DEBUG_ALPHA
#define	adbg(x)		if (dodebug) xprintf x
#else
#define	adbg(x)		/* nothing */
#endif

void
_rtld_setup_pltgot(const Obj_Entry *obj)
{
	uint32_t word0;

	/*
	 * The PLTGOT on the Alpha looks like this:
	 *
	 *	PLT HEADER
	 *	.
	 *	. 32 bytes
	 *	.
	 *	PLT ENTRY #0
	 *	.
	 *	. 12 bytes
	 *	.
	 *	PLT ENTRY #1
	 *	.
	 *	. 12 bytes
	 *	.
	 *	etc.
	 *
	 * The old-format entries look like (displacements filled in
	 * by the linker):
	 *
	 *	ldah	$28, 0($31)		# 0x279f0000
	 *	lda	$28, 0($28)		# 0x239c0000
	 *	br	$31, plt0		# 0xc3e00000
	 *
	 * The new-format entries look like:
	 *
	 *	br	$28, plt0		# 0xc3800000
	 *					# 0x00000000
	 *					# 0x00000000
	 *
	 * What we do is fetch the first PLT entry and check to
	 * see the first word of it matches the first word of the
	 * old format.  If so, we use a binding routine that can
	 * handle the old format, otherwise we use a binding routine
	 * that handles the new format.
	 *
	 * Note that this is done on a per-object basis, we can mix
	 * and match shared objects build with both the old and new
	 * linker.
	 */
	word0 = *(uint32_t *)(((char *) obj->pltgot) + 32);
	if ((word0 & 0xffff0000) == 0x279f0000) {
		/* Old PLT entry format. */
		adbg(("ALPHA: object %p has old PLT format\n", obj));
		obj->pltgot[2] = (Elf_Addr) &_rtld_bind_start_old;
		obj->pltgot[3] = (Elf_Addr) obj;
	} else {
		/* New PLT entry format. */
		adbg(("ALPHA: object %p has new PLT format\n", obj));
		obj->pltgot[2] = (Elf_Addr) &_rtld_bind_start;
		obj->pltgot[3] = (Elf_Addr) obj;
	}

	__asm __volatile("imb");
}

int
_rtld_relocate_nonplt_objects(obj, dodebug)
	Obj_Entry *obj;
	bool dodebug;
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

		case R_TYPE(REFQUAD):
			def = _rtld_find_symdef(symnum, obj, &defobj, false);
			if (def == NULL)
				return -1;

			tmp = (Elf_Addr)(defobj->relocbase + def->st_value) +
			    *where + rela->r_addend;
			if (*where != tmp)
				*where = tmp;
			rdbg(dodebug, ("REFQUAD %s in %s --> %p in %s",
			    obj->strtab + obj->symtab[symnum].st_name,
			    obj->path, (void *)*where, defobj->path));
			break;

		case R_TYPE(GLOB_DAT):
			def = _rtld_find_symdef(symnum, obj, &defobj, false);
			if (def == NULL)
				return -1;

			tmp = (Elf_Addr)(defobj->relocbase + def->st_value) +
			    rela->r_addend;
			if (*where != tmp)
				*where = tmp;
			rdbg(dodebug, ("GLOB_DAT %s in %s --> %p in %s",
			    obj->strtab + obj->symtab[symnum].st_name,
			    obj->path, (void *)*where, defobj->path));
			break;

		case R_TYPE(RELATIVE):
		    {
			extern Elf_Addr	_GLOBAL_OFFSET_TABLE_[];
			extern Elf_Addr	_GOT_END_[];

			/* This is the ...iffy hueristic. */
			if (!dodebug ||
			    (caddr_t)where < (caddr_t)_GLOBAL_OFFSET_TABLE_ ||
			    (caddr_t)where >= (caddr_t)_GOT_END_) {
				*where += (Elf_Addr)obj->relocbase;
				rdbg(dodebug, ("RELATIVE in %s --> %p",
				    obj->path, (void *)*where));
			} else
				rdbg(dodebug, ("RELATIVE in %s stays at %p",
				    obj->path, (void *)*where));
			break;
		    }

		case R_TYPE(COPY):
			/*
			 * These are deferred until all other relocations have
			 * been done.  All we do here is make sure that the
			 * COPY relocation is not in a shared library.  They
			 * are allowed only in executable files.
			 */
			if (!obj->mainprog) {
				_rtld_error(
			"%s: Unexpected R_COPY relocation in shared library",
				    obj->path);
				return -1;
			}
			rdbg(dodebug, ("COPY (avoid in main)"));
			break;

		default:
			rdbg(dodebug, ("sym = %lu, type = %lu, offset = %p, "
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
_rtld_relocate_plt_lazy(obj, dodebug)
	Obj_Entry *obj;
	bool dodebug;
{
	const Elf_Rela *rela;

	if (obj->mainprog)
		return 0;

	for (rela = obj->pltrela; rela < obj->pltrelalim; rela++) {
		Elf_Addr *where = (Elf_Addr *)(obj->relocbase + rela->r_offset);

		assert(ELF_R_TYPE(rela->r_info) == R_TYPE(JMP_SLOT));

		/* Just relocate the GOT slots pointing into the PLT */
		*where += (Elf_Addr)obj->relocbase;
		rdbg(dodebug, ("fixup !main in %s --> %p", obj->path,
		    (void *)*where));
	}

	return 0;
}

int
_rtld_relocate_plt_object(obj, rela, addrp, dodebug)
	Obj_Entry *obj;
	const Elf_Rela *rela;
	caddr_t *addrp;
	bool dodebug;
{
	Elf_Addr *where = (Elf_Addr *)(obj->relocbase + rela->r_offset);
	Elf_Addr new_value;
	const Elf_Sym  *def;
	const Obj_Entry *defobj;

	assert(ELF_R_TYPE(rela->r_info) == R_TYPE(JMP_SLOT));

	def = _rtld_find_symdef(ELF_R_SYM(rela->r_info), obj, &defobj, true);
	if (def == NULL)
		return -1;

	new_value = (Elf_Addr)(defobj->relocbase + def->st_value);
	rdbg(dodebug, ("bind now/fixup in %s --> old=%p new=%p",
	    defobj->strtab + def->st_name, (void *)*where, (void *)new_value));
	if (*where != new_value)
		*where = new_value;

	*addrp = (caddr_t)new_value;
	return 0;
}
