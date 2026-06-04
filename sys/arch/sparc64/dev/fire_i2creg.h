/* $NetBSD: fire_i2creg.h,v 1.1 2026/06/04 13:43:24 jdc Exp $ */

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

#ifndef _FIRE_I2C_H_
#define _FIRE_I2C_H_

/*
 * Register definitions for "Mentor Grapics MI2C / Fire I2C"
 * Information taken from:
 * - Inventra MI2C Product Specification
 * - Fire Programmer's Reference Manual for Fire 2.1
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fire_i2creg.h,v 1.1 2026/06/04 13:43:24 jdc Exp $");

/* Register offsets */
#define FIREI2C_ADDR		0x00	/* Slave address */
#define FIREI2C_XADDR		0x08	/* Extended slave address */
#define FIREI2C_DATA		0x10	/* Data byte */
#define FIREI2C_CTRL		0x18	/* Control register */
#define FIREI2C_STAT		0x20	/* Status register (RO) */
#define FIREI2C_CCR		0x28	/* Clock cntrol register (WO) */
#define FIREI2C_SRST		0x30	/* Software reset */

/* 0x00 - Slave (our) address */
#define FIREI2C_ADDR_GC_ENBL	0x01	/* General call address enable */
#define FIREI2C_ADDR_7		0xfe	/* 7-bit addr address mask */
#define FIREI2C_ADDR_SHIFT	1
#define FIREI2C_XADDR_9		0x06	/* 9-bit address mask (bits 8 + 9) */
#define FIREI2C_XADDR_9_SHIFT	8

/* 0x08 - Extended slave address */
#define FIREI2C_XADDR_7		0xff	/* 9-bit address mask (bits 0 - 7) */

/* 0x10 - Data byte - sent or received on the bus (including slave addr) */
#define FIREI2C_DATA_SHIFT	1

/* 0x18 - Control register */
#define FIREI2C_CTRL_AAK	0x04	/* Assert acknowledge */
#define FIREI2C_CTRL_IFLG	0x08	/* Interrupt flag */
#define FIREI2C_CTRL_STP	0x10	/* Master mode stop */
#define FIREI2C_CTRL_STA	0x20	/* Master mode start */
#define FIREI2C_CTRL_ENAB	0x40	/* Bus enable */
#define FIREI2C_CTRL_IEN	0x80	/* Interrupt enable */

/* 0x20 - Status register (RO) */
#define FIREI2C_STAT_BERR	0x00	/* Bus error */
#define FIREI2C_STAT_STA	0x08	/* Start transmitted */
#define FIREI2C_STAT_REPSTA	0x10	/* Repeated start transmitted */
#define FIREI2C_STAT_AWR_ACK	0x18	/* Addr + Write sent -> ACK rcvd */
#define FIREI2C_STAT_AWR_NAK	0x20	/* Addr + Write sent -> no ACK rcvd */
#define FIREI2C_STAT_DAT_ACK	0x28	/* Data sent = ACK rcvd */
#define FIREI2C_STAT_DAT_NAK	0x30	/* Data sent = no ACK rcvd */
#define FIREI2C_STAT_ARB_LST	0x38	/* Arbitration lost */
#define FIREI2C_STAT_ARE_ACK	0x40	/* Addr + Read sent -> ACK rcvd */
#define FIREI2C_STAT_ARE_NAK	0x48	/* Addr + Read sent -> no ACK rcvd */
#define FIREI2C_STAT_MDAT_ACK	0x50	/* Data rcvd master -> ACK sent */
#define FIREI2C_STAT_MDAT_NAK	0x58	/* Data rcvd master -> not ACK sent */
#define FIREI2C_STAT_SLW_ACK	0x60	/* Slave + Write rcvd -> ACK sent */
#define FIREI2C_STAT_ARB_SLW	0x68	/* Arb. lost, slv+wr rcvd, ACK sent */
#define FIREI2C_STAT_GC_ACK	0x70	/* Gen. call rcvd, ACK sent */
#define FIREI2C_STAT_ARB_GC	0x78	/* Arb. lost, gen call rcvd, ACK sent */
#define FIREI2C_STAT_SDAT_ACK	0x80	/* Data rcvd slave -> ACK sent */
#define FIREI2C_STAT_SDAT_NAK	0x88	/* Data rcvd slave -> not ACK sent */
#define FIREI2C_STAT_GDAT_ACK	0x90	/* Data rcvd gen call -> ACK sent */
#define FIREI2C_STAT_GDAT_NAK	0x98	/* Data rcvd gen call -> not ACK sent */
#define FIREI2C_STAT_STP_REP	0xa0	/* Stop or Rep Start rcvd slave */
#define FIREI2C_STAT_SLR_ACK	0xa8	/* Slave + Read rcvd -> ACK sent */
#define FIREI2C_STAT_ARB_SLR	0xb0	/* Arb. lost, slv+rd rcvd, ACK sent */
#define FIREI2C_STAT_SLV_ACK	0xb8	/* Data sent slave -> ACK rcvd */
#define FIREI2C_STAT_SLV_NAK	0xc0	/* Data sent slave -> ACK not rcvd */
#define FIREI2C_STAT_SLV_LAS	0xc8	/* Last byte sent slave -> ACK rcvd */
#define FIREI2C_STAT_2AW_ACK	0xd0	/* 2nd Addr + Write sent -> ACK rcvd */
#define FIREI2C_STAT_2AW_NAK	0xd8	/* 2nd Addr + Wr sent -> no ACK rcvd */
#define FIREI2C_STAT_IDLE	0xf8	/* No relevant status (default) */

/* 0x28 - Clock cntrol register (WO) */
#define FIREI2C_CCR_POW2	0x07	/* Clock divider 1 = 2^N (sample clk) */
#define FIREI2C_CCR_FACT	0x78	/* Clock divider 2 = M (i2c clock) */
#define FIREI2C_CCR_FACT_SHIFT	3
#define FIREI2C_CCR_POW2_MAX	7
#define FIREI2C_CCR_FACT_MAX	15

/* 0x30 - Software reset - write any value to reset */
#define FIREI2C_SRST_RST	0x01

#endif /* _FIRE_I2C_H_ */
