/*	$NetBSD: x1226reg.h,v 1.1 2003/09/23 14:45:15 shige Exp $	*/

/*
 * Copyright 2003 Shigeyuki Fukushima.
 * All rights reserved.
 *
 * Written by Shigeyuki Fukushima.
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

#ifndef __X1226REG_H__
#define __X1226REG_H__

/* XICOR X1226 Device Identifier */
#define X1226_DEVID_CCR		0x6f
#define X1226_DEVID_EEPROM	0x57

/* XICOR X1226 Watchdog RTC registers */
#define	X1226_SR		0x3f
#define	X1226_RTC_Y2K		0x37	/* century (19/20) */
#define	X1226_RTC_DW		0x36	/* day of week (0-6) */
#define	X1226_RTC_YR		0x35	/* year (0-99) */
#define	X1226_RTC_MO		0x34	/* month (1-12) */
#define	X1226_RTC_DT		0x33	/* day (1-31) */
#define	X1226_RTC_HR		0x32	/* hour (0-23) */
#define	X1226_RTC_MN		0x31	/* minute (0-59) */
#define	X1226_RTC_SC		0x30	/* second (0-59) */
#define	X1226_RTC_BASE		0x30
#define	X1226_CTRL_DTR		0x13
#define	X1226_CTRL_ATR		0x12
#define	X1226_CTRL_INT		0x11
#define	X1226_CTRL_BL		0x10

/* XICOR X1226 RTC flags */
#define X1226_SR_RTCF		0x01
#define X1226_SR_WEL		0x02
#define X1226_SR_RWEL		0x04
#define X1226_SR_AL0		0x20
#define X1226_SR_AL1		0x40
#define X1226_SR_BAT		0x80
#define X1226_RTC_HR_H21	0x20
#define X1226_RTC_HR_MIL	0x80

/* XICOR X1226 size */
#define X1226_SIZE		0x0100	/* 4kbit (512 x 8bit) */

/* Y2K definitions */
#define	EPOCH			2000
#define	SYS_EPOCH		1900

#endif /* __X1226REG_H__ */
