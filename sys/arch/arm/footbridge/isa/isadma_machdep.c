/*	$NetBSD: isadma_machdep.c,v 1.16 2012/09/21 14:21:58 matt Exp $	*/

#define ISA_DMA_STATS

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: isadma_machdep.c,v 1.16 2012/09/21 14:21:58 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/mbuf.h>

#define _ARM32_BUS_DMA_PRIVATE
#include <sys/bus.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <uvm/uvm_extern.h>

/*
 * ISA has a 24-bit address limitation, so at most it has a 16M
 * DMA range.  However, some platforms have a more limited range,
 * e.g. the Shark NC.  On these systems, we are provided with
 * a set of DMA ranges.  The pmap module is aware of these ranges
 * and places DMA-safe memory for them onto an alternate free list
 * so that they are protected from being used to service page faults,
 * etc. (unless we've run out of memory elsewhere).
 */
#define	ISA_DMA_BOUNCE_THRESHOLD	(16 * 1024 * 1024)

struct arm32_dma_range *footbridge_isa_dma_ranges;
int footbridge_isa_dma_nranges;

/*
 * Entry points for ISA DMA.  These are mostly wrappers around
 * the generic functions that understand how to deal with bounce
 * buffers, if necessary.
 */
struct arm32_bus_dma_tag isa_bus_dma_tag = {
	_BUS_DMAMAP_FUNCS,
	_BUS_DMAMEM_FUNCS,
	_BUS_DMATAG_FUNCS,
};

/*
 * Initialize ISA DMA.
 */
void
isa_dma_init(void)
{

	isa_bus_dma_tag._ranges = footbridge_isa_dma_ranges;
	isa_bus_dma_tag._nranges = footbridge_isa_dma_nranges;
}
