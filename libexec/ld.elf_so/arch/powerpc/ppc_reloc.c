/*	$NetBSD: ppc_reloc.c,v 1.5 1999/10/25 13:57:12 kleink Exp $	*/

/*-
 * Copyright (C) 1998	Tsubai Masanari
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <machine/cpu.h>

#include "debug.h"
#include "rtld.h"

caddr_t _rtld_bind_powerpc __P((const Obj_Entry *, Elf_Word));
void _rtld_powerpc_pltcall __P((Elf_Word));
void _rtld_powerpc_pltresolve __P((Elf_Word, Elf_Word));

static Elf_Addr _rtld_bind_pltgot __P((const Obj_Entry *, const Elf_RelA *));

#define ha(x) ((((u_int32_t)(x) & 0x8000) ? \
			((u_int32_t)(x) + 0x10000) : (u_int32_t)(x)) >> 16)
#define l(x) ((u_int32_t)(x) & 0xffff)

/*
 * Bind a pltgot slot indexed by reloff.
 */
caddr_t
_rtld_bind_powerpc(obj, reloff)
	const Obj_Entry *obj;
	Elf_Word reloff;
{
	Elf_Addr addr;
	const Elf_RelA *rela;

	if (reloff < 0 || reloff >= 0x8000) {
		dbg(("_rtld_bind_powerpc: broken reloff %x", reloff));
		_rtld_die();
	}

	rela = obj->pltrela + reloff;
	addr = _rtld_bind_pltgot(obj, rela);
	if (addr == 0)
		_rtld_die();

	return (caddr_t)addr;
}

/*
 * Make a pltgot slot.
 * Initial value is "pltresolve" unless bind_now is true.
 */
int
_rtld_reloc_powerpc_plt(
	const Obj_Entry *obj,
	const Elf_RelA *rela,
	bool bind_now)
{
	if (bind_now) {
		if (_rtld_bind_pltgot(obj, rela) == 0)
			return -1;
	} else {
		Elf_Word *where = (Elf_Word *)(obj->relocbase + rela->r_offset);
		char *pltresolve = (void *)&obj->pltgot[8];
		int index = rela - obj->pltrela;
		int distance;

		if (index < 0 || index >= 0x8000)
			return -1;
		distance = pltresolve - (char *)(where + 1);

	        /* li	r11,index */
		/* b	pltresolve   */
		where[0] = 0x39600000 | index;
		where[1] = 0x48000000 | (distance & 0x03fffffc);
		/* __syncicache(where, 8); */
	}
	return 0;
}

/*
 * Bind a pltgot entry to the symbol value.
 */
Elf_Addr
_rtld_bind_pltgot(obj, rela)
	const Obj_Entry *obj;
	const Elf_RelA *rela;
{
	Elf_Word *where = (Elf_Word *)(obj->relocbase + rela->r_offset);
	const Elf_Sym *def;
	const Obj_Entry *defobj;
	Elf_Addr targ_addr;
	int distance;

	assert(ELFDEFNNAME(ST_TYPE)(rela->r_info) == R_TYPE(JMP_SLOT));

	def = _rtld_find_symdef(_rtld_objlist, rela->r_info, NULL, obj,
				&defobj, true);
	dbg(("sym='%s'", def ? defobj->strtab + def->st_name : "??"));
	if (def == NULL) {
		dbg(("symbol not found"));
		return 0;
	}

	targ_addr = (Elf_Addr)(defobj->relocbase + def->st_value);
	distance = targ_addr - (Elf_Addr)where;

	if (abs(distance) < 32*1024*1024) {		/* inside 32MB? */
		/* b	targ_addr	# branch directly */
		*where = 0x48000000 | (distance & 0x03fffffc);
		__syncicache(where, 4);
	} else {
		Elf_Addr *pltcall, *jmptab;
		int N = obj->pltrelalim - obj->pltrela;
		int reloff = rela - obj->pltrela;

		if (reloff < 0 || reloff >= 0x8000)
			return 0;

		pltcall = obj->pltgot;

		jmptab = pltcall + 18 + N * 2;
		jmptab[reloff] = targ_addr;

		distance = (Elf_Addr)pltcall - (Elf_Addr)(where + 1);

		/* li	r11,reloff */
		/* b	pltcall		# use pltcall routine */
		where[0] = 0x39600000 | reloff;
		where[1] = 0x48000000 | (distance & 0x03fffffc);
		__syncicache(where, 8);
	}

	return targ_addr;
}

/*
 * Setup the plt glue routines.
 */
#define PLTCALL_SIZE	20
#define PLTRESOLVE_SIZE	24

void
_rtld_setup_powerpc_plt(obj)
	const Obj_Entry * obj;
{
	Elf_Word *pltcall, *pltresolve;
	Elf_Word *jmptab;
	int N = obj->pltrelalim - obj->pltrela;

	pltcall = obj->pltgot;

	memcpy(pltcall, _rtld_powerpc_pltcall, PLTCALL_SIZE);

	jmptab = pltcall + 18 + N * 2;
	pltcall[1] |= ha(jmptab);
	pltcall[2] |= l(jmptab);

	pltresolve = &pltcall[8];

	memcpy(pltresolve, _rtld_powerpc_pltresolve, PLTRESOLVE_SIZE);
	pltresolve[0] |= ha(_rtld_bind_start);
	pltresolve[1] |= l(_rtld_bind_start);
	pltresolve[3] |= ha(obj);
	pltresolve[4] |= l(obj);

	__syncicache(pltcall, 72 + N * 8);
}
