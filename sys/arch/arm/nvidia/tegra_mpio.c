/* $NetBSD: tegra_mpio.c,v 1.1 2015/05/07 23:55:11 jmcneill Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "locators.h"
#include "opt_ddb.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tegra_mpio.c,v 1.1 2015/05/07 23:55:11 jmcneill Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/gpio.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_mpioreg.h>
#include <arm/nvidia/tegra_var.h>

static int	tegra_mpio_match(device_t, cfdata_t, void *);
static void	tegra_mpio_attach(device_t, device_t, void *);

struct tegra_mpio_padgrp {
	u_int			pg_reg;
	uint32_t		pg_preemp;
	uint32_t		pg_hsm;
	uint32_t		pg_schmt;
	uint32_t		pg_drv_type;
	uint32_t		pg_drvdn;
	uint32_t		pg_drvup;
	uint32_t		pg_slwr;
	uint32_t		pg_slwf;
};

#define PADGRP(_n, _p, _dt, _dd, _du, _slwf)	\
	{					\
		.pg_reg = PADGRP_ ## _n ## _REG,\
		.pg_preemp = (_p),		\
		.pg_hsm = __BIT(2),		\
		.pg_schmt = __BIT(3),		\
		.pg_drv_type = (_dt),		\
		.pg_drvdn = (_dd),		\
		.pg_drvup = (_du),		\
		.pg_slwr = __BITS(29,28),	\
		.pg_slwf = (_slwf)		\
	}

static const struct tegra_mpio_padgrp tegra_mpio_padgrp[] = {
	PADGRP(GMACFG, __BIT(0), __BITS(7,6), __BITS(18,14), __BITS(24,20), __BITS(31,30)),
	PADGRP(SDIO1CFG, 0, 0, __BITS(18,12), __BITS(26,20), __BITS(31,30)),
	PADGRP(SDIO3CFG, 0, 0, __BITS(18,12), __BITS(26,20), __BITS(31,30)),
	PADGRP(SDIO4CFG, 0, 0, __BITS(16,12), __BITS(24,20), __BITS(31,30)),
	PADGRP(AOCFG0, 0, 0, __BITS(16,12), __BITS(24,20), __BITS(31,30)),
	PADGRP(AOCFG1, 0, 0, __BITS(16,12), __BITS(24,20), __BITS(31,30)),
	PADGRP(AOCFG2, 0, 0, __BITS(16,12), __BITS(24,20), __BITS(31,30)),
	PADGRP(AOCFG3, 0, 0, __BITS(16,12), 0, 0),
	PADGRP(AOCFG4, 0, __BITS(7,6), __BITS(18,12), __BITS(26,20), __BITS(31,30)),
	PADGRP(CDEV1CFG, 0, 0, __BITS(16,12), __BITS(24,20), __BITS(31,30)),
	PADGRP(CDEV2CFG, 0, 0, __BITS(16,12), __BITS(24,20), __BITS(31,30)),
	PADGRP(CECCFG, 0, 0, __BITS(16,12), __BITS(24,20), __BITS(31,30)),
	PADGRP(DAP1CFG, 0, 0, __BITS(16,12), __BITS(24,20), __BITS(31,30)),
	PADGRP(DAP2CFG, 0, 0, __BITS(16,12), __BITS(24,20), __BITS(31,30)),
	PADGRP(DAP3CFG, 0, 0, __BITS(16,12), __BITS(24,20), __BITS(31,30)),
	PADGRP(DAP4CFG, 0, 0, __BITS(16,12), __BITS(24,20), __BITS(31,30)),
	PADGRP(DAP5CFG, 0, 0, __BITS(16,12), __BITS(24,20), __BITS(31,30)),
	PADGRP(DBGCFG, 0, 0, __BITS(16,12), __BITS(24,20), __BITS(31,30)),
	PADGRP(DDCCFG, 0, 0, __BITS(16,12), __BITS(24,20), __BITS(31,30)),
	PADGRP(DEV3CFG, 0, 0, __BITS(16,12), __BITS(24,20), __BITS(31,30)),
	PADGRP(OWRCFG, 0, 0, __BITS(16,12), __BITS(24,20), __BITS(31,30)),
	PADGRP(SPICFG, 0, 0, __BITS(16,12), __BITS(24,20), __BITS(31,30)),
	PADGRP(UAACFG, 0, 0, __BITS(16,12), __BITS(24,20), __BITS(31,30)),
	PADGRP(UABCFG, 0, 0, __BITS(16,12), __BITS(24,20), __BITS(31,30)),
	PADGRP(UART2CFG, 0, 0, __BITS(16,12), __BITS(24,20), __BITS(31,30)),
	PADGRP(UART3CFG, 0, 0, __BITS(16,12), __BITS(24,20), __BITS(31,30)),
	PADGRP(UDACFG, 0, 0, __BITS(16,12), __BITS(24,20), __BITS(31,30)),
	PADGRP(ATCFG1, 0, __BITS(7,6), __BITS(18,12), __BITS(26,20), __BITS(31,30)),
	PADGRP(ATCFG2, 0, __BITS(7,6), __BITS(18,12), __BITS(26,20), __BITS(31,30)),
	PADGRP(ATCFG3, 0, __BITS(7,6), __BITS(18,12), __BITS(26,20), __BITS(31,30)),
	PADGRP(ATCFG4, 0, __BITS(7,6), __BITS(18,12), __BITS(26,20), __BITS(31,30)),
	PADGRP(ATCFG5, 0, 0, __BITS(18,14), __BITS(23,19), __BITS(31,30)),
	PADGRP(ATCFG6, 0, __BITS(7,6), __BITS(18,12), __BITS(26,20), __BITS(31,30)),
	PADGRP(GMECFG, 0, 0, __BITS(18,14), __BITS(23,19), __BITS(31,30)),
	PADGRP(GMFCFG, 0, 0, __BITS(18,14), __BITS(23,19), __BITS(31,30)),
	PADGRP(GMGCFG, 0, 0, __BITS(18,14), __BITS(23,19), __BITS(31,30)),
	PADGRP(GMHCFG, 0, 0, __BITS(18,14), __BITS(23,19), __BITS(31,30)), 
	PADGRP(HVCFG0, 0, 0, __BITS(16,12), 0, 0),
	PADGRP(GPVCFG, 0, 0, __BITS(16,12), __BITS(24,20), __BITS(31,30)),
	PADGRP(USB_VBUS_EN_CFG, 0, 0, __BITS(16,12), __BITS(24,20), __BITS(31,30)),
};

struct tegra_mpio_pinmux {
	u_int			pm_reg;
	const char *		pm_func[4];
	int			pm_flags;
#define MPIOF_OD		0x1
#define MPIOF_IO_RESET		0x2
#define MPIOF_RCV_SEL		0x4
};

static const struct tegra_mpio_pinmux tegra_mpio_pinmux[] = {
	{ PINMUX_AUX_ULPI_DATA0_REG,	{ "spi3", "hsi", "uarta", "ulpi" }, 0 },
	{ PINMUX_AUX_ULPI_DATA1_REG,	{ "spi3", "hsi", "uarta", "ulpi" }, 0 },
	{ PINMUX_AUX_ULPI_DATA2_REG,	{ "spi3", "hsi", "uarta", "ulpi" }, 0 },
	{ PINMUX_AUX_ULPI_DATA3_REG,	{ "spi3", "hsi", "uarta", "ulpi" }, 0 },
	{ PINMUX_AUX_ULPI_DATA4_REG,	{ "spi3", "hsi", "uarta", "ulpi" }, 0 },
	{ PINMUX_AUX_ULPI_DATA5_REG,	{ "spi3", "hsi", "uarta", "ulpi" }, 0 },
	{ PINMUX_AUX_ULPI_DATA6_REG,	{ "spi3", "hsi", "uarta", "ulpi" }, 0 },
	{ PINMUX_AUX_ULPI_DATA7_REG,	{ "spi3", "hsi", "uarta", "ulpi" }, 0 },
	{ PINMUX_AUX_ULPI_CLK_REG,	{ "spi1", "spi5", "uartd", "ulpi" }, 0 },
	{ PINMUX_AUX_ULPI_DIR_REG,	{ "spi1", "spi5", "uartd", "ulpi" }, 0 },
	{ PINMUX_AUX_ULPI_NXT_REG,	{ "spi1", "spi5", "uartd", "ulpi" }, 0 },
	{ PINMUX_AUX_ULPI_STP_REG,	{ "spi1", "spi5", "uartd", "ulpi" }, 0 },
	{ PINMUX_AUX_DAP3_FS_REG,	{ "i2s2", "spi5", "displaya", "displayb" }, 0 },
	{ PINMUX_AUX_DAP3_DIN_REG,	{ "i2s2", "spi5", "displaya", "displayb" }, 0 },
	{ PINMUX_AUX_DAP3_DOUT_REG,	{ "i2s2", "spi5", "displaya", NULL }, 0 },
	{ PINMUX_AUX_DAP3_SCLK_REG,	{ "i2s2", "spi5", NULL, "displayb" }, 0 },
	{ PINMUX_AUX_GPIO_PV0_REG,	{ NULL, NULL, NULL, NULL }, 0 },
	{ PINMUX_AUX_GPIO_PV1_REG,	{ NULL, NULL, NULL, NULL }, 0 },
	{ PINMUX_AUX_SDMMC1_CLK_REG,	{ "sdmmc1", "clk12", NULL, NULL }, 0 },
	{ PINMUX_AUX_SDMMC1_CMD_REG,	{ "sdmmc1", "spdif", "spi4", "uarta" }, 0 },
	{ PINMUX_AUX_SDMMC1_DAT3_REG,	{ "sdmmc1", "spdif", "spi4", "uarta" }, 0 },
	{ PINMUX_AUX_SDMMC1_DAT2_REG,	{ "sdmmc1", "pwm0", "spi4", "uarta" }, 0 },
	{ PINMUX_AUX_SDMMC1_DAT1_REG,	{ "sdmmc1", "pwm1", "spi4", "uarta" }, 0 },
	{ PINMUX_AUX_SDMMC1_DAT0_REG,	{ "sdmmc1", NULL, "spi4", "uarta" }, 0 },
	{ PINMUX_AUX_CLK2_OUT_REG,	{ "extperiph2", NULL, NULL, NULL }, 0 },
	{ PINMUX_AUX_CLK2_REQ_REG,	{ "dap", NULL, NULL, NULL }, 0 },
	{ PINMUX_AUX_HDMI_INT_REG,	{ NULL, NULL, NULL, NULL }, MPIOF_RCV_SEL },
	{ PINMUX_AUX_DDC_SCL_REG,	{ "i2c4", NULL, NULL, NULL }, MPIOF_RCV_SEL },
	{ PINMUX_AUX_DDC_SDA_REG,	{ "i2c4", NULL, NULL, NULL }, MPIOF_RCV_SEL },
	{ PINMUX_AUX_UART2_RXD_REG,	{ "irda", "spdif", "uarta", "spi4" }, 0 },
	{ PINMUX_AUX_UART2_TXD_REG,	{ "irda", "spdif", "uarta", "spi4" }, 0 },
	{ PINMUX_AUX_UART2_RTS_N_REG,	{ "uarta", "uartb", "gmi", "spi4" }, 0 },
	{ PINMUX_AUX_UART2_CTS_N_REG,	{ "uarta", "uartb", "gmi", "spi4" }, 0 },
	{ PINMUX_AUX_UART3_TXD_REG,	{ "uartc", NULL, "gmi", "spi4" }, 0 },
	{ PINMUX_AUX_UART3_RXD_REG,	{ "uartc", NULL, "gmi", "spi4" }, 0 },
	{ PINMUX_AUX_UART3_CTS_N_REG,	{ "uartc", "sdmmc1", "dtv", "gmi" }, 0 },
	{ PINMUX_AUX_UART3_RTS_N_REG,	{ "uartc", "pwm0", "dtv", "gmi" }, 0 },
	{ PINMUX_AUX_GPIO_PU0_REG,	{ "owr", "uarta", "gmi", NULL }, 0 },
	{ PINMUX_AUX_GPIO_PU1_REG,	{ NULL, "uarta", "gmi", NULL }, 0 },
	{ PINMUX_AUX_GPIO_PU2_REG,	{ NULL, "uarta", "gmi", NULL }, 0 },
	{ PINMUX_AUX_GPIO_PU3_REG,	{ "pwm0", "uarta", "gmi", "displayb" }, 0 },
	{ PINMUX_AUX_GPIO_PU4_REG,	{ "pwm1", "uarta", "gmi", "displayb" }, 0 },
	{ PINMUX_AUX_GPIO_PU5_REG,	{ "pwm2", "uarta", "gmi", "displayb" }, 0 },
	{ PINMUX_AUX_GPIO_PU6_REG,	{ "pwm3", "uarta", NULL, "gmi" }, 0 },
	{ PINMUX_AUX_GEN1_I2C_SDA_REG,	{ "i2c1", NULL, NULL, NULL }, MPIOF_OD },
	{ PINMUX_AUX_GEN1_I2C_SCL_REG,	{ "i2c1", NULL, NULL, NULL }, MPIOF_OD },
	{ PINMUX_AUX_DAP4_FS_REG,	{ "i2s3", "gmi", "dtv", NULL }, 0 },
	{ PINMUX_AUX_DAP4_DIN_REG,	{ "i2s3", "gmi", NULL, NULL }, 0 },
	{ PINMUX_AUX_DAP4_DOUT_REG,	{ "i2s3", "gmi", "dtv", NULL }, 0 },
	{ PINMUX_AUX_DAP4_SCLK_REG,	{ "i2s3", "gmi", NULL, NULL }, 0 },
	{ PINMUX_AUX_CLK3_OUT_REG,	{ "extperiph3", NULL, NULL, NULL }, 0 },
	{ PINMUX_AUX_CLK3_REQ_REG,	{ "dev3", NULL, NULL, NULL }, 0 },
	{ PINMUX_AUX_GPIO_PC7_REG,	{ NULL, NULL, "gmi", "gmi_alt" }, 0 },
	{ PINMUX_AUX_GPIO_PI5_REG,	{ "sdmmc2a", NULL, "gmi", NULL }, 0 },
	{ PINMUX_AUX_GPIO_PI7_REG,	{ NULL, "trace", "gmi", "dtv" }, 0 },
	{ PINMUX_AUX_GPIO_PK0_REG,	{ NULL, "sdmmc3", "gmi", "soc" }, 0 },
	{ PINMUX_AUX_GPIO_PK1_REG,	{ "sdmmc2a", "trace", "gmi", NULL }, 0 },
	{ PINMUX_AUX_GPIO_PJ0_REG,	{ NULL, NULL, "gmi", "usb" }, 0 },
	{ PINMUX_AUX_GPIO_PJ2_REG,	{ NULL, NULL, "gmi", "soc" }, 0 },
	{ PINMUX_AUX_GPIO_PK3_REG,	{ "sdmmc2a", "trace", "gmi", "ccla" }, 0 },
	{ PINMUX_AUX_GPIO_PK4_REG,	{ "sdmmc2a", NULL, "gmi", "gmi_alt" }, 0 },
	{ PINMUX_AUX_GPIO_PK2_REG,	{ NULL, NULL, "gmi", NULL }, 0 },
	{ PINMUX_AUX_GPIO_PI3_REG,	{ NULL, NULL, "gmi", "spi4" }, 0 },
	{ PINMUX_AUX_GPIO_PI6_REG,	{ NULL, NULL, "gmi", "sdmmc2a" }, 0 },
	{ PINMUX_AUX_GPIO_PG0_REG,	{ NULL, NULL, "gmi", NULL }, 0 },
	{ PINMUX_AUX_GPIO_PG1_REG,	{ NULL, NULL, "gmi", NULL }, 0 },
	{ PINMUX_AUX_GPIO_PG2_REG,	{ NULL, "trace", "gmi", NULL }, 0 },
	{ PINMUX_AUX_GPIO_PG3_REG,	{ NULL, "trace", "gmi", NULL }, 0 },
	{ PINMUX_AUX_GPIO_PG4_REG,	{ NULL, "tmds", "gmi", "spi4" }, 0 },
	{ PINMUX_AUX_GPIO_PG5_REG,	{ NULL, NULL, "gmi", "spi4" }, 0 },
	{ PINMUX_AUX_GPIO_PG6_REG,	{ NULL, NULL, "gmi", "spi4" }, 0 },
	{ PINMUX_AUX_GPIO_PG7_REG,	{ NULL, NULL, "gmi", "spi4" }, 0 },
	{ PINMUX_AUX_GPIO_PH0_REG,	{ "pwm0", "trace", "gmi", "dtv" }, 0 },
	{ PINMUX_AUX_GPIO_PH1_REG,	{ "pwm1", "tmds", "gmi", "displaya" }, 0 },
	{ PINMUX_AUX_GPIO_PH2_REG,	{ "pwm2", "tmds", "gmi", "cldvfs" }, 0 },
	{ PINMUX_AUX_GPIO_PH3_REG,	{ "pwm3", "spi4", "gmi", "cldvfs" }, 0 },
	{ PINMUX_AUX_GPIO_PH4_REG,	{ "sdmmc2a", NULL, "gmi", NULL }, 0 },
	{ PINMUX_AUX_GPIO_PH5_REG,	{ "sdmmc2a", NULL, "gmi", NULL }, 0 },
	{ PINMUX_AUX_GPIO_PH6_REG,	{ "sdmmc2a", "trace", "gmi", "dtv" }, 0 },
	{ PINMUX_AUX_GPIO_PH7_REG,	{ "sdmmc2a", "trace", "gmi", "dtv" }, 0 },
	{ PINMUX_AUX_GPIO_PJ7_REG,	{ "uartd", NULL, "gmi", "gmi_alt" }, 0 },
	{ PINMUX_AUX_GPIO_PB0_REG,	{ "uartd", NULL, "gmi", NULL }, 0 },
	{ PINMUX_AUX_GPIO_PB1_REG,	{ "uartd", NULL, "gmi", NULL }, 0 },
	{ PINMUX_AUX_GPIO_PK7_REG,	{ "uartd", NULL, "gmi", NULL }, 0 },
	{ PINMUX_AUX_GPIO_PI0_REG,	{ NULL, NULL, "gmi", NULL }, 0 },
	{ PINMUX_AUX_GPIO_PI1_REG,	{ NULL, NULL, "gmi", NULL }, 0 },
	{ PINMUX_AUX_GPIO_PI2_REG,	{ "sdmmc2a", "trace", "gmi", NULL }, 0 },
	{ PINMUX_AUX_GPIO_PI4_REG,	{ "spi4", "trace", "gmi", "displaya" }, 0 },
	{ PINMUX_AUX_GEN2_I2C_SCL_REG,	{ "i2c2", NULL, "gmi", NULL }, MPIOF_OD },
	{ PINMUX_AUX_GEN2_I2C_SDA_REG,	{ "i2c2", NULL, "gmi", NULL }, MPIOF_OD },
	{ PINMUX_AUX_SDMMC4_CLK_REG,	{ "sdmmc4", NULL, "gmi", NULL }, MPIOF_IO_RESET },
	{ PINMUX_AUX_SDMMC4_CMD_REG,	{ "sdmmc4", NULL, "gmi", NULL }, MPIOF_IO_RESET },
	{ PINMUX_AUX_SDMMC4_DAT0_REG,	{ "sdmmc4", "spi3", "gmi", NULL }, MPIOF_IO_RESET },
	{ PINMUX_AUX_SDMMC4_DAT1_REG,	{ "sdmmc4", "spi3", "gmi", NULL }, MPIOF_IO_RESET },
	{ PINMUX_AUX_SDMMC4_DAT2_REG,	{ "sdmmc4", "spi3", "gmi", NULL }, MPIOF_IO_RESET },
	{ PINMUX_AUX_SDMMC4_DAT3_REG,	{ "sdmmc4", "spi3", "gmi", NULL }, MPIOF_IO_RESET },
	{ PINMUX_AUX_SDMMC4_DAT4_REG,	{ "sdmmc4", "spi3", "gmi", NULL }, MPIOF_IO_RESET },
	{ PINMUX_AUX_SDMMC4_DAT5_REG,	{ "sdmmc4", "spi3", NULL, NULL }, MPIOF_IO_RESET },
	{ PINMUX_AUX_SDMMC4_DAT6_REG,	{ "sdmmc4", "spi3", "gmi", NULL }, MPIOF_IO_RESET },
	{ PINMUX_AUX_SDMMC4_DAT7_REG,	{ "sdmmc4", NULL, "gmi", NULL }, MPIOF_IO_RESET },
	{ PINMUX_AUX_CAM_MCLK_REG,	{ "vi", "vi_alt1", "vi_alt3", "sdmmc2b" }, 0 },
	{ PINMUX_AUX_GPIO_PCC1_REG,	{ "i2s4", NULL, NULL, "sdmmc2b" }, 0 },
	{ PINMUX_AUX_GPIO_PBB0_REG,	{ "vgp6", "vimclk2", "sdmmc2b", "vimclk2_alt" }, 0 },
	{ PINMUX_AUX_CAM_I2C_SCL_REG,	{ "vgp1", "i2c3", NULL, "sdmmc2b" }, MPIOF_OD },
	{ PINMUX_AUX_CAM_I2C_SDA_REG,	{ "vgp2", "i2c3", NULL, "sdmmc2b" }, MPIOF_OD },
	{ PINMUX_AUX_GPIO_PBB3_REG,	{ "vgp3", "displaya", "displayb", "sdmmc2b" }, 0 },
	{ PINMUX_AUX_GPIO_PBB4_REG,	{ "vgp4", "displaya", "displayb", "sdmmc2b" }, 0 },
	{ PINMUX_AUX_GPIO_PBB5_REG,	{ "vgp5", "displaya", NULL, "sdmmc2b" }, 0 },
	{ PINMUX_AUX_GPIO_PBB6_REG,	{ "i2s4", NULL, "displayb", "sdmmc2b" }, 0 },
	{ PINMUX_AUX_GPIO_PBB7_REG,	{ "i2s4", NULL, NULL, "sdmmc2b" }, 0 },
	{ PINMUX_AUX_GPIO_PCC2_REG,	{ "i2s4", NULL, "sdmmc3", "sdmmc2b" }, 0 },
	{ PINMUX_AUX_JTAG_RTCK_REG,	{ "rtck", NULL, NULL, NULL }, 0 },
	{ PINMUX_AUX_PWR_I2C_SCL_REG,	{ "i2cpwr", NULL, NULL, NULL }, MPIOF_OD },
	{ PINMUX_AUX_PWR_I2C_SDA_REG,	{ "i2cpwr", NULL, NULL, NULL }, MPIOF_OD },
	{ PINMUX_AUX_KB_ROW0_REG,	{ "kbc", NULL, NULL, NULL }, 0 },
	{ PINMUX_AUX_KB_ROW1_REG,	{ "kbc", NULL, NULL, NULL }, 0 },
	{ PINMUX_AUX_KB_ROW2_REG,	{ "kbc", NULL, NULL, NULL }, 0 },
	{ PINMUX_AUX_KB_ROW3_REG,	{ "kbc", "displaya", "sys", "displayb" }, 0 },
	{ PINMUX_AUX_KB_ROW4_REG,	{ "kbc", "displaya", NULL, "displayb" }, 0 },
	{ PINMUX_AUX_KB_ROW5_REG,	{ "kbc", "displaya", NULL, "displayb" }, 0 },
	{ PINMUX_AUX_KB_ROW6_REG,	{ "kbc", "displaya", "displaya_alt", "displayb" }, 0 },
	{ PINMUX_AUX_KB_ROW7_REG,	{ "kbc", NULL, "cldvfs", "uarta" }, 0 },
	{ PINMUX_AUX_KB_ROW8_REG,	{ "kbc", NULL, "cldvfs", "uarta" }, 0 },
	{ PINMUX_AUX_KB_ROW9_REG,	{ "kbc", NULL, NULL, "uarta" }, 0 },
	{ PINMUX_AUX_KB_ROW10_REG,	{ "kbc", NULL, NULL, "uarta" }, 0 },
	{ PINMUX_AUX_KB_ROW11_REG,	{ "kbc", NULL, NULL, "irda" }, 0 },
	{ PINMUX_AUX_KB_ROW12_REG,	{ "kbc", NULL, NULL, "irda" }, 0 },
	{ PINMUX_AUX_KB_ROW13_REG,	{ "kbc", NULL, "spi2", NULL }, 0 },
	{ PINMUX_AUX_KB_ROW14_REG,	{ "kbc", NULL, "spi2", NULL }, 0 },
	{ PINMUX_AUX_KB_ROW15_REG,	{ "kbc", "soc", NULL, NULL }, 0 },
	{ PINMUX_AUX_KB_COL0_REG,	{ "kbc", NULL, "spi2", NULL }, 0 },
	{ PINMUX_AUX_KB_COL1_REG,	{ "kbc", NULL, "spi2", NULL }, 0 },
	{ PINMUX_AUX_KB_COL2_REG,	{ "kbc", NULL, "spi2", NULL }, 0 },
	{ PINMUX_AUX_KB_COL3_REG,	{ "kbc", "displaya", "pwm2", "uarta" }, 0 },
	{ PINMUX_AUX_KB_COL4_REG,	{ "kbc", "owr", "sdmmc3", "uarta" }, 0 },
	{ PINMUX_AUX_KB_COL5_REG,	{ "kbc", NULL, "sdmmc3", NULL }, 0 },
	{ PINMUX_AUX_KB_COL6_REG,	{ "kbc", NULL, "spi2", "uartd" }, 0 },
	{ PINMUX_AUX_KB_COL7_REG,	{ "kbc", NULL, "spi2", "uartd" }, 0 },
	{ PINMUX_AUX_CLK_32K_OUT_REG,	{ "blink", "soc", NULL, NULL }, 0 },
	{ PINMUX_AUX_CORE_PWR_REQ_REG,	{ "pwron", NULL, NULL, NULL }, 0 },
	{ PINMUX_AUX_CPU_PWR_REQ_REG,	{ "cpu", NULL, NULL, NULL }, 0 },
	{ PINMUX_AUX_PWR_INT_N_REG,	{ "pmi", NULL, NULL, NULL }, 0 },
	{ PINMUX_AUX_CLK_32K_IN_REG,	{ "clk", NULL, NULL, NULL }, 0 },
	{ PINMUX_AUX_OWR_REG,		{ "owr", NULL, NULL, NULL }, MPIOF_RCV_SEL },
	{ PINMUX_AUX_DAP1_FS_REG,	{ "i2s0", "hda", "gmi", NULL }, 0 },
	{ PINMUX_AUX_DAP1_DIN_REG,	{ "i2s0", "hda", "gmi", NULL }, 0 },
	{ PINMUX_AUX_DAP1_DOUT_REG,	{ "i2s0", "hda", "gmi", "sata" }, 0 },
	{ PINMUX_AUX_DAP1_SCLK_REG,	{ "i2s0", "hda", "gmi", NULL }, 0 },
	{ PINMUX_AUX_DAP_MCLK1_REQ_REG,	{ "dap", "dap1", "sata", NULL }, 0 },
	{ PINMUX_AUX_DAP_MCLK1_REG,	{ "extperiph1", "dap2", NULL, NULL }, 0 },
	{ PINMUX_AUX_SPDIF_IN_REG,	{ "spdif", NULL, NULL, "i2c3" }, 0 },
	{ PINMUX_AUX_SPDIF_OUT_REG,	{ "spdif", NULL, NULL, "i2c3" }, 0 },
	{ PINMUX_AUX_DAP2_FS_REG,	{ "i2s1", "hda", "gmi", NULL }, 0 },
	{ PINMUX_AUX_DAP2_DIN_REG,	{ "i2s1", "hda", "gmi", NULL }, 0 },
	{ PINMUX_AUX_DAP2_DOUT_REG,	{ "i2s1", "hda", "gmi", NULL }, 0 },
	{ PINMUX_AUX_DAP2_SCLK_REG,	{ "i2s1", "hda", "gmi", NULL }, 0 },
	{ PINMUX_AUX_DVFS_PWM_REG,	{ "spi6", "cldvfs", "gmi", NULL }, 0 },
	{ PINMUX_AUX_GPIO_X1_AUD_REG,	{ "spi6", NULL, "gmi", NULL }, 0 },
	{ PINMUX_AUX_GPIO_X3_AUD_REG,	{ "spi6", "spi1", "gmi", NULL }, 0 },
	{ PINMUX_AUX_DVFS_CLK_REG,	{ "spi6", "cldvfs", "gmi", NULL }, 0 },
	{ PINMUX_AUX_GPIO_X4_AUD_REG,	{ "gmi", "spi1", "spi2", "dap2" }, 0 },
	{ PINMUX_AUX_GPIO_X5_AUD_REG,	{ "gmi", "spi1", "spi2", NULL }, 0 },
	{ PINMUX_AUX_GPIO_X6_AUD_REG,	{ "spi6", "spi1", "spi2", "gmi" }, 0 },
	{ PINMUX_AUX_GPIO_X7_AUD_REG,	{ NULL, "spi1", "spi2", NULL }, 0 },
	{ PINMUX_AUX_SDMMC3_CLK_REG,	{ "sdmmc3", NULL, NULL, "spi3" }, 0 },
	{ PINMUX_AUX_SDMMC3_CMD_REG,	{ "sdmmc3", "pwm3", "uarta", "spi3" }, 0 },
	{ PINMUX_AUX_SDMMC3_DAT0_REG,	{ "sdmmc3", NULL, NULL, "spi3" }, 0 },
	{ PINMUX_AUX_SDMMC3_DAT1_REG,	{ "sdmmc3", "pwm2", "uarta", "spi3" }, 0 },
	{ PINMUX_AUX_SDMMC3_DAT2_REG,	{ "sdmmc3", "pwm1", "displaya", "spi3" }, 0 },
	{ PINMUX_AUX_SDMMC3_DAT3_REG,	{ "sdmmc3", "pwm0", "dipslayb", "spi3" }, 0 },
	{ PINMUX_AUX_PEX_L0_RST_N_REG,	{ "pe0", NULL, NULL, NULL }, 0 },
	{ PINMUX_AUX_PEX_L0_CLKREQ_N_REG, { "pe0", NULL, NULL, NULL }, 0 },
	{ PINMUX_AUX_PEX_WAKE_N_REG,	{ "pe", NULL, NULL, NULL }, 0 },
	{ PINMUX_AUX_PEX_L1_RST_N_REG,	{ "pe1", NULL, NULL, NULL }, 0 },
	{ PINMUX_AUX_PEX_L1_CLKREQ_N_REG, { "pe1", NULL, NULL, NULL }, 0 },
	{ PINMUX_AUX_HDMI_CEC_REG,	{ "cec", NULL, NULL, NULL }, MPIOF_OD },
	{ PINMUX_AUX_SDMMC1_WP_N_REG,	{ "sdmmc1", "clk12", "spi4", "uarta" }, 0 },
	{ PINMUX_AUX_SDMMC3_CD_N_REG,	{ "sdmmc3", "owr", NULL, NULL }, 0 },
	{ PINMUX_AUX_GPIO_W2_AUD_REG,	{ "spi6", NULL, "spi2", "i2c1" }, 0 },
	{ PINMUX_AUX_GPIO_W3_AUD_REG,	{ "spi6", "spi1", "spi2", "i2c1" }, 0 },
	{ PINMUX_AUX_USB_VBUS_EN0_REG,	{ "usb", NULL, NULL, NULL }, MPIOF_OD },
	{ PINMUX_AUX_USB_VBUS_EN1_REG,	{ "usb", NULL, NULL, NULL }, MPIOF_OD },
	{ PINMUX_AUX_SDMMC3_CLK_LB_IN_REG, { "sdmmc3", NULL, NULL, NULL }, 0 },
	{ PINMUX_AUX_SDMMC3_CLK_LB_OUT_REG, { "sdmmc3", NULL, NULL, NULL }, 0 },
	{ PINMUX_AUX_GMI_CLK_LB_REG,	{ "sdmmc2a", NULL, "gmi", NULL }, 0 },
	{ PINMUX_AUX_RESET_OUT_N_REG,	{ NULL, NULL, NULL, "reset_out_n" }, 0 },
	{ PINMUX_AUX_KB_ROW16_REG,	{ "kbc", NULL, NULL, "uartc" }, 0 },
	{ PINMUX_AUX_KB_ROW17_REG,	{ "kbc", NULL, NULL, "uartc" }, 0 },
	{ PINMUX_AUX_USB_VBUS_EN2_REG,	{ "usb", NULL, NULL, NULL }, MPIOF_OD },
	{ PINMUX_AUX_GPIO_PFF2_REG,	{ "sata", NULL, NULL, NULL }, MPIOF_OD },
	{ PINMUX_AUX_DP_HPD_REG,	{ "dp", NULL, NULL, NULL }, 0 },
};

struct tegra_mpio_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
};

CFATTACH_DECL_NEW(tegra_mpio, sizeof(struct tegra_mpio_softc),
	tegra_mpio_match, tegra_mpio_attach, NULL, NULL);

#define MPIO_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define MPIO_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))

static int
tegra_mpio_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
tegra_mpio_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_mpio_softc * const sc = device_private(self);
	struct tegraio_attach_args * const tio = aux;
	const struct tegra_locators * const loc = &tio->tio_loc;

	sc->sc_dev = self;
	sc->sc_bst = tio->tio_bst;
	bus_space_subregion(tio->tio_bst, tio->tio_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	aprint_naive("\n");
	aprint_normal(": MPIO\n");
}

static struct tegra_mpio_softc *
tegra_mpio_lookup_softc(void)
{
	device_t dev = device_find_by_driver_unit("tegrampio", 0);
	KASSERT(dev != NULL);
	return device_private(dev);
}

static const struct tegra_mpio_padgrp *
tegra_mpio_lookup_padgrp(u_int reg)
{
	u_int n;

	for (n = 0; n < __arraycount(tegra_mpio_padgrp); n++) {
		if (tegra_mpio_padgrp[n].pg_reg == reg) {
			return &tegra_mpio_padgrp[n];
		}
	}

	return NULL;
}

static const struct tegra_mpio_pinmux *
tegra_mpio_lookup_pinmux(u_int reg)
{
	u_int n;

	for (n = 0; n < __arraycount(tegra_mpio_pinmux); n++) {
		if (tegra_mpio_pinmux[n].pm_reg == reg) {
			return &tegra_mpio_pinmux[n];
		}
	}

	return NULL;
}

void
tegra_mpio_padctlgrp_read(u_int reg, struct tegra_mpio_padctlgrp *ctl)
{
	struct tegra_mpio_softc * const sc = tegra_mpio_lookup_softc();
	const struct tegra_mpio_padgrp *pg;

	pg = tegra_mpio_lookup_padgrp(reg);
	KASSERT(pg != NULL);

	const uint32_t val = MPIO_READ(sc, reg);

	ctl->preemp = pg->pg_preemp ? __SHIFTOUT(val, pg->pg_preemp) : -1;
	ctl->hsm = __SHIFTOUT(val, pg->pg_hsm);
	ctl->schmt = __SHIFTOUT(val, pg->pg_schmt);
	ctl->drv_type = pg->pg_drv_type ? __SHIFTOUT(val, pg->pg_drv_type) : -1;
	ctl->drvdn = __SHIFTOUT(val, pg->pg_drvdn);
	ctl->drvup = pg->pg_drvup ? __SHIFTOUT(val, pg->pg_drvup) : -1;
	ctl->slwr = __SHIFTOUT(val, pg->pg_slwr);
	ctl->slwf = pg->pg_slwf ? __SHIFTOUT(val, pg->pg_slwf) : -1;
}

void
tegra_mpio_padctlgrp_write(u_int reg, const struct tegra_mpio_padctlgrp *ctl)
{
	struct tegra_mpio_softc * const sc = tegra_mpio_lookup_softc();
	const struct tegra_mpio_padgrp *pg;
	uint32_t val;

	val = 0;
	pg = tegra_mpio_lookup_padgrp(reg);
	KASSERT(pg != NULL);

	val |= pg->pg_preemp ? __SHIFTIN(ctl->preemp, pg->pg_preemp) : 0;
	val |= __SHIFTIN(ctl->hsm, pg->pg_hsm);
	val |= __SHIFTIN(ctl->schmt, pg->pg_schmt);
	val |= pg->pg_drv_type ? __SHIFTIN(ctl->drv_type, pg->pg_drv_type) : 0;
	val |= __SHIFTIN(ctl->drvdn, pg->pg_drvdn);
	val |= pg->pg_drvup ? __SHIFTIN(ctl->drvup, pg->pg_drvup) : 0;
	val |= __SHIFTIN(ctl->slwr, pg->pg_slwr);
	val |= pg->pg_slwf ? __SHIFTIN(ctl->slwf, pg->pg_slwf) : 0;
	MPIO_WRITE(sc, reg, val);
}

void
tegra_mpio_pinmux_set_config(u_int reg, int flags, const char *func)
{
	struct tegra_mpio_softc * const sc = tegra_mpio_lookup_softc();
	uint32_t val;

	val = MPIO_READ(sc, reg);

	if (flags & GPIO_PIN_INPUT) {
		val |= PINMUX_AUX_E_INPUT;
	} else if (flags & GPIO_PIN_OUTPUT) {
		val &= ~PINMUX_AUX_E_INPUT;
	}

	if (flags & GPIO_PIN_TRISTATE) {
		val |= PINMUX_AUX_TRISTATE;
	} else {
		val &= ~PINMUX_AUX_TRISTATE;
	}
	
	val &= ~PINMUX_AUX_PUPD;
	if (flags & GPIO_PIN_PULLUP) {
		val |= __SHIFTIN(PINMUX_AUX_PUPD_PULL_UP,
				 PINMUX_AUX_PUPD);
	} else if (flags & GPIO_PIN_PULLDOWN) {
		val |= __SHIFTIN(PINMUX_AUX_PUPD_PULL_DOWN,
				 PINMUX_AUX_PUPD);
	} else {
		val |= __SHIFTIN(PINMUX_AUX_PUPD_NORMAL,
				 PINMUX_AUX_PUPD);
	}

	if (flags & GPIO_PIN_OPENDRAIN) {
		val |= PINMUX_AUX_OD;
	} else {
		val &= ~PINMUX_AUX_OD;
	}

	MPIO_WRITE(sc, reg, val);
}

void
tegra_mpio_pinmux_set_io_reset(u_int reg, bool set)
{
	struct tegra_mpio_softc * const sc = tegra_mpio_lookup_softc();
	uint32_t val;

	val = MPIO_READ(sc, reg);
	if (set) {
		val |= PINMUX_AUX_IO_RESET;
	} else {
		val &= ~PINMUX_AUX_IO_RESET;
	}
	MPIO_WRITE(sc, reg, val);
}

void
tegra_mpio_pinmux_set_rcv_sel(u_int reg, bool set)
{
	struct tegra_mpio_softc * const sc = tegra_mpio_lookup_softc();
	uint32_t val;

	val = MPIO_READ(sc, reg);
	if (set) {
		val |= PINMUX_AUX_RCV_SEL;
	} else {
		val &= ~PINMUX_AUX_RCV_SEL;
	}
	MPIO_WRITE(sc, reg, val);
}

void
tegra_mpio_pinmux_get_config(u_int reg, int *flags, const char **func)
{
	struct tegra_mpio_softc * const sc = tegra_mpio_lookup_softc();
	const struct tegra_mpio_pinmux *pm;

	pm = tegra_mpio_lookup_pinmux(reg);
	KASSERT(pm != NULL);

	const uint32_t val = MPIO_READ(sc, reg);
	*flags = 0;
	if (func) {
		*func = pm->pm_func[__SHIFTOUT(val, PINMUX_AUX_PM)];
	}
	switch (__SHIFTOUT(val, PINMUX_AUX_PUPD)) {
	case PINMUX_AUX_PUPD_PULL_DOWN:
		*flags |= GPIO_PIN_PULLDOWN;
		break;
	case PINMUX_AUX_PUPD_PULL_UP:
		*flags |= GPIO_PIN_PULLUP;
		break;
	}
	if (val & PINMUX_AUX_TRISTATE) {
		*flags |= GPIO_PIN_TRISTATE;
	}
	if (val & PINMUX_AUX_E_INPUT) {
		*flags |= GPIO_PIN_INPUT;
	} else {
		*flags |= GPIO_PIN_OUTPUT;
	}
	if (val & PINMUX_AUX_OD) {
		*flags |= GPIO_PIN_OPENDRAIN;
	}
}

bool
tegra_mpio_pinmux_get_io_reset(u_int reg)
{
	struct tegra_mpio_softc * const sc = tegra_mpio_lookup_softc();
	const uint32_t val = MPIO_READ(sc, reg);

	return !!(val & PINMUX_AUX_IO_RESET);
}

bool
tegra_mpio_pinmux_get_rcv_sel(u_int reg)
{
	struct tegra_mpio_softc * const sc = tegra_mpio_lookup_softc();
	const uint32_t val = MPIO_READ(sc, reg);

	return !!(val & PINMUX_AUX_RCV_SEL);
}

#if defined(DDB)
void	tegra_mpio_dump(void);

void
tegra_mpio_dump(void)
{
	const struct tegra_mpio_padgrp *pg;
	const struct tegra_mpio_pinmux *pm;
	struct tegra_mpio_padctlgrp ctl;
	const char *func;
	int config;
	device_t dev;
	u_int n;

	dev = device_find_by_driver_unit("tegrampio", 0);
	if (dev == NULL)
		return;

	printf("Dumping pad groups:\n");
	for (n = 0; n < __arraycount(tegra_mpio_padgrp); n++) {
		pg = &tegra_mpio_padgrp[n];
		tegra_mpio_padctlgrp_read(pg->pg_reg, &ctl);
		printf(" #%u [+%#x]: preemp=%d, hsm=%d, schmt=%d,"
		    " drv_type=%d, drvdn=%d, drvup=%d, slwr=%d, slwf=%d\n",
		    n, pg->pg_reg, ctl.preemp, ctl.hsm, ctl.schmt,
		    ctl.drv_type, ctl.drvdn, ctl.drvup, ctl.slwr, ctl.slwf);
	}

	printf("Dumping pin muxes:\n");
	for (n = 0; n < __arraycount(tegra_mpio_pinmux); n++) {
		pm = &tegra_mpio_pinmux[n];
		tegra_mpio_pinmux_get_config(pm->pm_reg, &config, &func);
		printf(" #%u [+%#x]: config=%#x func=%s\n",
		    n, pm->pm_reg, config, func);
	}
}
#endif
