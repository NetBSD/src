/*	$NetBSD: kobj_machdep.c,v 1.3.22.2 2017/12/03 11:35:51 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: kobj_machdep.c,v 1.3.22.2 2017/12/03 11:35:51 jdolecek Exp $");

#define	ELFSIZE		ARCH_ELFSIZE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kobj.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <sys/kmem.h>
#include <sys/ksyms.h>
#include <sys/kobj_impl.h>

#include <arm/cpufunc.h>
#include <arm/locore.h>

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
	int error;

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
		error = kobj_sym_lookup(ko, symidx, &addr);
		if (error)
			break;
		*where = addr + addend;
		return 0;

	case R_ARM_COPY:	/* none */
		/* There shouldn't be copy relocations in kernel objects. */
		break;

	case R_ARM_JUMP_SLOT:
		error = kobj_sym_lookup(ko, symidx, &addr);
		if (error)
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
		error = kobj_sym_lookup(ko, symidx, &addr);
		if (error)
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

		error = kobj_sym_lookup(ko, symidx, &addr);
		if (error)
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
		error = kobj_sym_lookup(ko, symidx, &addr);
		if (error)
			break;

		addend += (uintptr_t)addr - (uintptr_t)where;
		*where = addend;
		return 0;

	case R_ARM_PREL31:	/* ((S + A) | T) -  P */
		/* Sign extend if necessary */
		if (addend & 0x40000000)
			addend |= 0xc0000000;
		/* T = 0 for now */
		error = kobj_sym_lookup(ko, symidx, &addr);
		if (error)
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

#if __ARMEB__

enum be8_magic_sym_type {
	Other, ArmStart, ThumbStart, DataStart
};

struct be8_marker {
	enum be8_magic_sym_type type;
	void *addr;
};

struct be8_marker_list {
	size_t cnt;
	struct be8_marker *markers;
};

/*
 * See ELF for the ARM Architecture, Section 4.5.5: Mapping Symbols
 * ARM reserves $a/$d/$t (and variants like $a.2) to mark start of
 * arm/thumb code sections to allow conversion from ARM32-EB to -BE8
 * format.
 */
static enum be8_magic_sym_type
be8_sym_type(const char *name, int info)
{
	if (ELF_ST_BIND(info) != STB_LOCAL)
		return Other;
	if (ELF_ST_TYPE(info) != STT_NOTYPE)
		return Other;
	if (name[0] != '$' || name[1] == '\0' ||
	    (name[2] != '\0' && name[2] != '.'))
		return Other;

	switch (name[1]) {
	case 'a':
		return ArmStart;
	case 'd':
		return DataStart;
	case 't':
		return ThumbStart;
	default:
		return Other;
	}
}

static int
be8_ksym_count(const char *name, int symindex, void *value, uint32_t size,
	int info, void *cookie)
{
	size_t *res = cookie;
	enum be8_magic_sym_type t = be8_sym_type(name, info);

	if (t != Other)
		(*res)++;
	return 0;
}

static int
be8_ksym_add(const char *name, int symindex, void *value, uint32_t size,
	int info, void *cookie)
{
	size_t ndx;
	struct be8_marker_list *list = cookie;
	enum be8_magic_sym_type t = be8_sym_type(name, info);

	if (t == Other)
		return 0;

	ndx = list->cnt++;
	list->markers[ndx].type = t;
	list->markers[ndx].addr = value;

	return 0;
}

static int
be8_ksym_comp(const void *a, const void *b)
{
	const struct be8_marker *ma = a, *mb = b;
	uintptr_t va = (uintptr_t)ma->addr, vb = (uintptr_t)mb->addr;

	if (va == vb)
		return 0;
	if (va < vb)
		return -1;
	return 1;
}

static void
be8_ksym_swap(void *start, size_t size, const struct be8_marker_list *list)
{
	uintptr_t va_end = (uintptr_t)start + size;
	size_t i;
	uint32_t *p32, *p32_end, v32;
	uint16_t *p16, *p16_end, v16;

	/* find first relevant list entry */
	for (i = 0; i < list->cnt; i++)
		if (start <= list->markers[i].addr)
			break;

	/* swap all arm and thumb code parts of this section */
	for ( ; i < list->cnt; i++) {
		switch (list->markers[i].type) {
		case ArmStart:
			p32 = (uint32_t*)list->markers[i].addr;
			p32_end = (uint32_t*)va_end;
			if (i+1 < list->cnt) {
				if ((uintptr_t)list->markers[i+1].addr
				    < va_end)
					p32_end = (uint32_t*)
						list->markers[i+1].addr;
			}
			while (p32 < p32_end) {
				v32 = bswap32(*p32);
				*p32++ = v32;
			}
			break;
		case ThumbStart:
			p16 = (uint16_t*)list->markers[i].addr;
			p16_end = (uint16_t*)va_end;
			if (i+1 < list->cnt) {
				if ((uintptr_t)list->markers[i+1].addr
				    < va_end)
					p16_end = (uint16_t*)
						list->markers[i+1].addr;
			}
			while (p16 < p16_end) {
				v16 = bswap16(*p16);
				*p16++ = v16;
			}
			break;
		default:
			break;
		}
	}
}
 
static void
kobj_be8_fixup(kobj_t ko)
{
	size_t relsym_cnt = 0, i, msize;
	struct be8_marker_list list;
	struct be8_marker tmp;

	/*
	 * Count all special relocations symbols
	 */
	ksyms_mod_foreach(ko->ko_name, be8_ksym_count, &relsym_cnt);

	/*
	 * Provide storage for the address list and add the symbols
	 */
	list.cnt = 0;
	msize = relsym_cnt*sizeof(*list.markers);
	list.markers = kmem_alloc(msize, KM_SLEEP);
	ksyms_mod_foreach(ko->ko_name, be8_ksym_add, &list);
	KASSERT(list.cnt == relsym_cnt);

	/*
	 * Sort symbols by ascending address
	 */
	if (kheapsort(list.markers, relsym_cnt, sizeof(*list.markers),
	    be8_ksym_comp, &tmp) != 0)
		panic("could not sort be8 marker symbols");

	/*
	 * Apply swaps to the .text section (XXX we do not have the
	 * section header available any more, it has been jetisoned
	 * already, so we can not check for all PROGBIT sections).
	 */
	for (i = 0; i < ko->ko_nprogtab; i++) {
		if (strcmp(ko->ko_progtab[i].name, ".text") != 0)
			continue;
		be8_ksym_swap(ko->ko_progtab[i].addr,
		    (size_t)ko->ko_progtab[i].size,
		    &list);
	}

	/*
	 * Done, free list
	 */
	kmem_free(list.markers, msize);
}
#endif

int
kobj_machdep(kobj_t ko, void *base, size_t size, bool load)
{

	if (load) {
#if __ARMEB__
		if (CPU_IS_ARMV7_P() && base == (void*)ko->ko_text_address)
			kobj_be8_fixup(ko);
#endif
#ifndef _RUMPKERNEL
		cpu_idcache_wbinv_range((vaddr_t)base, size);
		cpu_tlb_flushID();
#endif
	}

	return 0;
}
