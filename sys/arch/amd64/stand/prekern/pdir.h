/*	$NetBSD: pdir.h,v 1.2 2017/11/05 16:27:18 maxv Exp $	*/

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

#define PREKERNBASE		0x0
#define PREKERNTEXTOFF	(PREKERNBASE + 0x100000)

#define L4_SLOT_PREKERN	0 /* pl4_i(PREKERNBASE) */
#define L4_SLOT_PTE		255

#define PDIR_SLOT_KERN	L4_SLOT_PREKERN
#define PDIR_SLOT_PTE	L4_SLOT_PTE

#define PTE_BASE	((pt_entry_t *)(L4_SLOT_PTE * NBPD_L4))

#define L1_BASE	PTE_BASE
#define L2_BASE	((pd_entry_t *)((char *)L1_BASE + L4_SLOT_PTE * NBPD_L3))
#define L3_BASE	((pd_entry_t *)((char *)L2_BASE + L4_SLOT_PTE * NBPD_L2))
#define L4_BASE	((pd_entry_t *)((char *)L3_BASE + L4_SLOT_PTE * NBPD_L1))

#define NKL4_KIMG_ENTRIES	1
#define NKL3_KIMG_ENTRIES	1
#define NKL2_KIMG_ENTRIES	32

/*
 * Now we define various constants for playing with virtual addresses.
 */
#define L1_SHIFT	12
#define L2_SHIFT	21
#define L3_SHIFT	30
#define L4_SHIFT	39
#define NBPD_L1		(1UL << L1_SHIFT) /* # bytes mapped by L1 ent (4K) */
#define NBPD_L2		(1UL << L2_SHIFT) /* # bytes mapped by L2 ent (2MB) */
#define NBPD_L3		(1UL << L3_SHIFT) /* # bytes mapped by L3 ent (1G) */
#define NBPD_L4		(1UL << L4_SHIFT) /* # bytes mapped by L4 ent (512G) */

#define L4_MASK		0x0000ff8000000000
#define L3_MASK		0x0000007fc0000000
#define L2_MASK		0x000000003fe00000
#define L1_MASK		0x00000000001ff000

#define L4_FRAME	L4_MASK
#define L3_FRAME	(L4_FRAME|L3_MASK)
#define L2_FRAME	(L3_FRAME|L2_MASK)
#define L1_FRAME	(L2_FRAME|L1_MASK)

/*
 * Mask to get rid of the sign-extended part of addresses.
 */
#define VA_SIGN_MASK		0xffff000000000000
#define VA_SIGN_NEG(va)		((va) | VA_SIGN_MASK)
/* XXXfvdl this one's not right */
#define VA_SIGN_POS(va)		((va) & ~VA_SIGN_MASK)

/*
 * pl*_i: generate index into pde/pte arrays in virtual space
 *
 * pl_i(va, X) == plX_i(va) <= pl_i_roundup(va, X)
 */
#define pl1_i(VA)	(((VA_SIGN_POS(VA)) & L1_FRAME) >> L1_SHIFT)
#define pl2_i(VA)	(((VA_SIGN_POS(VA)) & L2_FRAME) >> L2_SHIFT)
#define pl3_i(VA)	(((VA_SIGN_POS(VA)) & L3_FRAME) >> L3_SHIFT)
#define pl4_i(VA)	(((VA_SIGN_POS(VA)) & L4_FRAME) >> L4_SHIFT)

