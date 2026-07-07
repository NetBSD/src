/* $NetBSD: adt7462reg.h,v 1.2 2026/07/07 12:27:03 jdc Exp $ */

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
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

#ifndef _DEV_I2C_ADT7462REG_H_
#define _DEV_I2C_ADT7462REG_H_

/*
 * Register definitions for "ADT7462 Flexible Temperature, Voltage Monitor,
 * and System Fan Controller".
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: adt7462reg.h,v 1.2 2026/07/07 12:27:03 jdc Exp $");

#define ADT7462_ADDR1		0x58
#define ADT7462_ADDR2		0x5c

#define ADT7462_CONF0		0x00
#define ADT7462_CONF1		0x01
#define ADT7462_CONF2		0x02
#define ADT7462_CONF3		0x03
#define ADT7462_TACH_EN		0x07
#define ADT7462_TACH_CFG	0x08
#define ADT7462_GPIO1_CFG	0x09
#define ADT7462_GPIO2_CFG	0x0a
#define ADT7462_TMIN_CAL1	0x0b
#define ADT7462_TMIN_CAL2	0x0c
#define ADT7462_THERM_CONF	0x0d
#define ADT7462_CONF_THERM1	0x0e
#define ADT7462_CONF_THERM2	0x0f
#define ADT7462_PIN_CONF1	0x10
#define ADT7462_PIN_CONF2	0x11
#define ADT7462_PIN_CONF3	0x12
#define ADT7462_PIN_CONF4	0x13
#define ADT7462_EASY_CONF	0x14
#define ADT7462_EDO_ENABLE	0x16
#define ADT7462_ATTEN1_EN	0x18
#define ADT7462_ATTEN2_EN	0x19
#define ADT7462_ACCOUSTICS1	0x1a
#define ADT7462_ACCOUSTICS2	0x1b
#define ADT7462_FAN_TEST	0x1c
#define ADT7462_FANS_PRESENT	0x1d
#define ADT7462_FAN_TEST_EN	0x1e
#define ADT7462_PWM1_CFG	0x21
#define ADT7462_PWM2_CFG	0x22
#define ADT7462_PWM3_CFG	0x23
#define ADT7462_PWM4_CFG	0x24
#define ADT7462_PWM12_FREQ	0x25
#define ADT7462_PWM34_FREQ	0x26
#define ADT7462_PWM1_MIN	0x28	/* Minimum PWM1 duty cycle */
#define ADT7462_PWM2_MIN	0x29	/* Minimum PWM2 duty cycle */
#define ADT7462_PWM3_MIN	0x2a	/* Minimum PWM3 duty cycle */
#define ADT7462_PWM4_MIN	0x2b	/* Minimum PWM4 duty cycle */
#define ADT7462_PWM1234_MAX	0x2c	/* Maximum all PWM duty cycle */
#define ADT7462_THERM_MASK1	0x30
#define ADT7462_THERM_MASK2	0x31
#define ADT7462_VOLT_MASK1	0x32
#define ADT7462_VOLT_MASK2	0x33
#define ADT7462_FAN_MASK	0x34
#define ADT7462_DIG_MASK	0x35
#define ADT7462_GPIO_MASK	0x36
#define ADT7462_EDO_MASK1	0x37
#define ADT7462_EDO_MASK2	0x38
#define ADT7462_DEV_ID		0x3d
#define ADT7462_COMP_ID		0x3e
#define ADT7462_REV_ID		0x3f
#define ADT7462_LOCAL_LOW	0x44
#define ADT7462_REM1_LOW	0x45	/* Register contents depend on */
#define ADT7462_PIN15V_LOW	0x45	/* pin configuration. */
#define ADT7462_REM2_LOW	0x46
#define ADT7462_REM3_LOW	0x47
#define ADT7462_PIN19V_LOW	0x47
#define ADT7462_LOCAL_HIGH	0x48
#define ADT7462_REM1_HIGH	0x49
#define ADT7462_PIN15V_HIGH	0x49
#define ADT7462_REM2_HIGH	0x4a
#define ADT7462_REM3_HIGH	0x4b
#define ADT7462_PIN19V_HIGH	0x4b
#define ADT7462_LOCAL_THERM1	0x4c
#define ADT7462_15V2_HIGH	0x4c
#define ADT7462_REM1_THERM1	0x4d
#define ADT7462_REM2_THERM1	0x4e
#define ADT7462_REM3_THERM1	0x4f
#define ADT7462_LOCAL_THERM2	0x50
#define ADT7462_15V1_HIGH	0x50
#define ADT7462_REM1_THERM2	0x51
#define ADT7462_REM2_THERM2	0x52
#define ADT7462_REM3_THERM2	0x53
#define ADT7462_LOCREM1_HYST	0x54
#define ADT7462_REM23_HYST	0x55
#define ADT7462_LOCAL_OFF	0x56
#define ADT7462_REM1_OFF	0x57
#define ADT7462_REM2_OFF	0x58
#define ADT7462_REM3_OFF	0x59
#define ADT7462_REM1_OPP	0x5a
#define ADT7462_REM2_OPP	0x5b
#define ADT7462_LOCAL_MIN	0x5c
#define ADT7462_REM1_MIN	0x5d
#define ADT7462_REM2_MIN	0x5e
#define ADT7462_REM3_MIN	0x5f
#define ADT7462_LOCAL_RANGE	0x60
#define ADT7462_REM1_RANGE	0x61
#define ADT7462_REM2_RANGE	0x62
#define ADT7462_REM3_RANGE	0x63
#define ADT7462_OPP_HYST	0x64
#define ADT7462_33V_HIGH	0x68
#define ADT7462_PIN23V_HIGH	0x69
#define ADT7462_PIN24V_HIGH	0x6a
#define ADT7462_PIN25V_HIGH	0x6b
#define ADT7462_PIN26V_HIGH	0x6c
#define ADT7462_12V1_LOW	0x6d
#define ADT7462_12V2_LOW	0x6e
#define ADT7462_12V3_LOW	0x6f
#define ADT7462_33V_LOW		0x70
#define ADT7462_5V_LOW		0x71
#define ADT7462_PIN23V_LOW	0x72
#define ADT7462_PIN24V_LOW	0x73
#define ADT7462_PIN25V_LOW	0x74
#define ADT7462_PIN26V_LOW	0x75
#define ADT7462_15V1_LOW	0x76
#define ADT7462_15V2_LOW	0x77
#define ADT7462_TACH1_LIMIT	0x78
#define ADT7462_VID_LIMIT	0x78
#define ADT7462_TACH2_LIMIT	0x79
#define ADT7462_TACH3_LIMIT	0x7a
#define ADT7462_TACH4_LIMIT	0x7b
#define ADT7462_TACH5_LIMIT	0x7c
#define ADT7462_12V1_HIGH	0x7c
#define ADT7462_TACH6_LIMIT	0x7d
#define ADT7462_12V2_HIGH	0x7d
#define ADT7462_TACH7_LIMIT	0x7e
#define ADT7462_5V_HIGH		0x7e
#define ADT7462_TACH8_LIMIT	0x7f
#define ADT7462_12V3_HIGH	0x7f
#define ADT7462_THERM1_TIMELIM	0x80
#define ADT7462_THERM2_TIMELIM	0x81
#define ADT7462_LOCAL_VAL_LSB	0x88
#define ADT7462_LOCAL_VAL_MSB	0x89
#define ADT7462_REM1_VAL_LSB	0x8a
#define ADT7462_REM1_VAL_MSB	0x8b
#define ADT7462_PIN15V_VAL	0x8b
#define ADT7462_REM2_VAL_LSB	0x8c
#define ADT7462_REM2_VAL_MSB	0x8d
#define ADT7462_REM3_VAL_LSB	0x8e
#define ADT7462_REM3_VAL_MSB	0x8f
#define ADT7462_PIN19V_VAL	0x8f
#define ADT7462_PIN23V_VAL	0x90
#define ADT7462_PIN24V_VAL	0x91
#define ADT7462_PIN25V_VAL	0x92
#define ADT7462_PIN26V_VAL	0x93
#define ADT7462_15V1_VAL	0x94
#define ADT7462_15V2_VAL	0x95
#define ADT7462_33V_VAL		0x96
#define ADT7462_VID_VAL		0x97
#define ADT7462_TACH1_VAL_LSB	0x98
#define ADT7462_TACH1_VAL_MSB	0x99
#define ADT7462_TACH2_VAL_LSB	0x9a
#define ADT7462_TACH2_VAL_MSB	0x9b
#define ADT7462_TACH3_VAL_LSB	0x9c
#define ADT7462_TACH3_VAL_MSB	0x9d
#define ADT7462_TACH4_VAL_LSB	0x9e
#define ADT7462_TACH4_VAL_MSB	0x9f
#define ADT7462_TACH5_VAL_LSB	0xa2
#define ADT7462_TACH5_VAL_MSB	0xa3
#define ADT7462_12V1_VAL	0xa3
#define ADT7462_TACH6_VAL_LSB	0xa4
#define ADT7462_TACH6_VAL_MSB	0xa5
#define ADT7462_12V2_VAL	0xa5
#define ADT7462_TACH7_VAL_LSB	0xa6
#define ADT7462_TACH7_VAL_MSB	0xa7
#define ADT7462_5V_VAL		0xa7
#define ADT7462_TACH8_VAL_LSB	0xa8
#define ADT7462_TACH8_VAL_MSB	0xa9
#define ADT7462_12V3_VAL	0xa9
#define ADT7462_PWM1_DUTY	0xaa
#define ADT7462_PWM2_DUTY	0xab
#define ADT7462_PWM3_DUTY	0xac
#define ADT7462_PWM4_DUTY	0xad
#define ADT7462_THERM1_ON_TIM	0xae
#define ADT7462_THERM2_ON_TIM	0xaf
#define ADT7462_TEMP1_STAT_H	0xb8
#define ADT7462_TEMP2_STAT_H	0xb9
#define ADT7462_TEMP3_STAT_H	0xba
#define ADT7462_VOLT1_STAT_H	0xbb
#define ADT7462_VOLT2_STAT_H	0xbc
#define ADT7462_FAN_STAT_H	0xbd
#define ADT7462_DIG_STAT_H	0xbe
#define ADT7462_GPIO_STAT_H	0xbf
#define ADT7462_TEMP1_STAT_B	0xc0
#define ADT7462_TEMP2_STAT_B	0xc1
#define ADT7462_VOLT1_STAT_B	0xc3
#define ADT7462_VOLT2_STAT_B	0xc4
#define ADT7462_FAN_STAT_B	0xc5
#define ADT7462_DIG_STAT_B	0xc6

/* 0x00: Configuration register 0 */
#define ADT7462_CONF0_BLK_MASK	0x3f	/* # registers for block read */
#define ADT7462_CONF0_VID_SPEC	0x40	/* 0 = VR10, 1 = VR11 */
#define ADT7462_CONF0_RESET	0x80	/* Reset all unlocked registers */

/* 0x01: Configuration register 1 */
#define ADT7462_CONF1_MONITOR	0x01	/* Start monitoring temps and volts */
#define ADT7462_CONF1_ALERT	0x08	/* 0 = SMB, 1 = comparator */
#define ADT7462_CONF1_FAST_SPIN	0x10	/* 1 = no fast spin-up */
#define ADT7462_CONF1_COMPLETE	0x20	/* Setup complete, start monitoring */
#define ADT7462_CONF1_LOCK	0x40	/* Lock limit registers */
#define ADT7462_CONF1_READY	0x80	/* Ready to start monitoring */

/* 0x02: Configuration register 2 */
#define ADT7462_CONF2_FAN_HF	0x01	/* Fast fan measurements */
#define ADT7462_CONF2_PWM_HF	0x04	/* High frequency PWM measurements */
#define ADT7462_CONF2_VRD1	0x08	/* Full speed fans on VRD1 */
#define ADT7462_CONF2_VRD2	0x10	/* Full speed fans on VRD2 */
#define ADT7462_CONF2_FFULL	0x20	/* Full speed fans */
#define ADT7462_CONF2_TACH	0xc0	/* Tach pulse to measure */
#define ADT7462_CONF2_TACH1	0x00	/*   1 */
#define ADT7462_CONF2_TACH2	0x40	/*   2 */
#define ADT7462_CONF2_TACH3	0x80	/*   3 */
#define ADT7462_CONF2_TACH4	0xc0	/*   4 */

/* 0x03: Configuration register 3 */
#define ADT7462_CONF3_GPIO_EN	0x01	/* Enable GPIO's */
#define ADT7462_CONF3_SCL_TO	0x02	/* Enable SCL timeout */
#define ADT7462_CONF3_SDA_TO	0x04	/* Enable SDA timeout */
#define ADT7462_CONF3_VID_LVL	0x08	/* Set low VID threshold */
#define ADT7462_CONF3_THERM_LVL	0x10	/* Set low therm threshold */
#define ADT7462_CONF3_CI_RESET	0x20	/* Reset chassis intrusion circuit */
#define ADT7462_CONF3_XOR_TEST	0x40	/* Enable XOR tree test */
#define ADT7462_CONF3_VCORE_LOW	0x80	/* Set V_core_low */

/* 0x07: Tach enable register */
#define ADT7462_TACH1_ENABLE	0x01	/* Enable tach1 measurement */
#define ADT7462_TACH2_ENABLE	0x02	/* Enable tach2 measurement */
#define ADT7462_TACH3_ENABLE	0x04	/* Enable tach3 measurement */
#define ADT7462_TACH4_ENABLE	0x08	/* Enable tach4 measurement */
#define ADT7462_TACH5_ENABLE	0x10	/* Enable tach5 measurement */
#define ADT7462_TACH6_ENABLE	0x20	/* Enable tach6 measurement */
#define ADT7462_TACH7_ENABLE	0x40	/* Enable tach7 measurement */
#define ADT7462_TACH8_ENABLE	0x80	/* Enable tach8 measurement */

#define ADT7462_FAN_TACH_EN(val, x)	((val & (1 << x)) != 0)

/* 0x08: Tach configuration register */
#define ADT7462_TACH15_CONT	0x01	/* Continuous tach1+5 measurement */
#define ADT7462_TACH26_CONT	0x02	/* Continuous tach2+6 measurement */
#define ADT7462_TACH37_CONT	0x04	/* Continuous tach3+7 measurement */
#define ADT7462_TACH48_CONT	0x08	/* Continuous tach4+8 measurement */

/* 0x09: GPIO configuration register 1 */
#define ADT7462_GPIO1_POL	0x01	/* GPIO1 polarity (0 low, 1 high) */
#define ADT7462_GPIO1_DIR	0x02	/* GPIO1 direction (0 in, 1 out) */
#define ADT7462_GPIO2_POL	0x04	/* GPIO2 polarity (0 low, 1 high) */
#define ADT7462_GPIO2_DIR	0x08	/* GPIO2 direction (0 in, 1 out) */
#define ADT7462_GPIO3_POL	0x10	/* GPIO3 polarity (0 low, 1 high) */
#define ADT7462_GPIO3_DIR	0x20	/* GPIO3 direction (0 in, 1 out) */
#define ADT7462_GPIO4_POL	0x40	/* GPIO4 polarity (0 low, 1 high) */
#define ADT7462_GPIO4_DIR	0x80	/* GPIO4 direction (0 in, 1 out) */

/* 0x0a: GPIO configuration register 2 */
#define ADT7462_GPIO5_POL	0x01	/* GPIO5 polarity (0 low, 1 high) */
#define ADT7462_GPIO5_DIR	0x02	/* GPIO5 direction (0 in, 1 out) */
#define ADT7462_GPIO6_POL	0x04	/* GPIO6 polarity (0 low, 1 high) */
#define ADT7462_GPIO6_DIR	0x08	/* GPIO6 direction (0 in, 1 out) */
#define ADT7462_GPIO7_POL	0x10	/* GPIO7 polarity (0 low, 1 high) */
#define ADT7462_GPIO7_DIR	0x20	/* GPIO7 direction (0 in, 1 out) */
#define ADT7462_GPIO8_POL	0x40	/* GPIO8 polarity (0 low, 1 high) */
#define ADT7462_GPIO8_DIR	0x80	/* GPIO8 direction (0 in, 1 out) */

/* 0x0b: Dynamix Tmin control register 1 */
#define ADT7462_REM1_EN		0x01	/* Enable dynamic rem 1 Tmin control */
#define ADT7462_REM2_EN		0x02	/* Enable dynamic rem 2 Tmin control */
#define ADT7462_P1_R1		0x04	/* Copy rem 1 val to op if therm1 */
#define ADT7462_P1_R2		0x08	/* Copy rem 2 val to op if therm1 */
#define ADT7462_P2_R1		0x10	/* Copy rem 1 val to op if therm2 */
#define ADT7462_P2_R2		0x20	/* Copy rem 2 val to op if therm2 */

/* 0x0c: Dynamic Tmin control register 2 */
#define ADT7462_REM1_CYCLE	0x07	/* Rem 1 cycle mask */
#define ADT7462_REM2_CYCLE	0x38	/* Rem 2 cycle mask */
#define ADT7462_CTRL_LOOP	0x40	/* Control loop select */

/* 0x0d: Therm configuration register */
#define ADT7462_BOOST1		0x01	/* Max fan PWM if therm1 */
#define ADT7462_BOOST2		0x02	/* Max fan PWM if therm2 */
#define ADT7462_THERM1_TIMER	0x1c	/* Therm1 timer window mask */
#define ADT7462_THERM2_TIMER	0xe0	/* Therm2 timer window mask */

/* 0x0e: Therm1 configuration register */
#define ADT7462_THERM1_TIM_EN	0x01	/* Enable therm1 timer circuit */
#define ADT7462_THERM1_LOCAL	0x02	/* Therm1 assert on local */
#define ADT7462_THERM1_REM1	0x04	/* Therm1 assert on remote1 */
#define ADT7462_THERM1_REM2	0x08	/* Therm1 assert on remote2 */
#define ADT7462_THERM1_REM3	0x10	/* Therm1 assert on remote3 */

/* 0x0f: Therm2 configuration register */
#define ADT7462_THERM2_TIM_EN	0x01	/* Enable therm2 timer circuit */
#define ADT7462_THERM2_LOCAL	0x02	/* Therm2 assert on local */
#define ADT7462_THERM2_REM1	0x04	/* Therm2 assert on remote1 */
#define ADT7462_THERM2_REM2	0x08	/* Therm2 assert on remote2 */
#define ADT7462_THERM2_REM3	0x10	/* Therm2 assert on remote3 */

/* 0x10: Pin configuration register 1 */
#define ADT7462_PIN7_CONF	0x01	/* 0 = +12v1, 1 = Tach5 */
#define ADT7462_PIN4_CONF	0x02	/* 0 = GPIO4, 1 = Tach4 */
#define ADT7462_PIN3_CONF	0x04	/* 0 = GPIO3, 1 = Tach3 */
#define ADT7462_PIN2_CONF	0x08	/* 0 = GPIO2, 1 = Tach2 */
#define ADT7462_PIN1_CONF	0x10	/* 0 = GPIO1, 1 = Tach1 */
#define ADT7462_DIODE3_CONF	0x20	/* 0 = volt/SCSI, 1 = D3+/D3- */
#define ADT7462_DIODE1_CONF	0x40	/* 0 = volt/SCSI, 1 = D1+/D1- */
#define ADT7462_VID_EN		0x80	/* 1 = En. VID's pins 1-4 28 31 32 */

#define ADT7462_PCR1_TACH(val, x)	(x > 4 || \
    (x == 4 && (val & ADT7462_PIN7_CONF)) ||  \
    (!(val & ADT7462_VID_EN) && (val & (1 << (4 - x)))))
#define ADT7462_PCR1_TEMP(val, x)	(x== 0 || x == 2 || \
    (x == 3 && (val & ADT7462_DIODE3_CONF)) || \
    (x == 1 && (val & ADT7462_DIODE1_CONF)))
#define ADT7462_PCR1_PIN7_V(val)	(!(val & ADT7462_PIN7_CONF))
#define ADT7462_PCR1_PIN19_V(val)	(!(val & ADT7462_DIODE3_CONF))
#define ADT7462_PCR1_PIN15_V(val)	(!(val & ADT7462_DIODE1_CONF))
#define ADT7462_PCR1_VIDS(val)		((val & ADT7462_VID_EN) != 0)

/* 0x11: Pin configuration register 2 */
#define ADT7462_PIN23_CONF	0x03	/* 00 = Vcc, 2.5v, 1.8v, 11 = 1.5v */
#define ADT7462_PIN22_CONF	0x04	/* 0 = 12v3, 1 = Tach8 */
#define ADT7462_PIN21_CONF	0x08	/* 0 = 5v, 1 = Tach7 */
#define ADT7462_PIN19_CONF	0x10	/* 0 = 1.25v, 1 = 0.9v */
#define ADT7462_PIN15_CONF	0x20	/* 0 = 2.5v, 1 = 1.8v */
#define ADT7462_PIN13_CONF	0x40	/* 0 = 3.3v, 1 = PWM4 */
#define ADT7462_PIN8_CONF	0x80	/* 0 = 12v2, 1 = Tach6 */

#define ADT7462_PCR2_P23_25V(val)	((val & ADT7462_PIN23_CONF) == 0x01)
#define ADT7462_PCR2_P23_18V(val)	((val & ADT7462_PIN23_CONF) == 0x02)
#define ADT7462_PCR2_P23_15V(val)	((val & ADT7462_PIN23_CONF) == 0x03)
#define ADT7462_PCR2_P22_TACH8(val)	((val & ADT7462_PIN22_CONF) == 0x04)
#define ADT7462_PCR2_P21_TACH7(val)	((val & ADT7462_PIN21_CONF) == 0x08)
#define ADT7462_PCR2_P19_09V(val)	((val & ADT7462_PIN19_CONF) == 0x10)
#define ADT7462_PCR2_P15_18V(val)	((val & ADT7462_PIN15_CONF) == 0x20)
#define ADT7462_PCR2_P13_PWM4(val)	((val & ADT7462_PIN13_CONF) == 0x40)
#define ADT7462_PCR2_P8_TACH6(val)	((val & ADT7462_PIN8_CONF) == 0x80)
#define ADT7462_PCR2_TACH(val, x)	(x < 5 || \
    (x == 5 && ADT7462_PCR2_P8_TACH6(val)) || \
    (x == 6 && ADT7462_PCR2_P21_TACH7(val)) || \
    (x == 7 && ADT7462_PCR2_P22_TACH8(val)))

/* 0x12: Pin configuration register 3 */
#define ADT7462_PIN27_CONF	0x02	/* 0 = fan2max, 1 = chassis intrus */
#define ADT7462_PIN26_CONF	0x0c	/* 00 = Vbatt, 1.2v2, 10/11=vr_hot2 */
#define ADT7462_PIN25_CONF	0x30	/* 00 = 3.3v, 1.2v1, 10/11=vr_hot1 */
#define ADT7462_PIN24_CONF	0xc0	/* 00 = Vccp, 2.5v, 1.8v, 11 = 1.5v */

#define ADT7462_PCR3_P24_25V(val)	((val & ADT7462_PIN24_CONF) == 0x40)
#define ADT7462_PCR3_P24_18V(val)	((val & ADT7462_PIN24_CONF) == 0x80)
#define ADT7462_PCR3_P24_15V(val)	((val & ADT7462_PIN24_CONF) == 0xc0)
#define ADT7462_PCR3_P25_33V(val)	((val & ADT7462_PIN25_CONF) == 0x00)
#define ADT7462_PCR3_P25_12V(val)	((val & ADT7462_PIN25_CONF) == 0x10)
#define ADT7462_PCR3_P26_VBAT(val)	((val & ADT7462_PIN26_CONF) == 0x00)
#define ADT7462_PCR3_P26_12V(val)	((val & ADT7462_PIN26_CONF) == 0x04)

/* 0x13: Pin configuration register 4 */
#define ADT7462_PIN32_CONF	0x04	/* 0 = GPIO6, 1 = PWM2 */
#define ADT7462_PIN31_CONF	0x08	/* 0 = GPIO5, 1 = PWM1 */
#define ADT7462_PIN29_CONF	0x30	/* 00 = GPIO8, 1.5v, 10/11 = therm2 */
#define ADT7462_PIN28_CONF	0xc0	/* 00 = GPIO7, 1.5v, 10/11 = therm1 */

#define ADT7462_PCR4_P29_15V(val)	((val & ADT7462_PIN29_CONF) == 0x10)
#define ADT7462_PCR4_P28_15V(val)	((val & ADT7462_PIN28_CONF) == 0x10)

/* 0x14: Easy configuration options */
#define ADT7462_EASY1		0x01	/* Enable easy option 1 */
#define ADT7462_EASY2		0x02	/* Enable easy option 2 */
#define ADT7462_EASY3		0x04	/* Enable easy option 3 */
#define ADT7462_EASY4		0x08	/* Enable easy option 4 */

/* 0x16: EDO/Single channel enable */
#define ADT7462_EDO_EN1		0x01	/* Enable EDO on GPIO5 */
#define ADT7462_EDO_EN2		0x02	/* Enable EDO on GPIO6 */
#define ADT7462_1CHAN_MODE	0x04	/* Select single-channel mode */
#define ADT7462_CHAN_SEL	0xf8	/* Select channel mask */

/* 0x18: Voltage attenuator configuration 1 */
#define ADT7462_ATT_PIN7	0x02	/* Enable attenuator on pin7 */
#define ADT7462_ATT_PIN8	0x04	/* Enable attenuator on pin8 */
#define ADT7462_ATT_PIN13	0x08	/* Enable attenuator on pin13 */
#define ADT7462_ATT_PIN15	0x10	/* Enable attenuator on pin15 */
#define ADT7462_ATT_PIN19	0x20	/* Enable attenuator on pin19 */
#define ADT7462_ATT_PIN21	0x40	/* Enable attenuator on pin21 */
#define ADT7462_ATT_PIN22	0x80	/* Enable attenuator on pin22 */

/* 0x19: Voltage attenuator configuration 2 */
#define ADT7462_ATT_PIN23	0x01	/* Enable attenuator on pin23 */
#define ADT7462_ATT_PIN24	0x02	/* Enable attenuator on pin24 */
#define ADT7462_ATT_PIN25	0x04	/* Enable attenuator on pin25 */
#define ADT7462_ATT_PIN28	0x10	/* Enable attenuator on pin28 */
#define ADT7462_ATT_PIN29	0x20	/* Enable attenuator on pin29 */

/* 0x1a: Enhanced acoustics register 1 */
#define ADT7462_AC_EN1		0x01	/* En. enhanced acoustics for PWM1 */
#define ADT7462_AC_EN2		0x02	/* En. enhanced acoustics for PWM2 */
#define ADT7462_AC_RATE1	0x1c	/* Ramp rate for PWM1 */
#define ADT7462_AC_RATE2	0xe0	/* Ramp rate for PWM2 */

/* 0x1b: Enhanced acoustics register 2 */
#define ADT7462_AC_EN3		0x01	/* En. enhanced acoustics for PWM3 */
#define ADT7462_AC_EN4		0x02	/* En. enhanced acoustics for PWM4 */
#define ADT7462_AC_RATE3	0x1c	/* Ramp rate for PWM3 */
#define ADT7462_AC_RATE4	0xe0	/* Ramp rate for PWM4 */

/* 0x1c: Fan freewheeling test */
#define ADT7462_FAN_FREE1_END	0x01	/* Freewheeling for fan1 complete */
#define ADT7462_FAN_FREE2_END	0x02	/* Freewheeling for fan2 complete */
#define ADT7462_FAN_FREE3_END	0x04	/* Freewheeling for fan3 complete */
#define ADT7462_FAN_FREE4_END	0x08	/* Freewheeling for fan4 complete */
#define ADT7462_FAN_FREE5_END	0x10	/* Freewheeling for fan5 complete */
#define ADT7462_FAN_FREE6_END	0x20	/* Freewheeling for fan6 complete */
#define ADT7462_FAN_FREE7_END	0x40	/* Freewheeling for fan7 complete */
#define ADT7462_FAN_FREE8_END	0x80	/* Freewheeling for fan8 complete */

/* 0x1d: Fans present */
#define ADT7462_FAN1_PRESENT	0x01	/* Fan1 present */
#define ADT7462_FAN2_PRESENT	0x02	/* Fan2 present */
#define ADT7462_FAN3_PRESENT	0x04	/* Fan3 present */
#define ADT7462_FAN4_PRESENT	0x08	/* Fan4 present */
#define ADT7462_FAN5_PRESENT	0x10	/* Fan5 present */
#define ADT7462_FAN6_PRESENT	0x20	/* Fan6 present */
#define ADT7462_FAN7_PRESENT	0x40	/* Fan7 present */
#define ADT7462_FAN8_PRESENT	0x80	/* Fan8 present */

/* 0x1e: Fan freewheeling test */
#define ADT7462_FAN_FREE1	0x01	/* Start freewheeling test for fan1 */
#define ADT7462_FAN_FREE2	0x02	/* Start freewheeling test for fan2 */
#define ADT7462_FAN_FREE3	0x04	/* Start freewheeling test for fan3 */
#define ADT7462_FAN_FREE4	0x08	/* Start freewheeling test for fan4 */
#define ADT7462_FAN_FREE5	0x10	/* Start freewheeling test for fan5 */
#define ADT7462_FAN_FREE6	0x20	/* Start freewheeling test for fan6 */
#define ADT7462_FAN_FREE7	0x40	/* Start freewheeling test for fan7 */
#define ADT7462_FAN_FREE8	0x80	/* Start freewheeling test for fan8 */

/* 0x21 - 0x24 : PWM configuration registers */
#define ADT7462_PWM_CFG(x)	(0x21 + x)
#define ADT7462_PWM_SPIN_TIMEO	0x07	/* Fan startup and test timeout */
#define ADT7462_PWM_SLOW	0x08	/* Slow en. acoustics mode start*/
#define ADT7462_PWM_INV		0x10	/* PWM output 0 = low, 1 = high */
#define ADT7462_PWM_BHVR_MASK	0xe0	/* PWM control channel mask */
#define ADT7462_PWM_BHVR_SHFT	5	/* PWM control channel shift */
#define ADT7462_PWM_BHVR_LOCAL	0x00	/* PWM control = local */
#define ADT7462_PWM_BHVR_REM1	0x20	/* PWM control = remote 1 */
#define ADT7462_PWM_BHVR_REM2	0x40	/* PWM control = remote 2 */
#define ADT7462_PWM_BHVR_REM3	0x60	/* PWM control = remote 3 */
#define ADT7462_PWM_BHVR_OFF	0x80	/* PWM control = off */
#define ADT7462_PWM_BHVR_LR3	0xa0	/* PWM control = local + rem 3 */
#define ADT7462_PWM_BHVR_ALL	0xc0	/* PWM control = local + rem 1/2/3 */
#define ADT7462_PWM_BHVR_MAN	0xe0	/* PWM control = manual */

/* 0x25: PWM1, PWM2 frequency */
#define ADT7462_PWM1_TMIN	0x01	/* PWM1 is off or min at Tmin */
#define ADT7462_PWM2_TMIN	0x02	/* PWM2 is off or min at Tmin */
#define ADT7462_PWM1_LFREQ	0x1c	/* PWM1 low frequency mask */
#define ADT7462_PWM2_LFREQ	0xe0	/* PWM2 low frequency mask */

/* 0x26: PWM3, PWM4 frequency */
#define ADT7462_PWM3_TMIN	0x01	/* PWM3 is off or min at Tmin */
#define ADT7462_PWM4_TMIN	0x02	/* PWM4 is off or min at Tmin */
#define ADT7462_PWM3_LFREQ	0x1c	/* PWM3 low frequency mask */
#define ADT7462_PWM4_LFREQ	0xe0	/* PWM4 low frequency mask */

/* 0x30: Thermal mask register 1 */
#define ADT7462_MASK_LOCAL	0x02	/* Mask local temp. out of limit */
#define ADT7462_MASK_REMOTE1	0x04	/* Mask remote1 temp. out of limit */
#define ADT7462_MASK_REMOTE2	0x08	/* Mask remote2 temp. out of limit */
#define ADT7462_MASK_REMOTE3	0x10	/* Mask remote3 temp. out of limit */
#define ADT7462_MASK_DIODE1	0x20	/* Mask remote1 temp. error */
#define ADT7462_MASK_DIODE2	0x40	/* Mask remote2 temp. error */
#define ADT7462_MASK_DIODE3	0x80	/* Mask remote3 temp. error */

/* 0x31: Thermal mask register 2 */
#define ADT7462_MASK_TH1_PCT	0x01	/* Mask therm1 percent alert */
#define ADT7462_MASK_TH1_ASRT	0x02	/* Mask therm1 assert */
#define ADT7462_MASK_TH1_STAT	0x04	/* Mask therm1 state alert */
#define ADT7462_MASK_TH2_PCT	0x08	/* Mask therm2 percent alert */
#define ADT7462_MASK_TH2_ASRT	0x10	/* Mask therm2 assert */
#define ADT7462_MASK_TH2_STAT	0x20	/* Mask therm2 state alert */
#define ADT7462_MASK_VRD1	0x40	/* Mask VRD1 assert */
#define ADT7462_MASK_VRD2	0x80	/* Mask VRD2 assert */

/* 0x32: Voltage mask register 1 */
#define ADT7462_MASK_12V1	0x01	/* Mask 12V1 alert */
#define ADT7462_MASK_12V2	0x02	/* Mask 12V2 alert */
#define ADT7462_MASK_12V3	0x04	/* Mask 12V3 alert */
#define ADT7462_MASK_33V	0x08	/* Mask 3.3V alert */
#define ADT7462_MASK_PIN15V	0x10	/* Mask pin15v alert */
#define ADT7462_MASK_PIN19V	0x20	/* Mask pin19v alert */
#define ADT7462_MASK_5V		0x40	/* Mask 5V alert */
#define ADT7462_MASK_PIN23V	0x80	/* Mask pin23v alert */

/* 0x33: Voltage mask register 2 */
#define ADT7462_MASK_PIN24V	0x08	/* Mask pin24v alert */
#define ADT7462_MASK_PIN25V	0x10	/* Mask pin25v alert */
#define ADT7462_MASK_PIN26V	0x20	/* Mask pin26v alert */
#define ADT7462_MASK_15V2	0x40	/* Mask 1.5V2 alert */
#define ADT7462_MASK_15V1	0x80	/* Mask 1.5V1 alert */

/* 0x34: Fan mask register */
#define ADT7462_MASK_FAN1_FAULT	0x01	/* Mask fan1 fault */
#define ADT7462_MASK_FAN2_FAULT	0x02	/* Mask fan2 fault */
#define ADT7462_MASK_FAN3_FAULT	0x04	/* Mask fan3 fault */
#define ADT7462_MASK_FAN4_FAULT	0x08	/* Mask fan4 fault */
#define ADT7462_MASK_FAN5_FAULT	0x10	/* Mask fan5 fault */
#define ADT7462_MASK_FAN6_FAULT	0x20	/* Mask fan6 fault */
#define ADT7462_MASK_FAN7_FAULT	0x40	/* Mask fan7 fault */
#define ADT7462_MASK_FAN8_FAULT	0x80	/* Mask fan8 fault */

/* 0x35: Digital mask register */
#define ADT7462_MASK_FAN2MAX	0x08	/* Mask fan2max alert */
#define ADT7462_MASK_SCSI1	0x10	/* Mask SCSI1 alert */
#define ADT7462_MASK_SCSI2	0x20	/* Mask SCSI2 alert */
#define ADT7462_MASK_VID	0x40	/* Mask VID comparison alert */
#define ADT7462_MASK_CHASSIS	0x80	/* Mask chassis intrusion alert */

/* 0x36: GPIO mask register */
#define ADT7462_MASK_GPIO1_FLT	0x01	/* Mask GPIO1 fault */
#define ADT7462_MASK_GPIO2_FLT	0x02	/* Mask GPIO2 fault */
#define ADT7462_MASK_GPIO3_FLT	0x04	/* Mask GPIO3 fault */
#define ADT7462_MASK_GPIO4_FLT	0x08	/* Mask GPIO4 fault */
#define ADT7462_MASK_GPIO5_FLT	0x10	/* Mask GPIO5 fault */
#define ADT7462_MASK_GPIO6_FLT	0x20	/* Mask GPIO6 fault */
#define ADT7462_MASK_GPIO7_FLT	0x40	/* Mask GPIO7 fault */
#define ADT7462_MASK_GPIO8_FLT	0x80	/* Mask GPIO8 fault */

/* 0x36: EDO mask register 1 */
#define ADT7462_EDO1_GPIO1	0x01	/* Mask GPIO1 for EDO1 */
#define ADT7462_EDO1_GPIO2	0x02	/* Mask GPIO2 for EDO1 */
#define ADT7462_EDO1_GPIO3	0x04	/* Mask GPIO3 for EDO1 */
#define ADT7462_EDO1_GPIO4	0x08	/* Mask GPIO4 for EDO1 */
#define ADT7462_EDO1_FAN	0x20	/* Mask fan fault for EDO1 */
#define ADT7462_EDO1_TEMP	0x40	/* Mask therm for EDO1 */
#define ADT7462_EDO2_VOLT	0x80	/* Mask volt limit for EDO1 */

/* 0x36: EDO mask register 2 */
#define ADT7462_EDO2_GPIO1	0x01	/* Mask GPIO1 for EDO2 */
#define ADT7462_EDO2_GPIO2	0x02	/* Mask GPIO2 for EDO2 */
#define ADT7462_EDO2_GPIO3	0x04	/* Mask GPIO3 for EDO2 */
#define ADT7462_EDO2_GPIO4	0x08	/* Mask GPIO4 for EDO2 */
#define ADT7462_EDO2_FAN	0x20	/* Mask fan fault for EDO2 */
#define ADT7462_EDO2_TEMP	0x40	/* Mask therm for EDO2 */
#define ADT7462_EDO2_VOLT	0x80	/* Mask volt limit for EDO2 */

/* 0x3d: Device ID register */
#define ADT7462_DEV_ID_VAL	0x62	/* ADT7462 device ID */

/* 0x3e: Company ID register */
#define ADT7462_COMP_ID_VAL	0x41	/* ADT7462 company ID */

/* 0x3f: Revision ID register */
#define ADT7462_REV_ID_VAL	0x04	/* ADT7462 revision ID */

/* 0x44 - 0x53: Temperature limit registers: -64'C + value */
#define ADT7462_TEMP_BASE	209150000
#define ADT7462_TEMP_OFFSET	64
#define ADT7462_TEMP_LOW(x)	(0x44 + x)
#define ADT7462_TEMP_HIGH(x)	(0x48 + x)
#define ADT7462_TEMP_THERM1(x)	(0x4c + x)
#define ADT7462_TEMP_THERM2(x)	(0x50 + x)

/* 0x54 - Local/remote 1 temperature hysteresis */
#define ADT7462_REMOTE1_TH_HYST	0x0f	/* Remote 1 therm hyst. (0 - 15) */
#define ADT7462_LOCAL_TH_HYST	0xf0	/* Local therm hyst. (0 - 15) */

/* 0x55 - Remote 2/remote 3 temperature hysteresis */
#define ADT7462_REMOTE3_TH_HYST	0x0f	/* Remote 3 therm hyst. (0 - 15) */
#define ADT7462_REMOTE2_TH_HYST	0xf0	/* Remote 2 therm hyst. (0 - 15) */

/* 0x56 - 0x59: Offset registers: resolution = 0.5'C */

/* 0x5a - 0x5b: Operating point registers */
#define ADT7462_OP_POINT(x)	(0x59 + x)	/* Only remote 1 and 2 */

/* 0x5c - 0x5f: Tmin registers */
#define ADT7462_TMIN(x)		(0x5c + x)

/* 0x60 - 0x63: Trange/Hysteresis registers */
#define ADT7462_HYST_TRANGE(x)	(0x60 + x)
#define ADT7462_HTR_HYST_MASK	0x0f	/* Auto fan loop hyst. (0-15) */
#define ADT7462_HTR_RNGE_MASK	0xf0	/* Trange value (2 - 80) */
#define ADT7462_HTR_RNGE_SHFT	4	/* Trange shift */

/* 0x64: Operating point hysteresis */
#define ADT7462_OPT_HYST	0xf0	/* Tmin loop hyst. (0 - 15) */

/* 0x68 - 0x77: Voltage limit registers */

/* 0x78 - 0x7f: Tach limit registers */
#define ADT7462_TACH_LIMIT(x)	(0x78 + x)

/* 0x80 - 0x81: Therm timer limit registers */

/* 0x88 - 0x8f: Temperature value registers */
#define ADT7462_TEMP_LSB(x)	(0x88 + 2 * x)

/* 0x90 - 0x96: Voltage value registers */

/* 0x97: VID value register */

/* 0x98 - 0x9f, 0xa2 - 0xa9: Tach value registers */
#define ADT7462_TACH_VAL_LSB(x)	(x < 4 ? 0x98 + 2 * x : 0x9a + 2 * x)
#define ADT7462_TACH_PERIOD	(90000 * 60)

/* 0xaa - 0xad: PWM current duty cycle registers */
#define ADT7462_PWM_DUTY(x)	(0xaa + x)

/* 0xae - 0xaf: Therm time value registers */

/* 0xb8, 0xc0: Host/BMC thermal status register 1 */
#define ADT7462_LOCAL_TRIP	0x02
#define ADT7462_REMOTE1_TRIP	0x04
#define ADT7462_REMOTE2_TRIP	0x08
#define ADT7462_REMOTE3_TRIP	0x10
#define ADT7462_DIODE1_ERR	0x20
#define ADT7462_DIODE2_ERR	0x40
#define ADT7462_DIODE3_ERR	0x80

/* 0xb9, 0xc1: Host/BMC thermal status register 2 */
#define ADT7462_THERM1_PCT	0x01
#define ADT7462_THERM1_ASSERT	0x02
#define ADT7462_THERM1_STATE	0x04
#define ADT7462_THERM2_PCT	0x08
#define ADT7462_THERM2_ASSERT	0x10
#define ADT7462_THERM2_STATE	0x20
#define ADT7462_VRD1_ASSERT	0x40
#define ADT7462_VRD2_ASSERT	0x80

/* 0xba: Host thermal status register 3 */
#define ADT7462_THERM1_L_EXCD	0x01
#define ADT7462_THERM1_R1_EXCD	0x02
#define ADT7462_THERM1_R2_EXCD	0x04
#define ADT7462_THERM1_R3_EXCD	0x08
#define ADT7462_THERM2_L_EXCD	0x10
#define ADT7462_THERM2_R1_EXCD	0x20
#define ADT7462_THERM2_R2_EXCD	0x40
#define ADT7462_THERM2_R3_EXCD	0x80

/* 0xbb, 0xc3: Host/BMC voltage status register 1 */
#define ADT7462_12V1_TRIP	0x01
#define ADT7462_12V2_TRIP	0x02
#define ADT7462_12V3_TRIP	0x04
#define ADT7462_33V_TRIP	0x08
#define ADT7462_PIN15V_TRIP	0x10
#define ADT7462_PIN19V_TRIP	0x20
#define ADT7462_5V_TRIP		0x40
#define ADT7462_PIN23V_TRIP	0x80

/* 0xbc, 0xc4: Host/BMC voltage status register 2 */
#define ADT7462_PIN24V_TRIP	0x04
#define ADT7462_PIN25V_TRIP	0x10
#define ADT7462_PIN26V_TRIP	0x20
#define ADT7462_15V2_TRIP	0x40
#define ADT7462_15V1_TRIP	0x80

/* 0xbd, 0xc5: Host/BMC fan status register */
#define ADT7462_FAN1_FAULT	0x01
#define ADT7462_FAN2_FAULT	0x02
#define ADT7462_FAN3_FAULT	0x04
#define ADT7462_FAN4_FAULT	0x08
#define ADT7462_FAN5_FAULT	0x10
#define ADT7462_FAN6_FAULT	0x20
#define ADT7462_FAN7_FAULT	0x40
#define ADT7462_FAN8_FAULT	0x80

/* 0xbe, 0xc6: Host/BMC digital status register */
#define ADT7462_FAN2MAX_ASSERT	0x08
#define ADT7462_SCSI1_ASSERT	0x10
#define ADT7462_SCSI2_ASSERT	0x20
#define ADT7462_VID_COMP_FLT	0x40
#define ADT7462_CHASSIS_ASSERT	0x80

/* 0xbe, 0xc6: Host/BMC GPIO status register */
#define ADT7462_GPIO1_ASSERT	0x01
#define ADT7462_GPIO2_ASSERT	0x02
#define ADT7462_GPIO3_ASSERT	0x04
#define ADT7462_GPIO4_ASSERT	0x08
#define ADT7462_GPIO5_ASSERT	0x10
#define ADT7462_GPIO6_ASSERT	0x20
#define ADT7462_GPIO7_ASSERT	0x40
#define ADT7462_GPIO8_ASSERT	0x80

#endif /* _DEV_I2C_ADT7462REG_H_ */
