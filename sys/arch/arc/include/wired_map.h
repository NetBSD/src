/*	$NetBSD: wired_map.h,v 1.2.28.1 2007/02/27 16:49:04 yamt Exp $	*/

/*-
 * Copyright (C) 2000 Shuichiro URATA.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define MIPS3_NWIRED_ENTRY	8
#define MIPS3_WIRED_SIZE	MIPS3_PG_SIZE_MASK_TO_SIZE(MIPS3_PG_SIZE_16M)

#ifndef ARC_THRESHOLD_TO_USE_WIRED_TLB	/* tunable */
#define ARC_THRESHOLD_TO_USE_WIRED_TLB	(256 * 1024)
#endif

#include <mips/wired_map.h>

void	arc_init_wired_map(void);
void	arc_wired_enter_page(vaddr_t, paddr_t, vsize_t);
vaddr_t	arc_contiguously_wired_mapped(paddr_t, vsize_t);
vaddr_t	arc_map_wired(paddr_t, vsize_t);
bool	 arc_wired_map_extract(vaddr_t, paddr_t *);
