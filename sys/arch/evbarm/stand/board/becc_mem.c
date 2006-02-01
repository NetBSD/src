/*	$NetBSD: becc_mem.c,v 1.2.2.1 2006/02/01 14:51:26 yamt Exp $	*/

/*
 * Copyright (c) 2002, 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This file provides the mem_init() function for boards using the
 * ADI BECC companion chip.
 */

#include <sys/types.h>
#include <lib/libsa/stand.h>

#include <arm/xscale/beccreg.h>

#include <dev/pci/pcireg.h>

#include "board.h"

#define	BECC_PCICORE_READ(x)						\
	*((volatile uint32_t *)					\
	  (BECC_PCI_CONF_BASE | (1U << BECC_IDSEL_BIT) | (x)))

#define	BECC_PCICORE_WRITE(x, v)					\
	*((volatile uint32_t *)					\
	  (BECC_PCI_CONF_BASE | (1U << BECC_IDSEL_BIT) | (x))) = (v)

#define	BECC_SDRAM_BAR	(PCI_MAPREG_START + 0x08)

void
mem_init(void)
{
	uint32_t start, size, reg, save, heap;

	start = BECC_SDRAM_BASE;

	/*
	 * BECC <= Rev7 can only address 64M through the inbound PCI
	 * windows.  Limit memory to 64M on those revs.
	 *
	 * On >= Rev8, there is a BAR which covers all of SDRAM which
	 * we can probe for the memory size.
	 */
	reg = BECC_PCICORE_READ(PCI_CLASS_REG);
#ifdef BECC_SUPPORT_V7
	if (PCI_REVISION(reg) <= BECC_REV_V7)
		size = (64U * 1024 * 1024);
	else {
#endif
		save = BECC_PCICORE_READ(BECC_SDRAM_BAR);
		BECC_PCICORE_WRITE(BECC_SDRAM_BAR, 0xffffffffU);
		reg = BECC_PCICORE_READ(BECC_SDRAM_BAR);
		BECC_PCICORE_WRITE(BECC_SDRAM_BAR, save);
		save = BECC_PCICORE_READ(BECC_SDRAM_BAR);

		size = PCI_MAPREG_MEM_SIZE(reg);
#ifdef BECC_SUPPORT_V7
	}
#endif

	heap = (start + size) - BOARD_HEAP_SIZE;

	printf(">> RAM 0x%x - 0x%x, heap at 0x%x\n",
	    start, (start + size) - 1, heap);
	setheap((void *)heap, (void *)(start + size - 1));
}
