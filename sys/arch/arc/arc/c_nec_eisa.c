/*	$NetBSD: c_nec_eisa.c,v 1.1 2001/06/13 15:22:49 soda Exp $	*/

/*-
 * Copyright (C) 2000 Shuichiro URATA.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * for NEC EISA generation machines.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/pio.h>
#include <machine/platform.h>
#include <mips/pte.h>

#include <dev/isa/isavar.h>

#include <arc/arc/wired_map.h>
#include <arc/jazz/pica.h>
#include <arc/jazz/rd94.h>
#include <arc/jazz/jazziovar.h>
#include <arc/isa/isabrvar.h>

/*
 * chipset-dependent isa bus configuration
 */

int isabr_nec_eisa_intr_status __P((void));

struct isabr_config isabr_nec_eisa_conf = {
	isabr_nec_eisa_intr_status,
};

int
isabr_nec_eisa_intr_status()
{
	return (in32(RD94_SYS_INTSTAT2) & (ICU_LEN - 1));
}

/*
 * chipset-dependent jazzio bus configuration
 */

void jazzio_nec_eisa_set_iointr_mask __P((int));

struct jazzio_config jazzio_nec_eisa_conf = {
	RD94_SYS_INTSTAT1,
	jazzio_nec_eisa_set_iointr_mask,
	RD94_SYS_TL_BASE,
	RD94_SYS_DMA0_REGS,
};

void
jazzio_nec_eisa_set_iointr_mask(mask)
	int mask;
{
	out16(RD94_SYS_LB_IE2, mask);
}

/*
 * critial i/o space, interrupt, and other chipset related initialization.
 */
void
c_nec_eisa_init()
{
	/*
	 * Initialize I/O address offset
	 */

	arc_bus_space_init(&jazzio_bus, "jazzio",
	    RD94_P_LOCAL_IO_BASE, RD94_V_LOCAL_IO_BASE,
	    RD94_V_LOCAL_IO_BASE, RD94_S_LOCAL_IO_BASE);

	/* XXX - not really confirmed */
	arc_bus_space_init(&arc_bus_io, "r94eisaio",
	    RD94_P_PCI_IO, RD94_V_PCI_IO, 0, RD94_S_PCI_IO);
	arc_bus_space_init(&arc_bus_mem, "r94eisamem",
	    RD94_P_PCI_MEM, RD94_V_PCI_MEM, 0, RD94_S_PCI_MEM);

	/*
	 * Initialize wired TLB for I/O space which is used on early stage
	 */
	arc_enter_wired(RD94_V_LOCAL_IO_BASE, RD94_P_LOCAL_IO_BASE, 0,
	    MIPS3_PG_SIZE_256K);
	arc_enter_wired(PICA_V_LOCAL_VIDEO_CTRL, PICA_P_LOCAL_VIDEO_CTRL,
	    PICA_P_LOCAL_VIDEO_CTRL + PICA_S_LOCAL_VIDEO_CTRL/2,
	    MIPS3_PG_SIZE_1M);
	arc_enter_wired(PICA_V_EXTND_VIDEO_CTRL, PICA_P_EXTND_VIDEO_CTRL,
	    PICA_P_EXTND_VIDEO_CTRL + PICA_S_EXTND_VIDEO_CTRL/2,
	    MIPS3_PG_SIZE_1M);
	arc_enter_wired(PICA_V_LOCAL_VIDEO, PICA_P_LOCAL_VIDEO,
	    PICA_P_LOCAL_VIDEO + PICA_S_LOCAL_VIDEO/2, MIPS3_PG_SIZE_4M);
	arc_enter_wired(RD94_V_PCI_IO, RD94_P_PCI_IO, RD94_P_PCI_MEM,
	    MIPS3_PG_SIZE_16M);

	/*
	 * Initialize interrupt priority
	 */
	splvec.splnet = MIPS_INT_MASK_SPL2;
	splvec.splbio = MIPS_INT_MASK_SPL2;
	splvec.splvm = MIPS_INT_MASK_SPL2;
	splvec.spltty = MIPS_INT_MASK_SPL2;
	splvec.splclock = MIPS_INT_MASK_SPL5;
	splvec.splstatclock = MIPS_INT_MASK_SPL5;

	/*
	 * Disable all interrupts. New masks will be set up
	 * during system configuration
	 */
	out16(RD94_SYS_LB_IE1, 0);
	out16(RD94_SYS_LB_IE2, 0);
	out32(RD94_SYS_EXT_IMASK, 0);

	/*
	 * common configuration between NEC EISA and PCI platforms
	 */
	c_nec_jazz_init();

	/* common configuration for Magnum derived and NEC EISA machines */
	c_jazz_eisa_init();

	/* chipset-dependent isa bus configuration */
	isabr_conf = &isabr_nec_eisa_conf;

	/* chipset-dependent jazzio bus configuration */
	jazzio_conf = &jazzio_nec_eisa_conf;
}
