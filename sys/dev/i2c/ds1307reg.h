/*	$NetBSD: ds1307reg.h,v 1.2.114.1 2012/02/18 07:34:13 mrg Exp $	*/

/*
 * Copyright (c) 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _DEV_I2C_DS1307REG_H_
#define _DEV_I2C_DS1307REG_H_

/*
 * DS1307 64x8 Serial Real-Time Clock
 */

#define	DS1307_ADDR		0x68	/* Fixed I2C Slave Address */

#define DS1307_SECONDS		0x00
#define DS1307_MINUTES		0x01
#define DS1307_HOURS		0x02
#define DS1307_DAY		0x03
#define DS1307_DATE		0x04
#define DS1307_MONTH		0x05
#define DS1307_YEAR		0x06
#define DS1307_CONTROL		0x07
#define	DS1307_NVRAM_START	0x08
#define	DS1307_NVRAM_END	0x3f

#define	DS1307_NRTC_REGS	7
#define	DS1307_NVRAM_SIZE	((DS1307_NVRAM_END - DS1307_NVRAM_START) + 1)

/*
 * Bit definitions.
 */
#define	DS1307_SECONDS_CH	(1u << 7)	/* Clock Hold */
#define	DS1307_SECONDS_MASK	0x7f
#define	DS1307_MINUTES_MASK	0x7f
#define	DS1307_HOURS_12HRS_MODE	(1u << 6)	/* Set for 12 hour mode */
#define	DS1307_HOURS_12HRS_PM	(1u << 5)	/* If 12 hr mode, set = PM */
#define	DS1307_HOURS_12MASK	0x1f
#define	DS1307_HOURS_24MASK	0x3f
#define	DS1307_DAY_MASK		0x07
#define	DS1307_DATE_MASK	0x3f
#define	DS1307_MONTH_MASK	0x1f
#define	DS1307_CONTROL_OUT	(1u << 7)	/* OSC/OUT pin value */
#define	DS1307_CONTROL_SQWE	(1u << 4)	/* Enable square wave output */
#define	DS1307_CONTROL_1HZ	0
#define	DS1307_CONTROL_4096HZ	1
#define	DS1307_CONTROL_8192HZ	2
#define	DS1307_CONTROL_32768HZ	3

#endif /* _DEV_I2C_DS1307REG_H_ */
