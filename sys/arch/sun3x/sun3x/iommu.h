/*	$NetBSD: iommu.h,v 1.2.6.1 1997/03/12 14:22:18 is Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
 * Structure and definition of descriptors used in the I/O Mapper.
 */
#ifndef _SUN3X_IOMMU_H
#define _SUN3X_IOMMU_H

/* The I/O Mapper is a special type of MMU in the sun3x architecture
 * (and supposedly in the sun4m as well) that translates an addresses
 * access by a DMA device during a DMA access, into a physical destination
 * address; it is an MMU that stands between DMA devices and physical memory.
 *
 * The input address space managed by the I/O mapper is 24 bits wide and broken
 * into pages of 8K-byte size.  The output address space is a full 32 bits
 * wide.  The mapping of each input page is described by a page entry
 * descriptor.  There are exactly 2048 such descriptors in the I/O mapper, the
 * first entry of which is located at physical address 0x60000000 in sun3x
 * machines.
 * 
 * Since not every device transfers to a full 24 bit address space, each
 * device is wired so that its address space is always flush against the
 * high end of the I/O mapper.  That is, a device with a 16 bit address space
 * can only access 64k of memory.  This 64k is wired to the top 64k in the
 * I/O mapper's input address space.
 * 
 * In addition to describing address mappings, a page entry also indicates
 * whether the page is read-only, inhibits system caches from caching data
 * addresses to or from it, and whether or not DMA transfers must be completed
 * in 16 byte blocks.  (This is used for cache optimization in sun3x systems
 * with special DMA caches.)
 */

/** I/O MAPPER Page Entry Descriptor
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
 * <BX> FULL BLOCK XFER - When set, requires that all devices must transfer
 *                        data in multiples of 16 bytes in size.
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
		u_int32_t	raw;	/* For unstructured access to the above */
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
#define	IOMMU_NENT	2048	/* Number of entries in the map */

#ifdef _KERNEL
/* Interfaces for manipulating the I/O mapper */
void iommu_enter __P((u_int32_t va, u_int32_t pa));
void iommu_remove __P((u_int32_t va, u_int32_t len));
#endif /* _KERNEL */

#endif	/* _SUN3X_IOMMU_H */
