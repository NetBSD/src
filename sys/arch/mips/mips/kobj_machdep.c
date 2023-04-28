/*	$NetBSD: kobj_machdep.c,v 1.2 2023/04/28 07:33:56 skrll Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Simon Burge.
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

__RCSID("$NetBSD");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/exec_elf.h>
#include <sys/kernel.h>
#include <sys/kobj.h>
#include <sys/xcall.h>

#include <mips/cache.h>

#ifndef __mips_n64
#error update for non-N64 abis		/* XXX */
#endif

#define	RELOC_LO16(x)		((x) & 0xffff)
#define	RELOC_HI16(x)		((((int64_t)(x) + 0x8000LL) >> 16) & 0xffff)
#define	RELOC_HIGHER(x)		((((int64_t)(x) + 0x80008000LL) >> 32) & 0xffff)
#define	RELOC_HIGHEST(x)	((((int64_t)(x) + 0x800080008000LL) >> 48) & 0xffff)

#undef MIPS_MODULE_DEBUG
#ifdef MIPS_MODULE_DEBUG
#define	DPRINTF(fmt, args...)   \
	do { printf(fmt, ## args); } while (0)
#else
#define	DPRINTF(fmt, args...)   __nothing
#endif

int
kobj_reloc(kobj_t ko, uintptr_t relocbase, const void *data,
    bool isrela, bool local)
{
	Elf_Addr addr, addend, *where;
	Elf32_Addr *where32;
	Elf_Word rtype, symidx;
	const Elf_Rela *rela;
	uint32_t *insn;
	int error;

	DPRINTF("%s(kobj %p, reloc %#lx,\n    data %p, rela %d, local %d)\n",
	    __func__, ko, relocbase, data, isrela, local);

	if (!isrela) {
		printf("%s: REL relocations not supported", __func__);
		return -1;
	}

	rela = (const Elf_Rela *)data;
	where = (Elf_Addr *)(relocbase + rela->r_offset);
	addend = rela->r_addend;
	rtype = ELF_R_TYPE(rela->r_info);
	symidx = ELF_R_SYM(rela->r_info);

	const Elf_Sym *sym = kobj_symbol(ko, symidx);

	if (!local && ELF_ST_BIND(sym->st_info) == STB_LOCAL) {
		return 0;
	}

	/* pointer to 32bit value and instruction */
	insn = (void *)where;

	/* check alignment */
	KASSERT(((intptr_t)where & (sizeof(int32_t) - 1)) == 0);

	switch (rtype) {
	case R_TYPE(NONE):	/* none */
		DPRINTF("    reloc R_MIPS_NONE\n");
		break;
	/*   R_TYPE(16) */
	case R_TYPE(32):	/* S + A */
		DPRINTF("    reloc R_MIPS_32\n");
		error = kobj_sym_lookup(ko, symidx, &addr);
		if (error)
			return -1;
		addr += addend;
		where32 = (void *)where;
		DPRINTF("    orig = 0x%08x\n", *where32);
		*where32 = addr;
		DPRINTF("    new  = 0x%08x\n", *where32);
		break;
	/*   R_TYPE(REL32) */
	case R_TYPE(26):	/* (((A << 2) | (P & 0xf0000000)) + S) >> 2 */
		/* XXXXXX untested */
		DPRINTF("    reloc R_MIPS_26 (untested)\n");
		error = kobj_sym_lookup(ko, symidx, &addr);
		if (error)
			return -1;

		addend &= __BITS(25, 0);	/* mask off lower 26 bits */
		addend <<= 2;

		addr += ((intptr_t)where & 0xf0000000) | addend;
		addr >>= 2;

		KASSERT((*insn & 0x3ffffff) == 0);
		DPRINTF("    orig insn = 0x%08x\n", *insn);
		*insn |= addr;
		DPRINTF("    new  insn = 0x%08x\n", *insn);

		break;
	case R_TYPE(HI16):	/* %high(AHL + S) = (x - (short)x) >> 16 */
		DPRINTF("    reloc R_MIPS_HI16\n");
		error = kobj_sym_lookup(ko, symidx, &addr);
		if (error)
			return -1;

		addr += addend;
		KASSERT((*insn & 0xffff) == 0);
		DPRINTF("    orig insn = 0x%08x\n", *insn);
		DPRINTF("    HI16(%#lx) = 0x%04llx\n", addr, RELOC_HI16(addr));
		*insn |= RELOC_HI16(addr);
		DPRINTF("    new  insn = 0x%08x\n", *insn);
		break;
	case R_TYPE(LO16):	/* AHL + S */
		DPRINTF("    reloc R_MIPS_LO16\n");
		error = kobj_sym_lookup(ko, symidx, &addr);
		if (error)
			return -1;

		addr += addend;
		KASSERT((*insn & 0xffff) == 0);
		DPRINTF("    orig insn = 0x%08x\n", *insn);
		DPRINTF("    LO16(%#lx) = 0x%04lx\n", addr, RELOC_LO16(addr));
		*insn |= RELOC_LO16(addr);
		DPRINTF("    new  insn = 0x%08x\n", *insn);
		break;
	/*   R_TYPE(GPREL16) */
	/*   R_TYPE(LITERAL) */
	/*   R_TYPE(GOT16) */
	/*   R_TYPE(PC16) */
	/*   R_TYPE(CALL16) */
#ifdef _LP64
	/*   R_TYPE(GPREL32) */
	/*   R_TYPE(SHIFT5) */
	/*   R_TYPE(SHIFT6) */
	case R_TYPE(64):	/* S + A */
		DPRINTF("    reloc R_MIPS_64\n");
		error = kobj_sym_lookup(ko, symidx, &addr);
		if (error)
			return -1;

		addr += addend;
		DPRINTF("    orig = 0x%016lx\n", *where);
		*where = addr;
		DPRINTF("    new  = 0x%016lx\n", *where);
		break;
	/*   R_TYPE(GOT_DISP) */
	/*   R_TYPE(GOT_PAGE) */
	/*   R_TYPE(GOT_OFST) */
	/*   R_TYPE(GOT_HI16) */
	/*   R_TYPE(GOT_LO16) */
	/*   R_TYPE(SUB) */
	/*   R_TYPE(INSERT_A) */
	/*   R_TYPE(INSERT_B) */
	/*   R_TYPE(DELETE) */
	case R_TYPE(HIGHER):	/* %higher(A + S) =
			(((long long)x + 0x80008000LL) >> 32) & 0xffff */
		DPRINTF("    reloc R_MIPS_HIGHER\n");
		error = kobj_sym_lookup(ko, symidx, &addr);
		if (error)
			return -1;

		addr += addend;
		KASSERT((*insn & 0xffff) == 0);
		DPRINTF("    orig insn = 0x%08x\n", *insn);
		DPRINTF("    HIGHER(%#lx) = 0x%04llx\n", addr, RELOC_HIGHER(addr));
		*insn |= RELOC_HIGHER(addr);
		DPRINTF("    new  insn = 0x%08x\n", *insn);
		break;
	case R_TYPE(HIGHEST):	/* %highest(A + S) =
			(((long long)x + 0x800080008000LL) >> 48) & 0xffff */
		DPRINTF("    reloc R_MIPS_HIGHEST\n");
		error = kobj_sym_lookup(ko, symidx, &addr);
		if (error)
			return -1;

		addr += addend;
		KASSERT((*insn & 0xffff) == 0);
		DPRINTF("    orig insn = 0x%08x\n", *insn);
		DPRINTF("    HIGHEST(%#lx) = 0x%04llx\n", addr, RELOC_HIGHEST(addr));
		*insn |= RELOC_HIGHEST(addr);
		DPRINTF("    new  insn = 0x%08x\n", *insn);
		break;
	/*   R_TYPE(CALL_HI16) */
	/*   R_TYPE(CALL_LO16) */
	/*   R_TYPE(SCN_DISP) */
	/*   R_TYPE(REL16) */
	/*   R_TYPE(ADD_IMMEDIATE) */
	/*   R_TYPE(PJUMP) */
	/*   R_TYPE(RELGOT) */
	/*   R_TYPE(JALR) */
#endif /* _LP64 */
	default:
		printf("%s: unknown reloc type %d @ %p\n",
		    __func__, rtype, where);
		return -1;
	}

	return 0;
}

static void
kobj_idcache_wbinv_all(void)
{

	mips_icache_sync_all();
	mips_dcache_wbinv_all();	/* XXX needed? */
}

int
kobj_machdep(kobj_t ko, void *base, size_t size, bool load)
{

	uint64_t where;

	DPRINTF("%s(kobj %p, base %p,\n    size %zu, load %d)\n",
	    __func__, ko, base, size, load);
	if (load) {
		if (cold) {
			kobj_idcache_wbinv_all();
		} else {
			where = xc_broadcast(0,
			    (xcfunc_t)kobj_idcache_wbinv_all, NULL, NULL);
			xc_wait(where);
		}
	}

	return 0;
}
