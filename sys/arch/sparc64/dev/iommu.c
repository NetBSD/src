/*	$NetBSD: iommu.c,v 1.1 1999/06/04 13:48:48 mrg Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: NetBSD: sbus.c,v 1.13 1999/05/23 07:24:02 mrg Exp
 *	from: @(#)sbus.c	8.1 (Berkeley) 6/11/93
 */

#include <sys/param.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <vm/vm.h>

#include <machine/bus.h>
#include <sparc64/sparc64/vaddrs.h>
#include <sparc64/dev/iommureg.h>
#include <sparc64/dev/iommuvar.h>

#include <machine/autoconf.h>
#include <machine/ctlreg.h>
#include <machine/cpu.h>

#ifdef DEBUG
#define IDB_DVMA	0x1
#define IDB_INTR	0x2
int iommudebug = 0;
#endif

/*
 * initialise the UltraSPARC IOMMU (SBUS or PCI):
 *	- allocate and setup the iotsb.
 *	- enable the IOMMU
 *	- initialise the streaming buffers
 *	- create a private DVMA map.
 *
 */
void
iommu_init(name, is, tsbsize)
	char *name;
	struct iommu_state *is;
	int tsbsize;
{

	/*
	 * Setup the iommu.
	 *
	 * The sun4u iommu is part of the SBUS controller so we will
	 * deal with it here.  We could try to fake a device node so
	 * we can eventually share it with the PCI bus run by psycho,
	 * but I don't want to get into that sort of cruft.
	 *
	 * First we need to allocate a IOTSB.  Problem is that the IOMMU
	 * can only access the IOTSB by physical address, so all the 
	 * pages must be contiguous.  Luckily, the smallest IOTSB size
	 * is one 8K page.
	 */
	if (tsbsize != 0)
		panic("tsbsize != 0; FIX ME");	/* XXX */
	
	/* we want 8K pages */
	is->is_cr = IOMMUCR_8KPG | IOMMUCR_EN;

	/* set the tsbsize and get the dvmabase */
	switch (tsbsize) {
	case 0:
		is->is_dvmabase = 0x1fffffff;
		is->is_cr |= IOMMUCR_TSB1K;
		break;

#if 0
	/* XXX are these right? got the values from the linux driver */
	case 1:
		is->is_dvmabase = 0x3fffffff;
		is->is_cr |= IOMMUCR_TSB2K;
		break;

	case 2:
		is->is_dvmabase = 0x7fffffff;
		is->is_cr |= IOMMUCR_TSB4K;
		break;
#endif

	default:
		panic("unknown tsbsize, fix me");
	}

	is->is_tsbsize = tsbsize;
	is->is_tsb = malloc(NBPG, M_DMAMAP, M_WAITOK);	/* XXX */
	is->is_ptsb = pmap_extract(pmap_kernel(), (vaddr_t)is->is_tsb);

#ifdef DEBUG
	if (iommudebug & IDB_DVMA)
	{
		/* Probe the iommu */
		struct iommureg *regs = is->is_iommu;
		int64_t cr, tsb;

		printf("iommu regs at: cr=%lx tsb=%lx flush=%lx\n", &regs->iommu_cr,
		       &regs->iommu_tsb, &regs->iommu_flush);
		cr = regs->iommu_cr;
		tsb = regs->iommu_tsb;
		printf("iommu cr=%lx tsb=%lx\n", (long)cr, (long)tsb);
		printf("TSB base %p phys %p\n", (long)is->is_tsb, (long)is->is_ptsb);
		delay(1000000); /* 1 s */
	}
#endif

	/*
	 * Initialize streaming buffer.
	 */
	is->is_flushpa = pmap_extract(pmap_kernel(), (vaddr_t)&is->is_flush);

	/*
	 * now actually start up the IOMMU
	 */
	iommu_reset(is);

	/*
	 * Now all the hardware's working we need to allocate a dvma map.
	 *
	 * The IOMMU address space always ends at 0xffffe000, but the starting
	 * address depends on the size of the map.  The map size is 1024 * 2 ^
	 * is->is_tsbsize entries, where each entry is 8 bytes.  The start of
	 * the map can be calculated by (0xffffe000 << (8 + is->is_tsbsize)).
	 *
	 * Note: the stupid IOMMU ignores the high bits of an address, so a
	 * NULL DMA pointer will be translated by the first page of the IOTSB.
	 * To trap bugs we'll skip the first entry in the IOTSB.
	 */
	is->is_dvmamap = extent_create(name,
				       IOTSB_VSTART(is->is_tsbsize) + NBPG, IOTSB_VEND,
				       M_DEVBUF, 0, 0, EX_NOWAIT);
}

void
iommu_reset(is)
	struct iommu_state *is;
{

	/* Need to do 64-bit stores */
	bus_space_write_8(is->is_bustag, &is->is_iommu->iommu_cr, 0, is->is_cr);
	bus_space_write_8(is->is_bustag, &is->is_iommu->iommu_tsb, 0, is->is_ptsb);
	/* Enable diagnostics mode? */
	bus_space_write_8(is->is_bustag, &is->is_sb->strbuf_ctl, 0, STRBUF_EN);
}
