/*	$NetBSD: p5pbreg.h,v 1.1 2011/08/04 17:48:51 rkujawa Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
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
 * 0xFFFA0000 - (unknown)
 * 0xFFFC0000 - PCI configuration mechanism #1 data, 128KB
 * 0xFFFE0000 - PCI configuration mechanism #1 address, 4KB
 * 
 * 0xE0000000 - Permedia RAM on CVPPC/BVPPC (1st aperture), 8MB
 * 0xE0800000 - Permedia RAM on CVPPC/BVPPC (2nd aperture), 8MB
 * 0xE1000000 - Permedia registers, 128KB
 *
 * Note: this map may not look the same for every firmware revision. 
 * 
 * The bridge is probably capable of DMA and interrupts, but this would 
 * need further reverse engineering, and is not really needed to drive
 * the Permedia 2 chip.
 */
#ifndef _AMIGA_P5PBREG_H_

#define P5BUS_PCI_CONF_BASE	0xFFFA0000
#define P5BUS_PCI_CONF_SIZE	0x00041000

#define P5BUS_PCI_MEM_BASE	0xE0000000
#define P5BUS_PCI_MEM_SIZE	0x01010000	/* actually 0x01020000 */

#define OFF_PCI_CONF_DATA	0x00020000 
#define OFF_PCI_CONF_ADDR	0x00040000

#define P5BUS_CONF_ENDIAN	0x0000          /* PCI_CONF_ADDR + offset */
#define P5BUS_CONF_ENDIAN_BIG	0x02            /* to switch into BE mode */
#define P5BUS_CONF_INTR		0x0010          /* ? XXX interrupt enable? */
#define P5BUS_CONF_INTR_INT2	0x01            /* ? XXX INT2? */

/* typical configuration of Permedia 2 on CVPPC/BVPPC */
#define OFF_P2_APERTURE_1	0x0 
#define OFF_P2_APERTURE_2	0x00800000
#define OFF_P2_REGS		0x01000000 
/* #define OFF_P2_REGS		0x0F000000 */   /* ? alt. Permedia regs */

#endif /* _AMIGA_P5PBREG_H_ */
