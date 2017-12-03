/*	$NetBSD: imx7_board.c,v 1.3.2.2 2017/12/03 11:35:53 jdolecek Exp $	*/

/*
 * Copyright (c) 2012  Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imx7_board.c,v 1.3.2.2 2017/12/03 11:35:53 jdolecek Exp $");

#include "opt_imx.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <arm/locore.h>
#include <arm/mainbus/mainbus.h>
#include <arm/cortex/gtmr_var.h>
#include <arm/imx/imx7var.h>
#include <arm/imx/imx7reg.h>
#include <arm/imx/imx7_gpcreg.h>
#include <arm/imx/imx7_ccmreg.h>
#include <arm/imx/imx7_srcreg.h>
#include <arm/imx/imxwdogreg.h>

static void imx7_soc_init(void);

bus_space_tag_t imx7_ioreg_bst = &armv7_generic_bs_tag;
bus_space_handle_t imx7_ioreg_bsh;
bus_space_tag_t imx7_armcore_bst = &armv7_generic_bs_tag;
bus_space_handle_t imx7_armcore_bsh;

void
imx7_bootstrap(vaddr_t iobase)
{
	int error;

	/* imx7_ioreg is mapped AIPS1+AIPS2+AIPS3 region */
	imx7_ioreg_bsh = (bus_space_handle_t)iobase;
	error = bus_space_map(imx7_ioreg_bst, IMX7_IOREG_PBASE,
	    IMX7_IOREG_SIZE, 0, &imx7_ioreg_bsh);
	if (error)
		panic("%s: failed to map Imx %s registers: %d",
		    __func__, "io", error);

	imx7_armcore_bsh = (bus_space_handle_t) iobase + IMX7_IOREG_SIZE;
	error = bus_space_map(imx7_armcore_bst, IMX7_ARMCORE_PBASE,
	    IMX7_ARMCORE_SIZE, 0, &imx7_armcore_bsh);
	if (error)
		panic("%s: failed to map Imx %s registers: %d",
		    __func__, "armcore", error);

	imx7_soc_init();

#ifdef MULTIPROCESSOR
	arm_cpu_max = __SHIFTOUT(armreg_l2ctrl_read(), L2CTRL_NUMCPU) + 1;
	membar_producer();
#endif /* MULTIPROCESSOR */
}

uint32_t
imx7_chip_id(void)
{
	uint32_t v;

	v = bus_space_read_4(imx7_ioreg_bst, imx7_ioreg_bsh,
	    AIPS1_CCM_ANALOG_BASE + CCM_ANALOG_DIGPROG);
	return v;
}

static void
imx7_soc_init(void)
{
	uint32_t v;

	/* enable clocks */
	bus_space_write_4(imx7_ioreg_bst, imx7_ioreg_bsh,
	    AIPS1_CCM_BASE + CCM_CLKROOT_IPG_CLK_ROOT, 0x10000001);
	bus_space_write_4(imx7_ioreg_bst, imx7_ioreg_bsh,
	    AIPS1_CCM_BASE + CCM_CLKROOT_ENET1_REF_CLK_ROOT, 0x11000000);
	bus_space_write_4(imx7_ioreg_bst, imx7_ioreg_bsh,
	    AIPS1_CCM_BASE + CCM_CLKROOT_ENET2_REF_CLK_ROOT, 0x11000000);

	/* XXX: from linux imx_gpcv2_init(void) */
	bus_space_write_4(imx7_ioreg_bst, imx7_ioreg_bsh,
	    AIPS1_GPC_BASE + GPC_IMR1_CORE0_A7, 0xffffffff);
	bus_space_write_4(imx7_ioreg_bst, imx7_ioreg_bsh,
	    AIPS1_GPC_BASE + GPC_IMR2_CORE0_A7, 0xffffffff);
	bus_space_write_4(imx7_ioreg_bst, imx7_ioreg_bsh,
	    AIPS1_GPC_BASE + GPC_IMR3_CORE0_A7, 0xffffffff);
	bus_space_write_4(imx7_ioreg_bst, imx7_ioreg_bsh,
	    AIPS1_GPC_BASE + GPC_IMR4_CORE0_A7, 0xffffffff);
	bus_space_write_4(imx7_ioreg_bst, imx7_ioreg_bsh,
	    AIPS1_GPC_BASE + GPC_IMR1_CORE1_A7, 0xffffffff);
	bus_space_write_4(imx7_ioreg_bst, imx7_ioreg_bsh,
	    AIPS1_GPC_BASE + GPC_IMR2_CORE1_A7, 0xffffffff);
	bus_space_write_4(imx7_ioreg_bst, imx7_ioreg_bsh,
	    AIPS1_GPC_BASE + GPC_IMR3_CORE1_A7, 0xffffffff);
	bus_space_write_4(imx7_ioreg_bst, imx7_ioreg_bsh,
	    AIPS1_GPC_BASE + GPC_IMR4_CORE1_A7, 0xffffffff);

	bus_space_write_4(imx7_ioreg_bst, imx7_ioreg_bsh,
	    AIPS1_GPC_BASE + GPC_IMR1_CORE0_A7, 0);
	bus_space_write_4(imx7_ioreg_bst, imx7_ioreg_bsh,
	    AIPS1_GPC_BASE + GPC_IMR2_CORE0_A7, 0);
	bus_space_write_4(imx7_ioreg_bst, imx7_ioreg_bsh,
	    AIPS1_GPC_BASE + GPC_IMR3_CORE0_A7, 0);
	bus_space_write_4(imx7_ioreg_bst, imx7_ioreg_bsh,
	    AIPS1_GPC_BASE + GPC_IMR4_CORE0_A7, 0);
	bus_space_write_4(imx7_ioreg_bst, imx7_ioreg_bsh,
	    AIPS1_GPC_BASE + GPC_IMR1_CORE1_A7, 0);
	bus_space_write_4(imx7_ioreg_bst, imx7_ioreg_bsh,
	    AIPS1_GPC_BASE + GPC_IMR2_CORE1_A7, 0);
	bus_space_write_4(imx7_ioreg_bst, imx7_ioreg_bsh,
	    AIPS1_GPC_BASE + GPC_IMR3_CORE1_A7, 0);
	bus_space_write_4(imx7_ioreg_bst, imx7_ioreg_bsh,
	    AIPS1_GPC_BASE + GPC_IMR4_CORE1_A7, 0);

	v = bus_space_read_4(imx7_ioreg_bst, imx7_ioreg_bsh,
	    AIPS1_GPC_BASE + GPC_LPCR_A7_BSC);
	v |= GPC_LPCR_A7_BSC_IRQ_SRC_A7_WUP;
	bus_space_write_4(imx7_ioreg_bst, imx7_ioreg_bsh,
	    AIPS1_GPC_BASE + GPC_LPCR_A7_BSC, v);

	bus_space_write_4(imx7_ioreg_bst, imx7_ioreg_bsh,
	    AIPS1_GPC_BASE + GPC_PGC_CPU_MAPPING,
	    GPC_PGC_CPU_MAPPING_FASTMEGA_A7_DOMAIN);

	bus_space_write_4(imx7_ioreg_bst, imx7_ioreg_bsh,
	    AIPS1_GPC_BASE + GPC_PGC_SCU_AUXSW,
	    __SHIFTIN(0x51, GPC_PGC_SCU_AUXSW_MEMPWR_TRC1_TMC) |
	    __SHIFTIN(0x59, GPC_PGC_SCU_AUXSW_L2RETN_TRC1_TMC_TMR) |
	    __SHIFTIN(0x5b, GPC_PGC_SCU_AUXSW_DFTRAM_TRC1_TMC_TMR_TCD2));

	bus_space_write_4(imx7_ioreg_bst, imx7_ioreg_bsh,
	    AIPS1_GPC_BASE + GPC_PGC_ACK_SEL_A7,
	    GPC_PGC_ACK_SEL_A7_A7_PGC_PUP_ACK |
	    GPC_PGC_ACK_SEL_A7_A7_PGC_PDN_ACK);

	v = bus_space_read_4(imx7_ioreg_bst, imx7_ioreg_bsh,
	    AIPS1_GPC_BASE + GPC_SLPCR);
	v &= ~GPC_SLPCR_EN_DSM;
	v &= ~GPC_SLPCR_RBC_EN;
	v &= ~GPC_SLPCR_VSTBY;
	v &= ~GPC_SLPCR_SBYOS;
	v &= ~GPC_SLPCR_BYPASS_PMIC_READY;
	v |= GPC_SLPCR_EN_A7_FASTWUP_WAIT_MODE;
	bus_space_write_4(imx7_ioreg_bst, imx7_ioreg_bsh,
	    AIPS1_GPC_BASE + GPC_SLPCR, v);

	v = bus_space_read_4(imx7_ioreg_bst, imx7_ioreg_bsh,
	    AIPS1_GPC_BASE + GPC_MLPCR);
	v |= GPC_MLPCR_MEMLP_CTL_DIS;
	bus_space_write_4(imx7_ioreg_bst, imx7_ioreg_bsh,
	    AIPS1_GPC_BASE + GPC_MLPCR, v);
}

void
imx7_reset(void)
{
	delay(100000);	/* wait for flushing FIFO of serial console */

	/* reset */
	cpsid(I32_bit|F32_bit);

	/* software reset signal on wdog */
	bus_space_write_2(imx7_ioreg_bst, imx7_ioreg_bsh,
	    AIPS1_WDOG1_BASE + IMX_WDOG_WCR, WCR_WDE);

	for (;;)
		__asm("wfi");
}

uint32_t
imx7_armrootclk(void)
{
	uint32_t clk;
	uint32_t v;

	v = bus_space_read_4(imx7_ioreg_bst, imx7_ioreg_bsh,
	    AIPS1_CCM_ANALOG_BASE + CCM_ANALOG_PLL_ARM);
	clk = IMX7_OSC_FREQ * (v & CCM_ANALOG_PLL_ARM_DIV_SELECT) / 2;
	return clk;
}

void
imx7_device_register(device_t self, void *aux)
{
	prop_dictionary_t dict = device_properties(self);

	if (device_is_a(self, "armperiph") &&
	    device_is_a(device_parent(self), "mainbus")) {
		/*
		 * XXX KLUDGE ALERT XXX
		 * The iot mainbus supplies is completely wrong since it scales
		 * addresses by 2.  The simpliest remedy is to replace with our
		 * bus space used for the armcore registers
		 * (which armperiph uses).
		 */
		struct mainbus_attach_args * const mb = aux;
		mb->mb_iot = imx7_armcore_bst;
		return;
	}

	/*
	 * We need to tell the armgtmr what frequency it runs at.
	 */
	if (device_is_a(self, "armgtmr")) {
		prop_dictionary_set_uint32(dict, "frequency",
		    armreg_cnt_frq_read());
		return;
	}
}

#ifdef MULTIPROCESSOR
void
imx7_cpu_hatch(struct cpu_info *ci)
{
	gtmr_init_cpu_clock(ci);
}
#endif
