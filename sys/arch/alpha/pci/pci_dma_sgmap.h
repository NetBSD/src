/* $NetBSD: pci_dma_sgmap.h,v 1.1.2.1 1997/05/23 21:46:43 thorpej Exp $ */

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

#include <sys/extent.h>
#include <machine/bus.h>

/*
 * A PCI SGMAP page table entry looks like this:
 *
 * 63                           21 20            1   0
 * |           Reserved           | Page address | V |
 *
 * The page address is bits <32:13> (<31:13> on the LCA) of the physical
 * address of the page.  The V bit it set if the PTE holds a valid mapping.
 */
#define	PSGPTE_PGADDR_MASK	0x00000000001ffffeUL
#define	PSGPTE_PGADDR_SHIFT	12
#define	PSGPTE_VALID		0x0000000000000001UL

/*
 * Bits n:13 of the PCI DMA address are derived from the byte offset
 * of the PTE.  The shift for this is 10.  However, we manage PTEs
 * by index.  Since a PTE is 8 bytes, this gives us a shift of 13.
 */
#define	PCIADDR_PTEIDX_SHIFT	13

/*
 * An Alpha PCI SGMAP's state information.
 */
struct alpha_pci_sgmap {
	struct extent *aps_ex;		/* extent map to manage sgva space */
	u_int64_t *aps_pt;		/* page table */
	bus_addr_t aps_ptpa;		/* page table physical address */
	bus_addr_t aps_base;		/* base of the sgva space */
	bus_size_t aps_size;		/* size of the sgva space */
};

/*
 * Cookie crumb used to store SGMAP state for a DMA mapping.  We
 * could compute some of this at unload time, but storing it
 * is convenient.
 */
struct alpha_pci_dma_cookie {
	int	apdc_pteidx;		/* PTE index */
	int	apdc_ptecnt;		/* PTE count */
	u_long	apdc_sgva;		/* allocated sgva */
	bus_size_t apdc_len;		/* sgva size */
	int	apdc_flags;
};

/* apdc_flags */
#define	APDC_HAS_SGMAP		0x01	/* sgva/len are valid */
#define	APDC_USING_SGMAP	0x02	/* map is using SGMAP */

void	pci_dma_sgmap_init __P((bus_dma_tag_t, struct alpha_pci_sgmap *,
	    const char *, bus_addr_t, bus_size_t));

int	pci_dma_sgmap_alloc __P((bus_dmamap_t, bus_size_t,
	    struct alpha_pci_sgmap *, struct alpha_pci_dma_cookie *, int));
void	pci_dma_sgmap_free __P((struct alpha_pci_sgmap *,
	    struct alpha_pci_dma_cookie *));

int	pci_dma_sgmap_load __P((bus_dma_tag_t, bus_dmamap_t, void *,
	    bus_size_t, struct proc *, int, struct alpha_pci_sgmap *,
	    struct alpha_pci_dma_cookie *));
int	pci_dma_sgmap_load_mbuf __P((bus_dma_tag_t, bus_dmamap_t,
	    struct mbuf *, int, struct alpha_pci_sgmap *,
	    struct alpha_pci_dma_cookie *));
int	pci_dma_sgmap_load_uio __P((bus_dma_tag_t, bus_dmamap_t,
	    struct uio *, int, struct alpha_pci_sgmap *,
	    struct alpha_pci_dma_cookie *));
int	pci_dma_sgmap_load_raw __P((bus_dma_tag_t, bus_dmamap_t,
	    bus_dma_segment_t *, int, bus_size_t, int,
	    struct alpha_pci_sgmap *, struct alpha_pci_dma_cookie *));

void	pci_dma_sgmap_unload __P((bus_dma_tag_t, bus_dmamap_t,
	    struct alpha_pci_sgmap *, struct alpha_pci_dma_cookie *));
