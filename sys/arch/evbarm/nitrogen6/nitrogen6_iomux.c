/*	$NetBSD: nitrogen6_iomux.c,v 1.1.2.2 2015/09/22 12:05:40 skrll Exp $	*/

/*
 * Copyright (c) 2015 Ryo Shimizu <ryo@nerv.org>
 * All rights reserved.
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nitrogen6_iomux.c,v 1.1.2.2 2015/09/22 12:05:40 skrll Exp $");

#include "opt_evbarm_boardtype.h"
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/param.h>
#include <arm/imx/imx6_reg.h>
#include <arm/imx/imx6var.h>
#include <arm/imx/imx6_iomuxreg.h>
#include <evbarm/nitrogen6/platform.h>

void nitrogen6_setup_iomux(void);
static void nitrogen6_mux_config(const struct iomux_conf *);

#define	nitrogen6x	__LINE__
#define	nitrogen6max	__LINE__
#define	cubox_i		__LINE__

static const struct iomux_conf iomux_data[] = {
#if (EVBARM_BOARDTYPE == nitrogen6x) || \
    (EVBARM_BOARDTYPE == nitrogen6max)
	/* UART1 RX */
	{
		.pin = MUX_PIN(SD3_DATA6),
		.mux = IOMUX_CONFIG_ALT1,
		.pad = PAD_CTL_HYS | PAD_CTL_PUS_22K_PU | PAD_CTL_PULL |
		    PAD_CTL_SPEED_100MHZ | PAD_CTL_DSE_40OHM | PAD_CTL_SRE_FAST
	},
	{
		.pin = IOMUX_PIN(IOMUX_MUX_NONE,
		    IOMUXC_UART1_UART_RX_DATA_SELECT_INPUT),
		.pad = INPUT_DAISY_3
	},
	/* UART1 TX */
	{
		.pin = MUX_PIN(SD3_DATA7),
		.mux = IOMUX_CONFIG_ALT1,
		.pad = PAD_CTL_HYS | PAD_CTL_PUS_22K_PU | PAD_CTL_PULL |
		    PAD_CTL_ODE |
		    PAD_CTL_SPEED_100MHZ | PAD_CTL_DSE_40OHM | PAD_CTL_SRE_FAST
	},
#endif

	/* end of table */
	{.pin = IOMUX_CONF_EOT}
};

#define AIPS1_WRITE(addr, val)	\
	(*(volatile uint32_t *)(KERNEL_IO_IOREG_VBASE + (addr)) = (val))
#define IOMUX_WRITE(reg, val)	\
	AIPS1_WRITE(AIPS1_IOMUXC_BASE + (reg), val)

static void
nitrogen6_mux_config(const struct iomux_conf *conflist)
{
	int i;
	uint32_t reg;

	for (i = 0; conflist[i].pin != IOMUX_CONF_EOT; i++) {
		reg = IOMUX_PIN_TO_PAD_ADDRESS(conflist[i].pin);
		if (reg != IOMUX_PAD_NONE)
			IOMUX_WRITE(reg, conflist[i].pad);

		reg = IOMUX_PIN_TO_MUX_ADDRESS(conflist[i].pin);
		if (reg != IOMUX_MUX_NONE)
			IOMUX_WRITE(reg, conflist[i].mux);
	}
}

void
nitrogen6_setup_iomux(void)
{
	nitrogen6_mux_config(iomux_data);
}
