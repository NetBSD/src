/*	$NetBSD: obs600_autoconf.c,v 1.8 2019/02/18 06:27:10 msaitoh Exp $	*/

/*
 * Copyright 2004 Shigeyuki Fukushima.
 * All rights reserved.
 *
 * Written by Shigeyuki Fukushima for The NetBSD Project.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: obs600_autoconf.c,v 1.8 2019/02/18 06:27:10 msaitoh Exp $");

#include "dwctwo.h"

#include <sys/systm.h>
#include <sys/device.h>
#include <sys/cpu.h>
#include <sys/intr.h>

#include <machine/obs600.h>

#include <powerpc/ibm4xx/cpu.h>
#include <powerpc/ibm4xx/dcr4xx.h>

#include <dev/ic/comreg.h>

#if NDWCTWO > 0
#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>

#include <dwc2/dwc2.h>
#include <dwc2/dwc2var.h>
#include "dwc2_core.h"

/* This parameters was set from u-boot. */
static struct dwc2_core_params dwctwo_obs600_params = {
	.otg_cap			= 0,	/* HNP/SRP capable */
	.otg_ver			= 0,	/* 1.3 */
	.dma_enable			= 1,
	.dma_desc_enable		= 0,
	.speed				= 0,	/* High Speed */
	.enable_dynamic_fifo		= 1,
	.en_multiple_tx_fifo		= 0,
	.host_rx_fifo_size		= 531,	/* 531 DWORDs */
	.host_nperio_tx_fifo_size	= 256,	/* 256 DWORDs */
	.host_perio_tx_fifo_size	= 256,	/* 256 DWORDs */
	.max_transfer_size		= 524287,
	.max_packet_count		= 1023,
	.host_channels			= 4,
	.phy_type			= 2,	/* ULPI */
	.phy_utmi_width			= 8,	/* 8 bits */
	.phy_ulpi_ddr			= 0,	/* Single */
	.phy_ulpi_ext_vbus		= 0,
	.i2c_enable			= 0,
	.ulpi_fs_ls			= 0,
	.host_support_fs_ls_low_power	= 0,
	.host_ls_low_power_phy_clk	= 0,	/* 48 MHz */
	.ts_dline			= 0,
	.reload_ctl			= 0,
	.ahbcfg				= 0x10,
	.uframe_sched			= 1,
};
#endif


/*
 * Determine device configuration for a machine.
 */
void
cpu_configure(void)
{

	/* Initialize intr and add UICs */
	intr_init();
	pic_add(&pic_uic1);
	pic_add(&pic_uic2);

	calc_delayconst();

	/* Make sure that timers run at CPU frequency */
	mtdcr(DCR_CPC0_CR1, mfdcr(DCR_CPC0_CR1) & ~CPC0_CR1_CETE);

	if (config_rootfound("plb", NULL) == NULL)
		panic("configure: mainbus not configured");

	pic_finish_setup();

	printf("biomask %x netmask %x ttymask %x\n",
	    imask[IPL_BIO], imask[IPL_NET], imask[IPL_TTY]);

	(void)spl0();
}

void
device_register(device_t dev, void *aux)
{

#if NDWCTWO > 0
	if (device_is_a(dev, "dwctwo")) {
		prop_dictionary_t dict = device_properties(dev);

		prop_dictionary_set_uint32(dict, "params",
		    (uint32_t)&dwctwo_obs600_params);
		return;
	}
#endif

	obs405_device_register(dev, aux, OBS600_COM_FREQ);
}
