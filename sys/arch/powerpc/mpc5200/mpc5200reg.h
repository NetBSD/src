/*	$NetBSD: mpc5200reg.h,v 1.1 2026/06/27 13:28:35 rkujawa Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa and Robert Swindells.
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

#ifndef _POWERPC_MPC5200_MPC5200REG_H_
#define _POWERPC_MPC5200_MPC5200REG_H_

/*
 * On-chip peripheral register map for the Freescale MPC5200B.
 */

#define MPC5200_MBAR_DEFAULT	0xf0000000	/* firmware default MBAR */
#define MPC5200_MBAR_SIZE	0x00010000	/* 64KB peripheral window */

/* MBAR-relative block offsets. */
#define MPC5200_REG_MBAR	0x0000	/* MBAR / arbiter config */
#define MPC5200_REG_SDRAM	0x0100	/* SDRAM/DDR memory controller */
#define MPC5200_REG_CDM		0x0200	/* clock distribution module */
#define MPC5200_REG_SIU		0x0500	/* SIU interrupt controller */
#define MPC5200_REG_GPT		0x0600	/* general purpose timers 0-7 */
#define MPC5200_REG_SLT		0x0700	/* slice timers */
#define MPC5200_REG_RTC		0x0800	/* real-time clock */
#define MPC5200_REG_MSCAN1	0x0900	/* CAN 1 */
#define MPC5200_REG_MSCAN2	0x0980	/* CAN 2 */
#define MPC5200_REG_GPIO	0x0b00	/* simple GPIO */
#define MPC5200_REG_GPIO_WKUP	0x0c00	/* wakeup GPIO */
#define MPC5200_REG_PCI		0x0d00	/* PCI controller */
#define MPC5200_REG_SPI		0x0f00	/* dedicated SPI */
#define MPC5200_REG_USB		0x1000	/* USB OHCI */
#define MPC5200_REG_SDMA	0x1200	/* BestComm SDMA */
#define MPC5200_REG_XLB		0x1f00	/* XLB arbiter */
#define MPC5200_REG_PSC1	0x2000	/* PSC1 (console UART) */
#define MPC5200_REG_PSC2	0x2200	/* PSC2 (AC97 codec) */
#define MPC5200_REG_PSC3	0x2400	/* PSC3 */
#define MPC5200_REG_PSC4	0x2600	/* PSC4 */
#define MPC5200_REG_PSC5	0x2800	/* PSC5 */
#define MPC5200_REG_PSC6	0x2c00	/* PSC6 */
#define MPC5200_REG_FEC		0x3000	/* fast ethernet controller */
#define MPC5200_REG_ATA		0x3a00	/* ATA controller */
#define MPC5200_REG_I2C1	0x3d00	/* I2C 1 */
#define MPC5200_REG_I2C2	0x3d40	/* I2C 2 */
#define MPC5200_REG_SRAM	0x8000	/* on-chip SRAM (16KB) */

#define MPC5200_PSC_SIZE	0x100	/* per-PSC register window */
#define MPC5200_SRAM_SIZE	0x4000	/* on-chip SRAM size (16KB) */
#define MPC5200_USB_SIZE	0x200	/* USB OHCI register window */

#endif /* _POWERPC_MPC5200_MPC5200REG_H_ */
