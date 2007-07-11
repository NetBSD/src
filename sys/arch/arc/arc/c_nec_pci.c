/*	$NetBSD: c_nec_pci.c,v 1.13.32.1 2007/07/11 19:57:56 mjf Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: c_nec_pci.c,v 1.13.32.1 2007/07/11 19:57:56 mjf Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kcore.h>
#include <sys/device.h>
#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/pio.h>
#include <machine/platform.h>
#include <machine/wired_map.h>
#include <mips/pte.h>

#include <dev/clock_subr.h>
#include <dev/ic/mc146818var.h>

#include <dev/pci/pcivar.h>

#include <arc/arc/arcbios.h>
#include <arc/jazz/pica.h>
#include <arc/jazz/rd94.h>
#include <arc/jazz/jazziovar.h>
#include <arc/jazz/mcclock_jazziovar.h>
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

const char *c_nec_pci_mainbusdevs[] = {
	"jazzio",
	"necpb",
	NULL,
};

/*
 * chipset-dependent mcclock routines.
 */

static u_int	mc_nec_pci_read(struct mc146818_softc *, u_int);
static void	mc_nec_pci_write(struct mc146818_softc *, u_int, u_int);

struct mcclock_jazzio_config mcclock_nec_pci_conf = {
	0x80004000,		/* I/O base */
	2,			/* I/O size */
	mc_nec_pci_read,	/* read function */
	mc_nec_pci_write	/* write function */
};

/*
 * This is a mask of bits to clear in the SR when we go to a
 * given interrupt priority level.
 */
static const uint32_t nec_pci_ipl_sr_bits[_IPL_N] = {
	[IPL_NONE] = 0,
	[IPL_SOFT] =
	    MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTCLOCK] =
	    MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTNET] =
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1,
	[IPL_SOFTSERIAL] =
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1,
	[IPL_BIO] =
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1 |
	    MIPS_INT_MASK_0 |
	    MIPS_INT_MASK_1 |
	    MIPS_INT_MASK_2,
	[IPL_NET] =
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1 |
	    MIPS_INT_MASK_0 |
	    MIPS_INT_MASK_1 |
	    MIPS_INT_MASK_2,
	[IPL_TTY] =
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1 |
	    MIPS_INT_MASK_0 |
	    MIPS_INT_MASK_1 | 
	    MIPS_INT_MASK_2,
	[IPL_CLOCK] =
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1 |
	    MIPS_INT_MASK_0 |
	    MIPS_INT_MASK_1 |
	    MIPS_INT_MASK_2 |
	    MIPS_INT_MASK_3 |
	    MIPS_INT_MASK_4 |
	    MIPS_INT_MASK_5,
};

static u_int
mc_nec_pci_read(struct mc146818_softc *sc, u_int reg)
{
	u_int i, as;

	as = bus_space_read_1(sc->sc_bst, sc->sc_bsh, 1) & 0x80;
	bus_space_write_1(sc->sc_bst, sc->sc_bsh, 1, as | reg);
	i = bus_space_read_1(sc->sc_bst, sc->sc_bsh, 0);
	return i;
}

static void
mc_nec_pci_write(struct mc146818_softc *sc, u_int reg, u_int datum)
{
	u_int as;

	as = bus_space_read_1(sc->sc_bst, sc->sc_bsh, 1) & 0x80;
	bus_space_write_1(sc->sc_bst, sc->sc_bsh, 1, as | reg);
	bus_space_write_1(sc->sc_bst, sc->sc_bsh, 0, datum);
}

/*
 * chipset-dependent jazzio bus configuration
 */

void jazzio_nec_pci_set_iointr_mask(int);

struct jazzio_config jazzio_nec_pci_conf = {
	RD94_SYS_INTSTAT1,
	jazzio_nec_pci_set_iointr_mask,
	RD94_SYS_TL_BASE,
	RD94_SYS_DMA0_REGS,
};

void
jazzio_nec_pci_set_iointr_mask(int mask)
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
c_nec_pci_init(void)
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
	arc_wired_enter_page(RD94_V_LOCAL_IO_BASE, RD94_P_LOCAL_IO_BASE,
	    RD94_S_LOCAL_IO_BASE);
	/*
	 * allocate only 16M for PCM MEM space for now to save wired TLB entry;
	 * Other regions will be allocalted by bus_space_large.c later.
	 */
	arc_wired_enter_page(RD94_V_PCI_IO, RD94_P_PCI_IO, RD94_S_PCI_IO);
	arc_wired_enter_page(RD94_V_PCI_MEM, RD94_P_PCI_MEM, RD94_S_PCI_IO);

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
	 * port-arc-maintainer@NetBSD.org.
	 *
	 * kseg2iobufsize will be refered from pmap_bootstrap().
	 */
	kseg2iobufsize = 0x02000000; /* 32MB: consumes 32KB for PTEs */

	/*
	 * Initialize interrupt priority
	 */
	ipl_sr_bits = nec_pci_ipl_sr_bits;

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

	/* chipset-dependent mcclock configuration */
	mcclock_jazzio_conf = &mcclock_nec_pci_conf;

	/* chipset-dependent jazzio bus configuration */
	jazzio_conf = &jazzio_nec_pci_conf;
}

/*
 * console initialization
 */
void
c_nec_pci_cons_init(void)
{

	if (!com_console) {
		if (strcmp(arc_displayc_id, "10110004") == 0) {
			/* NEC RISCstation 2200 PCI TGA [NEC-RA94] */
			/* NEC RISCstation 2250 PCI TGA [NEC-RD94] */
			/* NEC Express 5800/230 R4400 PCI TGA [NEC-JC94] */
			/* NEC Express 5800/230 R10000 PCI TGA [NEC-J95] */
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
	    com_console_speed, com_freq, COM_TYPE_NORMAL, com_console_mode);
#endif
}
