/*	$NetBSD: kobj_machdep.c,v 1.3.22.1 2014/08/20 00:02:45 tls Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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

/*-
 * Copyright 1996-1998 John D. Polstra.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kobj_machdep.c,v 1.3.22.1 2014/08/20 00:02:45 tls Exp $");

#define	ELFSIZE		ARCH_ELFSIZE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kobj.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>

#include <arm/cpufunc.h>

int
kobj_reloc(kobj_t ko, uintptr_t relocbase, const void *data,
	   bool isrela, bool local)
{
	Elf_Addr *where;
	Elf_Addr addr;
	Elf_Addr addend;
	Elf_Word rtype, symidx;
	const Elf_Rel *rel;
	const Elf_Rela *rela;

	if (isrela) {
		rela = (const Elf_Rela *)data;
		where = (Elf_Addr *) (relocbase + rela->r_offset);
		addend = rela->r_addend;
		rtype = ELF_R_TYPE(rela->r_info);
		symidx = ELF_R_SYM(rela->r_info);
	} else {
		rel = (const Elf_Rel *)data;
		where = (Elf_Addr *) (relocbase + rel->r_offset);
		addend = *where;
		rtype = ELF_R_TYPE(rel->r_info);
		symidx = ELF_R_SYM(rel->r_info);
	}

	switch (rtype) {
	case R_ARM_NONE:	/* none */
	case R_ARM_V4BX:	/* none */
		return 0;

	case R_ARM_ABS32:
		addr = kobj_sym_lookup(ko, symidx);
		if (addr == 0)
			break;
		*where = addr + addend;
		return 0;

	case R_ARM_COPY:	/* none */
		/* There shouldn't be copy relocations in kernel objects. */
		break;

	case R_ARM_JUMP_SLOT:
		addr = kobj_sym_lookup(ko, symidx);
		if (addr == 0)
			break;
		*where = addr;
		return 0;

	case R_ARM_RELATIVE:	/* A + B */
		addr = relocbase + addend;
		if (*where != addr)
			*where = addr;
		return 0;

	case R_ARM_MOVW_ABS_NC:	/* (S + A) | T */
	case R_ARM_MOVT_ABS:
		if ((*where & 0x0fb00000) != 0x03000000)
			break;
		addr = kobj_sym_lookup(ko, symidx);
		if (addr == 0)
			break;
		if (rtype == R_ARM_MOVT_ABS)
			addr >>= 16;
		*where = (*where & 0xfff0f000)
		    | ((addr << 4) & 0x000f0000) | (addr & 0x00000fff);
		return 0;

	case R_ARM_CALL:	/* ((S + A) | T) -  P */
	case R_ARM_JUMP24:
	case R_ARM_PC24:	/* Deprecated */
		if (local && (*where & 0x00ffffff) != 0x00fffffe)
			return 0;

		/* Remove the instruction from the 24 bit offset */
		addend &= 0x00ffffff;

		/* Sign extend if necessary */
		if (addend & 0x00800000)
			addend |= 0xff000000;

		addend <<= 2;

		addr = kobj_sym_lookup(ko, symidx);
		if (addr == 0)
			break;

		addend += (uintptr_t)addr - (uintptr_t)where;

		if (addend & 3) {
			printf ("Relocation %x unaligned @ %p\n", addend, where);
			return -1;
		}

		if ((addend & 0xfe000000) != 0x00000000 &&
		    (addend & 0xfe000000) != 0xfe000000) {
			printf ("Relocation %x too far @ %p\n", addend, where);
			return -1;
		}
		*where = (*where & 0xff000000) | ((addend >> 2) & 0x00ffffff);
		return 0;

	case R_ARM_REL32:	/* ((S + A) | T) -  P */
		/* T = 0 for now */
		addr = kobj_sym_lookup(ko, symidx);
		if (addr == 0)
			break;

		addend += (uintptr_t)addr - (uintptr_t)where;
		*where = addend;
		return 0;

	case R_ARM_PREL31:	/* ((S + A) | T) -  P */
		/* Sign extend if necessary */
		if (addend & 0x40000000)
			addend |= 0xc0000000;
		/* T = 0 for now */
		addr = kobj_sym_lookup(ko, symidx);
		if (addr == 0)
			break;

		addend += (uintptr_t)addr - (uintptr_t)where;

		if ((addend & 0x80000000) != 0x00000000 &&
		    (addend & 0x80000000) != 0x80000000) {
			printf ("Relocation %x too far @ %p\n", addend, where);
			return -1;
		}

		*where = (*where & 0x80000000) | (addend & 0x7fffffff);

	default:
		break;
	}

	printf("kobj_reloc: unexpected/invalid relocation type %d @ %p symidx %u\n",
	    rtype, where, symidx);
	return -1;
}

int
kobj_machdep(kobj_t ko, void *base, size_t size, bool load)
{

	if (load) {
#ifndef _RUMPKERNEL
		cpu_idcache_wbinv_range((vaddr_t)base, size);
		cpu_tlb_flushID();
#endif
	}

	return 0;
}
