/*	$NetBSD: jazzdmatlbvar.h,v 1.1 2000/06/09 05:22:23 soda Exp $	*/

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

void	jazz_dmatlb_init __P((bus_space_tag_t iot, bus_addr_t ioaddr));
jazz_dma_pte_t *jazz_dmatlb_alloc __P((int npte, bus_size_t boundary,
	    int flags, bus_addr_t *addr));
void	jazz_dmatlb_free __P((bus_addr_t addr, int npte));
void	jazz_dmatlb_map_va __P((struct proc *p, vaddr_t va, vsize_t size,
	    jazz_dma_pte_t *dma_pte));
void	jazz_dmatlb_map_pa __P((paddr_t pa, psize_t size,
	    jazz_dma_pte_t *dma_pte));
void	jazz_dmatlb_flush __P((void));

