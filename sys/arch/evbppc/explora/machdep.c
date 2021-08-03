/*	$NetBSD: machdep.c,v 1.52 2021/08/03 09:25:43 rin Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juergen Hannken-Illjes.
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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.52 2021/08/03 09:25:43 rin Exp $");

#include "opt_explora.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/systm.h>

#include <prop/proplib.h>

#include <machine/explora.h>

#include <powerpc/spr.h>
#include <powerpc/ibm4xx/spr.h>

#include <powerpc/ibm4xx/cpu.h>
#include <powerpc/ibm4xx/dcr403cgx.h>
#include <powerpc/ibm4xx/tlb.h>

#define TLB_PG_SIZE	(16*1024*1024)

static const unsigned int cpuspeed = 66000000;

void		initppc(vaddr_t, vaddr_t);

void
initppc(vaddr_t startkernel, vaddr_t endkernel)
{
	u_int i, j, t, br[4];
	u_int maddr, msize, size;

	br[0] = mfdcr(DCR_BR4);
	br[1] = mfdcr(DCR_BR5);
	br[2] = mfdcr(DCR_BR6);
	br[3] = mfdcr(DCR_BR7);

	for (i = 0; i < 4; i++)
		for (j = i+1; j < 4; j++)
			if (br[j] < br[i])
				t = br[j], br[j] = br[i], br[i] = t;

	for (i = 0, size = 0; i < 4; i++) {
		if (((br[i] >> 19) & 3) != 3)
			continue;
		maddr = ((br[i] >> 24) & 0xff) << 20;
		msize = 1 << (20 + ((br[i] >> 21) & 7));
		if (maddr + msize > size)
			size = maddr + msize;
	}

	/*
	 * Setup initial tlbs.
	 * Kernel memory and console device are
	 * mapped into the first (reserved) tlbs.
	 */

	for (maddr = 0; maddr < endkernel; maddr += TLB_PG_SIZE)
		ppc4xx_tlb_reserve(maddr, maddr, TLB_PG_SIZE, TLB_EX);

	/* Map PCKBC, PCKBC2, COM, LPT. This is far beyond physmem. */
	ppc4xx_tlb_reserve(BASE_ISA, BASE_ISA, TLB_PG_SIZE, TLB_I | TLB_G);

#ifndef COM_IS_CONSOLE
	ppc4xx_tlb_reserve(BASE_FB,  BASE_FB,  TLB_PG_SIZE, TLB_I | TLB_G);
	ppc4xx_tlb_reserve(BASE_FB2, BASE_FB2, TLB_PG_SIZE, TLB_I | TLB_G);
#endif

	/* Disable all external interrupts */
	mtdcr(DCR_EXIER, 0);

	/* Disable all timer interrupts */
	mtspr(SPR_TCR, 0);

	ibm40x_memsize_init(size, startkernel);
	ibm4xx_init(startkernel, endkernel, pic_ext_intr);
}

void
cpu_startup(void)
{
	prop_number_t pn;

	/*
	 * cpu common startup
	 */
	ibm4xx_cpu_startup("NCD Explora 450");

	/*
	 * Set up the board properties database.
	 */
	board_info_init();

	pn = prop_number_create_integer(ctob(physmem));
	KASSERT(pn != NULL);
	if (prop_dictionary_set(board_properties, "mem-size", pn) == false)
		panic("setting mem-size");
	prop_object_release(pn);

	pn = prop_number_create_integer(cpuspeed);
	KASSERT(pn != NULL);
	if (prop_dictionary_set(board_properties, "processor-frequency",
				pn) == false)
		panic("setting processor-frequency");
	prop_object_release(pn);

	/*
	 * Now that we have VM, malloc()s are OK in bus_space.
	 */
	bus_space_mallocok();

	/*
	 * no fake mapiodev
	 */
	fake_mapiodev = 0;
}
