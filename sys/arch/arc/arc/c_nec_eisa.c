/*	$NetBSD: c_nec_eisa.c,v 1.18 2018/02/08 09:05:17 dholland Exp $	*/

/*-
 * Copyright (c) 2003 Izumi Tsutsui.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: c_nec_eisa.c,v 1.18 2018/02/08 09:05:17 dholland Exp $");

#define __INTR_PRIVATE
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kcore.h>
#include <sys/device.h>
#include <sys/intr.h>

#include <uvm/uvm_extern.h>

#include <mips/locore.h>

#include <machine/autoconf.h>
#include <machine/pio.h>
#include <machine/platform.h>
#include <machine/wired_map.h>
#include <mips/pte.h>

#include <dev/isa/isavar.h>

#include <arc/arc/arcbios.h>
#include <arc/jazz/pica.h>
#include <arc/jazz/rd94.h>
#include <arc/jazz/jazziovar.h>
#include <arc/isa/isabrvar.h>

#include "vga_isa.h"
#if NVGA_ISA > 0
#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#include <dev/ic/vgareg.h>
#include <dev/ic/vgavar.h>

#ifdef VGA_RESET
static void gd54xx_initregs(struct vga_handle *);
#endif
#endif

/*
 * chipset-dependent isa bus configuration
 */

int isabr_nec_eisa_intr_status(void);

struct isabr_config isabr_nec_eisa_conf = {
	isabr_nec_eisa_intr_status,
};

/*
 * This is a mask of bits to clear in the SR when we go to a
 * given interrupt priority level.
 */
static const struct ipl_sr_map nec_eisa_ipl_sr_map = {
    .sr_bits = {
	[IPL_NONE] =		0,
	[IPL_SOFTCLOCK] =	MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTNET] =		MIPS_SOFT_INT_MASK,
	[IPL_VM] =		MIPS_SOFT_INT_MASK
				| MIPS_INT_MASK_0
				| MIPS_INT_MASK_1
				| MIPS_INT_MASK_2,
	[IPL_SCHED] =		MIPS_INT_MASK,
	[IPL_DDB] =		MIPS_INT_MASK,
	[IPL_HIGH] =		MIPS_INT_MASK,
    },
};

int
isabr_nec_eisa_intr_status(void)
{

	return in32(RD94_SYS_INTSTAT2) & (ICU_LEN - 1);
}

/*
 * chipset-dependent jazzio bus configuration
 */

void jazzio_nec_eisa_set_iointr_mask(int);

struct jazzio_config jazzio_nec_eisa_conf = {
	RD94_SYS_INTSTAT1,
	jazzio_nec_eisa_set_iointr_mask,
	RD94_SYS_TL_BASE,
	RD94_SYS_DMA0_REGS,
};

void
jazzio_nec_eisa_set_iointr_mask(int mask)
{

	out16(RD94_SYS_LB_IE2, mask);
}

/*
 * critial i/o space, interrupt, and other chipset related initialization.
 */
void
c_nec_eisa_init(void)
{

	/*
	 * Initialize interrupt priority
	 */
	ipl_sr_map = nec_eisa_ipl_sr_map;

	/*
	 * Initialize I/O address offset
	 */
	arc_bus_space_init(&jazzio_bus, "jazzio",
	    RD94_P_LOCAL_IO_BASE, RD94_V_LOCAL_IO_BASE,
	    RD94_V_LOCAL_IO_BASE, RD94_S_LOCAL_IO_BASE);

	arc_bus_space_init(&arc_bus_io, "r94eisaio",
	    RD94_P_PCI_IO, RD94_V_EISA_IO, 0, RD94_S_EISA_IO);
	arc_bus_space_init(&arc_bus_mem, "r94eisamem",
	    RD94_P_PCI_MEM, RD94_V_EISA_MEM, 0, RD94_S_EISA_MEM);

	/*
	 * Initialize wired TLB for I/O space which is used on early stage
	 */
	arc_init_wired_map();
	arc_wired_enter_page(RD94_V_LOCAL_IO_BASE, RD94_P_LOCAL_IO_BASE,
	    RD94_S_LOCAL_IO_BASE);

	arc_wired_enter_page(RD94_V_EISA_IO, RD94_P_EISA_IO, RD94_S_EISA_IO);
	arc_wired_enter_page(RD94_V_EISA_MEM, RD94_P_EISA_MEM,
	    MIPS3_PG_SIZE_MASK_TO_SIZE(MIPS3_PG_SIZE_16M));

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

void
c_nec_eisa_cons_init(void)
{

#if NVGA_ISA > 0
	if (!com_console) {
		if (strcmp(arc_displayc_id, "necvdfrb") == 0) {
			/* NEC RISCserver 2200 R4400 EISA [NEC-R96] */
			/* NEC Express5800/240 R4400 EISA [NEC-J96A] */
#ifdef VGA_RESET
			struct vga_handle handle;

			handle.vh_memt = &arc_bus_mem;
			handle.vh_iot = &arc_bus_io;
			vga_reset(&handle, gd54xx_initregs);
#endif

			vga_no_builtinfont = 1;
		}
	}
#endif

	c_jazz_eisa_cons_init();
}

#if NVGA_ISA > 0 && defined(VGA_RESET)

/* values to initialize cirrus GD54xx specific ext registers */
/* XXX these values are taken from PC XXX */
static const uint8_t vga_ts_gd54xx[] = {
	0x0f,	/* 05: ??? */
	0x12,	/* 06: enable ext reg (?) */
	0x00,	/* 07: reset ext sequence (?) */
	0x00,	/* 08: ??? */
	0x5c,	/* 09: ??? */
	0x09,	/* 0A: BIOS Scratch register for 542x (?) */
	0x4a,	/* 0B: ??? */
	0x5b,	/* 0C: ??? */
	0x42,	/* 0D: VCLK2 frequency */
	0x00,	/* 0E: VCLK3 frequency */
	0x09,	/* 0F: ??? */
	0x00,	/* 10: ??? */
	0x00,	/* 11: ??? */
	0x00,	/* 12: ??? */
	0x00,	/* 13: ??? */
	0x00,	/* 14: BIOS scratch register for 546x (?) */
	0x00,	/* 15: ??? */
	0xd8,	/* 16: ??? */
	0x39,	/* 17: ??? */
	0x00,	/* 18: ??? */
	0x01,	/* 19: ??? */
	0x00,	/* 1A: ??? */
	0x2b,	/* 1B: ??? */
	0x2f,	/* 1C: ??? */
	0x1f,	/* 1D: VCLK2 denominator and post-scalar value */
	0x00,	/* 1E: VCLK3 denominator and post-scalar value */
	0x19	/* 1F: MCLK (?) */
};

static void
gd54xx_initregs(struct vga_handle *vh)
{
	int i;

	/* disable video */
	vga_ts_write(vh, mode, vga_ts_read(vh, mode) | VGA_TS_MODE_BLANK);

	/* enable access to GD54xx ext regs */
	_vga_ts_write(vh, 0x06, 0x12);

	/* setup GD54xx ext regs */
	for (i = 0; i < sizeof(vga_ts_gd54xx); i++)
		_vga_ts_write(vh, VGA_TS_NREGS + i, vga_ts_gd54xx[i]);
	
	/* disable access to GD54xx ext regs */
	_vga_ts_write(vh, 0x06, 0x0);

	/* reenable video */
	vga_ts_write(vh, mode, vga_ts_read(vh, mode) & ~VGA_TS_MODE_BLANK);
}
#endif
