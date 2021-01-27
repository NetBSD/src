/* $NetBSD: if_mipsnetreg.h,v 1.1 2021/01/27 05:24:16 simonb Exp $ */

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Simon Burge.
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
 * MIPSSIM emulator MIPSNET ethernet registers
 */

#define	MN_DEVID0		0x00	/* device info */
#define	MN_DEVID1		0x00	/* device info */
#define	MN_BUSY			0x08	/* rx/tx in progress */
#define	MN_RXDATACOUNT		0x0c	/* bytes in rx data buffer */
#define	MN_TXDATACOUNT		0x10	/* bytes for tx data buffer */
#define	MN_INTR			0x14	/* interrupt control */
#define	  MN_INTR_TXDONE	  __BIT(0)	/* tx done interrupt */
#define	  MN_INTR_RXDONE	  __BIT(1)	/* rx data available interrupt */
#define	  MN_INTR_TEST		  __BIT(31)	/* interrupt test */
#define	MN_INTRINFO		0x18	/* core-specific interrupt info */
#define	MN_RXDATA		0x1c	/* rx data fifo */
#define	MN_TXDATA		0x20	/* tx data fifo */

#define	MN_NPORTS		0x24	/* size to map for registers */

#define	MN_MAXDATA		32768	/* largest transfer size */

#define	MIPSNET_DEVID0	0x4d495053	/* ascii "MIPS" */
#define	MIPSNET_DEVID1	0x4e455430	/* ascii "NET0" */
