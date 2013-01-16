/* $Id: power_prep.c,v 1.3.2.2 2013/01/16 05:32:56 yamt Exp $ */

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

#include <arm/imx/imx23_powerreg.h>

#include <lib/libsa/stand.h>

#include "common.h"

void charge_4p2_capacitance(void);
void enable_4p2_linreg(void);
void enable_dcdc(void);
void enable_vbusvalid_comparator(void);
void set_targets(void);
void dcdc4p2_enable_dcdc(void);
void p5vctrl_enable_dcdc(void);
void enable_vddmem(void);

/*
 * Power rail voltage targets, brownout levels and linear regulator
 * offsets from the target.
 *
 * Supply	Target	BO	LinReg offset
 * ------------------------------------------
 * VDDD		1.550 V	1.450 V	-25 mV
 * VDDA		1.750 V	1.575 V	-25 mV
 * VDDIO	3.100 V	2.925 V	-25 mV
 * VDDMEM	2.500 V <na>	<na>
 *
 * BO = Brownout level below target.
 */
#define VDDD_TARGET 0x1e
#define VDDD_BO_OFFSET 0x4
#define VDDD_LINREG_OFFSET 0x02

#define VDDA_TARGET 0x0A
#define VDDA_BO_OFFSET 0x07
#define VDDA_LINREG_OFFSET 0x02

#define VDDIO_TARGET 0x0C
#define VDDIO_BO_OFFSET 0x07
#define VDDIO_LINREG_OFFSET 0x02

#define VDDMEM_TARGET 0x10

/*
 * Threshold voltage for the VBUSVALID comparator.
 * Always make sure that the VDD5V voltage level is higher.
 */
#define VBUSVALID_TRSH 0x02	/* 4.1 V */

/* Limits for BATT charger + 4P2 current */
#define P4P2_ILIMIT_MIN 0x01	/* 10 mA */
#define P4P2_ILIMIT_MAX 0x3f	/* 780 mA */

/*
 * Trip point for the comparison between the DCDC_4P2 and BATTERY pin.
 * If this voltage comparation is true then 5 V originated power will supply
 * the DCDC. Otherwise battery will be used.
 */
#define DCDC4P2_CMPTRIP 0x00	/* DCDC_4P2 pin > 0.85 * BATTERY pin */

/*
 * Adjust the behavior of the DCDC and 4.2 V circuit.
 * Two MSBs control the VDD4P2 brownout below the DCDC4P2_TRG before the
 * regulation circuit steals battery charge. Two LSBs control which power
 * source is selected by the DCDC.
 */
#define DCDC4P2_DROPOUT_CTRL_BO_200 0x0C
#define DCDC4P2_DROPOUT_CTRL_BO_100 0x08
#define DCDC4P2_DROPOUT_CTRL_BO_050 0x04
#define DCDC4P2_DROPOUT_CTRL_BO_025 0x00

#define DCDC4P2_DROPOUT_CTRL_SEL_4P2 0x00	/* Don't use battery at all. */
#define DCDC4P2_DROPOUT_CTRL_SEL_BATT_IF_GT_4P2 0x01	/* BATT if 4P2 < BATT */
#define DCDC4P2_DROPOUT_CTRL_SEL_HIGHER 0x02	/* Selects which ever is
						 * higher. */

#define DCDC4P2_DROPOUT_CTRL (DCDC4P2_DROPOUT_CTRL_BO_200 |\
	    DCDC4P2_DROPOUT_CTRL_SEL_4P2)

/*
 * Prepare system for a 5 V operation.
 *
 * The system uses inefficient linear regulators as a power source after boot.
 * This code enables the use of more energy efficient DC-DC converter as a
 * power source.
 */
int
power_prep(void)
{

	/* Enable clocks to the power block */
	REG_WRITE(HW_POWER_BASE + HW_POWER_CTRL_CLR, HW_POWER_CTRL_CLKGATE);

	set_targets();
	enable_vbusvalid_comparator();
	enable_4p2_linreg();
	charge_4p2_capacitance();
	enable_dcdc();
	enable_vddmem();

	return 0;
}

/*
 * Set switching converter voltage targets, brownout levels and linear
 * regulator output offsets.
 */
void
set_targets(void)
{
	uint32_t vddctrl;

	/* VDDD */
	vddctrl = REG_READ(HW_POWER_BASE + HW_POWER_VDDDCTRL);

	vddctrl &= ~(HW_POWER_VDDDCTRL_LINREG_OFFSET |
	    HW_POWER_VDDDCTRL_BO_OFFSET |
	    HW_POWER_VDDDCTRL_TRG);
	vddctrl |=
	    __SHIFTIN(VDDD_LINREG_OFFSET, HW_POWER_VDDDCTRL_LINREG_OFFSET) |
	    __SHIFTIN(VDDD_BO_OFFSET, HW_POWER_VDDDCTRL_BO_OFFSET) |
	    __SHIFTIN(VDDD_TARGET, HW_POWER_VDDDCTRL_TRG);

	REG_WRITE(HW_POWER_BASE + HW_POWER_VDDDCTRL, vddctrl);

	/* VDDA */
	vddctrl = REG_READ(HW_POWER_BASE + HW_POWER_VDDACTRL);

	vddctrl &= ~(HW_POWER_VDDACTRL_LINREG_OFFSET |
	    HW_POWER_VDDACTRL_BO_OFFSET |
	    HW_POWER_VDDACTRL_TRG);
	vddctrl |=
	    __SHIFTIN(VDDA_LINREG_OFFSET, HW_POWER_VDDACTRL_LINREG_OFFSET) |
	    __SHIFTIN(VDDA_BO_OFFSET, HW_POWER_VDDACTRL_BO_OFFSET) |
	    __SHIFTIN(VDDA_TARGET, HW_POWER_VDDACTRL_TRG);

	REG_WRITE(HW_POWER_BASE + HW_POWER_VDDACTRL, vddctrl);

	/* VDDIO */
	vddctrl = REG_READ(HW_POWER_BASE + HW_POWER_VDDIOCTRL);

	vddctrl &= ~(HW_POWER_VDDIOCTRL_LINREG_OFFSET |
	    HW_POWER_VDDIOCTRL_BO_OFFSET |
	    HW_POWER_VDDIOCTRL_TRG);
	vddctrl |=
	    __SHIFTIN(VDDIO_LINREG_OFFSET, HW_POWER_VDDACTRL_LINREG_OFFSET) |
	    __SHIFTIN(VDDIO_BO_OFFSET, HW_POWER_VDDACTRL_BO_OFFSET) |
	    __SHIFTIN(VDDIO_TARGET, HW_POWER_VDDACTRL_TRG);

	REG_WRITE(HW_POWER_BASE + HW_POWER_VDDIOCTRL, vddctrl);

	/* VDDMEM */
	vddctrl = REG_READ(HW_POWER_BASE + HW_POWER_VDDMEMCTRL);
	vddctrl &= ~(HW_POWER_VDDMEMCTRL_TRG);
	vddctrl |= __SHIFTIN(VDDMEM_TARGET, HW_POWER_VDDMEMCTRL_TRG);

	REG_WRITE(HW_POWER_BASE + HW_POWER_VDDMEMCTRL, vddctrl);

	return;
}

/*
 * VBUSVALID comparator is accurate method to determine the presence of 5 V.
 * Turn on the comparator, set its voltage treshold and instruct DC-DC to
 * use it.
 */
void
enable_vbusvalid_comparator()
{
	uint32_t p5vctrl;

	/*
	 * Disable 5 V brownout detection temporarily because setting
	 * VBUSVALID_5VDETECT can cause false brownout.
	 */
	REG_WRITE(HW_POWER_BASE + HW_POWER_5VCTRL_CLR,
	    HW_POWER_5VCTRL_PWDN_5VBRNOUT);

	p5vctrl = REG_READ(HW_POWER_BASE + HW_POWER_5VCTRL);

	p5vctrl &= ~HW_POWER_5VCTRL_VBUSVALID_TRSH;
	p5vctrl |=
	    /* Turn on VBUS comparators. */
	    (HW_POWER_5VCTRL_PWRUP_VBUS_CMPS |
	    /* Set treshold for VBUSVALID comparator. */
	    __SHIFTIN(VBUSVALID_TRSH, HW_POWER_5VCTRL_VBUSVALID_TRSH) |
	    /* Set DC-DC to use VBUSVALID comparator. */
	    HW_POWER_5VCTRL_VBUSVALID_5VDETECT);

	REG_WRITE(HW_POWER_BASE + HW_POWER_5VCTRL, p5vctrl);

	/* Enable temporarily disabled 5 V brownout detection. */
	REG_WRITE(HW_POWER_BASE + HW_POWER_5VCTRL_SET,
	    HW_POWER_5VCTRL_PWDN_5VBRNOUT);

	return;
}

/*
 * Enable 4P2 linear regulator.
 */
void
enable_4p2_linreg(void)
{
	uint32_t dcdc4p2;
	uint32_t p5vctrl;

	dcdc4p2 = REG_READ(HW_POWER_BASE + HW_POWER_DCDC4P2);
	/* Set the 4P2 target to 4.2 V and BO to 3.6V by clearing TRG and BO
	 * field. */
	dcdc4p2 &= ~(HW_POWER_DCDC4P2_TRG | HW_POWER_DCDC4P2_BO);
	/* Enable the 4P2 circuitry to control the LinReg. */
	dcdc4p2 |= HW_POWER_DCDC4P2_ENABLE_4P2;
	REG_WRITE(HW_POWER_BASE + HW_POWER_DCDC4P2, dcdc4p2);

	/* The 4P2 LinReg needs a static load to operate correctly. Since the
	 * DC-DC is not yet loading the LinReg, another load must be used. */
	REG_WRITE(HW_POWER_BASE + HW_POWER_CHARGE_SET,
	    HW_POWER_CHARGE_ENABLE_LOAD);

	p5vctrl = REG_READ(HW_POWER_BASE + HW_POWER_5VCTRL);
	p5vctrl &= ~(HW_POWER_5VCTRL_CHARGE_4P2_ILIMIT |
	    /* Power on the 4P2 LinReg. ON = 0x0, OFF = 0x1 */
	    HW_POWER_5VCTRL_PWD_CHARGE_4P2);
	p5vctrl |=
	    /* Provide an initial current limit for the 4P2 LinReg with the
	     * smallest value possible. */
	    __SHIFTIN(P4P2_ILIMIT_MIN, HW_POWER_5VCTRL_CHARGE_4P2_ILIMIT);
	REG_WRITE(HW_POWER_BASE + HW_POWER_5VCTRL, p5vctrl);

	/* Ungate the path from 4P2 LinReg to DC-DC. */
	dcdc4p2_enable_dcdc();

	return;
}

/*
 * There is capacitor on the 4P2 output which must be charged before powering
 * on the 4P2 linear regulator to avoid brownouts on the 5 V source.
 * Charging is done by slowly increasing current limit until it reaches
 * P4P2_ILIMIT_MAX.
 */
void
charge_4p2_capacitance(void)
{
	uint32_t ilimit;
	uint32_t p5vctrl;

	p5vctrl = REG_READ(HW_POWER_BASE + HW_POWER_5VCTRL);
	ilimit = __SHIFTOUT(p5vctrl, HW_POWER_5VCTRL_CHARGE_4P2_ILIMIT);

	/* Increment current limit slowly. */
	while (ilimit < P4P2_ILIMIT_MAX) {
		ilimit++;
		p5vctrl &= ~(HW_POWER_5VCTRL_CHARGE_4P2_ILIMIT);
		p5vctrl |= __SHIFTIN(ilimit, HW_POWER_5VCTRL_CHARGE_4P2_ILIMIT);
		REG_WRITE(HW_POWER_BASE + HW_POWER_5VCTRL, p5vctrl);
		delay_us(10000);
	}

	return;
}

/*
 * Enable DCDC to use 4P2 regulator and set its power source selection logic.
 */
void
enable_dcdc(void)
{
	uint32_t dcdc4p2;
	uint32_t vddctrl;

	dcdc4p2 = REG_READ(HW_POWER_BASE + HW_POWER_DCDC4P2);
	dcdc4p2 &= ~(HW_POWER_DCDC4P2_CMPTRIP | HW_POWER_DCDC4P2_DROPOUT_CTRL);
	/* Comparison between the DCDC_4P2 pin and BATTERY pin to choose which
	 * will supply the DCDC. */
	dcdc4p2 |= __SHIFTIN(DCDC4P2_CMPTRIP, HW_POWER_DCDC4P2_CMPTRIP);
	/* DC-DC brownout and select logic. */
	dcdc4p2 |= __SHIFTIN(DCDC4P2_DROPOUT_CTRL,
	    HW_POWER_DCDC4P2_DROPOUT_CTRL);
	REG_WRITE(HW_POWER_BASE + HW_POWER_DCDC4P2, dcdc4p2);

	/* Disable the automatic DC-DC startup when 5 V is lost (it is on
	 * already) */
	REG_WRITE(HW_POWER_BASE + HW_POWER_5VCTRL_CLR,
	    HW_POWER_5VCTRL_DCDC_XFER);

	p5vctrl_enable_dcdc();

	/* Enable switching converter outputs and disable linear regulators
	 * for VDDD, VDDIO and VDDA. */
	vddctrl = REG_READ(HW_POWER_BASE + HW_POWER_VDDDCTRL);
	vddctrl &= ~(HW_POWER_VDDDCTRL_DISABLE_FET |
	    HW_POWER_VDDDCTRL_ENABLE_LINREG);
	REG_WRITE(HW_POWER_BASE + HW_POWER_VDDDCTRL, vddctrl);

	vddctrl = REG_READ(HW_POWER_BASE + HW_POWER_VDDIOCTRL);
	vddctrl &= ~HW_POWER_VDDIOCTRL_DISABLE_FET;
	REG_WRITE(HW_POWER_BASE + HW_POWER_VDDIOCTRL, vddctrl);

	vddctrl = REG_READ(HW_POWER_BASE + HW_POWER_VDDACTRL);
	vddctrl &= ~(HW_POWER_VDDACTRL_DISABLE_FET |
	    HW_POWER_VDDACTRL_ENABLE_LINREG);
	REG_WRITE(HW_POWER_BASE + HW_POWER_VDDACTRL, vddctrl);

	/* The 4P2 LinReg needs a static load to operate correctly. Since the
	 * DC-DC is already running we can remove extra 100 ohm load enabled
	 * before. */
	REG_WRITE(HW_POWER_BASE + HW_POWER_CHARGE_CLR,
	    HW_POWER_CHARGE_ENABLE_LOAD);

	return;
}

/*
 * DCDC4P2 DCDC enable sequence according to errata #5837
 */
void
dcdc4p2_enable_dcdc(void)
{
	uint32_t dcdc4p2;
	uint32_t p5vctrl;
	uint32_t p5vctrl_saved;
	uint32_t pctrl;
	uint32_t pctrl_saved;

	pctrl_saved = REG_READ(HW_POWER_BASE + HW_POWER_CTRL);
	p5vctrl_saved = REG_READ(HW_POWER_BASE + HW_POWER_5VCTRL);

	/* Disable the power rail brownout interrupts. */
	REG_WRITE(HW_POWER_BASE + HW_POWER_CTRL_CLR,
	    (HW_POWER_CTRL_ENIRQ_VDDA_BO |
	    HW_POWER_CTRL_ENIRQ_VDDD_BO |
	    HW_POWER_CTRL_ENIRQ_VDDIO_BO));

	/* Set the HW_POWER_5VCTRL PWRUP_VBUS_CMPS bit (may already be set) */
	REG_WRITE(HW_POWER_BASE + HW_POWER_5VCTRL_SET,
	    HW_POWER_5VCTRL_PWRUP_VBUS_CMPS);

	/* Set the HW_POWER_5VCTRL VBUSVALID_5VDETECT bit to 0 */
	REG_WRITE(HW_POWER_BASE + HW_POWER_5VCTRL_CLR,
	    HW_POWER_5VCTRL_VBUSVALID_5VDETECT);

	/* Set the HW_POWER_5VCTRL VBUSVALID_TRSH to 0x0 (2.9 V) */
	REG_WRITE(HW_POWER_BASE + HW_POWER_5VCTRL_CLR,
	    HW_POWER_5VCTRL_VBUSVALID_TRSH);

	/* Disable VBUSDROOP status and interrupt. */
	REG_WRITE(HW_POWER_BASE + HW_POWER_CTRL_CLR,
	    (HW_POWER_CTRL_VDD5V_DROOP_IRQ | HW_POWER_CTRL_ENIRQ_VDD5V_DROOP));

	/* Set the ENABLE_DCDC bit in HW_POWER_DCDC4P2. */
	dcdc4p2 = REG_READ(HW_POWER_BASE + HW_POWER_DCDC4P2);
	dcdc4p2 |= HW_POWER_DCDC4P2_ENABLE_DCDC;
	REG_WRITE(HW_POWER_BASE + HW_POWER_DCDC4P2, dcdc4p2);

	delay_us(100);

	pctrl = REG_READ(HW_POWER_BASE + HW_POWER_CTRL);
	/* VBUSVALID_IRQ is set. */
	if (__SHIFTOUT(pctrl, HW_POWER_CTRL_VBUSVALID_IRQ)) {
		/* Set and clear the PWD_CHARGE_4P2 bit to repower on the 4P2
		 * regulator because it is automatically shut off on a
		 * VBUSVALID false condition. */
		REG_WRITE(HW_POWER_BASE + HW_POWER_5VCTRL_SET,
		    HW_POWER_5VCTRL_PWD_CHARGE_4P2);
		REG_WRITE(HW_POWER_BASE + HW_POWER_5VCTRL_CLR,
		    HW_POWER_5VCTRL_PWD_CHARGE_4P2);
		/* Ramp up the CHARGE_4P2_ILIMIT value at this point. */
		charge_4p2_capacitance();
	}

	/* Restore modified bits back to HW_POWER_CTRL. */
	if (pctrl_saved & HW_POWER_CTRL_ENIRQ_VDDA_BO)
		REG_WRITE(HW_POWER_BASE + HW_POWER_CTRL_SET,
		    HW_POWER_CTRL_ENIRQ_VDDA_BO);
	else
		REG_WRITE(HW_POWER_BASE + HW_POWER_CTRL_CLR,
		    HW_POWER_CTRL_ENIRQ_VDDA_BO);

	if (pctrl_saved & HW_POWER_CTRL_ENIRQ_VDDD_BO)
		REG_WRITE(HW_POWER_BASE + HW_POWER_CTRL_SET,
		    HW_POWER_CTRL_ENIRQ_VDDD_BO);
	else
		REG_WRITE(HW_POWER_BASE + HW_POWER_CTRL_CLR,
		    HW_POWER_CTRL_ENIRQ_VDDD_BO);

	if (pctrl_saved & HW_POWER_CTRL_ENIRQ_VDDIO_BO)
		REG_WRITE(HW_POWER_BASE + HW_POWER_CTRL_SET,
		    HW_POWER_CTRL_ENIRQ_VDDIO_BO);
	else
		REG_WRITE(HW_POWER_BASE + HW_POWER_CTRL_CLR,
		    HW_POWER_CTRL_ENIRQ_VDDIO_BO);

	if (pctrl_saved & HW_POWER_CTRL_VDD5V_DROOP_IRQ)
		REG_WRITE(HW_POWER_BASE + HW_POWER_CTRL_SET,
		    HW_POWER_CTRL_VDD5V_DROOP_IRQ);
	else
		REG_WRITE(HW_POWER_BASE + HW_POWER_CTRL_CLR,
		    HW_POWER_CTRL_VDD5V_DROOP_IRQ);

	if (pctrl_saved & HW_POWER_CTRL_ENIRQ_VDD5V_DROOP)
		REG_WRITE(HW_POWER_BASE + HW_POWER_CTRL_SET,
		    HW_POWER_CTRL_ENIRQ_VDD5V_DROOP);
	else
		REG_WRITE(HW_POWER_BASE + HW_POWER_CTRL_CLR,
		    HW_POWER_CTRL_ENIRQ_VDD5V_DROOP);

	/* Restore modified bits back to HW_POWER_5VCTRL. */
	p5vctrl = REG_READ(HW_POWER_BASE + HW_POWER_5VCTRL);
	p5vctrl &= ~(HW_POWER_5VCTRL_PWRUP_VBUS_CMPS |
	    HW_POWER_5VCTRL_VBUSVALID_5VDETECT |
	    HW_POWER_5VCTRL_VBUSVALID_TRSH);
	p5vctrl |= __SHIFTOUT(p5vctrl_saved, HW_POWER_5VCTRL_VBUSVALID_TRSH) |
	    (p5vctrl_saved & HW_POWER_5VCTRL_PWRUP_VBUS_CMPS) |
	    (p5vctrl_saved & HW_POWER_5VCTRL_VBUSVALID_5VDETECT);
	REG_WRITE(HW_POWER_BASE + HW_POWER_5VCTRL, p5vctrl);

	return;
}

/*
 * 5VCTRL DCDC enable sequence according to errata #5837
 */
void
p5vctrl_enable_dcdc(void)
{
	uint32_t p5vctrl;
	uint32_t p5vctrl_saved;
	uint32_t pctrl;
	uint32_t pctrl_saved;

	pctrl_saved = REG_READ(HW_POWER_BASE + HW_POWER_CTRL);
	p5vctrl_saved = REG_READ(HW_POWER_BASE + HW_POWER_5VCTRL);

	/* Disable the power rail brownout interrupts. */
	REG_WRITE(HW_POWER_BASE + HW_POWER_CTRL_CLR,
	    HW_POWER_CTRL_ENIRQ_VDDA_BO |
	    HW_POWER_CTRL_ENIRQ_VDDD_BO |
	    HW_POWER_CTRL_ENIRQ_VDDIO_BO);

	/* Set the HW_POWER_5VCTRL PWRUP_VBUS_CMPS bit (may already be set) */
	REG_WRITE(HW_POWER_BASE + HW_POWER_5VCTRL_SET,
	    HW_POWER_5VCTRL_PWRUP_VBUS_CMPS);

	/* Set the HW_POWER_5VCTRL VBUSVALID_5VDETECT bit to 1 */
	REG_WRITE(HW_POWER_BASE + HW_POWER_5VCTRL_SET,
	    HW_POWER_5VCTRL_VBUSVALID_5VDETECT);

	/* Set the HW_POWER_5VCTRL VBUSVALID_TRSH to 0x0 (2.9 V) */
	REG_WRITE(HW_POWER_BASE + HW_POWER_5VCTRL_CLR,
	    HW_POWER_5VCTRL_VBUSVALID_TRSH);

	/* Disable VBUSDROOP status and interrupt. */
	REG_WRITE(HW_POWER_BASE + HW_POWER_CTRL_CLR,
	    (HW_POWER_CTRL_VDD5V_DROOP_IRQ | HW_POWER_CTRL_ENIRQ_VDD5V_DROOP));

	/* Work over errata #2816 by disabling 5 V brownout while modifying
	 * ENABLE_DCDC. */
	REG_WRITE(HW_POWER_BASE + HW_POWER_5VCTRL_CLR,
	    HW_POWER_5VCTRL_PWDN_5VBRNOUT);

	/* Set the ENABLE_DCDC bit in HW_POWER_5VCTRL. */
	REG_WRITE(HW_POWER_BASE + HW_POWER_5VCTRL_SET,
	    HW_POWER_5VCTRL_ENABLE_DCDC);

	REG_WRITE(HW_POWER_BASE + HW_POWER_5VCTRL_SET,
	    HW_POWER_5VCTRL_PWDN_5VBRNOUT);
 
	delay_us(100);

	pctrl = REG_READ(HW_POWER_BASE + HW_POWER_CTRL);
	/* VBUSVALID_IRQ is set. */
	if (__SHIFTOUT(pctrl, HW_POWER_CTRL_VBUSVALID_IRQ)) {
		/* repeat the sequence for enabling the 4P2 regulator and DCDC
		 * from 4P2. */
		enable_4p2_linreg();
	}
	/* Restore modified bits back to HW_POWER_CTRL. */
	if (pctrl_saved & HW_POWER_CTRL_ENIRQ_VDDA_BO)
		REG_WRITE(HW_POWER_BASE + HW_POWER_CTRL_SET,
		    HW_POWER_CTRL_ENIRQ_VDDA_BO);
	else
		REG_WRITE(HW_POWER_BASE + HW_POWER_CTRL_CLR,
		    HW_POWER_CTRL_ENIRQ_VDDA_BO);

	if (pctrl_saved & HW_POWER_CTRL_ENIRQ_VDDD_BO)
		REG_WRITE(HW_POWER_BASE + HW_POWER_CTRL_SET,
		    HW_POWER_CTRL_ENIRQ_VDDD_BO);
	else
		REG_WRITE(HW_POWER_BASE + HW_POWER_CTRL_CLR,
		    HW_POWER_CTRL_ENIRQ_VDDD_BO);

	if (pctrl_saved & HW_POWER_CTRL_ENIRQ_VDDIO_BO)
		REG_WRITE(HW_POWER_BASE + HW_POWER_CTRL_SET,
		    HW_POWER_CTRL_ENIRQ_VDDIO_BO);
	else
		REG_WRITE(HW_POWER_BASE + HW_POWER_CTRL_CLR,
		    HW_POWER_CTRL_ENIRQ_VDDIO_BO);

	if (pctrl_saved & HW_POWER_CTRL_VDD5V_DROOP_IRQ)
		REG_WRITE(HW_POWER_BASE + HW_POWER_CTRL_SET,
		    HW_POWER_CTRL_VDD5V_DROOP_IRQ);
	else
		REG_WRITE(HW_POWER_BASE + HW_POWER_CTRL_CLR,
		    HW_POWER_CTRL_VDD5V_DROOP_IRQ);

	if (pctrl_saved & HW_POWER_CTRL_ENIRQ_VDD5V_DROOP)
		REG_WRITE(HW_POWER_BASE + HW_POWER_CTRL_SET,
		    HW_POWER_CTRL_ENIRQ_VDD5V_DROOP);
	else
		REG_WRITE(HW_POWER_BASE + HW_POWER_CTRL_CLR,
		    HW_POWER_CTRL_ENIRQ_VDD5V_DROOP);

	/* Restore modified bits back to HW_POWER_5VCTRL. */
	p5vctrl = REG_READ(HW_POWER_BASE + HW_POWER_5VCTRL);
	p5vctrl &= ~(HW_POWER_5VCTRL_PWRUP_VBUS_CMPS |
	    HW_POWER_5VCTRL_VBUSVALID_5VDETECT |
	    HW_POWER_5VCTRL_VBUSVALID_TRSH);
	p5vctrl |= __SHIFTOUT(p5vctrl_saved, HW_POWER_5VCTRL_VBUSVALID_TRSH) |
	    (p5vctrl_saved & HW_POWER_5VCTRL_PWRUP_VBUS_CMPS) |
	    (p5vctrl_saved & HW_POWER_5VCTRL_VBUSVALID_5VDETECT);
	REG_WRITE(HW_POWER_BASE + HW_POWER_5VCTRL, p5vctrl);

	return;
}

void
enable_vddmem(void)
{
	uint32_t vddctrl;

	/* VDDMEM */
	vddctrl = REG_READ(HW_POWER_BASE + HW_POWER_VDDMEMCTRL);
	vddctrl |= (HW_POWER_VDDMEMCTRL_PULLDOWN_ACTIVE |
	    HW_POWER_VDDMEMCTRL_ENABLE_ILIMIT |
	    HW_POWER_VDDMEMCTRL_ENABLE_LINREG);
	REG_WRITE(HW_POWER_BASE + HW_POWER_VDDMEMCTRL, vddctrl);
	delay_us(500);
	vddctrl &= ~(HW_POWER_VDDMEMCTRL_PULLDOWN_ACTIVE |
	    HW_POWER_VDDMEMCTRL_ENABLE_ILIMIT);
	REG_WRITE(HW_POWER_BASE + HW_POWER_VDDMEMCTRL, vddctrl);
	delay_us(10000);

	return;
}
