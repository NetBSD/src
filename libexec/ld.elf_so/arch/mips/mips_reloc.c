/*	$NetBSD: mips_reloc.c,v 1.1.6.1 1999/12/27 18:30:15 wrstuden Exp $	*/

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
 * Relocate a MIPS GOT
 */

void
_rtld_relocate_mips_got(obj)
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
		def = _rtld_find_symdef(_rtld_objlist, 0,
		    sym->st_name + obj->strtab, obj, &defobj, true);
		if (def != NULL) {
			if (sym->st_shndx == SHN_UNDEF) {
#if 0	/* These don't seem to work? */

				if (ELFDEFNNAME(ST_TYPE)(sym->st_info) ==
				    STT_FUNC) {
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
			} else if (ELFDEFNNAME(ST_TYPE)(sym->st_info) ==
			    STT_FUNC &&
			    *got != sym->st_value) {
				*got += (Elf_Word)obj->relocbase;
			} else if (ELFDEFNNAME(ST_TYPE)(sym->st_info) ==
			    STT_SECTION && ELFDEFNNAME(ST_BIND)(sym->st_info) ==
			    STB_GLOBAL) {
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
}

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

	def = _rtld_find_symdef(_rtld_objlist, a0 << 8, NULL, obj, &defobj,
	    true);
	if (def) {
		u[obj->local_gotno + a0 - obj->gotsym] = (Elf_Addr)
		    (def->st_value + defobj->relocbase);
		return((caddr_t)(def->st_value + defobj->relocbase));
	}

	return(NULL);	/* XXX */
}
