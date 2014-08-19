/*	$NetBSD: if_smapreg.h,v 1.4.6.2 2014/08/20 00:03:17 tls Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 * SMAP memory access layer.
 *
 *    <-> FIFO <-> BUF  
 * CPU              +<-----> EMAC3
 *    <----------> DESC
 */

/*
 * Buffer (accessed by FIFO or EMAC3. indexed by descriptor)
 */
#define	SMAP_TXBUF_BASE		0x1000
#define SMAP_TXBUF_SIZE		0x1000
#define SMAP_RXBUF_BASE		0x4000
#define SMAP_RXBUF_SIZE		0x4000

/*
 * FIFO access
 */
#define	SMAP_TXFIFO_CTRL_REG8		MIPS_PHYS_TO_KSEG1(0x14001000)
#define	SMAP_TXFIFO_PTR_REG16		MIPS_PHYS_TO_KSEG1(0x14001004)
#define	SMAP_TXFIFO_FRAME_REG8		MIPS_PHYS_TO_KSEG1(0x1400100c)
#define	SMAP_TXFIFO_FRAME_INC_REG8	MIPS_PHYS_TO_KSEG1(0x14001010)
#define	SMAP_TXFIFO_DATA_REG		MIPS_PHYS_TO_KSEG1(0x14001100)
#define	SMAP_RXFIFO_CTRL_REG8		MIPS_PHYS_TO_KSEG1(0x14001030)
#define	SMAP_RXFIFO_PTR_REG16		MIPS_PHYS_TO_KSEG1(0x14001034)
#define	SMAP_RXFIFO_FRAME_REG8		MIPS_PHYS_TO_KSEG1(0x1400103c)
#define	SMAP_RXFIFO_FRAME_DEC_REG8	MIPS_PHYS_TO_KSEG1(0x14001040)
#define	SMAP_RXFIFO_DATA_REG		MIPS_PHYS_TO_KSEG1(0x14001200)
#define	  SMAP_FIFO_RESET	0x01

/*
 * Descriptor access
 */
#define SMAP_DESC_MODE_REG8		MIPS_PHYS_TO_KSEG1(0x14000102)
#define   SMAP_DESC_MODE_SWAP	0x0001

#define SMAP_TXDESC_BASE		MIPS_PHYS_TO_KSEG1(0x14003000)
#define SMAP_RXDESC_BASE		MIPS_PHYS_TO_KSEG1(0x14003200)
#define SMAP_DESC_MAX		64
struct smap_desc {
	u_int16_t stat;
	u_int16_t __reserved;
	u_int16_t sz;
	u_int16_t ptr;
}__attribute__((__packed__, __aligned__(8)));

/* TX Control */
#define	SMAP_TXDESC_READY	0x8000
#define	SMAP_TXDESC_GENFCS	0x0200
#define	SMAP_TXDESC_GENPAD	0x0100
#define	SMAP_TXDESC_INSSA	0x0080
#define	SMAP_TXDESC_RPLSA	0x0040
#define	SMAP_TXDESC_INSVLAN	0x0020
#define	SMAP_TXDESC_RPLVLAN	0x0010

/* TX Status */
#define	SMAP_TXDESC_READY	0x8000
#define	SMAP_TXDESC_BADFCS	0x0200
#define	SMAP_TXDESC_BADPKT	0x0100
#define	SMAP_TXDESC_LOSSCR	0x0080
#define	SMAP_TXDESC_EDEFER	0x0040
#define	SMAP_TXDESC_ECOLL	0x0020
#define	SMAP_TXDESC_LCOLL	0x0010
#define	SMAP_TXDESC_MCOLL	0x0008
#define	SMAP_TXDESC_SCOLL	0x0004
#define	SMAP_TXDESC_UNDERRUN	0x0002
#define	SMAP_TXDESC_SQE		0x0001

/* RX Control */
#define	SMAP_RXDESC_EMPTY	0x8000

/* RX Status */
#define	SMAP_RXDESC_EMPTY	0x8000
#define	SMAP_RXDESC_OVERRUN	0x0200
#define	SMAP_RXDESC_PFRM	0x0100
#define	SMAP_RXDESC_BADFRM	0x0080
#define	SMAP_RXDESC_RUNTFRM	0x0040
#define	SMAP_RXDESC_SHORTEVNT	0x0020
#define	SMAP_RXDESC_ALIGNERR	0x0010
#define	SMAP_RXDESC_BADFCS	0x0008
#define	SMAP_RXDESC_FRMTOOLONG	0x0004
#define	SMAP_RXDESC_OUTRANGE	0x0002
#define	SMAP_RXDESC_INRANGE	0x0001
