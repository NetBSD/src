/*	$NetBSD: mvsocrtcreg.h,v 1.1.6.2 2011/06/06 09:05:04 jruoho Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
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

#ifndef _MVSOCRTCREG_H_
#define _MVSOCRTCREG_H_

#define MVSOCRTC_SIZE			0x18

#define MVSOCRTC_TIME			0x00
#define   MVSOCRTC_SECOND_OFFSET	0
#define   MVSOCRTC_SECOND_MASK		0x7f
#define   MVSOCRTC_MINUTE_OFFSET	8
#define   MVSOCRTC_MINUTE_MASK		0x7f
#define   MVSOCRTC_HOUR_OFFSET		16
#define   MVSOCRTC_HOUR_MASK		0x3f
#define   MVSOCRTC_WDAY_OFFSET		24
#define   MVSOCRTC_WDAY_MASK		0x07

#define MVSOCRTC_DATE			0x04
#define   MVSOCRTC_DAY_OFFSET		0
#define   MVSOCRTC_DAY_MASK		0x3f
#define   MVSOCRTC_MONTH_OFFSET		8
#define   MVSOCRTC_MONTH_MASK		0x3f
#define   MVSOCRTC_YEAR_OFFSET		16
#define   MVSOCRTC_YEAR_MASK		0xff

#define MVSOCRTC_ALARMTIME		0x08
#define MVSOCRTC_ALARMDATE		0x0c
#define MVSOCRTC_INTMASK		0x10
#define MVSOCRTC_INTCAUSE		0x14

#endif /* ! _MVSOCRTCREG_H_ */
