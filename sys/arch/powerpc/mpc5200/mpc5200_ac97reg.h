/*	$NetBSD: mpc5200_ac97reg.h,v 1.1 2026/06/27 13:28:35 rkujawa Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
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

#ifndef _POWERPC_MPC5200_AC97REG_H_
#define _POWERPC_MPC5200_AC97REG_H_

/*
 * PSC AC97-mode register additions.
 */

/* AC97-mode register offsets not present in pscreg.h. */
#define	PSC_AC97SLOTS	0x24	/* AC97 Slots Register (wo, 32-bit)	*/
#define	PSC_AC97CMD	0x28	/* AC97 Command Register (rw, 32-bit)	*/
#define	PSC_AC97DATA	0x2c	/* AC97 Status Data Register (ro, 32-bit) */
#define	PSC_OP1		0x38	/* Output Port 1 Bit Set (wo, 8-bit)	*/
#define	PSC_OP0		0x3c	/* Output Port 0 Bit Set (wo, 8-bit)	*/
#define	PSC_RFCNTL	0x68	/* Rx FIFO Control (8-bit)		*/
#define	PSC_RFALARM	0x6e	/* Rx FIFO Alarm threshold (16-bit)	*/
#define	PSC_TFCNTL	0x88	/* Tx FIFO Control (8-bit)		*/
#define	PSC_TFALARM	0x8e	/* Tx FIFO Alarm threshold (16-bit)	*/

/*
 * Serial Interface Control Register (PSC_SICR, 0x40) bits for AC97 mode
 */
#define	SICR_SIM_AC97		0x03000000	/* SIM = 0b0011 (AC97)	*/
#define	SICR_ENAC97		0x00010000	/* normal (enhanced) AC97 */
#define	SICR_NORMAL_AC97	(SICR_SIM_AC97 | SICR_ENAC97)

/*
 * AC97 Slots Register (PSC_AC97SLOTS, 0x24)
 */
#define	AC97SLOTS_TX_SLOT(n)	(1u << (28 - (n)))	/* n = 3..12 */
#define	AC97SLOTS_RX_SLOT(n)	(1u << (15 - (n)))	/* n = 3..12 */
#define	AC97SLOTS_TX_STEREO	(AC97SLOTS_TX_SLOT(3) | AC97SLOTS_TX_SLOT(4))
#define	AC97SLOTS_RX_STEREO	(AC97SLOTS_RX_SLOT(3) | AC97SLOTS_RX_SLOT(4))

/*
 * AC97 Command Register (PSC_AC97CMD, 0x28)
 */
#define	AC97CMD_READ		0x80000000	/* bit 0: 1 = read	*/
#define	AC97CMD_INDEX(reg)	((uint32_t)((reg) & 0x7f) << 24)
#define	AC97CMD_DATA(val)	((uint32_t)((val) & 0xffff) << 8)
#define	AC97CMD_WRITE(reg, val)	(AC97CMD_INDEX(reg) | AC97CMD_DATA(val))

/*
 * AC97 Status Data Register (PSC_AC97DATA, 0x2c)
 */
#define	AC97DATA_VALUE(d)	(((d) >> 8) & 0xffff)

/*
 * Status Register (PSC_SR, 0x04)
 */
#define	AC97_SR_CMD_SEND	0x0008	/* AC97CMD written, not yet sent	*/
#define	AC97_SR_DATA_OVR	0x0004	/* read response overran	*/
#define	AC97_SR_DATA_VALID	0x0002	/* read response available	*/
#define	AC97_SR_UNEX_RX_SLOT	0x0001	/* unexpected receive slots	*/

/*
 * Output Port bit for the codec RES line
 */
#define	AC97_OP_RES		0x02

#endif /* _POWERPC_MPC5200_AC97REG_H_ */
