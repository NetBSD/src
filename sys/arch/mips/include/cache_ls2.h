/*	$NetBSD: cache_ls2.h,v 1.2.24.1 2017/12/03 11:36:27 jdolecek Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com>.
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

#ifndef _MIPS_CACHE_LS2_H_
#define _MIPS_CACHE_LS2_H_

/*
 * Cache definitions/operations for Loongson-style caches.
 */
#define	CACHEOP_LS2_I_INDEX_INV		0
#define	CACHEOP_LS2_D_INDEX_WB_INV	1
#define	CACHEOP_LS2_S_INDEX_WB_INV	3
#define	CACHEOP_LS2_D_HIT_INV		17
#define	CACHEOP_LS2_S_HIT_INV		19
#define	CACHEOP_LS2_D_HIT_WB_INV	21
#define	CACHEOP_LS2_S_HIT_WB_INV	23

#if !defined(_LOCORE)
/*
 * The way is encoded in the bottom 2 bits of VA.
 */

#define cache_op_ls2_8line_4way(va, op)					\
	__asm volatile(							\
                ".set noreorder					\n\t"	\
                "cache %1, 0x00(%0); cache %1, 0x20(%0)		\n\t"	\
                "cache %1, 0x40(%0); cache %1, 0x60(%0)		\n\t"	\
                "cache %1, 0x80(%0); cache %1, 0xa0(%0)		\n\t"	\
                "cache %1, 0xc0(%0); cache %1, 0xe0(%0)		\n\t"	\
                "cache %1, 0x01(%0); cache %1, 0x21(%0)		\n\t"	\
                "cache %1, 0x41(%0); cache %1, 0x61(%0)		\n\t"	\
                "cache %1, 0x81(%0); cache %1, 0xa1(%0)		\n\t"	\
                "cache %1, 0xc1(%0); cache %1, 0xe1(%0)		\n\t"	\
                "cache %1, 0x02(%0); cache %1, 0x22(%0)		\n\t"	\
                "cache %1, 0x42(%0); cache %1, 0x62(%0)		\n\t"	\
                "cache %1, 0x82(%0); cache %1, 0xa2(%0)		\n\t"	\
                "cache %1, 0xc2(%0); cache %1, 0xe2(%0)		\n\t"	\
                "cache %1, 0x03(%0); cache %1, 0x23(%0)		\n\t"	\
                "cache %1, 0x43(%0); cache %1, 0x63(%0)		\n\t"	\
                "cache %1, 0x83(%0); cache %1, 0xa3(%0)		\n\t"	\
                "cache %1, 0xc3(%0); cache %1, 0xe3(%0)		\n\t"	\
                ".set reorder"						\
            :								\
            : "r" (va), "i" (op)					\
            : "memory");

#define cache_op_ls2_line_4way(va, op)					\
	__asm volatile(							\
                ".set noreorder					\n\t"	\
                "cache %1, 0(%0); cache %1, 1(%0)		\n\t"	\
                "cache %1, 2(%0); cache %1, 3(%0)		\n\t"	\
                ".set reorder"						\
            :								\
            : "r" (va), "i" (op)					\
            : "memory");

#define cache_op_ls2_8line(va, op)					\
	__asm volatile(							\
                ".set noreorder					\n\t"	\
                "cache %1, 0x00(%0); cache %1, 0x20(%0)		\n\t"	\
                "cache %1, 0x40(%0); cache %1, 0x60(%0)		\n\t"	\
                "cache %1, 0x80(%0); cache %1, 0xa0(%0)		\n\t"	\
                "cache %1, 0xc0(%0); cache %1, 0xe0(%0)		\n\t"	\
                ".set reorder"						\
            :								\
            : "r" (va), "i" (op)					\
            : "memory");

#define cache_op_ls2_line(va, op)					\
	__asm volatile(							\
                ".set noreorder					\n\t"	\
                "cache %1, 0(%0)				\n\t"	\
                ".set reorder"						\
            :								\
            : "r" (va), "i" (op)					\
            : "memory");

void	ls2_icache_sync_all(void);
void	ls2_icache_sync_range(register_t, vsize_t);
void	ls2_icache_sync_range_index(vaddr_t, vsize_t);

void	ls2_pdcache_wbinv_all(void);
void	ls2_pdcache_wbinv_range(register_t, vsize_t);
void	ls2_pdcache_wbinv_range_index(vaddr_t, vsize_t);

void	ls2_pdcache_inv_range(register_t, vsize_t);
void	ls2_pdcache_wb_range(register_t, vsize_t);

void	ls2_sdcache_wbinv_all(void);
void	ls2_sdcache_wbinv_range(register_t, vsize_t);
void	ls2_sdcache_wbinv_range_index(vaddr_t, vsize_t);

void	ls2_sdcache_inv_range(register_t, vsize_t);
void	ls2_sdcache_wb_range(register_t, vsize_t);

#endif /* !_LOCORE */
#endif /* !_MIPS_CACHE_LS2_H_ */
