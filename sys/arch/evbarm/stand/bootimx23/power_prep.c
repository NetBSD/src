/* $Id: power_prep.c,v 1.3.2.3 2013/01/23 00:05:47 yamt Exp $ */

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
#include <sys/types.h>
#include <sys/cdefs.h>

#include <arm/imx/imx23_powerreg.h>

#include <lib/libsa/stand.h>

#include "common.h"

static void set_vddd_target(uint8_t);
static void set_vdda_target(uint8_t);
static void set_vddio_target(uint8_t);
static void set_vddmem_target(uint8_t);
#ifdef DEBUG
static void print_regs(void);
#endif

#define VDDD_TARGET	0x1e	/* 1550 mV */
#define VDDA_TARGET	0x0a	/* 1750 mV */
#define VDDIO_TARGET	0x0c	/* 3100 mV */
#define VDDMEM_TARGET	0x10	/* 2500 mV */

static int five_volts = 0;

/*
 * If 5V is present, all power rails are powered from the LinRegs.
 *
 * If powered from the battery, the DC-DC converter starts when battery power
 * is detected and the PSWITCH button is pressed. In this state, the VDDA and
 * VDDIO rails are powered on from DC-DC, but the VDDD rail is powered from its
 * LinReg.
 */
int
power_prep(void)
{
	if (REG_RD(HW_POWER_BASE + HW_POWER_STS) & HW_POWER_STS_VDD5V_GT_VDDIO)
		five_volts = 1;
#ifdef DEBUG
	if (five_volts)
		printf("Powered from 5V\n\r");
	else
		printf("Powered from the battery.\n\r");
	print_regs();
#endif
	set_vddd_target(VDDD_TARGET);
	set_vdda_target(VDDA_TARGET);
	set_vddio_target(VDDIO_TARGET);
	set_vddmem_target(VDDMEM_TARGET);
#ifdef DEBUG
	print_regs();
#endif
	return 0;
}

/*
 * Set VDDD target voltage.
 */
static void
set_vddd_target(uint8_t target)
{
	uint32_t vddd;
	uint8_t curtrg;

	vddd = REG_RD(HW_POWER_BASE + HW_POWER_VDDDCTRL);
	
	/*
	 * VDDD is always powered from the linear regulator.
	 *
	 * Setting LINREG_OFFSET to 0 is recommended when powering VDDD from
	 * the linear regulator. It is also recommended to set DISABLE_STEPPING
	 * when powering VDDD from the linear regulators.
	 */
	vddd &= ~(HW_POWER_VDDDCTRL_LINREG_OFFSET);
	vddd |= HW_POWER_VDDDCTRL_DISABLE_STEPPING;

	REG_WR(HW_POWER_BASE + HW_POWER_VDDDCTRL, vddd);
	delay(1000);

	curtrg = __SHIFTOUT(vddd, HW_POWER_VDDDCTRL_TRG);

	/* Because HW stepping is disabled, raise voltage to target slowly. */
	for (curtrg++; curtrg <= target; curtrg++) {
		vddd = REG_RD(HW_POWER_BASE + HW_POWER_VDDDCTRL);
		vddd &= ~(HW_POWER_VDDDCTRL_TRG);
		vddd |= __SHIFTIN(curtrg, HW_POWER_VDDDCTRL_TRG);
		REG_WR(HW_POWER_BASE + HW_POWER_VDDDCTRL, vddd);
		delay(1000);
	}

	return;
}

static void
set_vdda_target(uint8_t target)
{
	uint32_t vdda;
	uint8_t curtrg;

	vdda = REG_RD(HW_POWER_BASE + HW_POWER_VDDACTRL);

	/*
	 * Setting LINREG_OFFSET to 0 is recommended when powering VDDA from
	 * the linear regulator. It is also recommended to set DISABLE_STEPPING
	 * when powering VDDA from the linear regulators.
	 */
	if (five_volts) {
		vdda &= ~(HW_POWER_VDDACTRL_LINREG_OFFSET);
		vdda |= HW_POWER_VDDACTRL_DISABLE_STEPPING;
		REG_WR(HW_POWER_BASE + HW_POWER_VDDACTRL, vdda);
		delay(1000);

		curtrg = __SHIFTOUT(vdda, HW_POWER_VDDACTRL_TRG);

		/*
		 * Because HW stepping is disabled, raise voltage to target
		 * slowly.
		 */
		for (curtrg++; curtrg <= target; curtrg++) {
			vdda = REG_RD(HW_POWER_BASE + HW_POWER_VDDACTRL);
			vdda &= ~(HW_POWER_VDDACTRL_TRG);
			vdda |= __SHIFTIN(curtrg, HW_POWER_VDDACTRL_TRG);
			REG_WR(HW_POWER_BASE + HW_POWER_VDDACTRL, vdda);
			delay(1000);
		}
	} else {
		vdda |= __SHIFTIN(target, HW_POWER_VDDACTRL_TRG);
		REG_WR(HW_POWER_BASE + HW_POWER_VDDACTRL, vdda);
	}

	return;
}

static void
set_vddio_target(uint8_t target)
{
	uint32_t vddio;
	uint8_t curtrg;

	vddio = REG_RD(HW_POWER_BASE + HW_POWER_VDDIOCTRL);

	/*
	 * Setting LINREG_OFFSET to 0 is recommended when powering VDDIO from
	 * the linear regulator. It is also recommended to set DISABLE_STEPPING
	 * when powering VDDIO from the linear regulators.
	 */
	if (five_volts) {
		vddio &= ~(HW_POWER_VDDIOCTRL_LINREG_OFFSET);
		vddio |= HW_POWER_VDDIOCTRL_DISABLE_STEPPING;
		REG_WR(HW_POWER_BASE + HW_POWER_VDDIOCTRL, vddio);
		delay(1000);

		curtrg = __SHIFTOUT(vddio, HW_POWER_VDDIOCTRL_TRG);

		/*
		 * Because HW stepping is disabled, raise voltage to target
		 * slowly.
		 */
		for (curtrg++; curtrg <= target; curtrg++) {
			vddio = REG_RD(HW_POWER_BASE + HW_POWER_VDDIOCTRL);
			vddio &= ~(HW_POWER_VDDIOCTRL_TRG);
			vddio |= __SHIFTIN(curtrg, HW_POWER_VDDIOCTRL_TRG);
			REG_WR(HW_POWER_BASE + HW_POWER_VDDIOCTRL, vddio);
			delay(1000);
		}
	} else {
		vddio |= __SHIFTIN(target, HW_POWER_VDDIOCTRL_TRG);
		REG_WR(HW_POWER_BASE + HW_POWER_VDDIOCTRL, vddio);
	}

	return;
}

static void
set_vddmem_target(uint8_t target)
{
	uint32_t vddmem;

	/* Set target. */
	vddmem = REG_RD(HW_POWER_BASE + HW_POWER_VDDMEMCTRL);
	vddmem &= ~(HW_POWER_VDDMEMCTRL_TRG);
	vddmem |= __SHIFTIN(target, HW_POWER_VDDMEMCTRL_TRG);
	REG_WR(HW_POWER_BASE + HW_POWER_VDDMEMCTRL, vddmem);
	delay(1000);

	/* Enable VDDMEM */
	vddmem = REG_RD(HW_POWER_BASE + HW_POWER_VDDMEMCTRL);
	vddmem |= (HW_POWER_VDDMEMCTRL_PULLDOWN_ACTIVE |
	    HW_POWER_VDDMEMCTRL_ENABLE_ILIMIT |
	    HW_POWER_VDDMEMCTRL_ENABLE_LINREG);
	REG_WR(HW_POWER_BASE + HW_POWER_VDDMEMCTRL, vddmem);
	delay(500);
	vddmem &= ~(HW_POWER_VDDMEMCTRL_PULLDOWN_ACTIVE |
	    HW_POWER_VDDMEMCTRL_ENABLE_ILIMIT);
	REG_WR(HW_POWER_BASE + HW_POWER_VDDMEMCTRL, vddmem);

	return;
}
#ifdef DEBUG
#define PRINT_REG(REG)							\
	printf(#REG "\t%x\n\r", REG_RD(HW_POWER_BASE + REG));

static void
print_regs(void)
{
	PRINT_REG(HW_POWER_CTRL);
	PRINT_REG(HW_POWER_5VCTRL);
	PRINT_REG(HW_POWER_MINPWR);
	PRINT_REG(HW_POWER_CHARGE);
	PRINT_REG(HW_POWER_VDDDCTRL);
	PRINT_REG(HW_POWER_VDDACTRL);
	PRINT_REG(HW_POWER_VDDIOCTRL);
	PRINT_REG(HW_POWER_VDDMEMCTRL);
	PRINT_REG(HW_POWER_DCDC4P2);
	PRINT_REG(HW_POWER_MISC);
	PRINT_REG(HW_POWER_DCLIMITS);
	PRINT_REG(HW_POWER_LOOPCTRL);
	PRINT_REG(HW_POWER_STS);
	PRINT_REG(HW_POWER_SPEED);
	PRINT_REG(HW_POWER_BATTMONITOR);
	PRINT_REG(HW_POWER_RESET);
	PRINT_REG(HW_POWER_DEBUG);
	PRINT_REG(HW_POWER_SPECIAL);
	PRINT_REG(HW_POWER_VERSION);

	return;
}
#endif
