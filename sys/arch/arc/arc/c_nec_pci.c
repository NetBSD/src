/*	$NetBSD: c_nec_pci.c,v 1.1 2001/06/13 15:23:22 soda Exp $	*/

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
 * for NEC PCI generation machines.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kcore.h>
#include <sys/device.h>
#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/pio.h>
#include <machine/platform.h>
#include <mips/pte.h>

#include <dev/pci/pcivar.h>

#include <arc/arc/arcbios.h>
#include <arc/arc/wired_map.h>
#include <arc/jazz/pica.h>
#include <arc/jazz/rd94.h>
#include <arc/jazz/jazziovar.h>
#include <arc/pci/necpbvar.h>

#include "tga.h"
#if NTGA > 0
#include <dev/pci/tgavar.h>
#endif

#include "vga_pci.h"
#if NVGA_PCI > 0
#include <dev/pci/vga_pcivar.h>
#endif

#include "rasdisplay_jazzio.h"
#if NRASDISPLAY_JAZZIO > 0
#include <arc/jazz/rasdisplay_jazziovar.h>
#endif

#include "pckbc_jazzio.h"
#if NPCKBC_JAZZIO > 0
#include <dev/ic/pckbcvar.h>
#include <arc/jazz/pckbc_jazzioreg.h>
#endif

#include "com.h"
#if NCOM > 0
#include <sys/termios.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

char *c_nec_pci_mainbusdevs[] = {
	"jazzio",
	"necpb",
	NULL,
};

/*
 * chipset-dependent jazzio bus configuration
 */

void jazzio_nec_pci_set_iointr_mask __P((int));

struct jazzio_config jazzio_nec_pci_conf = {
	RD94_SYS_INTSTAT1,
	jazzio_nec_pci_set_iointr_mask,
	RD94_SYS_TL_BASE,
	RD94_SYS_DMA0_REGS,
};

void
jazzio_nec_pci_set_iointr_mask(mask)
	int mask;
{
	/* XXX: I don't know why, but firmware does. */
	if (in32(RD94_V_LOCAL_IO_BASE + 0x560) != 0)
		out16(RD94_SYS_LB_IE2, mask);
	else
		out16(RD94_SYS_LB_IE1, mask);
}

/*
 * critial i/o space, interrupt, and other chipset related initialization.
 */
void
c_nec_pci_init()
{
	/*
	 * Initialize I/O address offset
	 */
	arc_bus_space_init(&jazzio_bus, "jazzio",
	    RD94_P_LOCAL_IO_BASE, RD94_V_LOCAL_IO_BASE,
	    RD94_V_LOCAL_IO_BASE, RD94_S_LOCAL_IO_BASE);

	arc_bus_space_init(&arc_bus_io, "rd94pciio",
	    RD94_P_PCI_IO, RD94_V_PCI_IO, 0, RD94_S_PCI_IO);
	arc_bus_space_init(&arc_bus_mem, "rd94pcimem",
	    RD94_P_PCI_MEM, RD94_V_PCI_MEM, 0, RD94_S_PCI_MEM);

	/*
	 * Initialize wired TLB for I/O space which is used on early stage
	 */
	arc_enter_wired(RD94_V_LOCAL_IO_BASE, RD94_P_LOCAL_IO_BASE, 0,
	    MIPS3_PG_SIZE_256K);
	arc_enter_wired(RD94_V_PCI_IO, RD94_P_PCI_IO, RD94_P_PCI_MEM,
	    MIPS3_PG_SIZE_16M);

	/*
	 * By default, reserve 32MB in KSEG2 for PCI memory space.
	 * Since kseg2iobufsize/NBPG*4 bytes are used for Sysmap,
	 * this consumes 32KB physical memory.
	 *
	 * If a kernel with "options DIAGNOSTIC" panics with
	 * the message "pmap_enter: kva too big", you have to
	 * increase this value by a option like below:
	 *     options KSEG2IOBUFSIZE=0x1b000000 # 432MB consumes 432KB
	 * If you met this symptom, please report it to
	 * port-arc-maintainer@netbsd.org.
	 *
	 * kseg2iobufsize will be refered from pmap_bootstrap().
	 */
	kseg2iobufsize = 0x02000000; /* 32MB: consumes 32KB for PTEs */

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

	/* chipset-dependent jazzio bus configuration */
	jazzio_conf = &jazzio_nec_pci_conf;
}

/*
 * console initialization
 */
void
c_nec_pci_cons_init()
{
	if (!com_console) {
		if (strcmp(arc_displayc_id, "10110004") == 0) {
			/* NEC RISCstation 2200 PCI TGA [NEC-RA94] */
			/* NEC RISCstation 2250 PCI TGA [NEC-RD94] */
			/* NEC Express 5800/230 R4400 PCI TGA [NEC-JC94] */
			/* NEC Express 5800/230 R10000 PCI TGA [[NEC-J95] */
#if NTGA > 0
			necpb_init(&necpb_main_context);
			/* XXX device number is hardcoded */
			if (tga_cnattach(&necpb_main_context.nc_iot,
			    &necpb_main_context.nc_memt,
			    &necpb_main_context.nc_pc, 0, 3, 0) == 0) {
#if NPCKBC_JAZZIO > 0
				pckbc_cnattach(&jazzio_bus, PICA_SYS_KBD,
				    JAZZIO_KBCMDP, PCKBC_KBD_SLOT);
#endif
				return;
			}
#endif
		} else if (strcmp(arc_displayc_id, "53335631") == 0
			/* NEC RISCstation 2200 PCI VGA S3 ViRGE [NEC-RA'94] */
		    || strcmp(arc_displayc_id, "3D3D0001") == 0
			/* NEC RISCstation 2200 PCI VGA 3Dlab GLINT 300SX */
		    ) {
			/* XXX - the followings are not really tested */
#if NVGA_PCI > 0
			necpb_init(&necpb_main_context);
			/* XXX device number is hardcoded */
			if (vga_pci_cnattach(&necpb_main_context.nc_iot,
			    &necpb_main_context.nc_memt,
			    &necpb_main_context.nc_pc, 0, 3, 0) == 0) {
#if NPCKBC_JAZZIO > 0
				pckbc_cnattach(&jazzio_bus, PICA_SYS_KBD,
				    JAZZIO_KBCMDP, PCKBC_KBD_SLOT);
#endif
				return;
			}
#endif
		} else {
			printf("nec_pci: unknown display controller [%s]\n",
			    arc_displayc_id);
		}
	}

#if NCOM > 0
	if (com_console_address == 0)
		com_console_address = RD94_SYS_COM1;
	comcnattach(&jazzio_bus, com_console_address,
	    com_console_speed, com_freq, com_console_mode);
#endif
}
