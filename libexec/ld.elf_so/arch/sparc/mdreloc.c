/*	$NetBSD: mdreloc.c,v 1.8.6.1 1999/12/27 18:30:16 wrstuden Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "rtldenv.h"
#include "debug.h"
#include "rtld.h"

/*
 * The following table holds for each relocation type:
 *	- the width in bits of the memory location the relocation
 *	  applies to (not currently used)
 *	- the number of bits the relocation value must be shifted to the
 *	  right (i.e. discard least significant bits) to fit into
 *	  the appropriate field in the instruction word.
 *	- flags indicating whether
 *		* the relocation involves a symbol
 *		* the relocation is relative to the current position
 *		* the relocation is for a GOT entry
 *		* the relocation is relative to the load address
 *
 */
#define _RF_S		0x80000000		/* Resolve symbol */
#define _RF_A		0x40000000		/* Use addend */
#define _RF_P		0x20000000		/* Location relative */
#define _RF_G		0x10000000		/* GOT offset */
#define _RF_B		0x08000000		/* Load address relative */
#define _RF_SZ(s)	(((s) & 0xff) << 8)	/* memory target size */
#define _RF_RS(s)	( (s) & 0xff)		/* right shift */
static int reloc_target_flags[] = {
	0,							/* NONE */
	_RF_S|_RF_A|		_RF_SZ(8)  | _RF_RS(0),		/* RELOC_8 */
	_RF_S|_RF_A|		_RF_SZ(16) | _RF_RS(0),		/* RELOC_16 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* RELOC_32 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(8)  | _RF_RS(0),		/* DISP_8 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(16) | _RF_RS(0),		/* DISP_16 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(0),		/* DISP_32 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(2),		/* WDISP_30 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(2),		/* WDISP_22 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(10),	/* HI22 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* 22 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* 13 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* LO10 */
	_RF_G|			_RF_SZ(32) | _RF_RS(0),		/* GOT10 */
	_RF_G|			_RF_SZ(32) | _RF_RS(0),		/* GOT13 */
	_RF_G|			_RF_SZ(32) | _RF_RS(10),	/* GOT22 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(0),		/* PC10 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(10),	/* PC22 */
	      _RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(2),		/* WPLT30 */
				_RF_SZ(32) | _RF_RS(0),		/* COPY */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* GLOB_DAT */
				_RF_SZ(32) | _RF_RS(0),		/* JMP_SLOT */
	      _RF_A|	_RF_B|	_RF_SZ(32) | _RF_RS(0),		/* RELATIVE */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* UA_32 */

	/*unknown*/		_RF_SZ(32) | _RF_RS(0),		/* PLT32 */
	/*unknown*/		_RF_SZ(32) | _RF_RS(0),		/* HIPLT22 */
	/*unknown*/		_RF_SZ(32) | _RF_RS(0),		/* LOPLT10 */
	/*unknown*/		_RF_SZ(32) | _RF_RS(0),		/* LOPLT10 */
	/*unknown*/		_RF_SZ(32) | _RF_RS(0),		/* PCPLT22 */
	/*unknown*/		_RF_SZ(32) | _RF_RS(0),		/* PCPLT32 */
	_RF_S|_RF_A|/*unknown*/	_RF_SZ(32) | _RF_RS(0),		/* 10 */
	_RF_S|_RF_A|/*unknown*/	_RF_SZ(32) | _RF_RS(0),		/* 11 */
	_RF_S|_RF_A|/*unknown*/	_RF_SZ(32) | _RF_RS(0),		/* 64 */
	_RF_S|_RF_A|/*unknown*/	_RF_SZ(32) | _RF_RS(0),		/* OLO10 */
	_RF_S|_RF_A|/*unknown*/	_RF_SZ(32) | _RF_RS(0),		/* HH22 */
	_RF_S|_RF_A|/*unknown*/	_RF_SZ(32) | _RF_RS(0),		/* HM10 */
	_RF_S|_RF_A|/*unknown*/	_RF_SZ(32) | _RF_RS(0),		/* LM22 */
	_RF_S|_RF_A|_RF_P|/*unknown*/	_RF_SZ(32) | _RF_RS(0),	/* WDISP16 */
	_RF_S|_RF_A|_RF_P|/*unknown*/	_RF_SZ(32) | _RF_RS(0),	/* WDISP19 */
	/*unknown*/		_RF_SZ(32) | _RF_RS(0),		/* GLOB_JMP */
	/*unknown*/		_RF_SZ(32) | _RF_RS(0),		/* 7 */
	/*unknown*/		_RF_SZ(32) | _RF_RS(0),		/* 5 */
	/*unknown*/		_RF_SZ(32) | _RF_RS(0),		/* 6 */
};

#ifdef RTLD_DEBUG_RELOC
static const char *reloc_names[] = {
	"NONE", "RELOC_8", "RELOC_16", "RELOC_32", "DISP_8",
	"DISP_16", "DISP_32", "WDISP_30", "WDISP_22", "HI22",
	"22", "13", "LO10", "GOT10", "GOT13",
	"GOT22", "PC10", "PC22", "WPLT30", "COPY",
	"GLOB_DAT", "JMP_SLOT", "RELATIVE", "UA_32", "PLT32",
	"HIPLT22", "LOPLT10", "LOPLT10", "PCPLT22", "PCPLT32",
	"10", "11", "64", "OLO10", "HH22",
	"HM10", "LM22", "WDISP16", "WDISP19", "GLOB_JMP",
	"7", "5", "6"
};
#endif

#define RELOC_RESOLVE_SYMBOL(t)		((reloc_target_flags[t] & _RF_S) != 0)
#define RELOC_PC_RELATIVE(t)		((reloc_target_flags[t] & _RF_P) != 0)
#define RELOC_BASE_RELATIVE(t)		((reloc_target_flags[t] & _RF_B) != 0)
#define RELOC_TARGET_SIZE(t)		((reloc_target_flags[t] >> 8) & 0xff)
#define RELOC_VALUE_RIGHTSHIFT(t)	(reloc_target_flags[t] & 0xff)

static int reloc_target_bitmask[] = {
#define _BM(x)	(~(-(1ULL << (x))))
	0,				/* NONE */
	_BM(8), _BM(16), _BM(32),	/* RELOC_8, _16, _32 */
	_BM(8), _BM(16), _BM(32),	/* DISP8, DISP16, DISP32 */
	_BM(30), _BM(22),		/* WDISP30, WDISP22 */
	_BM(22), _BM(22),		/* HI22, _22 */
	_BM(13), _BM(10),		/* RELOC_13, _LO10 */
	_BM(10), _BM(13), _BM(22),	/* GOT10, GOT13, GOT22 */
	_BM(10), _BM(22),		/* _PC10, _PC22 */  
	_BM(30), 0,			/* _WPLT30, _COPY */
	-1, -1, -1,			/* _GLOB_DAT, JMP_SLOT, _RELATIVE */
	_BM(32), _BM(32),		/* _UA32, PLT32 */
	_BM(22), _BM(10),		/* _HIPLT22, LOPLT10 */
	_BM(32), _BM(22), _BM(10),	/* _PCPLT32, _PCPLT22, _PCPLT10 */
	_BM(10), _BM(11), -1,		/* _10, _11, _64 */
	_BM(10), _BM(22),		/* _OLO10, _HH22 */
	_BM(10), _BM(22),		/* _HM10, _LM22 */
	_BM(16), _BM(19),		/* _WDISP16, _WDISP19 */
	-1,				/* GLOB_JMP */
	_BM(7), _BM(5), _BM(6)		/* _7, _5, _6 */
#undef _BM
};
#define RELOC_VALUE_BITMASK(t)	(reloc_target_bitmask[t])

int
_rtld_relocate_nonplt_object(obj, rela, dodebug)
	Obj_Entry *obj;
	const Elf_RelA *rela;
	bool dodebug;
{
	Elf_Addr *where = (Elf_Addr *) (obj->relocbase + rela->r_offset);
	Elf_Word type, value, mask;
	const Elf_Sym *def = NULL;
	const Obj_Entry *defobj = NULL;

	type = ELF_R_TYPE(rela->r_info);
	if (type == R_TYPE(NONE))
		return (0);

	/* We do JMP_SLOTs in relocate_plt_object() below */
	if (type == R_TYPE(JMP_SLOT))
		return (0);

	/* COPY relocs are also handled elsewhere */
	if (type == R_TYPE(COPY))
		return (0);

	/*
	 * We use the fact that relocation types are an `enum'
	 * Note: R_SPARC_6 is currently numerically largest.
	 */
	if (type > R_TYPE(6))
		return (-1);

	value = rela->r_addend;

	/*
	 * Handle relative relocs here, because we might not
	 * be able to access globals yet.
	 */
	if (!dodebug && type == R_TYPE(RELATIVE)) {
		*where += (Elf_Addr)(obj->relocbase + value);
		return (0);
	}

	if (RELOC_RESOLVE_SYMBOL(type)) {

		/* Find the symbol */
		def = _rtld_find_symdef(_rtld_objlist, rela->r_info,
					NULL, obj, &defobj, false);
		if (def == NULL)
			return (-1);

		/* Add in the symbol's absolute address */
		value += (Elf_Word)(defobj->relocbase + def->st_value);
	}

	if (RELOC_PC_RELATIVE(type)) {
		value -= (Elf_Word)where;
	}

	if (RELOC_BASE_RELATIVE(type)) {
		/*
		 * Note that even though sparcs use `Elf_rela' exclusively
		 * we still need the implicit memory addend in relocations
		 * referring to GOT entries. Undoubtedly, someone f*cked
		 * this up in the distant past, and now we're stuck with
		 * it in the name of compatibility for all eternity..
		 *
		 * In any case, the implicit and explicit should be mutually
		 * exclusive. We provide a check for that here.
		 */
#define DIAGNOSTIC
#ifdef DIAGNOSTIC
		if (value != 0 && *where != 0) {
			xprintf("BASE_REL(%s): where=%p, *where 0x%x, "
				"addend=0x%x, base %p\n",
				obj->path, where, *where,
				rela->r_addend, obj->relocbase);
		}
#endif
		value += (Elf_Word)(obj->relocbase + *where);
	}

	mask = RELOC_VALUE_BITMASK(type);
	value >>= RELOC_VALUE_RIGHTSHIFT(type);
	value &= mask;

	/* We ignore alignment restrictions here */
	*where &= ~mask;
	*where |= value;
#ifdef RTLD_DEBUG_RELOC
	if (RELOC_RESOLVE_SYMBOL(type)) {
		rdbg(dodebug, ("%s %s in %s --> %p %s", 
		    reloc_names[type],
		    defobj->strtab + def->st_name, obj->path,
		    (void *)*where, defobj->path));
	}
	else {
		rdbg(dodebug, ("%s --> %p", reloc_names[type],
		    (void *)*where));
	}
#endif
	return (0);
}

int
_rtld_relocate_plt_object(obj, rela, addrp, bind_now, dodebug)
	Obj_Entry *obj;
	const Elf_RelA *rela;
	caddr_t *addrp;
	bool bind_now;
	bool dodebug;
{
	const Elf_Sym *def;
	const Obj_Entry *defobj;
	Elf_Addr *where = (Elf_Addr *) (obj->relocbase + rela->r_offset);
	Elf_Addr value;

	if (bind_now == 0 && obj->pltgot != NULL)
		return (0);

	/* Fully resolve procedure addresses now */

	assert(ELF_R_TYPE(rela->r_info) == R_TYPE(JMP_SLOT));

	def = _rtld_find_symdef(_rtld_objlist, rela->r_info,
				NULL, obj, &defobj, true);
	if (def == NULL)
		return (-1);

	value = (Elf_Addr) (defobj->relocbase + def->st_value);

	rdbg(dodebug, ("bind now %d/fixup in %s --> old=%p new=%p", 
	    (int)bind_now, defobj->strtab + def->st_name,
	    (void *)*where, (void *)value));

	/*
	 * At the PLT entry pointed at by `where', we now construct
	 * a direct transfer to the now fully resolved function
	 * address.  The resulting code in the jump slot is:
	 *
	 *	sethi	%hi(roffset), %g1
	 *	sethi	%hi(addr), %g1
	 *	jmp	%g1+%lo(addr)
	 *
	 * We write the third instruction first, since that leaves the
	 * previous `b,a' at the second word in place. Hence the whole
	 * PLT slot can be atomically change to the new sequence by
	 * writing the `sethi' instruction at word 2.
	 */
#define SETHI	0x03000000
#define JMP	0x81c06000
#define NOP	0x01000000
	where[2] = JMP   | (value & 0x000003ff);
	where[1] = SETHI | ((value >> 10) & 0x003fffff);
	__asm __volatile("iflush %0+8" : : "r" (where));
	__asm __volatile("iflush %0+4" : : "r" (where));

	if (addrp != NULL)
		*addrp = (caddr_t)value;

	return (0);
}
