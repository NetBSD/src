/*	$NetBSD: reloc.c,v 1.10 1999/02/24 12:20:30 pk Exp $	*/

/*
 * Copyright 1996 John D. Polstra.
 * Copyright 1996 Matt Thomas <matt@3am-software.com>
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by John Polstra.
 * 4. The name of the author may not be used to endorse or promote products
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

/*
 * Dynamic linker for ELF.
 *
 * John Polstra <jdp@polstra.com>.
 */

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <dirent.h>

#include "debug.h"
#include "rtld.h"

#if defined(__alpha__) || defined(__powerpc__) || defined(__i386__)
/*
 * XXX: These don't work for the alpha and i386; don't know about powerpc
 *	The alpha and the i386 avoid the problem by compiling everything PIC.
 *	These relocation are supposed to be writing the address of the
 *	function to be called on the bss.rel or bss.rela segment, but:
 *		- st_size == 0
 *		- on the i386 at least the call instruction is a direct call
 *		  not an indirect call.
 */
static int
_rtld_do_copy_relocation(
    const Obj_Entry *dstobj,
    const Elf_RelA *rela)
{
    void *dstaddr = (void *) (dstobj->relocbase + rela->r_offset);
    const Elf_Sym *dstsym = dstobj->symtab + ELF_R_SYM(rela->r_info);
    const char *name = dstobj->strtab + dstsym->st_name;
    unsigned long hash = _rtld_elf_hash(name);
    size_t size = dstsym->st_size;
    const void *srcaddr;
    const Elf_Sym *srcsym;
    Obj_Entry *srcobj;

    for (srcobj = dstobj->next;  srcobj != NULL;  srcobj = srcobj->next)
	if ((srcsym = _rtld_symlook_obj(name, hash, srcobj, false)) != NULL)
	    break;

    if (srcobj == NULL) {
	_rtld_error("Undefined symbol \"%s\" referenced from COPY"
	      " relocation in %s", name, dstobj->path);
	return -1;
    }

    srcaddr = (const void *) (srcobj->relocbase + srcsym->st_value);
    memcpy(dstaddr, srcaddr, size);
#ifdef RTLD_DEBUG_RELOC
    dbg("COPY %s %s %s --> src=%p dst=%p *dst= %p size %d", 
	dstobj->path, srcobj->path, name, (void *)srcaddr, (void *)dstaddr,
	(void *)*(long *)dstaddr, size);
#endif
    return 0;
}
#endif /* __alpha__ || __powerpc__ || __i386__ */

/*
 * Process the special R_xxx_COPY relocations in the main program.  These
 * copy data from a shared object into a region in the main program's BSS
 * segment.
 *
 * Returns 0 on success, -1 on failure.
 */
int
_rtld_do_copy_relocations(
    const Obj_Entry *dstobj)
{
    assert(dstobj->mainprog);	/* COPY relocations are invalid elsewhere */

#if defined(__alpha__) || defined(__powerpc__) || defined(__i386__)
    if (dstobj->rel != NULL) {
	const Elf_Rel *rel;
	for (rel = dstobj->rel;  rel < dstobj->rellim;  ++rel) {
	    if (ELF_R_TYPE(rel->r_info) == R_TYPE(COPY)) {
		Elf_RelA ourrela;
		ourrela.r_info = rel->r_info;
		ourrela.r_offset = rel->r_offset;
		ourrela.r_addend = 0;
		if (_rtld_do_copy_relocation(dstobj, &ourrela) < 0)
		    return -1;
	    }
	}
    }

    if (dstobj->rela != NULL) {
	const Elf_RelA *rela;
	for (rela = dstobj->rela;  rela < dstobj->relalim;  ++rela) {
	    if (ELF_R_TYPE(rela->r_info) == R_TYPE(COPY)) {
		if (_rtld_do_copy_relocation(dstobj, rela) < 0)
		    return -1;
	    }
	}
    }
#endif /* __alpha__ || __powerpc__ || __i386__ */

    return 0;
}

#ifdef __sparc__
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
	      _RF_A|		_RF_SZ(32) | _RF_RS(0),		/* RELATIVE */
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

#define RELOC_RESOLVE_SYMBOL(t)		((reloc_target_flags[t] & _RF_S) != 0)
#define RELOC_PC_RELATIVE(t)		((reloc_target_flags[t] & _RF_P) != 0)
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
	-1, -1, _BM(22),		/* _GLOB_DAT, JMP_SLOT, _RELATIVE */
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

static int
_rtld_relocate_nonplt_object(
	const Obj_Entry *obj,
	const Elf_RelA *rela)
{
	Elf_Addr *where = (Elf_Addr *) (obj->relocbase + rela->r_offset);
	Elf_Word type, value, mask;

	type = ELF_R_TYPE(rela->r_info);
	if (type == R_TYPE(NONE))
		return (0);

	/*
	 * We use the fact that relocation types are an `enum'
	 * Note: R_SPARC_6 is currently numerically largest.
	 */
	if (type > R_TYPE(6))
		return (-1);

	value = rela->r_addend;
	if (RELOC_RESOLVE_SYMBOL(type)) {
		const Elf_Sym *def;
		const Obj_Entry *defobj;

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

	mask = RELOC_VALUE_BITMASK(type);
	value >>= RELOC_VALUE_RIGHTSHIFT(type);
	value &= mask;

	/* We ignore alignment restrictions here */
	*where &= ~mask;
	*where |= value;
	return (0);
}

static int
__rtld_relocate_plt_object(
	const Obj_Entry *obj,
	const Elf_RelA *rela,
	bool bind_now,
	caddr_t *addrp)
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

#ifdef RTLD_DEBUG_RELOC
	dbg("bind now %d/fixup in %s --> old=%p new=%p", 
		(int)bind_now, 
		defobj->strtab + def->st_name,
		(void *)*where, (void *)value);
#endif

	/*
	 * At the PLT entry pointed at by `where', we now construct
	 * a direct transfer to the now fully resolved function
	 * address.  The resulting code in the jump slot is:
	 *
	 *	sethi	%hi(addr), %g1
	 *	jmp	%g1+%lo(addr)
	 *	nop	! delay slot
	 */
#define SETHI	0x03000000
#define JMP	0x81c06000
#define NOP	0x01000000
	where[0] = SETHI | ((value >> 10) & 0x003fffff);
	where[1] = JMP   | (value & 0x000003ff);
	where[2] = NOP;

	if (addrp != NULL)
		*addrp = (caddr_t)value;

	return (0);
}

#define _rtld_relocate_plt_object(obj, rela, bind_now) \
	__rtld_relocate_plt_object(obj, rela, bind_now, NULL)

caddr_t
_rtld_bind(
	const Obj_Entry *obj,
	Elf_Word reloff)
{
	const Elf_RelA *rela;
	Elf_RelA ourrela;
	caddr_t addr;

	if (obj->pltrel != NULL) {
		const Elf_Rel *rel;

		rel = (const Elf_Rel *) ((caddr_t) obj->pltrel + reloff);
		ourrela.r_info = rel->r_info;
		ourrela.r_offset = rel->r_offset;
		rela = &ourrela;
	} else {
		rela = (const Elf_RelA *) ((caddr_t) obj->pltrela + reloff);
	}

	if (__rtld_relocate_plt_object(obj, rela, true, &addr) < 0)
		_rtld_die();

	return (addr);
	return *(caddr_t *)(obj->relocbase + rela->r_offset);
}

#else /* __sparc__ */

static int
_rtld_relocate_nonplt_object(
    const Obj_Entry *obj,
    const Elf_RelA *rela)
{
    Elf_Addr *where = (Elf_Addr *) (obj->relocbase + rela->r_offset);

    switch (ELF_R_TYPE(rela->r_info)) {

    case R_TYPE(NONE):
	break;

#ifdef __i386__
    case R_TYPE(GOT32): {
	const Elf_Sym *def;
	const Obj_Entry *defobj;

	def = _rtld_find_symdef(_rtld_objlist, rela->r_info, NULL, obj, &defobj, false);
	if (def == NULL)
	    return -1;

	if (*where != (Elf_Addr) (defobj->relocbase + def->st_value))
	    *where = (Elf_Addr) (defobj->relocbase + def->st_value);
#ifdef RTLD_DEBUG_RELOC
	dbg("GOT32 %s in %s --> %p in %s", 
	    defobj->strtab + def->st_name, obj->path,
	    (void *)*where, defobj->path);
#endif
	break;
    }

    case R_TYPE(PC32):
	/*
	 * I don't think the dynamic linker should ever see this
	 * type of relocation.  But the binutils-2.6 tools sometimes
	 * generate it.
	 */
    {
	const Elf_Sym *def;
	const Obj_Entry *defobj;

	def = _rtld_find_symdef(_rtld_objlist, rela->r_info, NULL, obj, &defobj, false);
	if (def == NULL)
	    return -1;

	*where += (Elf_Addr) (defobj->relocbase + def->st_value)
	    - (Elf_Addr) where;
#ifdef RTLD_DEBUG_RELOC
	dbg("PC32 %s in %s --> %p in %s", 
	    defobj->strtab + def->st_name, obj->path,
	    (void *)*where, defobj->path);
#endif
	break;
    }

    case R_TYPE(32): {
	const Elf_Sym *def;
	const Obj_Entry *defobj;

	def = _rtld_find_symdef(_rtld_objlist, rela->r_info, NULL, obj, &defobj, false);
	if (def == NULL)
	    return -1;

	*where += (Elf_Addr)(defobj->relocbase + def->st_value);
#ifdef RTLD_DEBUG_RELOC
	dbg("32 %s in %s --> %p in %s", 
	    defobj->strtab + def->st_name, obj->path,
	    (void *)*where, defobj->path);
#endif
	break;
    }
#endif /* __i386__ */

#ifdef __alpha__
    case R_ALPHA_REFQUAD: {
	const Elf_Sym *def;
	const Obj_Entry *defobj;
	Elf_Addr tmp_value;

	def = _rtld_find_symdef(_rtld_objlist, rela->r_info, NULL, obj, &defobj, false);
	if (def == NULL)
	    return -1;

	tmp_value = (Elf_Addr) (defobj->relocbase + def->st_value)
	    + *where + rela->r_addend;
	if (*where != tmp_value)
	    *where = tmp_value;
#ifdef RTLD_DEBUG_RELOC
	dbg("REFQUAD %s in %s --> %p in %s", 
	    defobj->strtab + def->st_name, obj->path,
	    (void *)*where, defobj->path);
#endif
	break;
    }
#endif /* __alpha__ */

#if defined(__i386__) || defined(__alpha__)
    case R_TYPE(GLOB_DAT):
    {
	const Elf_Sym *def;
	const Obj_Entry *defobj;

	def = _rtld_find_symdef(_rtld_objlist, rela->r_info, NULL, obj, &defobj, false);
	if (def == NULL)
	    return -1;

	if (*where != (Elf_Addr) (defobj->relocbase + def->st_value))
	    *where = (Elf_Addr) (defobj->relocbase + def->st_value);
#ifdef RTLD_DEBUG_RELOC
	dbg("GLOB_DAT %s in %s --> %p in %s", 
	    defobj->strtab + def->st_name, obj->path,
	    (void *)*where, defobj->path);
#endif
	break;
    }

    case R_TYPE(RELATIVE): {
	extern Elf_Addr _GLOBAL_OFFSET_TABLE_[];
	extern Elf_Dyn _DYNAMIC;

	if (obj != &_rtld_objself ||
	    (caddr_t)where < (caddr_t)_GLOBAL_OFFSET_TABLE_ ||
	    (caddr_t)where >= (caddr_t)&_DYNAMIC) {
	    *where += (Elf_Addr) obj->relocbase;
#ifdef RTLD_DEBUG_RELOC
	    dbg("RELATIVE in %s --> %p", obj->path, (void *)*where);
#endif
	}
#ifdef RTLD_DEBUG_RELOC
	else
	    dbg("RELATIVE in %s stays at %p", obj->path, (void *)*where);
#endif
	break;
    }

    case R_TYPE(COPY): {
	/*
	 * These are deferred until all other relocations have
	 * been done.  All we do here is make sure that the COPY
	 * relocation is not in a shared library.  They are allowed
	 * only in executable files.
	 */
	if (!obj->mainprog) {
	    _rtld_error("%s: Unexpected R_COPY relocation in shared library",
		  obj->path);
	    return -1;
	}
#ifdef RTLD_DEBUG_RELOC
	dbg("COPY (avoid in main)");
#endif
	break;
    }
#endif /* __i386__ || __alpha__ */

#ifdef __mips__
    case R_TYPE(REL32): {
    		/* 32-bit PC-relative reference */

        const Elf_Sym *def;
        const Obj_Entry *defobj;

	def = obj->symtab + ELF_R_SYM(rela->r_info);

        if (ELF_SYM_BIND(def->st_info) == Elf_estb_local &&
          (ELF_SYM_TYPE(def->st_info) == Elf_estt_section ||
           ELF_SYM_TYPE(def->st_info) == Elf_estt_notype)) {
            *where += (Elf_Addr) obj->relocbase;
#ifdef RTLD_DEBUG_RELOC
	    dbg("REL32 in %s --> %p", obj->path, (void *)*where);
#endif
        } else {
/* XXX maybe do something re: bootstrapping? */
            def = _rtld_find_symdef(_rtld_objlist, rela->r_info, NULL, obj,
	        &defobj, false);
            if (def == NULL)
                return -1;
	    *where += (Elf_Addr)(defobj->relocbase + def->st_value);
#ifdef RTLD_DEBUG_RELOC
	    dbg("REL32 %s in %s --> %p in %s", 
		defobj->strtab + def->st_name, obj->path,
		(void *)*where, defobj->path);
#endif
        }
        break;
    }
 
#endif /* mips */

#ifdef __powerpc__
    case R_TYPE(32):		/* word32 S + A */
    case R_TYPE(GLOB_DAT): {	/* word32 S + A */
	const Elf_Sym *def;
	const Obj_Entry *defobj;
	Elf_Addr x;

	def = _rtld_find_symdef(_rtld_objlist, rela->r_info, NULL, obj, &defobj, false);
	if (def == NULL)
	    return -1;

	x = (Elf_Addr)(defobj->relocbase + def->st_value + rela->r_addend);

	if (*where != x)
	    *where = x;
#ifdef RTLD_DEBUG_RELOC
	dbg("32/GLOB_DAT %s in %s --> %p in %s", 
	    defobj->strtab + def->st_name, obj->path,
	    (void *)*where, defobj->path);
#endif
	break;
    }

    case R_TYPE(COPY):
#ifdef RTLD_DEBUG_RELOC
	dbg("COPY");
#endif
	break;

    case R_TYPE(JMP_SLOT):
#ifdef RTLD_DEBUG_RELOC
	dbg("JMP_SLOT");
#endif
	break;

    case R_TYPE(RELATIVE): {	/* word32 B + A */
	if (obj == &_rtld_objself && 
	    *where == (Elf_Addr)obj->relocbase + rela->r_addend)
	    break;	/* GOT - already done */

	*where = (Elf_Addr)obj->relocbase + rela->r_addend;
#ifdef RTLD_DEBUG_RELOC
	dbg("RELATIVE in %s --> %p", obj->path, (void *)*where);
#endif
	break;
    }
#endif /* __powerpc__ */

    default: {
	const Elf_Sym *def;
	const Obj_Entry *defobj;

	def = _rtld_find_symdef(_rtld_objlist, rela->r_info, NULL, obj, &defobj, true);
	dbg("sym = %lu, type = %lu, offset = %p, addend = %p, contents = %p, symbol = %s",
	    (u_long)ELF_R_SYM(rela->r_info), (u_long)ELF_R_TYPE(rela->r_info),
	    (void *)rela->r_offset, (void *)rela->r_addend, (void *)*where,
	    def ? defobj->strtab + def->st_name : "??");
	_rtld_error("%s: Unsupported relocation type %d in non-PLT relocations\n",
	      obj->path, ELF_R_TYPE(rela->r_info));
	return -1;
    }
    }
    return 0;
}



static int
_rtld_relocate_plt_object(
    const Obj_Entry *obj,
    const Elf_RelA *rela,
    bool bind_now)
{
    Elf_Addr *where = (Elf_Addr *) (obj->relocbase + rela->r_offset);
    Elf_Addr new_value;

    /* Fully resolve procedure addresses now */

#if defined(__powerpc__)
    return _rtld_reloc_powerpc_plt(obj, rela, bind_now);
#endif

#if defined(__alpha__)	|| defined(__i386__) /* (jrs) */
    if (bind_now || obj->pltgot == NULL) {
	const Elf_Sym *def;
	const Obj_Entry *defobj;

	assert(ELF_R_TYPE(rela->r_info) == R_TYPE(JMP_SLOT));

	def = _rtld_find_symdef(_rtld_objlist, rela->r_info, NULL, obj, &defobj, true);
	if (def == NULL)
	    return -1;

	new_value = (Elf_Addr) (defobj->relocbase + def->st_value);
#ifdef RTLD_DEBUG_RELOC
	dbg("bind now %d/fixup in %s --> old=%p new=%p", 
	    (int)bind_now, 
	    defobj->strtab + def->st_name,
	    (void *)*where, (void *)new_value);
#endif
    } else
#endif	/* __alpha__ (jrs) */
    if (!obj->mainprog) {
	/* Just relocate the GOT slots pointing into the PLT */
	new_value = *where + (Elf_Addr) (obj->relocbase);
#ifdef RTLD_DEBUG_RELOC
	dbg("fixup !main in %s --> %p", obj->path, (void *)*where);
#endif
    } else {
	return 0;
    }
    /*
     * Since this page is probably copy-on-write, let's not write
     * it unless we really really have to.
     */
    if (*where != new_value)
	*where = new_value;
    return 0;
}

caddr_t
_rtld_bind(
    const Obj_Entry *obj,
    Elf_Word reloff)
{
    const Elf_RelA *rela;
    Elf_RelA ourrela;

    if (obj->pltrel != NULL) {
	ourrela.r_info =   ((const Elf_Rel *) ((caddr_t) obj->pltrel + reloff))->r_info;
	ourrela.r_offset = ((const Elf_Rel *) ((caddr_t) obj->pltrel + reloff))->r_offset;
	rela = &ourrela;
    } else {
	rela = (const Elf_RelA *) ((caddr_t) obj->pltrela + reloff);
    }

    if (_rtld_relocate_plt_object(obj, rela, true) < 0)
	_rtld_die();

    return *(caddr_t *)(obj->relocbase + rela->r_offset);
}
#endif /* __sparc__ */

/*
 * Relocate newly-loaded shared objects.  The argument is a pointer to
 * the Obj_Entry for the first such object.  All objects from the first
 * to the end of the list of objects are relocated.  Returns 0 on success,
 * or -1 on failure.
 */
int
_rtld_relocate_objects(
    Obj_Entry *first,
    bool bind_now)
{
    Obj_Entry *obj;
    int ok = 1;

    for (obj = first;  obj != NULL;  obj = obj->next) {

	if (obj->nbuckets == 0 || obj->nchains == 0
	        || obj->buckets == NULL || obj->symtab == NULL
	        || obj->strtab == NULL) {
	    _rtld_error("%s: Shared object has no run-time symbol table",
			obj->path);
	    return -1;
	}

	dbg(" relocating %s (%ld/%ld rel/rela, %ld/%ld plt rel/rela)",
	    obj->path,
	    (long)(obj->rellim - obj->rel), (long)(obj->relalim - obj->rela),
	    (long)(obj->pltrellim - obj->pltrel),
	    (long)(obj->pltrelalim - obj->pltrela));

	if (obj->textrel) {
	    /* There are relocations to the write-protected text segment. */
	    if (mprotect(obj->mapbase, obj->textsize,
			 PROT_READ|PROT_WRITE|PROT_EXEC) == -1) {
		_rtld_error("%s: Cannot write-enable text segment: %s",
			    obj->path, xstrerror(errno));
		return -1;
	    }
	}

	if (obj->rel != NULL) {
	    /* Process the non-PLT relocations. */
	    const Elf_Rel *rel;
	    for (rel = obj->rel;  rel < obj->rellim;  ++rel) {
		Elf_RelA ourrela;
		ourrela.r_info   = rel->r_info;
		ourrela.r_offset = rel->r_offset;
#if defined(__mips__)
		/* rel->r_offset is not valid on mips? */
		if (ELF_R_TYPE(ourrela.r_info) == R_TYPE(NONE))
		    ourrela.r_addend = 0;
		else
#endif
		ourrela.r_addend = *(Elf_Word *) (obj->relocbase + rel->r_offset);

		if (_rtld_relocate_nonplt_object(obj, &ourrela) < 0)
		    ok = 0;
	    }
	}

	if (obj->rela != NULL) {
	    /* Process the non-PLT relocations. */
	    const Elf_RelA *rela;
	    for (rela = obj->rela;  rela < obj->relalim;  ++rela) {
		if (_rtld_relocate_nonplt_object(obj, rela) < 0)
		    ok = 0;
	    }
	}

	if (obj->textrel) {	/* Re-protected the text segment. */
	    if (mprotect(obj->mapbase, obj->textsize,
			 PROT_READ|PROT_EXEC) == -1) {
		_rtld_error("%s: Cannot write-protect text segment: %s",
			    obj->path, xstrerror(errno));
		return -1;
	    }
	}

	/* Process the PLT relocations. */
	if (obj->pltrel != NULL) {
	    const Elf_Rel *rel;
	    for (rel = obj->pltrel; rel < obj->pltrellim;  ++rel) {
		Elf_RelA ourrela;
		ourrela.r_info   = rel->r_info;
		ourrela.r_offset = rel->r_offset;
		ourrela.r_addend = *(Elf_Word *) (obj->relocbase + rel->r_offset);
		if (_rtld_relocate_plt_object(obj, &ourrela, bind_now) < 0)
		    ok = 0;
	    }
	}

	if (obj->pltrela != NULL) {
	    const Elf_RelA *rela;
	    for (rela = obj->pltrela;  rela < obj->pltrelalim;  ++rela) {
		if (_rtld_relocate_plt_object(obj, rela, bind_now) < 0)
		    ok = 0;
	    }
	}

	if (!ok)
	    return -1;


	/* Set some sanity-checking numbers in the Obj_Entry. */
	obj->magic = RTLD_MAGIC;
	obj->version = RTLD_VERSION;

	/* Fill in the dynamic linker entry points. */
	obj->dlopen  = _rtld_dlopen;
	obj->dlsym   = _rtld_dlsym;
	obj->dlerror = _rtld_dlerror;
	obj->dlclose = _rtld_dlclose;

	/* Set the special PLTGOT entries. */
	if (obj->pltgot != NULL) {
#if defined(__i386__)
	    obj->pltgot[1] = (Elf_Addr) obj;
	    obj->pltgot[2] = (Elf_Addr) &_rtld_bind_start;
#endif
#if defined(__alpha__)
	    /* This function will be called to perform the relocation.  */
	    obj->pltgot[2] = (Elf_Addr) &_rtld_bind_start;
	    /* Identify this shared object */
	    obj->pltgot[3] = (Elf_Addr) obj;
#endif
#if defined(__mips__)
	    _rtld_relocate_mips_got(obj);

	    obj->pltgot[0] = (Elf_Addr) &_rtld_bind_start;
	    /* XXX only if obj->pltgot[1] & 0x80000000 ?? */
	    obj->pltgot[1] |= (Elf_Addr) obj;
#endif
#if defined(__powerpc__)
	    _rtld_setup_powerpc_plt(obj);
#endif
#if defined(__sparc__)
		/*
		 * PLTGOT is the PLT on the sparc.
		 * The first entry holds the call the dynamic linker.
		 * We construct a `call' instruction that transfers
		 * to `_rtld_bind_start()'.
		 * The second entry holds the object identification.
		 * Note: each PLT entry is three words long.
		 */
		obj->pltgot[1] = 0x40000000 |
		    ((Elf_Addr)&_rtld_bind_start - (Elf_Addr)&obj->pltgot[1]);
		obj->pltgot[3] = (Elf_Addr) obj;
#endif
	}
    }

    return 0;
}
