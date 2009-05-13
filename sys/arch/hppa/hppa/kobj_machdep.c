/*	$NetBSD: kobj_machdep.c,v 1.4.14.1 2009/05/13 17:17:47 jym Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: kobj_machdep.c,v 1.4.14.1 2009/05/13 17:17:47 jym Exp $");

#define	ELFSIZE		ARCH_ELFSIZE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kobj.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>

#include <uvm/uvm_extern.h>

#include <machine/cpufunc.h>
#include <machine/pmap.h>

static inline unsigned int
RND(unsigned int x)
{
        return ((x + 0x1000) & 0xffffe000);
}

static inline unsigned int
R(unsigned int x)
{
        return (x & 0x000007ff);
}

static inline unsigned int
L(unsigned int x)
{
        return (x & 0xfffff800);
}

static inline unsigned int
LR(unsigned int x, unsigned int constant)
{
        return L(x + RND(constant));
}

static inline unsigned int
RR(unsigned int x, unsigned int constant)
{
        return R(x + RND(constant)) + (constant - RND(constant));
}

int
kobj_reloc(kobj_t ko, uintptr_t relocbase, const void *data,
    bool isrela, bool local)
{
	Elf_Addr *where;
	Elf_Addr addr, value;
	Elf_Word rtype, symidx;
	const Elf_Rela *rela;

	if (!isrela) {
		printf("kobj_reloc: only support RELA relocations\n");
		return -1;
	}

	rela = (const Elf_Rela *)data;
	where = (Elf_Addr *) (relocbase + rela->r_offset);
	value = rela->r_addend;
	rtype = ELF_R_TYPE(rela->r_info);
	symidx = ELF_R_SYM(rela->r_info);

	switch (rtype) {
	case R_TYPE(NONE):
		break;

	case R_TYPE(DIR32):
		/* symbol + addend */
		addr = kobj_sym_lookup(ko, symidx);
		value += addr;
		if (*where != value)
			*where = value;
		break;

	case R_TYPE(PLABEL32):
		/* fptr(symbol) */
		addr = kobj_sym_lookup(ko, symidx);
		if (*where != addr)
			*where = addr;
		break;

	case R_TYPE(DIR14R):
		/* RR(symbol, addend) */
		addr = kobj_sym_lookup(ko, symidx);
		value = RR(addr, value);
		*where |=
		     (((value >>  0) & 0x1fff) << 1) |
		     (((value >> 13) & 0x1) << 0);
		break;

	case R_TYPE(DIR21L):
		/* LR(symbol, addend) */
		addr = kobj_sym_lookup(ko, symidx);
		value = LR(addr, value);
		*where |=
		    (((value >> 31) & 0x001) <<  0) |
		    (((value >> 20) & 0x3ff) <<  1) |
		    (((value >> 18) & 0x003) << 14) |
		    (((value >> 13) & 0x01f) << 16) |
		    (((value >> 11) & 0x003) << 12);
		break;

	case R_TYPE(PCREL17F):
		/* symbol - PC - 8 + addend */
		addr = kobj_sym_lookup(ko, symidx);
		value += addr - (Elf_Word)where - 8;
		value >>= 2;		/* bottom two bits not needed */

		*where |=
		    (((value & 0x10000) >> 16) << 0) |		/* w */
		    (((value & 0x0f800) >> 11) << 16) |		/* w1 */
		    (((value & 0x00400) >> 10) << 2) |
		    (((value & 0x003ff) << 1) << 2);		/* w2 */
		break;

#if 0
	case R_TYPE(DPREL17F):
	case R_TYPE(DPREL21L):
		break;
#endif
	case R_TYPE(SEGREL32):
		/* symbol - SB + addend */
		/* XXX SB */
		addr = kobj_sym_lookup(ko, symidx);
		value += addr;
		if (*where != addr)
			*where = addr;
		break;

	default:
		printf("%s: unexpected relocation type %d\n", __func__, rtype);
		return -1;
	}
	return 0;
}

int
kobj_machdep(kobj_t ko, void *base, size_t size, bool load)
{

	if (load)
		fdcache(HPPA_SID_KERNEL, (vaddr_t)base, size);

	return 0;
}
