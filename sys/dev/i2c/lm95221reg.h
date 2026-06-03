/* $NetBSD: lm95221reg.h,v 1.1 2026/06/03 11:07:01 jdc Exp $ */

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

#ifndef _DEV_I2C_LM95221REG_H_
#define _DEV_I2C_LM95221REG_H_

/* LM95221 temperature sensor register definitions */

#define	LM95221_ADDR		0x2b

/*
 * Temperature on the LM95221 is represented by a 10-bit (local) and 11-bit
 * (remote) 2's complement word or 11-bit (remote) unsigned binary word
 * with an LSB of 0.125C.  The data is left-justified in 2 * 8-bit registers.
 *
 * 11-bit 2's complement:
 *
 *	+125C	0111 1101 0000 0000	0x7d00
 *	+25C	0001 1001 0000 0000	0x1900
 *	+1C	0000 0001 0000 0000	0x0100
 *      +0.125C	0000 0000 0010 0000	0x0020
 *	 0C	0000 0000 0000 0000	0x0000
 *      -0.125C	1111 1111 1110 0000	0xffe0
 *	-1C	1111 1111 0000 0000	0xff00
 *	-25C	1110 0111 0000 0000	0xe700
 *	-55C	1100 1001 0000 0000	0xc900
 *
 * 11-bit unsigned binary:
 *
 *    +255.875C	1111 1111 1110 0000	0xffe0
 *	+255C	1111 1111 0000 0000	0xff90
 *	+201C	1100 1001 0000 0000	0xc900
 *	+125C	0111 1101 0000 0000	0x7d00
 *	+25C	0001 1001 0000 0000	0x1900
 *	+1C	0000 0001 0000 0000	0x0100
 *      +0.125C	0000 0000 0010 0000	0x0020
 *	 0C	0000 0000 0000 0000	0x0000
 *
 * 10-bit 2's complement is similar to 11-bit 2's complement
 * (but with 0.25C resolution)
 */

/* Command bits 0-2 select the register */
#define	LM95221_STATUS		0x02	/* Status */
#define	LM95221_CONFIG		0x03	/* Configuration */
#define	LM95221_1SHOT		0x0f	/* One conversion when in standby */
#define	LM95221_LOCAL_MSB	0x10	/* Reading MSB locks LSB 0x20 */
#define	LM95221_REMOTE1_MSB	0x11	/* Reading MSB locks LSB 0x21 */
#define	LM95221_REMOTE2_MSB	0x12	/* Reading MSB locks LSB 0x22 */
#define	LM95221_LOCAL_LSB	0x20	/* Reading LSB unlocks it */
#define	LM95221_REMOTE1_LSB	0x21	/* Reading LSB unlocks it */
#define	LM95221_REMOTE2_LSB	0x22	/* Reading LSB unlocks it */
#define	LM95221_MANUF		0xfe	/* Manufacturer ID (0x01) */
#define	LM95221_REVISION	0xff	/* Manufacturer ID (0x61) */

/* Status register */
#define LM95221_STAT_NO_REMOTE1	0x01	/* Remote 1 diode missing */
#define LM95221_STAT_NO_REMOTE2	0x02	/* Remote 1 diode missing */
#define LM95221_STAT_BUSY	0x80	/* Currently converting */

/* Configuration register */
#define LM95221_CONF_REMOTE1	0x02	/* Remote 1 signed format */
#define LM95221_CONF_REMOTE2	0x04	/* Remote 2 signed format */
#define LM95221_CONF_CONVRATE	0x30	/* Conversion rate */
#define LM95221_CONF_STANDBY	0x40	/* Disable conversions */
#define LM95221_CONVRATE_CONT	0x00	/* Continuous every 66ms */
#define LM95221_CONVRATE_200	0x10	/* Every 200ms */
#define LM95221_CONVRATE_1000	0x20	/* Every 1s */
#define LM95221_CONVRATE_3000	0x30	/* Every 3s */

/* Temperature registers */
#define LM95221_NSENSORS	3

#endif /* _DEV_I2C_LM95221REG_H_ */
