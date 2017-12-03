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

__RCSID("$NetBSD: kobj_machdep.c,v 1.2.2.2 2017/12/03 11:36:39 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kobj.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>

#include <riscv/locore.h>

int
kobj_reloc(kobj_t ko, uintptr_t relocbase, const void *data,
       bool isrela, bool local)
{
	if (!isrela) {
		panic("kobj_reloc: REL relocations not supported");
	}

	const Elf_Rela * const rela = (const Elf_Rela *)data;
	Elf_Addr * const where = (Elf_Addr *) (relocbase + rela->r_offset);
#if ELFSIZE == 64
	Elf64_Half * const wwhere = (Elf64_Half *) (relocbase + rela->r_offset);
#else
	Elf_Addr * const wwhere = where;
#endif
	Elf_Sword addend = rela->r_addend;	/* needs to be signed */
	const u_int rtype = ELF_R_TYPE(rela->r_info);
	const u_int symidx = ELF_R_SYM(rela->r_info);
	int error;

	switch (rtype) {
	case R_RISCV_NONE:
	       	break;

	case R_RISCV_PCREL_LO12_I:
		addend -= (intptr_t)where;
		break;

	case R_RISCV_BRANCH:
	case R_RISCV_JAL:
		if (symidx == 0)
			break;

	case R_RISCV_CALL_PLT:
	case R_RISCV_CALL:
	case R_RISCV_PCREL_HI20:
		addend -= (intptr_t)where;
		/* FALLTHOUGH */
#ifdef _LP64
	case R_RISCV_64:	/* doubleword64 S + A */
#endif
	case R_RISCV_LO12_I:
	case R_RISCV_LO12_S: {
		Elf_Addr addr;
		error = kobj_sym_lookup(ko, symidx, &addr);
		if (error)
			return -1;

#if 0
		/*
		 * addend values are sometimes relative to sections
		 * (i.e. .rodata) in rela, where in reality they
		 * are relative to relocbase. Detect this condition.
		 */
		if (addr > relocbase && addr <= (relocbase + addend))
			addr = relocbase + addend;
		else
#endif
		addend += addr;				/* A += S */
		break;
	}

	default:
		printf("kobj_reloc: unexpected relocation type %u\n", rtype);
		return -1;
	}


	switch (rtype) {
	case R_RISCV_64:	/* word32 S + A */
	       	*where = addend;
	       	break;

	case R_RISCV_32:	/* word32 S + A */
	       	*wwhere = addend;
	       	break;

	case R_RISCV_JAL:
		addend = __SHIFTIN(__SHIFTOUT(addend, __BIT(20)), __BIT(31))
		   | __SHIFTIN(__SHIFTOUT(addend, __BITS(10,1)), __BITS(30,21))
		   | __SHIFTIN(__SHIFTOUT(addend, __BIT(11)), __BIT(20))
		   | (addend & __BITS(19,12));
		/* FALLTHROUGH */
	case R_RISCV_HI20:	/* LUI/AUIPC (S + A - P) */
	case R_RISCV_PCREL_HI20:	/* LUI/AUIPC (S + A - P) */
		*wwhere = ((addend + 0x800) & 0xfffff000) & (*wwhere & 0xfff);
		break;
	case R_RISCV_PCREL_LO12_I:
		*wwhere += (addend << 20);
		break;
	case R_RISCV_LO12_I:	/* low12 I-type instructions */
		*wwhere = ((addend & 0xfff) << 20) | (*wwhere & 0xfffff);
		break;
	case R_RISCV_PCREL_LO12_S:
		addend += 
		    __SHIFTIN(__SHIFTOUT(addend, __BITS(31,25)), __BITS(11,5))
		    | __SHIFTIN(__SHIFTOUT(addend, __BITS(11,7)), __BITS(4,0));
		/* FALLTHROUGH */
	case R_RISCV_LO12_S:	/* low12 S-type instructions */
		*wwhere =
		    __SHIFTIN(__SHIFTOUT(addend, __BITS(11,5)), __BITS(31,25))
		    | __SHIFTIN(__SHIFTOUT(addend, __BITS(4,0)), __BITS(11,7))
		    | (*wwhere & (__BITS(24,12)|__BITS(6,0)));
		break;
	case R_RISCV_CALL:
	case R_RISCV_CALL_PLT:
		if (((int32_t)addend >> 20) == 0
		    || ((int32_t)addend >> 20) == -1) {
			// This falls into the range for the JAL instruction.
			addend = __SHIFTIN(__SHIFTOUT(addend, __BIT(20)), __BIT(31))
			    | __SHIFTIN(__SHIFTOUT(addend, __BITS(10,1)), __BITS(30,21))
			    | __SHIFTIN(__SHIFTOUT(addend, __BIT(11)), __BIT(20))
			    | (addend & __BITS(19,12));
			// Convert the AUIPC/JALR to a JAL/NOP
			wwhere[0] = addend | (wwhere[1] & 0xf80) | 0x6f;
			wwhere[1] = 0x00000013;	// NOP (ADDI x0, x0, 0)
			break;
		}
		wwhere[0] = ((addend + 0x800) & 0xfffff000)
		    | (wwhere[0] & 0xfff);
		wwhere[1] = (addend << 20) | (wwhere[1] & 0x000fffff);
		break;
	}

	return 0;
}

int
kobj_machdep(kobj_t ko, void *base, size_t size, bool load)
{
	if (load)
		__asm("fence rw,rw; fence.i");

	return 0;
}
