/*	$NetBSD: kobj_machdep.c,v 1.5.12.2 2017/12/03 11:36:38 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: kobj_machdep.c,v 1.5.12.2 2017/12/03 11:36:38 jdolecek Exp $");

#define	ELFSIZE		ARCH_ELFSIZE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kobj.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>

int
kobj_reloc(kobj_t ko, uintptr_t relocbase, const void *data,
	   bool isrela, bool local)
{
#ifdef _LP64
	Elf64_Half *wwhere;
#endif
	Elf_Addr *where;
	Elf32_Half *hwhere;
	Elf_Addr addr;
	Elf_Sword addend;		/* needs to be signed */
	u_int rtype, symidx;
	const Elf_Rela *rela;
	int error;

	if (!isrela) {
		panic("kobj_reloc: REL relocations not supported");
	}

	rela = (const Elf_Rela *)data;
	where = (Elf_Addr *) (relocbase + rela->r_offset);
	hwhere = (Elf32_Half *) (relocbase + rela->r_offset);
#ifdef _LP64
	wwhere = (Elf64_Half *) (relocbase + rela->r_offset);
#endif
	addend = rela->r_addend;
	rtype = ELF_R_TYPE(rela->r_info);
	symidx = ELF_R_SYM(rela->r_info);

	switch (rtype) {
	case R_PPC_NONE:
	       	break;

	case R_PPC_RELATIVE:	/* word32 B + A */
		addend += relocbase;			/* A += B */
	       	break;

	case R_PPC_REL32:	/* word32 S + A - P */
	case R_PPC_REL16:	/* half16* (S + A - P) */
	case R_PPC_REL16_LO:	/* half16 #lo(S + A - P) */
	case R_PPC_REL16_HI:	/* half16 #hi(S + A - P) */
	case R_PPC_REL16_HA:	/* half16 #ha(S + A - P) */
		addend -= relocbase + rela->r_offset;	/* A -= P */
		/* FALLTHROUGH */

#ifdef _LP64
	case R_PPC_ADDR64:	/* doubleword64 S + A */
#endif
	case R_PPC_ADDR32:	/* word32 S + A */
	case R_PPC_ADDR16:	/* half16* S + A */
	case R_PPC_ADDR16_LO:	/* half16 #lo(S + A) */
	case R_PPC_ADDR16_HA:	/* half16 #ha(S + A) */
	case R_PPC_ADDR16_HI:	/* half16 #hi(S + A) */
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

	default:
		printf("kobj_reloc: unexpected relocation type %u\n", rtype);
		return -1;
	}


	switch (rtype) {
	case R_PPC_REL32:	/* word32 S + A - P */
#ifdef _LP64
		*wwhere = addend;
		break;
#endif
	case R_PPC_RELATIVE:	/* doubleword64/word32 B + A */
	case R_PPC_ADDR32:	/* doubleword64/word32 S + A */
	       	*where = addend;
	       	break;

	case R_PPC_REL16:	/* half16* (S + A - P) */
	case R_PPC_ADDR16:	/* half16* S + A */
		if ((int16_t) addend != addend)
			return -1;
		/* FALLTHROUGH */
	case R_PPC_REL16_LO:	/* half16 #lo(S + A - P) */
	case R_PPC_ADDR16_LO:	/* half16 #lo(S + A) */
		*hwhere = addend & 0xffff;
		break;

	case R_PPC_REL16_HA:	/* half16 #ha(S + A - P) */
	case R_PPC_ADDR16_HA:	/* half16 #ha(S + A) */
		addend += 0x8000;
		/* FALLTHROUGH */
	case R_PPC_REL16_HI:	/* half16 #hi(S + A - P) */
	case R_PPC_ADDR16_HI:	/* half16 #hi(S + A) */
		*hwhere = (addend >> 16) & 0xffff;
		break;
	}

	return 0;
}

int
kobj_machdep(kobj_t ko, void *base, size_t size, bool load)
{
	if (load)
		__syncicache(base, size);

	return 0;
}
