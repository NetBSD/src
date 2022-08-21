/*	$NetBSD: prekern.h,v 1.25 2022/08/21 14:05:52 mlelstv Exp $	*/

/*
 * Copyright (c) 2017-2020 The NetBSD Foundation, Inc. All rights reserved.
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
#include <lib/libkern/libkern.h>

#include <machine/pmap.h>
#include <machine/pte.h>

#include <x86/bootspace.h>

#include "pdir.h"
#include "redef.h"

#define ASSERT(a) if (!(a)) fatal("ASSERT: " #a);
typedef uint64_t pte_prot_t;
#define WHITE_ON_BLACK 0x07
#define RED_ON_BLACK 0x04
#define GREEN_ON_BLACK 0x02
#define YELLOW_ON_BLACK 0x0e

#define HEAD_WINDOW_BASE	(KERNBASE - NBPD_L3)
#define HEAD_WINDOW_SIZE	NBPD_L3

#define KASLR_WINDOW_BASE	KERNBASE		/* max - 2GB */
#define KASLR_WINDOW_SIZE	(2LLU * (1 << 30))	/* 2GB */

typedef enum
{
	STATE_NORMAL = 0,
	STATE_ERROR,
	STATE_WARNING
} state_t;

/* -------------------------------------------------------------------------- */

/* console.c */
void init_cons(void);
void print_ext(int, char *);
void print(char *);
void print_state(state_t, char *);
void print_banner(void);

/* elf.c */
size_t elf_get_head_size(vaddr_t);
void elf_build_head(vaddr_t);
void elf_fixup_boot(vaddr_t, paddr_t);
void elf_map_sections(void);
void elf_build_info(void);
vaddr_t elf_kernel_reloc(void);

/* locore.S */
void cpuid(uint32_t, uint32_t, uint32_t *);
void lidt(void *);
uint64_t rdtsc(void);
int rdseed(uint64_t *);
int rdrand(uint64_t *);
void jump_kernel(vaddr_t);

/* mm.c */
void mm_init(paddr_t);
void mm_bootspace_mprotect(void);
vaddr_t mm_map_segment(int, paddr_t, size_t, size_t);
void mm_map_kernel(void);

/* prekern.c */
void fatal(char *);

/* prng.c */
void prng_init(void);
void prng_get_rand(void *, size_t);
