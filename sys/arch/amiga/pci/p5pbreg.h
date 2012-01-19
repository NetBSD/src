/*	$NetBSD: p5pbreg.h,v 1.6 2012/01/19 00:14:08 rkujawa Exp $ */

/*-
 * Copyright (c) 2011, 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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
 * Reverse engineered Phase5 PCI bridge memory map (firmware 44.71):
 * 
 * 0xFFFA0000 - PCI register space, 64KB
 * 0xFFFC0000 - PCI configuration mechanism #1 data, 128KB
 * 0xFFFE0000 - (unknown, probably PCI bridge configuration registers, 4KB)
 * 
 * 0xE0000000 - Permedia RAM on CVPPC/BVPPC (1st aperture), 8MB
 * 0xE0800000 - Permedia RAM on CVPPC/BVPPC (2nd aperture), 8MB
 * 0xE1000000 - Permedia registers on CVPPC/BVPPC, 128KB
 *
 * 0x80000000 - PCI cards memory space on G-REX, variable size
 *
 * Note: this map may not look the same for every firmware revision. 
 * 
 * The bridge is certainly capable of DMA, but this needs further reverse 
 * engineering.
 */
#ifndef _AMIGA_P5PBREG_H_

#define P5BUS_PCI_CONF_BASE	0xFFFC0000
#define P5BUS_PCI_CONF_SIZE	0x00020000	/* up to 128kB */

#define OFF_PCI_CONF_DATA	0x00001000	/* also 0 on CVPPC */
#define OFF_PCI_DEVICE		0x00002000
#define OFF_PCI_FUNCTION	0x00000100

#define P5BUS_PCI_IO_BASE	0xFFFA0000
#define P5BUS_PCI_IO_SIZE	0x00010000	/* 64kB */

/* Bridge configuration */
#define P5BUS_BRIDGE_BASE	0xFFFE0000
#define P5BUS_BRIDGE_SIZE	0x00001000	/* 64kB, 4kB on some fw revs */

#define OFF_BRIDGE_ENDIAN	0x0000          /* PCI_BRIDGE_BASE + offset */
#define P5BUS_BRIDGE_ENDIAN_BIG	0x02            /* to switch into BE mode */
#define OFF_BRIDGE_INTR		0x0010          /* ? XXX interrupt enable? */
#define P5BUS_BRIDGE_INTR_INT2	0x01            /* ? XXX INT2? */

/* CVPPC/BVPPC defaults. */ 
#define P5BUS_PCI_MEM_BASE	0xE0000000
/* #define P5BUS_PCI_MEM_BASE	0x80000000 */	/* default on G-REX */
#define P5BUS_PCI_MEM_SIZE	0x01020000

/* typical configuration of Permedia 2 on CVPPC/BVPPC */
#define OFF_P2_APERTURE_1	0x0 
#define OFF_P2_APERTURE_2	0x00800000
#define OFF_P2_REGS		0x01000000 
/* #define OFF_P2_REGS		0x0F000000 */   /* ? alt. Permedia regs */

/* Permedia 2 vendor and product IDs, for CVPPC/BVPPC probe. */
#define P5PB_PM2_VENDOR_ID	0x104C
#define P5PB_PM2_PRODUCT_ID	0x3D07

#endif /* _AMIGA_P5PBREG_H_ */

