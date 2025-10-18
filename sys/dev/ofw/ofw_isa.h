/*	$NetBSD: ofw_isa.h,v 1.1 2025/10/18 15:40:59 thorpej Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#ifndef _DEV_OFW_OFW_ISA_H_
#define	_DEV_OFW_OFW_ISA_H_

/*
 * ISA Bus Binding to:
 *
 * IEEE Std 1275-1994
 * Standard for Boot (Initialization Configuration) Firmware
 *
 * Revision: 0.4 (Unapproved Draft)
 * September 23, 1996
 */

/*
 * Section 2.2.1. Physical Address Formats
 *
 * An ISA physical address is represented by 2 address cells:
 *
 *	phys.hi cell:	00000000 00000000 00000000 00000vti
 *	phys.lo cell:	nnnnnnnn nnnnnnnn nnnnnnnn nnnnnnnn
 *
 *	v	address is 11-bit aliased
 *	t	address is 10-bit aliased
 *	i	address is in I/O space (vs memory space)
 *	n	32-bit address
 */

#define	OFW_ISA_PHYS_HI_IO		__BIT(0)
#define	OFW_ISA_PHYS_HI_10BIT		__BIT(1)
#define	OFW_ISA_PHYS_HI_11BIT		__BIT(2)

/*
 * This has the 2 32bit cell values, plus another to make up a 32-bit size.
 */
struct ofw_isa_register {
	uint32_t	phys_hi;
	uint32_t	phys_lo;
	uint32_t	size;
};

/*
 * Section 4.1.1.  Properties for child nodes.
 *
 * "interrupts"
 *
 * Interrupts are encoded with a prop-encoded-array of the following
 * integers:
 *
 *	irq		IRQ number (0-15)
 *	type		IRQ type	0 active low level trigger
 *					1 active high level trigger
 *					2 falling edge trigger
 *					3 rising edge trigger
 */

#define	OFW_ISA_INTR_TYPE_LOW_LEVEL	0
#define	OFW_ISA_INTR_TYPE_HIGH_LEVEL	1
#define	OFW_ISA_INTR_TYPE_FALLING_EDGE	2
#define	OFW_ISA_INTR_TYPE_RISING_EDGE	3

struct ofw_isa_interrupt {
	uint32_t	irq;
	uint32_t	type;
};

/*
 * Section 4.1.2.  Bus-specific properties for child nodes.
 *
 * "dma"
 *
 * DMA information is encoded in a prop-encoded-array of the following
 * integers:
 *
 *	dma#		DRQ (0-3, 5-7)
 *	mode		DMA channel mode
 *	width		DMA transfer size in bits (8, 16, 32)
 *	coundwidth	DMA transfer size count units (8, 16, 32)
 *	busmaster	1=bus master, 0=not-a-bus master
 */

#define	OFW_ISA_DMA_MODE_COMPAT		0
#define	OFW_ISA_DMA_MODE_A		1
#define	OFW_ISA_DMA_MODE_B		2
#define	OFW_ISA_DMA_MODE_F		3
#define	OFW_ISA_DMA_MODE_C		4

struct ofw_isa_dma {
	uint32_t	drq;
	uint32_t	mode;
	uint32_t	width;
	uint32_t	countwidth;
	uint32_t	busmaster;
};

#endif /* _DEV_OFW_OFW_ISA_H_ */
