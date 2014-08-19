/*	$NetBSD: iommu.h,v 1.8.44.1 2014/08/20 00:03:26 tls Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jeremy Cooper.
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

/*
 * Structure and definition of descriptors used in the I/O Mapper.
 */
#ifndef _SUN3X_IOMMU_H
#define _SUN3X_IOMMU_H

/* The I/O Mapper is a special type of MMU in the sun3x architecture
 * (and supposedly in the sun4m as well) that translates an address used by a
 * device during a DMA transfer into an address on the internal system bus.
 * In other words, it is an MMU that stands between devices wishing to do DMA
 * transfers and main memory.  In this description, the address issued by a
 * DMA device is called a ``DVMA address'', while the address as it is
 * translated and output from the I/O mapper is called a ``system bus address''
 * (sometimes known as a ``physical address'').
 *
 * The DVMA address space in the sun3x architecture is 24 bits wide, in
 * contrast with the system bus address space, which is 32.  The mapping of a
 * DVMA address to a system bus address is accomplished by dividing the DVMA
 * address space into 2048 8K pages.  Each DVMA page is then mapped to a
 * system bus address using a mapping described by a page descriptor entry
 * within the I/O Mapper.  This 2048 entry, page descriptor table is located
 * at physical address 0x60000000 in the sun3x architecture and can be
 * manipulated by the CPU with normal read and write cycles.
 *
 * In addition to describing an address mapping, a page descriptor entry also
 * indicates whether the DVMA page is read-only, should be inhibited from
 * caching by system caches, and whether or not DMA write transfers to it will
 * be completed in 16 byte aligned blocks.  (This last item is used for cache
 * optimization in sun3x systems with special DMA caches.)
 *
 * Since not every DMA device is capable of addressing all 24 bits of the
 * DVMA address space, each is wired so that the end of its address space is
 * always flush against the end of the DVMA address space.  That is, a device
 * with a 16 bit address space (and hence an address space size of 64k) is
 * wired such that it accesses the top 64k of DVMA space.
 */

/** I/O MAPPER Page Descriptor Entry
 *  31                                                             16
 *  +---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---+
 *  |              PAGE PHYSICAL ADDRESS BITS (31..13)              |
 *  +---.---.---+---.---.---.---.---.---+---+---+---+---+---+---.---+
 *  |           |          UNUSED       | CI| BX| M | U | WP|   DT  |
 *  +---.---.---+---.---.---.---.---.---+---+---+---+---+---+---.---+
 *  15                                                              0
 *
 * <CI> CACHE INHIBIT   - When set, prevents instructions and data from the
 *                        page from being cached in any system cache.
 * <BX> FULL BLOCK XFER - When set, acts as an indicator to the caching system
 *                        that all DMA transfers to this DVMA page will fill
 *                        complete I/O cache blocks, eliminating the need for
 *                        the cache block to be filled from main memory first
 *                        before the DMA write can proceed to it.
 * <M>  MODIFIED        - Set when the cpu has modified (written to) the
 *                        physical page.
 * <U>  USED            - Set when the cpu has accessed the physical page.
 * <WP> WRITE PROTECT   - When set, prevents all DMA devices from writing to
 *                        the page.
 * <DT> DESCRIPTOR TYPE - One of the following values:
 *                        00 = Invalid page
 *                        01 = Valid page
 *                        1x = Invalid code for a page descriptor.
 */
struct iommu_pde_struct {
	union {
		struct {
			u_int	pa:19;		/* Physical Address  */
			u_int	unused:6;	/* Unused bits       */
			u_int	ci:1;		/* Cache Inhibit     */
			u_int	bx:1;		/* Full Block Xfer   */
			u_int	m:1;		/* Modified bit      */
			u_int	u:1;		/* Used bit          */
			u_int	wp:1;		/* Write Protect bit */
			u_int	dt:2;		/* Descriptor type   */
			/* Masks for the above fields. */
#define	IOMMU_PDE_PA		0xFFFFE000
#define	IOMMU_PDE_UNUSED	0x00001F80
#define	IOMMU_PDE_CI		0x00000040
#define	IOMMU_PDE_BX		0x00000020
#define	IOMMU_PDE_M		0x00000010
#define	IOMMU_PDE_USED		0x00000008
#define	IOMMU_PDE_WP		0x00000004
#define IOMMU_PDE_DT		0x00000003
			/* The descriptor types */
#define	IOMMU_PDE_DT_INVALID	0x00000000	/* Invalid page      */
#define	IOMMU_PDE_DT_VALID	0x00000001	/* Valid page        */
		} stc;
		uint32_t	raw;	/* For unstructured access to the above */
	} addr;
};
typedef struct iommu_pde_struct iommu_pde_t;

/* Constants */
#define IOMMU_PAGE_SIZE		(8 * 1024)
#define	IOMMU_PAGE_SHIFT	13

/* Useful macros */
#define	IOMMU_PA_PDE(pde)	((pde).addr.raw & IOMMU_PDE_PA)
#define	IOMMU_VALID_DT(pde)	((pde).addr.raw & IOMMU_PDE_DT)	/* X1 */
#define IOMMU_BTOP(pa)		(((u_int) pa) >> IOMMU_PAGE_SHIFT)

/* X1: This macro will incorrectly report the validity for entries which
 * contain codes that are invalid.  (Do not confuse this with the code for
 * 'invalid entry', which means that the descriptor is properly formed, but
 * just not used.)
 */

/* Constants for the I/O mapper as used in the sun3x */
#define	IOMMU_NENT	2048	/* Number of PTEs in the map */
/* Similarly, the virtual address mask. */
#define IOMMU_VA_MASK 0xFFffff	/* 16MB */

#ifdef _KERNEL
/* Interfaces for manipulating the I/O mapper */
void iommu_enter(uint32_t, uint32_t);
void iommu_remove(uint32_t, uint32_t);
#endif /* _KERNEL */

#endif	/* _SUN3X_IOMMU_H */
