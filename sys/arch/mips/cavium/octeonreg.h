/*	$NetBSD: octeonreg.h,v 1.2 2020/06/18 13:52:08 simonb Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Simon Burge.
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


#ifndef _OCTEONREG_H_
#define	_OCTEONREG_H_

#define	OCTEON_PLL_REF_CLK		50000000	/* defined as 50MHz */

/* ---- virtual addressing */

/* CVMSEG virtual addresses */
#define	OCTEON_CVMSEG_LM		UINT64_C(0xffffffffffff8000)
#define	OCTEON_CVMSEG_IO		UINT64_C(0xffffffffffffa000)

#define	OCTEON_IOBDMA_GLOBAL_ADDR	UINT64_C(0xffffffffffffa200)
#define	OCTEON_IOBDMA_LOCAL_ADDR	UINT64_C(0xffffffffffffb200)
#define	OCTEON_LMTDMA_GLOBAL_ADDR	UINT64_C(0xffffffffffffa400)
#define	OCTEON_LMTDMA_LOCAL_ADDR	UINT64_C(0xffffffffffffb400)
/* use globally ordered by default */
#define	OCTEON_IOBDMA_ADDR		OCTEON_IOBDMA_GLOBAL_ADDR
#define	OCTEON_LMTDMA_ADDR		OCTEON_LMTDMA_GLOBAL_ADDR

/* ---- physical addressing */

/*
 * Cavium Octeon has a 49 bit physical address space.
 *
 * Bit 48 == 0 defines a L2 or DRAM address
 * Bit 48 == 1 defines an IO address
 *
 * For IO addresses:
 *	Bits 47-43:	Major DID - directs request to correct hardware block
 *	Bits 42-40:	Sub DID - directs request within the hardware block
 *	Bits 39-38:	reserved - 0
 *	Bits 37-36:	reserved - 0 (on Octeon and Octeon Plus)
 *	Bits 37-36:	Node - selects node/chip (Octeon II)
 *	Bits 35- 0:	IO bus device address with the DID
 */
#define	OCTEON_ADDR_IO			__BIT(48)
#define	OCTEON_ADDR_MAJOR_DID		__BITS(47,43)
#define	OCTEON_ADDR_SUB_DID		__BITS(42,40)
#define	OCTEON_ADDR_NODE		__BITS(37,36)
#define	OCTEON_ADDR_OFFSET		__BITS(35,0)

#define	OCTEON_ADDR_DID(major, sub)			(	\
	__SHIFTIN((major), OCTEON_ADDR_MAJOR_DID) | 		\
	__SHIFTIN((sub), OCTEON_ADDR_SUB_DID))

/* used to build addresses for load/store operations */
#define	OCTEON_ADDR_IO_DID(major, sub)				\
	(OCTEON_ADDR_IO | OCTEON_ADDR_DID((major), (sub)))


/* ---- core specific registers */

/* OCTEON II */
#define	MIO_RST_BOOT			UINT64_C(0x1180000001600)
#define	  MIO_RST_BOOT_C_MUL		__BITS(36,30)
#define	  MIO_RST_BOOT_PNR_MUL		__BITS(29,24)


/* OCTEON III */
#define	MIO_FUS_PDF			UINT64_C(0x1180000001428)
#define	  MIO_FUS_PDF_IS_71XX		__BIT(32)

#define	RST_BOOT			UINT64_C(0x1180006001600)
#define	  RST_BOOT_C_MUL		__BITS(36,30)
#define	  RST_BOOT_PNR_MUL		__BITS(29,24)
#define	RST_DELAY			UINT64_C(0x1180006001608)
#define	RST_CFG				UINT64_C(0x1180006001610)
#define	RST_OCX				UINT64_C(0x1180006001618)
#define	RST_INT				UINT64_C(0x1180006001628)
#define	RST_CKILL			UINT64_C(0x1180006001638)
#define	RST_CTL(n)			(UINT64_C(0x1180006001640) + (n) * 0x8)
#define	RST_SOFT_RST			UINT64_C(0x1180006001680)
#define	RST_SOFT_PRST(n)		(UINT64_C(0x11800060016c0) + (n) * 0x8)
#define	RST_PP_POWER			UINT64_C(0x1180006001700)
#define	RST_POWER_DBG			UINT64_C(0x1180006001708)
#define	RST_REF_CNTR			UINT64_C(0x1180006001758)
#define	RST_COLD_DATA(n)		(UINT64_C(0x11800060017c0) + (n) * 0x8)


/* ---- IOBDMA */

/* 4.7 IOBDMA Operations */
#define	IOBDMA_SCRADDR			__BITS(63,56)
#define	IOBDMA_LEN			__BITS(55,48)
/*	IOBDMA_MAJOR_DID		same as OCTEON_MAJOR_DID */
/*	IOBDMA_SUB_DID			same as OCTEON_SUB_DID */
/*					reserved 39:38 */
#define	IOBDMA_NODE			__BITS(37,36)	/* Octeon 3 only */
#define	IOBDMA_OFFSET			__BITS(35,0)
/* technically __BITS(2,0) are reserved as 0, address must be 64-bit aligned */

#define	IOBDMA_CREATE(major, sub, scr, len, offset)	(	\
	OCTEON_ADDR_DID((major), (sub)) |			\
	__SHIFTIN((scr), IOBDMA_SCRADDR) |			\
	__SHIFTIN((len), IOBDMA_LEN) |				\
	__SHIFTIN((offset), IOBDMA_OFFSET))


#endif /* !_OCTEONREG_H_ */
