/* $NetBSD: pca9685reg.h,v 1.1 2019/07/24 05:25:32 thorpej Exp $ */

/*-
 * Copyright (c) 2018 Jason R. Thorpe
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

#ifndef pca9685reg_h_included
#define	pca9685reg_h_included

/*
 * Hardware definitions for the NXP PCA9685 16-channel, 12-bit PWM FM+
 * I2C-bus LED controller.
 */

#define	PCA9685_NCHANNELS	16

/*
 * PCA9685 i2c addresses.
 */
	/* Default all-call address. */
#define	PCA9685_I2CADDR_ALLCALL_DEFAULT	0x70

	/* Default sub-addresses. */
#define	PCA9685_I2CADDR_SUBADR1_DEFAULT	0x71
#define	PCA9685_I2CADDR_SUBADR2_DEFAULT	0x72
#define	PCA9685_I2CADDR_SUBADR3_DEFAULT	0x74

	/*
	 * The PCA9685 has 6 address select pins, which must be
	 * pulled up or down to select the device's address.  The
	 * selected slave address is:
	 *
	 *	1 A5 A4 A3 A2 A1 A0
	 *
	 * There are some addresses, however, that cannot be selected
	 * even if they fall into the selectable range:
	 *
	 * - Default all-call address (0x70)
	 * - 10-bit slave address prefix (0x78)
	 * - Reserved for future purposes address (0x7c)
	 *
	 * Use of sub-addresses to control groups of devices also limits
	 * the available individual slave addresses.
	 */
#define	PCA9685_I2CADDR_MIN		0x40
#define	PCA9685_I2CADDR_MAX		0x7f

#define	PCA9685_I2CADDR_BLACKLIST	{ 0x78, 0x7c }

/*
 * PCA9685 has an internal 25MHz oscillator.  The maximum external
 * reference clock frequency is 50MHz.
 */
#define	PCA9685_INTERNAL_FREQ		25000000
#define	PCA9685_EXTERNAL_FREQ_MAX	50000000

/*
 * Data sheet sez:
 *
 * Maximum PWM frequency is 1526Hz -- prescale == 0x03.
 * Minimum PWM frequency is 24Hz -- prescale == 0xff.
 */
#define	PCA9685_FREQ_MIN		24
#define	PCA9685_FREQ_MAX		1526

#define	PCA9685_PRESCALE_MIN		0x03
#define	PCA9685_PRESCALE_MAX		0xff

/*
 * The PCA9685 pulse counter has a 12-bit resolution (4096 ticks).
 */
#define	PCA9685_PWM_TICKS		4096

/*
 * PCA9685 register definitions.
 */

	/* Mode register 1 */
#define	PCA9685_MODE1		0x00
#define	MODE1_ALLCALL		0x01	/* responds to LED All Call address */
#define	MODE1_SUB3		0x02	/* responds to subaddress 3 */
#define	MODE1_SUB2		0x04	/* responds to subaddress 2 */
#define	MODE1_SUB1		0x08	/* responds to subaddress 1 */
#define	MODE1_SLEEP		0x10	/* 1 = low power mode, oscillator off */
#define	MODE1_AI		0x20	/* register auto-increment enabled */
#define	MODE1_EXTCLK		0x40	/* external oscillatior enable */
#define	MODE1_RESTART		0x80	/* restart enabled */

	/* Mode register 2 */
#define	PCA9685_MODE2		0x01
#define	MODE2_OUTNE_MASK	0x03	/* see data sheet */
#define	MODE2_OUTDRV		0x04	/* 0 = open-drain, 1 = totem pole */
#define	MODE2_OCH		0x08	/* outputs change: 0 = STOP, 1 = ACK */
#define	MODE2_INVRT		0x10	/* Output logic state inverted */

	/* I2C subaddress 1 */
#define	PCA9685_SUBADR1		0x02

	/* I2C subaddress 2 */
#define	PCA9685_SUBADR2		0x03

	/* I2C subaddress 3 */
#define	PCA9685_SUBADR3		0x04

	/* LED All Call address */
#define	PCA9685_ALLCALLADR	0x05

	/* LEDx output and brightness control byte 0 */
#define	PCA9685_LEDx_ON_L(x)	(0x06 + ((x) * 4))

	/* LEDx output and brightness control byte 1 */
#define	PCA9685_LEDx_ON_H(x)	(0x07 + ((x) * 4))

	/* LEDx output and brightness control byte 2 */
#define	PCA9685_LEDx_OFF_L(x)	(0x08 + ((x) * 4))

	/* LEDx output and brightness control byte 3 */
#define	PCA9685_LEDx_OFF_H(x)	(0x09 + ((x) * 4))

	/* Load all LEDx_ON registers byte 0 */
#define	PCA9685_ALL_LED_ON_L	0xfa

	/* Load all LEDx_ON registers byte 1 */
#define	PCA9685_ALL_LED_ON_H	0xfb

	/* Load all LEDx_OFF registers byte 0 */
#define	PCA9685_ALL_LED_OFF_L	0xfc

	/* Load all LEDx_OFF registers byte 1 */
#define	PCA9685_ALL_LED_OFF_H	0xfd

	/* Prescaler for PWM output frequency */
#define	PCA9685_PRE_SCALE	0xfe

	/* Test mode */
#define	PCA9685_TEST_MODE	0xff

#endif /* pca9685reg_h_included */
