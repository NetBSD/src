/*	$NetBSD: wired_map.h,v 1.2.28.1 2007/02/27 16:52:02 yamt Exp $	*/

/*-
 * Copyright (c) 2005 Tadpole Computer Inc.
 * All rights reserved.
 *
 * Written by Garrett D'Amore for Tadpole Computer Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Tadpole Computer Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TADPOLE COMPUTER INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL TADPOLE COMPUTER INC.
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _MIPS_WIRED_MAP_H
#define _MIPS_WIRED_MAP_H

/*
 * Certain machines have peripheral busses which are only accessible
 * using the TLB.
 *
 * For example, certain Alchemy parts place PCI and PCMCIA busses at
 * physical address spaces which are beyond the normal 32-bit range.
 * In order to access these spaces TLB entries mapping 36-bit physical
 * addresses to 32-bit logical addresses must be used.
 *
 * Note that all wired mappings are must be 32 MB aligned.  This is
 * because we use 32 MB mappings in the TLB.  Changing this might get
 * us more effficent use of the address space, but it would greatly
 * complicate the code, and would also probably consume additional TLB
 * entries.
 *
 * Note that within a single 32 MB region, you can have multiple
 * decoders, but they must decode uniquely within the same 32MB of
 * physical address space.
 *
 * BEWARE: The start of KSEG2 (0xC0000000) is used by the NetBSD kernel
 * for context switching and is associated with wired entry 0.  So you
 * cannot use that, as I discovered the hard way.
 *
 * Note also that at the moment this is not supported on the MIPS-I
 * ISA (but it shouldn't need it anyway.)
 */

#ifndef MIPS3_WIRED_SIZE
#define MIPS3_WIRED_SIZE	MIPS3_PG_SIZE_MASK_TO_SIZE(MIPS3_PG_SIZE_16M)
#endif
#define MIPS3_WIRED_OFFMASK	(MIPS3_WIRED_SIZE - 1)

#define MIPS3_WIRED_ENTRY_SIZE(pgsize)	((pgsize) * 2)
#define MIPS3_WIRED_ENTRY_OFFMASK(pgsize) (MIPS3_WIRED_ENTRY_SIZE(pgsize) - 1)

/*
 * This defines the maximum number of wired TLB entries that the wired
 * map will be allowed to consume.  It can (and probably will!)
 * consume fewer, but it will not consume more.  Note that NetBSD also
 * uses one wired entry for context switching (see TLB_WIRED_UPAGES),
 * and that is not included in this number.
 */
#ifndef MIPS3_NWIRED_ENTRY
#define MIPS3_NWIRED_ENTRY	8	/* upper limit */
#endif

struct wired_map_entry {
	paddr_t	pa0;
	paddr_t	pa1;
	vaddr_t	va;
	vsize_t	pgmask;
};

extern struct wired_map_entry mips3_wired_map[];
extern int mips3_nwired_page;

/*
 * Wire down a region of the specified size.
 */
bool	mips3_wired_enter_region(vaddr_t, paddr_t, vsize_t);

/*
 * Wire down a single page using specified page size.
 */
bool	mips3_wired_enter_page(vaddr_t, paddr_t, vsize_t);

#endif	/* _MIPS_WIRED_MAP_H */
