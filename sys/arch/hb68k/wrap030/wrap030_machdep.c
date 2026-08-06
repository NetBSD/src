/*	$NetBSD: wrap030_machdep.c,v 1.1 2026/08/06 14:41:09 thorpej Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wrap030_machdep.c,v 1.1 2026/08/06 14:41:09 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/fdt_platform.h>

#include <uvm/uvm_extern.h>

#include <m68k/mmu_30.h>

/*
 * plat_bootstrap --
 *	Early bootstrap routine, mainly for the purpose of providing
 *	data necessary to configure the MMU.
 */
static paddr_t
wrap030_plat_bootstrap(paddr_t nextpa, vaddr_t reloff)
{
	/*
	 * We're going to use TT0 to map all of the device address space,
	 * which is located >= 0x80000000.  We have to:
	 *
	 *	- Initialize the TT register data.
	 * and
	 *	- Tell pmap_bootstrap1() about the TT-mapped region.
	 */
	uint32_t *tt = (uint32_t *)PMAP_BOOTSTRAP_RELOC_GLOB(mmu_tt30);
	tt[MMU_TTREG_TT0] =
	    0x80000000 |
	    __SHIFTIN(0x7f,TT30_LAM) |
	    TT30_E | TT30_CI | TT30_RWM |
	    TT30_SUPERD;

	struct pmap_bootmap *pmbm =
	    (struct pmap_bootmap *)PMAP_BOOTSTRAP_RELOC_GLOB(machine_bootmap);
	pmbm[0].pmbm_vaddr = 0x80000000;
	pmbm[0].pmbm_size  = 0x80000000;
	pmbm[0].pmbm_flags = PMBM_F_KEEPOUT;

	return nextpa;
}

/*
 * plat_uart_freq --
 *	Return the console UART clock frequency in Hz.  This is
 *	called if the information is not available in the device
 *	tree.
 */
static u_int
wrap030_plat_uart_freq(void)
{
	return 1843200;
}

static const struct fdt_platform wrap030_platform = {
	.fp_bootstrap = wrap030_plat_bootstrap,
	.fp_uart_freq = wrap030_plat_uart_freq,
};
FDT_PLATFORM(wrap030, "techav,wrap030", &wrap030_platform);
