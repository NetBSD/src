/* $Id: power_prep.c,v 1.3.6.3 2014/08/20 00:02:56 tls Exp $ */

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

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/types.h>

#include <arm/imx/imx23_powerreg.h>

#include <lib/libkern/libkern.h>
#include <lib/libsa/stand.h>

#include "common.h"

#define PWR_CTRL	(HW_POWER_BASE + HW_POWER_CTRL)
#define PWR_CTRL_S	(HW_POWER_BASE + HW_POWER_CTRL_SET)
#define PWR_CTRL_C	(HW_POWER_BASE + HW_POWER_CTRL_CLR)
#define PWR_5VCTRL	(HW_POWER_BASE + HW_POWER_5VCTRL)
#define PWR_5VCTRL_S	(HW_POWER_BASE + HW_POWER_5VCTRL_SET)
#define PWR_5VCTRL_C	(HW_POWER_BASE + HW_POWER_5VCTRL_CLR)
#define PWR_MINPWR	(HW_POWER_BASE + HW_POWER_MINPWR)
#define PWR_MINPWR_S	(HW_POWER_BASE + HW_POWER_MINPWR_SET)
#define PWR_MINPWR_C	(HW_POWER_BASE + HW_POWER_MINPWR_CLR)
#define PWR_CHARGE	(HW_POWER_BASE + HW_POWER_CHARGE)
#define PWR_CHARGE_S	(HW_POWER_BASE + HW_POWER_CHARGE_SET)
#define PWR_CHARGE_C	(HW_POWER_BASE + HW_POWER_CHARGE_CLR)
#define PWR_VDDDCTRL	(HW_POWER_BASE + HW_POWER_VDDDCTRL)
#define PWR_VDDACTRL	(HW_POWER_BASE + HW_POWER_VDDACTRL)
#define PWR_VDDIOCTRL	(HW_POWER_BASE + HW_POWER_VDDIOCTRL)
#define PWR_VDDMEMCTRL	(HW_POWER_BASE + HW_POWER_VDDMEMCTRL)
#define PWR_DCDC4P2	(HW_POWER_BASE + HW_POWER_DCDC4P2)
#define PWR_MISC	(HW_POWER_BASE + HW_POWER_MISC)
#define PWR_DCLIMITS	(HW_POWER_BASE + HW_POWER_DCLIMITS)
#define PWR_LOOPCTRL 	(HW_POWER_BASE + HW_POWER_LOOPCTRL)
#define PWR_LOOPCTRL_S	(HW_POWER_BASE + HW_POWER_LOOPCTRL_SET)
#define PWR_LOOPCTRL_C	(HW_POWER_BASE + HW_POWER_LOOPCTRL_CLR)
#define PWR_STATUS	(HW_POWER_BASE + HW_POWER_STS)
#define PWR_SPEED	(HW_POWER_BASE + HW_POWER_SPEED)
#define PWR_BATTMONITOR	(HW_POWER_BASE + HW_POWER_BATTMONITOR)
#define PWR_RESET	(HW_POWER_BASE + HW_POWER_RESET)
#define PWR_DEBUG	(HW_POWER_BASE + HW_POWER_DEBUG)
#define PWR_SPECIAL	(HW_POWER_BASE + HW_POWER_SPECIAL)
#define PWR_VERSION	(HW_POWER_BASE + HW_POWER_VERSION)

#define VBUSVALID_TRSH 5	/* 4.4V */
#define CHARGE_4P2_ILIMIT_MAX 0x3f
#define CMPTRIP 0x1f	/* DCDC_4P2 pin >= 1.05 * BATTERY pin. */
#define DROPOUT_CTRL 0xa /* BO 100mV, DCDC selects higher. */

void en_vbusvalid(void);
int vbusvalid(void);
void power_tune(void);
void en_4p2_reg(void);
void en_4p2_to_dcdc(void);
void power_vddd_from_dcdc(int, int);
void power_vdda_from_dcdc(int, int);
void power_vddio_from_dcdc(int, int);
void power_vddmem(int);

/*
 * Configure the DCDC control logic 5V detection to use VBUSVALID.
 */
void
en_vbusvalid(void)
{
	uint32_t tmp_r;

	tmp_r = REG_RD(PWR_5VCTRL);
	tmp_r &= ~HW_POWER_5VCTRL_VBUSVALID_TRSH;
	tmp_r |= __SHIFTIN(VBUSVALID_TRSH, HW_POWER_5VCTRL_VBUSVALID_TRSH);
	REG_WR(PWR_5VCTRL, tmp_r);

	REG_WR(PWR_5VCTRL_S, HW_POWER_5VCTRL_PWRUP_VBUS_CMPS);
	delay(1000);
	
	REG_WR(PWR_5VCTRL_S, HW_POWER_5VCTRL_VBUSVALID_5VDETECT);

	return;
}
/*
 * Test VBUSVALID.
 */
int
vbusvalid(void)
{
	if (REG_RD(PWR_STATUS) & HW_POWER_STS_VBUSVALID)
		return 1;
	else
		return 0;
}
/*
 * Set various registers.
 */
void
power_tune(void)
{
	uint32_t tmp_r;

	REG_WR(PWR_LOOPCTRL_S, HW_POWER_LOOPCTRL_TOGGLE_DIF |
		HW_POWER_LOOPCTRL_EN_CM_HYST |
		HW_POWER_LOOPCTRL_EN_DF_HYST |
		HW_POWER_LOOPCTRL_RCSCALE_THRESH |
		__SHIFTIN(3, HW_POWER_LOOPCTRL_EN_RCSCALE));

	REG_WR(PWR_MINPWR_S, HW_POWER_MINPWR_DOUBLE_FETS);

	REG_WR(PWR_5VCTRL_S, __SHIFTIN(4, HW_POWER_5VCTRL_HEADROOM_ADJ));

	tmp_r = REG_RD(PWR_DCLIMITS);
	tmp_r &= ~HW_POWER_DCLIMITS_POSLIMIT_BUCK;
	tmp_r |= __SHIFTIN(0x30, HW_POWER_DCLIMITS_POSLIMIT_BUCK);
	REG_WR(PWR_DCLIMITS, tmp_r);

	return;
}
/*
 * AN3883.pdf 2.1.3.1 Enabling the 4P2 LinReg
 */
void
en_4p2_reg(void)
{
	uint32_t tmp_r;
	int ilimit;

	/* TRG is 4.2V by default. */
	tmp_r = REG_RD(PWR_DCDC4P2);
	tmp_r |= HW_POWER_DCDC4P2_ENABLE_4P2;
	REG_WR(PWR_DCDC4P2, tmp_r);

	REG_WR(PWR_CHARGE_S, HW_POWER_CHARGE_ENABLE_LOAD);

	/* Set CHARGE_4P2_ILIMIT to minimum. */
	REG_WR(PWR_5VCTRL_C, HW_POWER_5VCTRL_CHARGE_4P2_ILIMIT);
	REG_WR(PWR_5VCTRL_S, __SHIFTIN(1, HW_POWER_5VCTRL_CHARGE_4P2_ILIMIT));

	/* Power up 4.2V regulation circuit. */
	REG_WR(PWR_5VCTRL_C, HW_POWER_5VCTRL_PWD_CHARGE_4P2);

	/* Ungate path from 4P2 reg to DCDC. */
	tmp_r = REG_RD(PWR_DCDC4P2);
	tmp_r |= HW_POWER_DCDC4P2_ENABLE_DCDC;
	REG_WR(PWR_DCDC4P2, tmp_r);

	delay(10000);

	/* Charge 4P2 capacitance. */
	tmp_r = REG_RD(PWR_5VCTRL);
	for (ilimit = 2; ilimit <= CHARGE_4P2_ILIMIT_MAX; ilimit++) {
		tmp_r &= ~HW_POWER_5VCTRL_CHARGE_4P2_ILIMIT;
		tmp_r |= __SHIFTIN(ilimit, HW_POWER_5VCTRL_CHARGE_4P2_ILIMIT);
		REG_WR(PWR_5VCTRL, tmp_r);
		delay(10000);
	}

	return;
}

/*
 * AN3883.pdf 2.1.3.3 Enabling 4P2 Input to DC-DC
 */
void en_4p2_to_dcdc(void)
{
	uint32_t tmp_r;

	tmp_r = REG_RD(PWR_DCDC4P2);

	tmp_r &= ~HW_POWER_DCDC4P2_CMPTRIP;
	tmp_r |= __SHIFTIN(CMPTRIP, HW_POWER_DCDC4P2_CMPTRIP);

	tmp_r &= ~HW_POWER_DCDC4P2_DROPOUT_CTRL;
	tmp_r |= __SHIFTIN(DROPOUT_CTRL, HW_POWER_DCDC4P2_DROPOUT_CTRL);

	REG_WR(PWR_DCDC4P2, tmp_r);

	REG_WR(PWR_5VCTRL_C, HW_POWER_5VCTRL_DCDC_XFER);

	/* Enabling DCDC triggers 5V brownout. */
	REG_WR(PWR_5VCTRL_C, HW_POWER_5VCTRL_PWDN_5VBRNOUT);
	REG_WR(PWR_5VCTRL_S, HW_POWER_5VCTRL_ENABLE_DCDC);
	delay(10000);
	REG_WR(PWR_5VCTRL_S, HW_POWER_5VCTRL_PWDN_5VBRNOUT);
	
	/* Now DCDC is using 4P2 so I can remove extra temporary load. */
	REG_WR(PWR_CHARGE_C, HW_POWER_CHARGE_ENABLE_LOAD);

	return;
}

/*
 * Configure VDDD to source power from DCDC.
 */
void
power_vddd_from_dcdc(int target, int brownout)
{
	uint32_t tmp_r;

	/* BO_OFFSET must be withing 800mV - 1475mV */
	if (brownout > 1475)
		brownout = 1475;
	else if (brownout < 800)
		brownout = 800;


	/* Set LINREG_OFFSET one step below TRG. */
	tmp_r = REG_RD(PWR_VDDDCTRL);
	tmp_r &= ~HW_POWER_VDDDCTRL_LINREG_OFFSET;
	tmp_r |= __SHIFTIN(2, HW_POWER_VDDDCTRL_LINREG_OFFSET);
	REG_WR(PWR_VDDDCTRL, tmp_r);
	delay(10000);

	/* Enable VDDD switching converter output. */
	tmp_r = REG_RD(PWR_VDDDCTRL);
	tmp_r &= ~HW_POWER_VDDDCTRL_DISABLE_FET;
	REG_WR(PWR_VDDDCTRL, tmp_r);
	delay(10000);

	/* Disable linear regulator output. */
	tmp_r = REG_RD(PWR_VDDDCTRL);
	tmp_r &= ~HW_POWER_VDDDCTRL_ENABLE_LINREG;
	REG_WR(PWR_VDDDCTRL, tmp_r);
	delay(10000);

	/* Set target voltage and brownout level. */
	tmp_r = REG_RD(PWR_VDDDCTRL);
	tmp_r &= ~(HW_POWER_VDDDCTRL_BO_OFFSET | HW_POWER_VDDDCTRL_TRG);
	tmp_r |= __SHIFTIN(((target - brownout) / 25),
		HW_POWER_VDDDCTRL_BO_OFFSET);
	tmp_r |= __SHIFTIN(((target - 800) / 25), HW_POWER_VDDDCTRL_TRG);
	REG_WR(PWR_VDDDCTRL, tmp_r);
	delay(10000);

	/* Enable PWDN_BRNOUT. */
	REG_WR(PWR_CTRL_C, HW_POWER_CTRL_VDDD_BO_IRQ);
	
	tmp_r = REG_RD(PWR_VDDDCTRL);
	tmp_r |= HW_POWER_VDDDCTRL_PWDN_BRNOUT;
	REG_WR(PWR_VDDDCTRL, tmp_r);

	return;
}
/*
 * Configure VDDA to source power from DCDC.
 */
void
power_vdda_from_dcdc(int target, int brownout)
{
	uint32_t tmp_r;

	/* BO_OFFSET must be withing 1400mV - 2175mV */
	if (brownout > 2275)
		brownout = 2275;
	else if (brownout < 1400)
		brownout = 1400;


	/* Set LINREG_OFFSET one step below TRG. */
	tmp_r = REG_RD(PWR_VDDACTRL);
	tmp_r &= ~HW_POWER_VDDACTRL_LINREG_OFFSET;
	tmp_r |= __SHIFTIN(2, HW_POWER_VDDACTRL_LINREG_OFFSET);
	REG_WR(PWR_VDDACTRL, tmp_r);
	delay(10000);

	/* Enable VDDA switching converter output. */
	tmp_r = REG_RD(PWR_VDDACTRL);
	tmp_r &= ~HW_POWER_VDDACTRL_DISABLE_FET;
	REG_WR(PWR_VDDACTRL, tmp_r);
	delay(10000);

	/* Disable linear regulator output. */
	tmp_r = REG_RD(PWR_VDDACTRL);
	tmp_r &= ~HW_POWER_VDDACTRL_ENABLE_LINREG;
	REG_WR(PWR_VDDACTRL, tmp_r);
	delay(10000);

	/* Set target voltage and brownout level. */
	tmp_r = REG_RD(PWR_VDDACTRL);
	tmp_r &= ~(HW_POWER_VDDACTRL_BO_OFFSET | HW_POWER_VDDACTRL_TRG);
	tmp_r |= __SHIFTIN(((target - brownout) / 25),
		HW_POWER_VDDACTRL_BO_OFFSET);
	tmp_r |= __SHIFTIN(((target - 1500) / 25), HW_POWER_VDDACTRL_TRG);
	REG_WR(PWR_VDDACTRL, tmp_r);
	delay(10000);

	/* Enable PWDN_BRNOUT. */
	REG_WR(PWR_CTRL_C, HW_POWER_CTRL_VDDA_BO_IRQ);
	
	tmp_r = REG_RD(PWR_VDDACTRL);
	tmp_r |= HW_POWER_VDDACTRL_PWDN_BRNOUT;
	REG_WR(PWR_VDDACTRL, tmp_r);

	return;
}
/*
 * Configure VDDIO to source power from DCDC.
 */
void
power_vddio_from_dcdc(int target, int brownout)
{
	uint32_t tmp_r;

	/* BO_OFFSET must be withing 2700mV - 3475mV */
	if (brownout > 3475)
		brownout = 3475;
	else if (brownout < 2700)
		brownout = 2700;


	/* Set LINREG_OFFSET one step below TRG. */
	tmp_r = REG_RD(PWR_VDDIOCTRL);
	tmp_r &= ~HW_POWER_VDDIOCTRL_LINREG_OFFSET;
	tmp_r |= __SHIFTIN(2, HW_POWER_VDDIOCTRL_LINREG_OFFSET);
	REG_WR(PWR_VDDIOCTRL, tmp_r);
	delay(10000);

	/* Enable VDDIO switching converter output. */
	tmp_r = REG_RD(PWR_VDDIOCTRL);
	tmp_r &= ~HW_POWER_VDDIOCTRL_DISABLE_FET;
	REG_WR(PWR_VDDIOCTRL, tmp_r);
	delay(10000);

	/* Set target voltage and brownout level. */
	tmp_r = REG_RD(PWR_VDDIOCTRL);
	tmp_r &= ~(HW_POWER_VDDIOCTRL_BO_OFFSET | HW_POWER_VDDIOCTRL_TRG);
	tmp_r |= __SHIFTIN(((target - brownout) / 25),
		HW_POWER_VDDIOCTRL_BO_OFFSET);
	tmp_r |= __SHIFTIN(((target - 2800) / 25), HW_POWER_VDDIOCTRL_TRG);
	REG_WR(PWR_VDDIOCTRL, tmp_r);
	delay(10000);

	/* Enable PWDN_BRNOUT. */
	REG_WR(PWR_CTRL_C, HW_POWER_CTRL_VDDIO_BO_IRQ);
	
	tmp_r = REG_RD(PWR_VDDIOCTRL);
	tmp_r |= HW_POWER_VDDIOCTRL_PWDN_BRNOUT;
	REG_WR(PWR_VDDIOCTRL, tmp_r);

	return;
}
/*
 * AN3883.pdf 2.3.1.2 Setting VDDMEM Target Voltage
 */
void
power_vddmem(int target)
{
	uint32_t tmp_r;

	/* Set target voltage. */
	tmp_r = REG_RD(PWR_VDDMEMCTRL);
	tmp_r &= ~(HW_POWER_VDDMEMCTRL_TRG);
	tmp_r |= __SHIFTIN(((target - 1700) / 50), HW_POWER_VDDMEMCTRL_TRG);
	REG_WR(PWR_VDDMEMCTRL, tmp_r);
	delay(10000);

	tmp_r = REG_RD(PWR_VDDMEMCTRL);
	tmp_r |= (HW_POWER_VDDMEMCTRL_PULLDOWN_ACTIVE |
		HW_POWER_VDDMEMCTRL_ENABLE_ILIMIT |
		HW_POWER_VDDMEMCTRL_ENABLE_LINREG);
	REG_WR(PWR_VDDMEMCTRL, tmp_r);

	delay(1000);

	tmp_r = REG_RD(PWR_VDDMEMCTRL);
	tmp_r &= ~(HW_POWER_VDDMEMCTRL_PULLDOWN_ACTIVE |
		HW_POWER_VDDMEMCTRL_ENABLE_ILIMIT);
	REG_WR(PWR_VDDMEMCTRL, tmp_r);

	return;
}
