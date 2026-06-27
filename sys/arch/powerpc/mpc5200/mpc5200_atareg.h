/*	$NetBSD: mpc5200_atareg.h,v 1.1 2026/06/27 13:28:35 rkujawa Exp $	*/

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

#ifndef _POWERPC_MPC5200_MPC5200_ATAREG_H_
#define _POWERPC_MPC5200_MPC5200_ATAREG_H_

/*
 * Register map of the MPC5200B on-chip ATA Controller.
 */

/* Host registers (32-bit). */
#define	ATA_CONFIG	0x00	/* ATA host configuration		*/
#define	ATA_HOST_STATUS	0x04	/* ATA host controller status		*/
#define	ATA_PIO1	0x08	/* PIO timing 1 (t0, t2_8, t2_16)	*/
#define	ATA_PIO2	0x0c	/* PIO timing 2 (t4, t1, ta)		*/
#define	ATA_DMA1	0x10	/* multiword DMA timing 1		*/
#define	ATA_DMA2	0x14	/* multiword DMA timing 2		*/
#define	ATA_UDMA1	0x18	/* ultra DMA timing 1			*/
#define	ATA_UDMA2	0x1c	/* ultra DMA timing 2			*/
#define	ATA_UDMA3	0x20	/* ultra DMA timing 3			*/
#define	ATA_UDMA4	0x24	/* ultra DMA timing 4			*/
#define	ATA_UDMA5	0x28	/* ultra DMA timing 5			*/

/* Taskfile (drive) registers. */
#define	ATA_DRIVE_CTRL	0x5c	/* device control (W) / alt status (R)	*/
#define	ATA_DRIVE_BASE	0x60	/* data, error/feature, ... command	*/
#define	ATA_DRIVE_STRIDE   4	/* 32-bit cell per taskfile register	*/
#define	ATA_REG_SIZE	0x80	/* covers host regs + taskfile (0x00..0x7f) */

/*
 * ATA host configuration register bits
 */
#define	ATA_CONFIG_SMR	0x80000000	/* state machine reset (bit 0)	*/
#define	ATA_CONFIG_FR	0x40000000	/* FIFO reset (bit 1)		*/
#define	ATA_CONFIG_IE	0x02000000	/* PIO drive interrupt enable (6) */
#define	ATA_CONFIG_IORDY 0x01000000	/* honor IORDY, needed PIO >=3 (7) */

/* ATA host status register bits */
#define	ATA_HOST_STATUS_TIP	0x80000000	/* transaction in progress */
#define	ATA_HOST_STATUS_UREP	0x40000000	/* UDMA read extended pause */
#define	ATA_HOST_STATUS_RERR	0x02000000	/* read of unimpl. register */
#define	ATA_HOST_STATUS_WERR	0x01000000	/* write of unimpl. register */

#endif /* _POWERPC_MPC5200_MPC5200_ATAREG_H_ */
