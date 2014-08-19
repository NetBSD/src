/* $Id: pinctrl_prep.c,v 1.3.4.3 2014/08/20 00:02:56 tls Exp $ */

/*
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
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
#include <sys/types.h>

#include <arm/imx/imx23_pinctrlreg.h>

#include <lib/libsa/stand.h>

#include "common.h"

#define CTRL		(HW_PINCTRL_BASE + HW_PINCTRL_CTRL)
#define CTRL_S		(HW_PINCTRL_BASE + HW_PINCTRL_CTRL_SET)
#define CTRL_C		(HW_PINCTRL_BASE + HW_PINCTRL_CTRL_CLR)
#define CTRL_MUX0	(HW_PINCTRL_BASE + HW_PINCTRL_MUXSEL0)
#define CTRL_MUX0_S	(HW_PINCTRL_BASE + HW_PINCTRL_MUXSEL0_SET)
#define CTRL_MUX0_C	(HW_PINCTRL_BASE + HW_PINCTRL_MUXSEL0_CLR)
#define CTRL_MUX1	(CTRL_MUX0 + 0x10)
#define CTRL_MUX1_S	(CTRL_MUX0_S + 0x10)
#define CTRL_MUX1_C	(CTRL_MUX0_C + 0x10)
#define CTRL_MUX2	(CTRL_MUX0 + 0x20)
#define CTRL_MUX2_S	(CTRL_MUX0_S + 0x20)
#define CTRL_MUX2_C	(CTRL_MUX0_C + 0x20)
#define CTRL_MUX3	(CTRL_MUX0 + 0x30)
#define CTRL_MUX3_S	(CTRL_MUX0_S + 0x30)
#define CTRL_MUX3_C	(CTRL_MUX0_C + 0x30)
#define CTRL_MUX4	(CTRL_MUX0 + 0x40)
#define CTRL_MUX4_S	(CTRL_MUX0_S + 0x40)
#define CTRL_MUX4_C	(CTRL_MUX0_C + 0x40)
#define CTRL_MUX5	(CTRL_MUX0 + 0x50)
#define CTRL_MUX5_S	(CTRL_MUX0_S + 0x50)
#define CTRL_MUX5_C	(CTRL_MUX0_C + 0x50)
#define CTRL_MUX6	(CTRL_MUX0 + 0x60)
#define CTRL_MUX6_S	(CTRL_MUX0_S + 0x60)
#define CTRL_MUX6_C	(CTRL_MUX0_C + 0x60)
#define CTRL_MUX7	(CTRL_MUX0 + 0x70)
#define CTRL_MUX7_S	(CTRL_MUX0_S + 0x70)
#define CTRL_MUX7_C	(CTRL_MUX0_C + 0x70)

#define CTRL_DRV0	(HW_PINCTRL_BASE + HW_PINCTRL_DRIVE0)
#define CTRL_DRV0_S	(HW_PINCTRL_BASE + HW_PINCTRL_DRIVE0_SET)
#define CTRL_DRV0_C	(HW_PINCTRL_BASE + HW_PINCTRL_DRIVE0_CLR)
#define CTRL_DRV8	(CTRL_DRV0 + 0x80)
#define CTRL_DRV8_S	(CTRL_DRV0_S + 0x80)
#define CTRL_DRV8_C	(CTRL_DRV0_C + 0x80)
#define CTRL_DRV9	(CTRL_DRV0 + 0x90)
#define CTRL_DRV9_S	(CTRL_DRV0_S + 0x90)
#define CTRL_DRV9_C	(CTRL_DRV0_C + 0x90)
#define CTRL_DRV10	(CTRL_DRV0 + 0xa0)
#define CTRL_DRV10_S	(CTRL_DRV0_S + 0xa0)
#define CTRL_DRV10_C	(CTRL_DRV0_C + 0xa0)
#define CTRL_DRV11	(CTRL_DRV0 + 0xb0)
#define CTRL_DRV11_S	(CTRL_DRV0_S + 0xb0)
#define CTRL_DRV11_C	(CTRL_DRV0_C + 0xb0)
#define CTRL_DRV12	(CTRL_DRV0 + 0xc0)
#define CTRL_DRV12_S	(CTRL_DRV0_S + 0xc0)
#define CTRL_DRV12_C	(CTRL_DRV0_C + 0xc0)
#define CTRL_DRV13	(CTRL_DRV0 + 0xd0)
#define CTRL_DRV13_S	(CTRL_DRV0_S + 0xd0)
#define CTRL_DRV13_C	(CTRL_DRV0_C + 0xd0)
#define CTRL_DRV14	(CTRL_DRV0 + 0xe0)
#define CTRL_DRV14_S	(CTRL_DRV0_S + 0xe0)
#define CTRL_DRV14_C	(CTRL_DRV0_C + 0xe0)

#define CTRL_PULL0	(HW_PINCTRL_BASE + HW_PINCTRL_PULL0)
#define CTRL_PULL1	(CTRL_PULL0 + 0x10)
#define CTRL_PULL2	(CTRL_PULL0 + 0x20)
#define CTRL_PULL3	(CTRL_PULL0 + 0x30)

/*
 * Configure initial pin settings.
 */
int
pinctrl_prep(void)
{

	REG_WR(CTRL_C, (HW_PINCTRL_CTRL_SFTRST | HW_PINCTRL_CTRL_CLKGATE));
	delay(10000);

	/*
	 * EMI MUX.
	 */
	REG_WR(CTRL_MUX4_C, 0xfffc0000);	/* A00:06 */
	REG_WR(CTRL_MUX5_C, 0xfc3fffff);	/* A07:12, BA0:1, CASN, CE0N,
						 * CE1N, CKE, RASN, WEN */
	REG_WR(CTRL_MUX6_C, 0xffffffff);	/* D00:15 */
	REG_WR(CTRL_MUX7_C, 0xfff);		/* DQM0:1, DQS0:1, CLK, CLKN */

	/*
	 * EMI pin drive strength and voltage to 12mA @ 2.5V.
	 */
	REG_WR(CTRL_DRV9, 0x22222220);	/* A00:06 */
	REG_WR(CTRL_DRV10, 0x22222222);	/* A07:A12, BA0:1 */
	REG_WR(CTRL_DRV11, 0x22200222);	/* CASN, CE0N, CE1N, CKE, RASN, WEN */
	REG_WR(CTRL_DRV12, 0x22222222);	/* D00:07 */
	REG_WR(CTRL_DRV13, 0x22222222);	/* D08:15 */
	REG_WR(CTRL_DRV14, 0x222222);	/* DQM0:1, DQS0:1, CLK, CLKN */

	/*
	 * Disable EMI pad keepers.
	 */
	REG_WR(CTRL_PULL3, 0x3ffff);	/* D00:D15, DQM0:1 */

	/*
	 * SSP MUX.
	 */
	REG_WR(CTRL_MUX4_C, 0x3ff3);	/* CMD, DATA0:3, SCK */
	REG_WR(CTRL_MUX4_S, 0xc);	/* SSP1_DETECT as GPIO */
	
	/*
	 * SSP pin drive strength.
	 */
	REG_WR(CTRL_DRV8, 0x01111101);	/* CMD, DATA0:3, SCK to 8mA
					 * SSP1_DETECT to 4mA */
	/*
	 * SSP pull ups.
	 */
	REG_WR(CTRL_PULL2, 0x3d);	/* Pull-up DATA0:3, CMD and
					 * no pull-up SSP1_DETECT */
	/*
	 * Debug UART MUX.
	 */
	REG_WR(CTRL_MUX3_C, 0xf00000);
	REG_WR(CTRL_MUX3_S, 0xa00000);

	return 0;
}
