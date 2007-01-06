/*	$NetBSD: omap_emifs.h,v 1.1 2007/01/06 00:29:52 christos Exp $ */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain this list of conditions
 *    and the following disclaimer.
 * 2. Redistributions in binary form must reproduce this list of conditions
 *    and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANY
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARM_OMAP_OMAP_EMIFS_H_
#define _ARM_OMAP_OMAP_EMIFS_H_

/* Extended Memory Interface Slow */

struct emifs_attach_args {
	bus_space_tag_t	emifs_iot;	/* Bus tag */
	bus_addr_t	emifs_addr;	/* Address */
	bus_size_t	emifs_size;	/* Size of peripheral address space */
	int		emifs_intr;	/* IRQ number */
	bus_dma_tag_t	emifs_dmac;	/* DMA channel */
	unsigned int	emifs_mult;	/* Offset multiplier */
};

extern struct bus_space omap_bs_tag;
extern struct arm32_bus_dma_tag omap_bus_dma_tag;
extern struct bus_space omap_a4x_bs_tag;
extern struct bus_space omap_a2x_bs_tag;
extern struct bus_space nobyteacc_bs_tag;

#endif /* _ARM_OMAP_OMAP_EMIFS_H_ */
