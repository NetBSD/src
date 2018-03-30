/*	$NetBSD: mdreloc.c,v 1.67.2.1 2018/03/30 06:20:10 pgoyette Exp $	*/

/*-
 * Copyright (c) 2000 Eduardo Horvath.
 * Copyright (c) 1999, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg and by Charles M. Hannum.
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
__RCSID("$NetBSD: mdreloc.c,v 1.67.2.1 2018/03/30 06:20:10 pgoyette Exp $");
#endif /* not lint */

#include <machine/elf_support.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
#define _RF_U		0x04000000		/* Unaligned */
#define _RF_SZ(s)	(((s) & 0xff) << 8)	/* memory target size */
#define _RF_RS(s)	( (s) & 0xff)		/* right shift */
static const int reloc_target_flags[R_TYPE(TLS_TPOFF64)+1] = {
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
	_RF_S|_RF_A|		_RF_SZ(64) | _RF_RS(0),		/* GLOB_DAT */
				_RF_SZ(32) | _RF_RS(0),		/* JMP_SLOT */
	      _RF_A|	_RF_B|	_RF_SZ(64) | _RF_RS(0),		/* RELATIVE */
	_RF_S|_RF_A|	_RF_U|	_RF_SZ(32) | _RF_RS(0),		/* UA_32 */

	      _RF_A|		_RF_SZ(32) | _RF_RS(0),		/* PLT32 */
	      _RF_A|		_RF_SZ(32) | _RF_RS(10),	/* HIPLT22 */
	      _RF_A|		_RF_SZ(32) | _RF_RS(0),		/* LOPLT10 */
	      _RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(0),		/* PCPLT32 */
	      _RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(10),	/* PCPLT22 */
	      _RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(0),		/* PCPLT10 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* 10 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* 11 */
	_RF_S|_RF_A|		_RF_SZ(64) | _RF_RS(0),		/* 64 */
	_RF_S|_RF_A|/*extra*/	_RF_SZ(32) | _RF_RS(0),		/* OLO10 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(42),	/* HH22 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(32),	/* HM10 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(10),	/* LM22 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(42),	/* PC_HH22 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(32),	/* PC_HM10 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(10),	/* PC_LM22 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(2),		/* WDISP16 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(32) | _RF_RS(2),		/* WDISP19 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* GLOB_JMP */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* 7 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* 5 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* 6 */
	_RF_S|_RF_A|_RF_P|	_RF_SZ(64) | _RF_RS(0),		/* DISP64 */
	      _RF_A|		_RF_SZ(64) | _RF_RS(0),		/* PLT64 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(10),	/* HIX22 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* LOX10 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(22),	/* H44 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(12),	/* M44 */
	_RF_S|_RF_A|		_RF_SZ(32) | _RF_RS(0),		/* L44 */
	_RF_S|_RF_A|		_RF_SZ(64) | _RF_RS(0),		/* REGISTER */
	_RF_S|_RF_A|	_RF_U|	_RF_SZ(64) | _RF_RS(0),		/* UA64 */
	_RF_S|_RF_A|	_RF_U|	_RF_SZ(16) | _RF_RS(0),		/* UA16 */
/* TLS relocs not represented here! */
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
	"HM10", "LM22", "PC_HH22", "PC_HM10", "PC_LM22", 
	"WDISP16", "WDISP19", "GLOB_JMP", "7", "5", "6",
	"DISP64", "PLT64", "HIX22", "LOX10", "H44", "M44", 
	"L44", "REGISTER", "UA64", "UA16",
	"TLS_GD_HI22", "TLS_GD_LO10", "TLS_GD_ADD", "TLS_GD_CALL",
	"TLS_LDM_HI22", "TLS_LDM_LO10", "TLS_LDM_ADD", "TLS_LDM_CALL",
	"TLS_LDO_HIX22", "TLS_LDO_LOX10", "TLS_LDO_ADD", "TLS_IE_HI22", 
	"TLS_IE_LO10", "TLS_IE_LD", "TLS_IE_LDX", "TLS_IE_ADD", "TLS_LE_HIX22", 
	"TLS_LE_LOX10", "TLS_DTPMOD32", "TLS_DTPMOD64", "TLS_DTPOFF32", 
	"TLS_DTPOFF64", "TLS_TPOFF32", "TLS_TPOFF64",
};
#endif

#define RELOC_RESOLVE_SYMBOL(t)		((reloc_target_flags[t] & _RF_S) != 0)
#define RELOC_PC_RELATIVE(t)		((reloc_target_flags[t] & _RF_P) != 0)
#define RELOC_BASE_RELATIVE(t)		((reloc_target_flags[t] & _RF_B) != 0)
#define RELOC_UNALIGNED(t)		((reloc_target_flags[t] & _RF_U) != 0)
#define RELOC_USE_ADDEND(t)		((reloc_target_flags[t] & _RF_A) != 0)
#define RELOC_TARGET_SIZE(t)		((reloc_target_flags[t] >> 8) & 0xff)
#define RELOC_VALUE_RIGHTSHIFT(t)	(reloc_target_flags[t] & 0xff)
#define RELOC_TLS(t)			(t >= R_TYPE(TLS_GD_HI22))

static const long reloc_target_bitmask[] = {
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
	-1, _BM(32), -1,		/* _GLOB_DAT, JMP_SLOT, _RELATIVE */
	_BM(32), _BM(32),		/* _UA32, PLT32 */
	_BM(22), _BM(10),		/* _HIPLT22, LOPLT10 */
	_BM(32), _BM(22), _BM(10),	/* _PCPLT32, _PCPLT22, _PCPLT10 */
	_BM(10), _BM(11), -1,		/* _10, _11, _64 */
	_BM(13), _BM(22),		/* _OLO10, _HH22 */
	_BM(10), _BM(22),		/* _HM10, _LM22 */
	_BM(22), _BM(10), _BM(22),	/* _PC_HH22, _PC_HM10, _PC_LM22 */
	_BM(16), _BM(19),		/* _WDISP16, _WDISP19 */
	-1,				/* GLOB_JMP */
	_BM(7), _BM(5), _BM(6),		/* _7, _5, _6 */
	-1, -1,				/* DISP64, PLT64 */
	_BM(22), _BM(13),		/* HIX22, LOX10 */
	_BM(22), _BM(10), _BM(12),	/* H44, M44, L44 */
	-1, -1, _BM(16),		/* REGISTER, UA64, UA16 */
#undef _BM
};
#define RELOC_VALUE_BITMASK(t)	(reloc_target_bitmask[t])

/*
 * Instruction templates:
 */


/* %hi(v)/%lo(v) with variable shift */
#define	HIVAL(v, s)	(((v) >> (s)) & 0x003fffff)
#define LOVAL(v, s)	(((v) >> (s)) & 0x000003ff)

void _rtld_bind_start_0(long, long);
void _rtld_bind_start_1(long, long);
void _rtld_relocate_nonplt_self(Elf_Dyn *, Elf_Addr);
caddr_t _rtld_bind(const Obj_Entry *, Elf_Word);

/*
 * Install rtld function call into this PLT slot.
 */
#define	SAVE		0x9de3bf50	/* i.e. `save %sp,-176,%sp' */
#define	SETHI_l0	0x21000000
#define	SETHI_l1	0x23000000
#define	OR_l0_l0	0xa0142000
#define	SLLX_l0_32_l0	0xa12c3020
#define	OR_l0_l1_l0	0xa0140011
#define	JMPL_l0_o0	0x91c42000
#define	MOV_g1_o1	0x92100001

void _rtld_install_plt(Elf_Word *, Elf_Addr);
static inline int _rtld_relocate_plt_object(const Obj_Entry *,
    const Elf_Rela *, Elf_Addr *);

void
_rtld_install_plt(Elf_Word *pltgot, Elf_Addr proc)
{
	pltgot[0] = SAVE;
	pltgot[1] = SETHI_l0  | HIVAL(proc, 42);
	pltgot[2] = SETHI_l1  | HIVAL(proc, 10);
	pltgot[3] = OR_l0_l0  | LOVAL(proc, 32);
	pltgot[4] = SLLX_l0_32_l0;
	pltgot[5] = OR_l0_l1_l0;
	pltgot[6] = JMPL_l0_o0 | LOVAL(proc, 0);
	pltgot[7] = MOV_g1_o1;
}

void
_rtld_setup_pltgot(const Obj_Entry *obj)
{
	/*
	 * On sparc64 we got troubles.
	 *
	 * Instructions are 4 bytes long.
	 * Elf[64]_Addr is 8 bytes long, so are our pltglot[]
	 * array entries.
	 * Each PLT entry jumps to PLT0 to enter the dynamic
	 * linker.
	 * Loading an arbitrary 64-bit pointer takes 6
	 * instructions and 2 registers.
	 *
	 * Somehow we need to issue a save to get a new stack
	 * frame, load the address of the dynamic linker, and
	 * jump there, in 8 instructions or less.
	 *
	 * Oh, we need to fill out both PLT0 and PLT1.
	 */
	{
		Elf_Word *entry = (Elf_Word *)obj->pltgot;

		/* Install in entries 0 and 1 */
		_rtld_install_plt(&entry[0], (Elf_Addr) &_rtld_bind_start_0);
		_rtld_install_plt(&entry[8], (Elf_Addr) &_rtld_bind_start_1);

		/* 
		 * Install the object reference in first slot
		 * of entry 2.
		 */
		obj->pltgot[8] = (Elf_Addr) obj;
	}
}

void
_rtld_relocate_nonplt_self(Elf_Dyn *dynp, Elf_Addr relocbase)
{
	const Elf_Rela *rela = 0, *relalim;
	Elf_Addr relasz = 0;
	Elf_Addr *where;

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
	relalim = (const Elf_Rela *)((const uint8_t *)rela + relasz);
	for (; rela < relalim; rela++) {
		where = (Elf_Addr *)(relocbase + rela->r_offset);
		*where = (Elf_Addr)(relocbase + rela->r_addend);
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
		Elf_Addr *where;
		Elf_Word type;
		Elf_Addr value = 0, mask;
		unsigned long symnum;

		where = (Elf_Addr *) (obj->relocbase + rela->r_offset);

		type = ELF_R_TYPE(rela->r_info);
		if (type == R_TYPE(NONE))
			continue;

		/* OLO10 relocations have extra info */
		if ((type & 0x00ff) == R_SPARC_OLO10)
			type = R_SPARC_OLO10;

		/* We do JMP_SLOTs in _rtld_bind() below */
		if (type == R_TYPE(JMP_SLOT))
			continue;

		/* IFUNC relocations are handled in _rtld_call_ifunc */
		if (type == R_TYPE(IRELATIVE)) {
			if (obj->ifunc_remaining_nonplt == 0)
				obj->ifunc_remaining_nonplt = rela - obj->rela + 1;
			continue;
		}

		/* COPY relocs are also handled elsewhere */
		if (type == R_TYPE(COPY))
			continue;

		/*
		 * We use the fact that relocation types are an `enum'
		 * Note: R_SPARC_TLS_TPOFF64 is currently numerically largest.
		 */
		if (type > R_TYPE(TLS_TPOFF64)) {
			dbg(("unknown relocation type %x at %p", type, rela));
			return -1;
		}

		value = rela->r_addend;

		if (RELOC_RESOLVE_SYMBOL(type) || RELOC_TLS(type)) {
			symnum = ELF_R_SYM(rela->r_info);
			if (last_symnum != symnum) {
				last_symnum = symnum;
				def = _rtld_find_symdef(symnum, obj, &defobj,
				    false);
				if (def == NULL)
					return -1;
			}
		}

		/*
		 * Handle TLS relocations here, they are different.
		 */
		if (RELOC_TLS(type)) {
			switch (type) {
			case R_TYPE(TLS_DTPMOD64):
				*where = (Elf64_Addr)defobj->tlsindex;

				rdbg(("TLS_DTPMOD64 %s in %s --> %p",
				    obj->strtab +
				    obj->symtab[symnum].st_name,
				    obj->path, (void *)*where));

				break;

			case R_TYPE(TLS_DTPOFF64):
				*where = (Elf64_Addr)(def->st_value
				    + rela->r_addend);

				rdbg(("DTPOFF64 %s in %s --> %p",
				    obj->strtab +
				        obj->symtab[symnum].st_name,
				    obj->path, (void *)*where));

				break;

			case R_TYPE(TLS_TPOFF64):
				if (!defobj->tls_done &&
					_rtld_tls_offset_allocate(obj))
					     return -1;

				*where = (Elf64_Addr)(def->st_value -
				    defobj->tlsoffset + rela->r_addend);

				rdbg(("TLS_TPOFF64 %s in %s --> %p",
				    obj->strtab + obj->symtab[symnum].st_name,
				    obj->path, (void *)*where));

				break;
			}
			continue;
		}

		/*
		 * Handle relative relocs here, as an optimization.
		 */
		if (type == R_TYPE(RELATIVE)) {
			*where = (Elf_Addr)(obj->relocbase + value);
			rdbg(("RELATIVE in %s --> %p", obj->path,
			    (void *)*where));
			continue;
		}

		if (RELOC_RESOLVE_SYMBOL(type)) {
			/* Add in the symbol's absolute address */
			value += (Elf_Addr)(defobj->relocbase + def->st_value);
		}

		if (type == R_SPARC_OLO10) {
			value = (value & 0x3ff)
			    + (((Elf64_Xword)rela->r_info<<32)>>40);
		}

		if (RELOC_PC_RELATIVE(type)) {
			value -= (Elf_Addr)where;
		}

		if (RELOC_BASE_RELATIVE(type)) {
			/*
			 * Note that even though sparcs use `Elf_rela'
			 * exclusively we still need the implicit memory addend
			 * in relocations referring to GOT entries.
			 * Undoubtedly, someone f*cked this up in the distant
			 * past, and now we're stuck with it in the name of
			 * compatibility for all eternity..
			 *
			 * In any case, the implicit and explicit should be
			 * mutually exclusive. We provide a check for that
			 * here.
			 */
#ifdef DIAGNOSTIC
			if (value != 0 && *where != 0) {
				xprintf("BASE_REL(%s): where=%p, *where 0x%lx, "
					"addend=0x%lx, base %p\n",
					obj->path, where, *where,
					rela->r_addend, obj->relocbase);
			}
#endif
			/* XXXX -- apparently we ignore the preexisting value */
			value += (Elf_Addr)(obj->relocbase);
		}

		mask = RELOC_VALUE_BITMASK(type);
		value >>= RELOC_VALUE_RIGHTSHIFT(type);
		value &= mask;

		if (RELOC_UNALIGNED(type)) {
			/* Handle unaligned relocations. */
			Elf_Addr tmp = 0;
			char *ptr = (char *)where;
			int i, size = RELOC_TARGET_SIZE(type)/8;

			/* Read it in one byte at a time. */
			for (i=0; i<size; i++)
				tmp = (tmp << 8) | ptr[i];

			tmp &= ~mask;
			tmp |= value;

			/* Write it back out. */
			for (i=0; i<size; i++)
				ptr[i] = ((tmp >> (8*i)) & 0xff);
#ifdef RTLD_DEBUG_RELOC
			value = (Elf_Addr)tmp;
#endif

		} else if (RELOC_TARGET_SIZE(type) > 32) {
			*where &= ~mask;
			*where |= value;
#ifdef RTLD_DEBUG_RELOC
			value = (Elf_Addr)*where;
#endif
		} else {
			Elf32_Addr *where32 = (Elf32_Addr *)where;

			*where32 &= ~mask;
			*where32 |= value;
#ifdef RTLD_DEBUG_RELOC
			value = (Elf_Addr)*where32;
#endif
		}

#ifdef RTLD_DEBUG_RELOC
		if (RELOC_RESOLVE_SYMBOL(type)) {
			rdbg(("%s %s in %s --> %p in %s", reloc_names[type],
			    obj->strtab + obj->symtab[symnum].st_name,
			    obj->path, (void *)value, defobj->path));
		} else {
			rdbg(("%s in %s --> %p", reloc_names[type],
			    obj->path, (void *)value));
		}
#endif
	}
	return (0);
}

int
_rtld_relocate_plt_lazy(Obj_Entry *obj)
{
	const Elf_Rela *rela;

	for (rela = obj->pltrelalim; rela-- > obj->pltrela; ) {
		if (ELF_R_TYPE(rela->r_info) == R_TYPE(JMP_IREL))
			obj->ifunc_remaining = obj->pltrelalim - rela + 1;
	}

	return 0;
}

caddr_t
_rtld_bind(const Obj_Entry *obj, Elf_Word reloff)
{
	const Elf_Rela *rela = obj->pltrela + reloff;
	Elf_Addr result;
	int err;

	result = 0;	/* XXX gcc */

	if (ELF_R_TYPE(obj->pltrela->r_info) == R_TYPE(JMP_SLOT) ||
	    ELF_R_TYPE(obj->pltrela->r_info) == R_TYPE(JMP_IREL)) {
		/*
		 * XXXX
		 *
		 * The first four PLT entries are reserved.  There is some
		 * disagreement whether they should have associated relocation
		 * entries.  Both the SPARC 32-bit and 64-bit ELF
		 * specifications say that they should have relocation entries,
		 * but the 32-bit SPARC binutils do not generate them, and now
		 * the 64-bit SPARC binutils have stopped generating them too.
		 * 
		 * So, to provide binary compatibility, we will check the first
		 * entry, if it is reserved it should not be of the type
		 * JMP_SLOT or JMP_REL.  If it is either of those, then
		 * the 4 reserved entries were not generated and our index
		 * is 4 entries too far.
		 */
		rela -= 4;
	}

	_rtld_shared_enter();
	err = _rtld_relocate_plt_object(obj, rela, &result);
	if (err)
		_rtld_die();
	_rtld_shared_exit();

	return (caddr_t)result;
}

int
_rtld_relocate_plt_objects(const Obj_Entry *obj)
{
	const Elf_Rela *rela;

	rela = obj->pltrela;

	/*
	 * Check for first four reserved entries - and skip them.
	 * See above for details.
	 */
	if (ELF_R_TYPE(obj->pltrela->r_info) != R_TYPE(JMP_SLOT) &&
	    ELF_R_TYPE(obj->pltrela->r_info) != R_TYPE(JMP_IREL))
		rela += 4;

	for (; rela < obj->pltrelalim; rela++)
		if (_rtld_relocate_plt_object(obj, rela, NULL) < 0)
			return -1;

	return 0;
}

static inline void
_rtld_write_plt(Elf_Word *where, Elf_Addr value, const Elf_Rela *rela,
    const Obj_Entry *obj)
{
	if (rela && rela->r_addend) {
		Elf_Addr *ptr = (Elf_Addr *)where;
		/*
		 * This entry is >= 32768.  The relocations points to a
		 * PC-relative pointer to the bind_0 stub at the top of the
		 * PLT section.  Update it to point to the target function.
		 */
		ptr[0] += value - (Elf_Addr)obj->pltgot;
	} else {
		sparc_write_branch(where + 1, (void *)value);
	}
}

/*
 * New inline function that is called by _rtld_relocate_plt_object and
 * _rtld_bind
 */
static inline int
_rtld_relocate_plt_object(const Obj_Entry *obj, const Elf_Rela *rela,
    Elf_Addr *tp)
{
	Elf_Word *where = (Elf_Word *)(obj->relocbase + rela->r_offset);
	const Elf_Sym *def;
	const Obj_Entry *defobj;
	Elf_Addr value;
	unsigned long info = rela->r_info;

	if (ELF_R_TYPE(info) == R_TYPE(JMP_IREL))
		return 0;

	assert(ELF_R_TYPE(info) == R_TYPE(JMP_SLOT));

	def = _rtld_find_plt_symdef(ELF_R_SYM(info), obj, &defobj, tp != NULL);
	if (__predict_false(def == NULL))
		return -1;
	if (__predict_false(def == &_rtld_sym_zero))
		return 0;

	if (ELF_ST_TYPE(def->st_info) == STT_GNU_IFUNC) {
		if (tp == NULL)
			return 0;
		value = _rtld_resolve_ifunc(defobj, def);
	} else {
		value = (Elf_Addr)(defobj->relocbase + def->st_value);
	}
	rdbg(("bind now/fixup in %s at %p --> new=%p", 
	    defobj->strtab + def->st_name, (void*)where, (void *)value));

	_rtld_write_plt(where, value, rela, obj);

	if (tp)
		*tp = value;

	return 0;
}

void
_rtld_call_ifunc(Obj_Entry *obj, sigset_t *mask, u_int cur_objgen)
{
	const Elf_Rela *rela;
	Elf_Addr *where;
	Elf_Word *where2;
	Elf_Addr target;

	while (obj->ifunc_remaining > 0 && _rtld_objgen == cur_objgen) {
		rela = obj->pltrelalim - --obj->ifunc_remaining;
		if (ELF_R_TYPE(rela->r_info) != R_TYPE(JMP_IREL))
			continue;
		where2 = (Elf_Word *)(obj->relocbase + rela->r_offset);
		target = (Elf_Addr)(obj->relocbase + rela->r_addend);
		_rtld_exclusive_exit(mask);
		target = _rtld_resolve_ifunc2(obj, target);
		_rtld_exclusive_enter(mask);
		sparc_write_branch(where2 + 1, (void *)target);
	}

	while (obj->ifunc_remaining_nonplt > 0 && _rtld_objgen == cur_objgen) {
		rela = obj->relalim - --obj->ifunc_remaining_nonplt;
		if (ELF_R_TYPE(rela->r_info) != R_TYPE(IRELATIVE))
			continue;
		where = (Elf_Addr *)(obj->relocbase + rela->r_offset);
		target = (Elf_Addr)(obj->relocbase + rela->r_addend);
		_rtld_exclusive_exit(mask);
		target = _rtld_resolve_ifunc2(obj, target);
		_rtld_exclusive_enter(mask);
		if (*where != target)
			*where = target;
	}
}
