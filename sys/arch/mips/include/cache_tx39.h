/*	$NetBSD: cache_tx39.h,v 1.2.4.3 2002/03/16 15:58:34 jdolecek Exp $	*/

/*-
 * Copyright (c) 1999, 2000, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi; and by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * Cache definitions/operations for TX3900-style caches.
 *
 * XXX THIS IS NOT YET COMPLETE.
 */

#define	CACHE_TX39_I			0
#define	CACHE_TX39_D			1

#define	CACHEOP_TX3900_INDEX_INV	(0 << 2)	/* I */
#define	CACHEOP_TX3900_ILRUC		(1 << 2)	/* I, D */
#define	CACHEOP_TX3900_ILCKC		(2 << 2)	/* D */
#define	CACHEOP_TX3900_HIT_INV		(4 << 2)	/* D */

#define	CACHEOP_TX3920_INDEX_INV	CACHEOP_TX3900_INDEX_INV
#define	CACHEOP_TX3920_INDEX_WB_INV	(0 << 2)	/* D */
#define	CACHEOP_TX3920_ILRUC		CACHEOP_TX3900_ILRUC
#define	CACHEOP_TX3920_INDEX_LOAD_TAG	(3 << 2)	/* I, D */
#define	CACHEOP_TX3920_HIT_INV		(4 << 2)	/* I, D */
#define	CACHEOP_TX3920_HIT_WB_INV	(5 << 2)	/* D */
#define	CACHEOP_TX3920_HIT_WB		(6 << 2)	/* D */
#define	CACHEOP_TX3920_ISTTAG		(7 << 2)	/* I, D */

#if defined(_KERNEL) && !defined(_LOCORE)

/*
 * cache_tx39_op_line:
 *
 *	Perform the specified cache operation on a single line.
 */
#define	cache_op_tx39_line(va, op)					\
do {									\
	__asm __volatile(						\
		".set noreorder					\n\t"	\
		".set push					\n\t"	\
		".set mips3					\n\t"	\
		"cache %1, 0(%0)				\n\t"	\
		".set pop					\n\t"	\
		".set reorder"						\
	    :								\
	    : "r" (va), "i" (op)					\
	    : "memory");						\
} while (/*CONSTCOND*/0)

/*
 * cache_tx39_op_32lines_4:
 *
 *	Perform the specified cache operation on 32 4-byte
 *	cache lines.
 */
#define	cache_tx39_op_32lines_4(va, op)					\
do {									\
	__asm __volatile(						\
		".set noreorder					\n\t"	\
		".set push					\n\t"	\
		".set mips3					\n\t"	\
		"cache %1, 0x00(%0); cache %1, 0x04(%0);	\n\t"	\
		"cache %1, 0x08(%0); cache %1, 0x0c(%0);	\n\t"	\
		"cache %1, 0x10(%0); cache %1, 0x14(%0);	\n\t"	\
		"cache %1, 0x18(%0); cache %1, 0x1c(%0);	\n\t"	\
		"cache %1, 0x20(%0); cache %1, 0x24(%0);	\n\t"	\
		"cache %1, 0x28(%0); cache %1, 0x2c(%0);	\n\t"	\
		"cache %1, 0x30(%0); cache %1, 0x34(%0);	\n\t"	\
		"cache %1, 0x38(%0); cache %1, 0x3c(%0);	\n\t"	\
		"cache %1, 0x40(%0); cache %1, 0x44(%0);	\n\t"	\
		"cache %1, 0x48(%0); cache %1, 0x4c(%0);	\n\t"	\
		"cache %1, 0x50(%0); cache %1, 0x54(%0);	\n\t"	\
		"cache %1, 0x58(%0); cache %1, 0x5c(%0);	\n\t"	\
		"cache %1, 0x60(%0); cache %1, 0x64(%0);	\n\t"	\
		"cache %1, 0x68(%0); cache %1, 0x6c(%0);	\n\t"	\
		"cache %1, 0x70(%0); cache %1, 0x74(%0);	\n\t"	\
		"cache %1, 0x78(%0); cache %1, 0x7c(%0);	\n\t"	\
		".set pop					\n\t"	\
		".set reorder"						\
	    :								\
	    : "r" (va), "i" (op)					\
	    : "memory");						\
} while (/*CONSTCOND*/0)

/*
 * cache_tx39_op_32lines_16:
 *
 *	Perform the specified cache operation on 32 16-byte
 *	cache lines.
 */
#define	cache_tx39_op_32lines_16(va, op)				\
do {									\
	__asm __volatile(						\
		".set noreorder					\n\t"	\
		".set push					\n\t"	\
		".set mips3					\n\t"	\
		"cache %1, 0x000(%0); cache %1, 0x010(%0);	\n\t"	\
		"cache %1, 0x020(%0); cache %1, 0x030(%0);	\n\t"	\
		"cache %1, 0x040(%0); cache %1, 0x050(%0);	\n\t"	\
		"cache %1, 0x060(%0); cache %1, 0x070(%0);	\n\t"	\
		"cache %1, 0x080(%0); cache %1, 0x090(%0);	\n\t"	\
		"cache %1, 0x0a0(%0); cache %1, 0x0b0(%0);	\n\t"	\
		"cache %1, 0x0c0(%0); cache %1, 0x0d0(%0);	\n\t"	\
		"cache %1, 0x0e0(%0); cache %1, 0x0f0(%0);	\n\t"	\
		"cache %1, 0x100(%0); cache %1, 0x110(%0);	\n\t"	\
		"cache %1, 0x120(%0); cache %1, 0x130(%0);	\n\t"	\
		"cache %1, 0x140(%0); cache %1, 0x150(%0);	\n\t"	\
		"cache %1, 0x160(%0); cache %1, 0x170(%0);	\n\t"	\
		"cache %1, 0x180(%0); cache %1, 0x190(%0);	\n\t"	\
		"cache %1, 0x1a0(%0); cache %1, 0x1b0(%0);	\n\t"	\
		"cache %1, 0x1c0(%0); cache %1, 0x1d0(%0);	\n\t"	\
		"cache %1, 0x1e0(%0); cache %1, 0x1f0(%0);	\n\t"	\
		".set pop					\n\t"	\
		".set reorder"						\
	    :								\
	    : "r" (va), "i" (op)					\
	    : "memory");						\
} while (/*CONSTCOND*/0)

void	tx3900_icache_sync_all_16(void);
void	tx3900_icache_sync_range_16(vaddr_t, vsize_t);

void	tx3900_pdcache_wbinv_all_4(void);

void	tx3900_pdcache_inv_range_4(vaddr_t, vsize_t);
void	tx3900_pdcache_wb_range_4(vaddr_t, vsize_t);

void	tx3920_icache_sync_all_16wb(void);
void	tx3920_icache_sync_range_16wt(vaddr_t, vsize_t);
void	tx3920_icache_sync_range_16wb(vaddr_t, vsize_t);

void	tx3920_pdcache_wbinv_all_16wt(void);
void	tx3920_pdcache_wbinv_all_16wb(void);
void	tx3920_pdcache_wbinv_range_16wb(vaddr_t, vsize_t);

void	tx3920_pdcache_inv_range_16(vaddr_t, vsize_t);
void	tx3920_pdcache_wb_range_16wt(vaddr_t, vsize_t);
void	tx3920_pdcache_wb_range_16wb(vaddr_t, vsize_t);

void	tx3900_icache_do_inv_index_16(vaddr_t, vsize_t);
void	tx3920_icache_do_inv_16(vaddr_t, vsize_t);

#endif /* _KERNEL && !_LOCORE */
