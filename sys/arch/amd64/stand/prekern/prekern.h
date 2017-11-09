/*	$NetBSD: prekern.h,v 1.6 2017/11/09 15:56:56 maxv Exp $	*/

/*
 * Copyright (c) 2017 The NetBSD Foundation, Inc. All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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
#include <sys/param.h>
#include <sys/stdbool.h>
#include <machine/pte.h>

#include "pdir.h"
#include "redef.h"

#define MM_PROT_READ	0x00
#define MM_PROT_WRITE	0x01
#define MM_PROT_EXECUTE	0x02

#define ASSERT(a) if (!(a)) fatal("ASSERT");
#define memcpy(d, v, l) __builtin_memcpy(d, v, l)
typedef uint64_t paddr_t;
typedef uint64_t vaddr_t;
typedef uint64_t pt_entry_t;
typedef uint64_t pd_entry_t;
typedef uint64_t pte_prot_t;
#define WHITE_ON_BLACK 0x07
#define RED_ON_BLACK 0x04
#define GREEN_ON_BLACK 0x02

#define HEAD_WINDOW_BASE	(KERNBASE - NBPD_L3)
#define HEAD_WINDOW_SIZE	NBPD_L3

#define KASLR_WINDOW_BASE	KERNBASE		/* max - 2GB */
#define KASLR_WINDOW_SIZE	(2LLU * (1 << 30))	/* 2GB */

/* -------------------------------------------------------------------------- */

static inline void
memset(void *dst, char c, size_t sz)
{
	char *bdst = dst;
	while (sz > 0) {
		*bdst = c;
		bdst++, sz--;
	}
}

static inline int
memcmp(const char *a, const char *b, size_t c)
{
	size_t i;
	for (i = 0; i < c; i++) {
		if (a[i] != b[i])
			return 1;
	}
	return 0;
}

static inline int
strcmp(char *a, char *b)
{
	size_t i;
	for (i = 0; a[i] != '\0'; i++) {
		if (a[i] != b[i])
			return 1;
	}
	return 0;
}

/* -------------------------------------------------------------------------- */

struct bootspace {
	struct {
		vaddr_t va;
		paddr_t pa;
		size_t sz;
	} head;
	struct {
		vaddr_t va;
		paddr_t pa;
		size_t sz;
	} text;
	struct {
		vaddr_t va;
		paddr_t pa;
		size_t sz;
	} rodata;
	struct {
		vaddr_t va;
		paddr_t pa;
		size_t sz;
	} data;
	struct {
		vaddr_t va;
		paddr_t pa;
		size_t sz;
	} boot;
	vaddr_t spareva;
	vaddr_t pdir;
	vaddr_t emodule;
};

/* console.c */
void init_cons();
void print_ext(int, char *);
void print(char *);
void print_state(bool, char *);
void print_banner();

/* elf.c */
size_t elf_get_head_size(vaddr_t);
void elf_build_head(vaddr_t);
void elf_get_text(paddr_t *, size_t *);
void elf_build_text(vaddr_t, paddr_t);
void elf_get_rodata(paddr_t *, size_t *);
void elf_build_rodata(vaddr_t, paddr_t);
void elf_get_data(paddr_t *, size_t *);
void elf_build_data(vaddr_t, paddr_t);
void elf_build_boot(vaddr_t, paddr_t);
vaddr_t elf_kernel_reloc();

/* locore.S */
void lidt(void *);
uint64_t rdtsc();
void jump_kernel();

/* mm.c */
void mm_init(paddr_t);
paddr_t mm_vatopa(vaddr_t);
void mm_bootspace_mprotect();
void mm_map_kernel();

/* prekern.c */
void fatal(char *);
