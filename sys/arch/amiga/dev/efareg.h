/*	$NetBSD: efareg.h,v 1.2.12.1 2014/08/20 00:02:43 tls Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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

#ifndef _AMIGA_EFAREG_H_

#define FATA1_BASE		0xDA2000

/* Offsets. Stride of 4 is used, so multiply any offset by 4. */
#define FATA1_PIO0_OFF		0x0 
#define FATA1_PIO3_OFF		0x4000
#define FATA1_PIO4_OFF		0x5000
#define FATA1_PIO5_OFF		0x4800

#define FATA1_CHAN_SIZE		0x400
#define FATA1_REGS_SIZE		0x4BC0

/* PIO0 */
#define FATA1_PIO0_OFF_DATA	0x0
#define FATA1_PIO0_OFF_ERROR	0x1
#define FATA1_PIO0_OFF_SECCNT	0x2
#define FATA1_PIO0_OFF_SECTOR	0x3
#define FATA1_PIO0_OFF_CYL_LO	0x4
#define FATA1_PIO0_OFF_CYL_HI	0x5
#define FATA1_PIO0_OFF_SDH	0x6
#define FATA1_PIO0_OFF_COMMAND	0x7

/* PIO3-5 */
#define FATA1_PION_OFF_DATA	0x82	/* 16-bit data port */
#define FATA1_PION_OFF_DATA32	0x0	/* 32-bit data port, 2 cycles to HD */
#define FATA1_PION_OFF_ERROR	0x80
#define FATA1_PION_OFF_SECCNT	0x100
#define FATA1_PION_OFF_SECTOR	0x180
#define FATA1_PION_OFF_CYL_LO	0x200
#define FATA1_PION_OFF_CYL_HI	0x280
#define FATA1_PION_OFF_SDH	0x300
#define FATA1_PION_OFF_COMMAND	0x380

#define FATA1_PION_OFF_INTST	0x140	/* FastATA interrupt status */

#define FATA1_INT_ANY		0x80
#define FATA1_INT_DRIVE0	0x40
#define FATA1_INT_DRIVE1	0x20

#endif /* _AMIGA_EFAREG_H_ */

