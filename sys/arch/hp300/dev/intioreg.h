/*	$NetBSD: intioreg.h,v 1.4.58.1 2008/06/02 13:22:06 mjf Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gregory McGarry.
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

/*
 *  Registers for interface to the wired devices.
 */

/* HP3xx/4xx registers */
#define INTIO_DEV_3xx_PAD0	0
#define INTIO_DEV_3xx_DATA	1
#define INTIO_DEV_3xx_PAD1	2
#define INTIO_DEV_3xx_CMD	3
#define INTIO_DEV_3xx_STAT	3

/* HP7xx registers - XXX check these */
#define INTIO_DEV_7xx_RSTHOLD	0
#define INTIO_DEV_7xx_DATA	2048
#define INTIO_DEV_7xx_CMD	2049
#define INTIO_DEV_7xx_STAT	2049
#define INTIO_DEV_7xx_RSTREL	3072

/* HP8xx registers - XXX check these */
#define INTIO_DEV_8xx_DATA	3
#define INTIO_DEV_8xx_CMD	11
#define INTIO_DEV_8xx_STAT	11

/* Status bits */
#define INTIO_DEV_DATA_READY	0x01
#define INTIO_DEV_BUSY		0x02
/* the top four bits specify a "service request" status code */
#define INTIO_DEV_SRSHIFT	4
#define INTIO_DEV_SRMASK	0x0f

/* Valid "service request" status codes */
#define INTIO_DEV_SR_DATAAVAIL	0x04
/* the specific wired device will overload the remaining bits */


/* Base address offsets from intiobase of each device */
#define FRODO_BASE	0x01c000
#define RTC_BASE	0x020000
#define HIL_BASE	0x028000
#define HPIB_BASE	0x078000
#define DMA_BASE	0x100000
#define FB_BASE		0x160000
