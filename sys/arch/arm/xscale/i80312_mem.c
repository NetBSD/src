/*	$NetBSD: i80312_mem.c,v 1.3 2003/07/15 00:24:53 lukem Exp $	*/

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
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
 * Intel i80312 Companion I/O memory controller support.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i80312_mem.c,v 1.3 2003/07/15 00:24:53 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/bus.h>

#include <arm/xscale/i80312reg.h>
#include <arm/xscale/i80312var.h>

/*
 * i80312_sdram_bounds:
 *
 *	Retrieve the start and size of SDRAM.
 */
void
i80312_sdram_bounds(bus_space_tag_t st, bus_space_handle_t sh,
    paddr_t *start, psize_t *size)
{
	uint32_t sdbr, sbr0, sbr1;
	uint32_t bank0, bank1;

	sdbr = bus_space_read_4(st, sh, I80312_MEM_SB);
	sbr0 = bus_space_read_4(st, sh, I80312_MEM_SB0);
	sbr1 = bus_space_read_4(st, sh, I80312_MEM_SB1);

#ifdef VERBOSE_INIT_ARM
	printf("i80312: SBDR = 0x%08x SBR0 = 0x%08x SBR1 = 0x%08x\n",
	    sdbr, sbr0, sbr1);
#endif

	*start = sdbr;

	sdbr = (sdbr >> 25) & 0xf;

	sbr0 = ((sbr0 >> 3) & 0x1f) - sdbr;
	sbr1 = ((sbr1 >> 3) & 0x1f) - sbr0;

	bank0 = sbr0 << 25;
	bank1 = sbr1 << 25;

#ifdef VERBOSE_INIT_ARM
	printf("i80312: BANK0 = 0x%08x BANK1 = 0x%08x\n", bank0, bank1);
#endif

	*size = bank0 + bank1;
}
