/*	$NetBSD: ds1743reg.h,v 1.1.8.2 2002/04/01 07:43:35 nathanw Exp $	*/

/*
 * Copyright 2001-2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge and Eduardo Horvath for Wasabi Systems, Inc.
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

#ifndef __DS1501_H__
#define __DS1501_H__

/* Dallas Semiconductor DS1501/DS1511 Watchdog RTC */

#define DS_SECONDS	0x1ff9
#define DS_MINUTES	0x1ffa
#define DS_HOURS	0x1ffb
#define DS_DAY		0x1ffc
#define DS_DATE		0x1ffd
#define DS_MONTH	0x1ffe
#define DS_YEAR		0x1fff
#define DS_CENTURY	0x1ff8


#define DS_CTL_R	0x40	/* R bit in the century register */
#define DS_CTL_W	0x80	/* W bit in the century register */
#define DS_CTL_RW	(DS_CTL_R|DS_CTL_W)

#define DS_CTL_OSC	0x80	/* ~OSC BIT in the seconds register */

#define DS_CTL_BF	0x80	/* BF(battery failure) bit in the day register */
#define DS_CTL_FT	0x40	/* FT(frequency test) bit in the day register */

#define DS_RAM_SIZE	0x1ff8

#define DS_SIZE		0x2000

#endif /* __DS1501_H__ */
