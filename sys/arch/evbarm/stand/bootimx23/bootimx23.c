/* $Id: bootimx23.c,v 1.1.10.2 2014/08/20 00:02:56 tls Exp $ */

/*
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Petri Laakso.
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

#include <sys/param.h>
#include <sys/cdefs.h>

#include <arm/imx/imx23_digctlreg.h>
#include <arm/imx/imx23_powerreg.h>
#include <arm/imx/imx23_clkctrlreg.h>

#include <lib/libsa/stand.h>

#include "common.h"

#define DRAM_REGS 41
#define HBUS_DIV 2
#define CPU_FRAC 22
#define EMI_DIV 2
#define EMI_FRAC 33
#define SSP_DIV 2
#define IO_FRAC 27

/*
 * Initialize i.MX233
 */
int
_start(void)
{

	volatile uint32_t *digctl_ctrl_rs;
	volatile uint32_t *digctl_ctrl_rc;
	volatile uint32_t *digctl_id_r;
	volatile uint32_t *pwr_status_r;
	uint8_t boot_reason;
	int on_batt;

	digctl_ctrl_rs = (uint32_t *)(HW_DIGCTL_BASE + HW_DIGCTL_CTRL_SET);
	digctl_ctrl_rc = (uint32_t *)(HW_DIGCTL_BASE + HW_DIGCTL_CTRL_CLR);
	pwr_status_r = (uint32_t *)(HW_POWER_BASE + HW_POWER_STS);
	digctl_id_r = (uint32_t *)(HW_DIGCTL_BASE + HW_DIGCTL_CHIPID);

	/* Enable SJTAG. */
	*digctl_ctrl_rs = HW_DIGCTL_CTRL_USE_SERIAL_JTAG;

	/* Enable microseconds timer. */
	*digctl_ctrl_rc = HW_DIGCTL_CTRL_XTAL24M_GATE;

	if (*pwr_status_r & HW_POWER_STS_VDD5V_GT_VDDIO)
		on_batt = 0;
	else
		on_batt = 1;

	printf("\r\nbootimx23: ");
	printf("HW revision TA%d, ",
	   (uint8_t)__SHIFTOUT(*digctl_id_r, HW_DIGCTL_CHIPID_REVISION) + 1);
	printf("boot reason ");
	boot_reason =
		(uint8_t) __SHIFTOUT(*pwr_status_r, HW_POWER_STS_PWRUP_SOURCE);

	switch (boot_reason)
	{
		case 0x20:
			printf("5V, ");
			break;
		case 0x10:
			printf("RTC, ");
			break;
		case 0x02:
			printf("high pswitch, ");
			break;
		case 0x01:
			printf("mid pswitch, ");
			break;
		default:
			printf("UNKNOWN, ");
	}

	printf("power source ");
	if (on_batt)
		printf("battery");
	else
		printf("5V");
	printf("\r\n");

	/* Power. */
	en_vbusvalid();
	if (!vbusvalid())
		printf("WARNING: !VBUSVALID\r\n");

	printf("Enabling 4P2 regulator...");
	en_4p2_reg();
	printf("done\r\n");

	power_tune();

	printf("Enabling 4P2 regulator output to DCDC...");
	en_4p2_to_dcdc();
	printf("done\r\n");

	printf("Enabling VDDMEM...");
	power_vddmem(2500);
	printf("done\r\n");

	printf("Powering VDDD from DCDC...");
	/* Boot fails if I set here TRG to +1500mV, do it later. */
	power_vddd_from_dcdc(1400, 1075);
	printf("done\r\n");

	printf("Powering VDDA from DCDC...");
	power_vdda_from_dcdc(1800, 1625);
	printf("done\r\n");

	/*
	 * VDDIO and thus SSP_CLK setup is postponed to
	 * imx23_olinuxino_machdep.c because SB is not able to load kernel if
	 * clocks are changed now.
	 */
	printf("Powering VDDIO from DCDC...");
	power_vddio_from_dcdc(3100, 2925);
	printf("done\r\n");

	/* Clocks */
	printf("Enabling clocks...");
	en_pll();
	bypass_saif(); /* Always set to zero bit. */
	set_cpu_frac(CPU_FRAC);
	set_hbus_div(HBUS_DIV);
	bypass_cpu();
	power_vddd_from_dcdc(1475, 1375);
	set_emi_div(EMI_DIV);
	set_emi_frac(EMI_FRAC);
	bypass_emi();
	printf("done\r\n");
	
	printf("Configuring pins...");
	pinctrl_prep();
	printf("done\r\n");
	printf("Configuring EMI...");
	emi_prep();
	printf("done\r\n");
	args_prep();

	return 0;
}
