/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman.
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

#ifndef _DEV_I2C_ADM1026REG_H_
#define _DEV_I2C_ADM1026REG_H_

/*
 * Register definitions for "ADM1026 Thermal System Management Fan Controller"
 * Datasheet available from (URL valid as of December 2015):
 *   http://www.onsemi.com/pub_link/Collateral/ADM1026-D.PDF
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: adm1026reg.h,v 1.1.18.2 2017/12/03 11:37:02 jdolecek Exp $");

#define	ADM1026_ADDRMASK	0x3f8
#define ADM1026_ADDR		0x2c

#define ADM1026_CONF1		0x00
#define ADM1026_CONF2		0x01
#define ADM1026_FAN_DIV1	0x02
#define ADM1026_FAN_DIV2	0x03
#define ADM1026_DAC_CTRL	0x04
#define ADM1026_PWM_CTRL	0x05
#define ADM1026_EEPROM		0x06
#define ADM1026_TVG_CONF	0x07	/* THERM, V-REF and GPIO16 */
#define ADM1026_GPIO_CONF1	0x08
#define ADM1026_GPIO_CONF2	0x09
#define ADM1026_GPIO_CONF3	0x0a
#define ADM1026_GPIO_CONF4	0x0b
#define ADM1026_EEPROM2		0x0c
#define ADM1026_INT_THERM_MAX	0x0d
#define ADM1026_TDM1_THERM_MAX	0x0e
#define ADM1026_TDM2_THERM_MAX	0x0f
#define ADM1026_INT_FAN_MIN	0x10
#define ADM1026_TDM1_FAN_MIN	0x11
#define ADM1026_TDM2_FAN_MIN	0x12
#define ADM1026_EEPROM3		0x13
#define ADM1026_TEST1		0x14
#define ADM1026_TEST2		0x15
#define ADM1026_ID		0x16
#define ADM1026_REV		0x17
#define ADM1026_MASK1		0x18	/* Temperature and supply voltage */
#define ADM1026_MASK2		0x19	/* Analogue input */
#define ADM1026_MASK3		0x1a	/* Fan */
#define ADM1026_MASK4		0x1b	/* Temp, V-Batt, A-In8, Therm,
					   AFC, CI, GPIO 16 */
#define ADM1026_MASK5		0x1c	/* GPIO 0-7 */
#define ADM1026_MASK6		0x1d	/* GPIO 8-15 */
#define ADM1026_INT_TEMP_OFF	0x1e
#define ADM1026_INT_TEMP_VAL	0x1f
#define ADM1026_STAT1		0x20	/* External temp, and supply voltage */
#define ADM1026_STAT2		0x21	/* Analogue input */
#define ADM1026_STAT3		0x22	/* Fan */
#define ADM1026_STAT4		0x23	/* Temp, V-Batt, A-In8, Therm,
					   AFC, CI, GPIO 16 */
#define ADM1026_STAT5		0x24	/* GPIO 0-7 */
#define ADM1026_STAT6		0x25	/* GPIO 8-15 */
#define ADM1026_VBAT_VAL	0x26
#define ADM1026_AIN8_VAL	0x27
#define ADM1026_TDM1_VAL	0x28
#define ADM1026_TDM2_AIN9_VAL	0x29
#define ADM1026_33VSTBY_VAL	0x2a
#define ADM1026_33VMAIN_VAL	0x2b
#define ADM1026_50V_VAL		0x2c
#define ADM1026_VCCP_VAL	0x2d
#define ADM1026_12V_VAL		0x2e
#define ADM1026_N12V_VAL	0x2f
/* 8 analog in registers: 0x30 to 0x37 */
#define ADM1026_AIN_VAL(x)	(0x30 + x)
/* 8 fan value registers: 0x38 to 0x3f */
#define ADM1026_FAN_VAL(x)	(0x38 + x)
#define ADM1026_TDM1_MAX	0x40
#define ADM1026_TDM2_AIN9_MAX	0x41
#define ADM1026_33V_STBY_MAX	0x42
#define ADM1026_33V_MAIN_MAX	0x43
#define ADM1026_50V_MAX		0x44
#define ADM1026_VCCP_MAX	0x45
#define ADM1026_12V_MAX		0x46
#define ADM1026_N12V_MAX	0x47
#define ADM1026_TDM1_MIN	0x48
#define ADM1026_TDM2_AIN9_MIN	0x49
#define ADM1026_33V_STBY_MIN	0x4a
#define ADM1026_33V_MAIN_MIN	0x4b
#define ADM1026_50V_MIN		0x4c
#define ADM1026_VCCP_MIN	0x4d
#define ADM1026_12V_MIN		0x4e
#define ADM1026_N12V_MIN	0x4f
/* 8 analog in high limit registers: 0x50 to 0x57 */
#define ADM1026_AIN_MAX(x)	(0x50 + x)
/* 8 analog in low limit registers: 0x58 to 0x5f */
#define ADM1026_AIN_MIN(x)	(0x58 + x)
/* 8 fan high limit registers: 0x60 to 0x67 (no low limit registers) */
#define ADM1026_FAN_MAX(x)	(0x60 + x)
#define ADM1026_INT_TEMP_MAX	0x68
#define ADM1026_INT_TEMP_MIN	0x69
#define ADM1026_VBATT_MAX	0x6a
#define ADM1026_VBATT_MIN	0x6b
#define ADM1026_AIN8_MAX	0x6c
#define ADM1026_AIN8_MIN	0x6d
#define ADM1026_TDM1_OFF	0x6e
#define ADM1026_TDM2_OFF	0x6f

/* 0x00: Configuration register 1 */
#define ADM1026_CONF1_MONITOR	0x01	/* Start monitoring */
#define ADM1026_PIN_IS_TDM2(val)	((val & 0x08) == 0)

/* 0x01: Configuration register 2 */
#define ADM1026_PIN_IS_FAN(val, x)	((val & (1 << x)) == 0)

/* 0x02/0x03: Fan 0-7 divisors */
#define ADM1026_FAN0_DIV(x)	(x & 0x03)
#define ADM1026_FAN1_DIV(x)	((x >> 2) & 0x03)
#define ADM1026_FAN2_DIV(x)	((x >> 4) & 0x03)
#define ADM1026_FAN3_DIV(x)	((x >> 6) & 0x03)
#define ADM1026_FAN4_DIV(x)	ADM1026_FAN0_DIV(x)
#define ADM1026_FAN5_DIV(x)	ADM1026_FAN1_DIV(x)
#define ADM1026_FAN6_DIV(x)	ADM1026_FAN2_DIV(x)
#define ADM1026_FAN7_DIV(x)	ADM1026_FAN3_DIV(x)

/* 0x16/0x17: Manufacturer and revision ID's */
#define ADM1026_MANF_ID		0x41	/* Manufacturer ID 0x41 */
#define ADM1026_REVISION(x)	((x & 0xf0) >> 4)
#define ADM1026_MANF_REV	0x04	/* Manufacturer revision 0x04 */
#define ADM1026_STEPPING(x)	(x & 0x0f)

#endif /* _DEV_I2C_ADM1026REG_H_ */
