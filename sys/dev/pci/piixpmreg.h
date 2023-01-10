/* $NetBSD: piixpmreg.h,v 1.13 2023/01/10 00:05:53 msaitoh Exp $ */
/*	$OpenBSD: piixreg.h,v 1.3 2006/01/03 22:39:03 grange Exp $	*/

/*-
 * Copyright (c) 2016 Andriy Gapon <avg@FreeBSD.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: head/sys/dev/amdsbwd/amd_chipset.h 333269 2018-05-05 05:22:11Z avg $
 */

/*
 * Copyright (c) 2005 Alexander Yurchenko <grange@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _DEV_PCI_PIIXREG_H_
#define _DEV_PCI_PIIXREG_H_

/*
 * Intel PCI-to-ISA / IDE Xcelerator (PIIX) register definitions.
 */

/*
 * Power management registers.
 */

/* PCI configuration registers */
#define PIIX_PM_BASE	0x40		/* Power management base address */
#define PIIX_PM_BASE_CSB5_RESET	0x10		/* CSB5 PM reset */
#define PIIX_DEVACTA	0x54		/* Device activity A (function 3) */
#define PIIX_DEVACTB	0x58		/* Device activity B (function 3) */
#define PIIX_PMREGMISC	0x80		/* Misc. Power management */
#define PIIX_SMB_BASE	0x90		/* SMBus base address */
#define PIIX_SMB_HOSTC	0xd0		/* SMBus host configuration */
#define PIIX_SMB_HOSTC_HSTEN	(1 << 16)	/* enable host controller */
#define PIIX_SMB_HOSTC_SMI	(0 << 17)	/* SMI */
#define PIIX_SMB_HOSTC_IRQ	(4 << 17)	/* IRQ 9 */
#define PIIX_SMB_HOSTC_INTMASK	(7 << 17)

/* SMBus I/O registers */
#define PIIX_SMB_HS	0x00		/* host status */
#define PIIX_SMB_HS_BUSY	(1 << 0)	/* running a command */
#define PIIX_SMB_HS_INTR	(1 << 1)	/* command completed */
#define PIIX_SMB_HS_DEVERR	(1 << 2)	/* command error */
#define PIIX_SMB_HS_BUSERR	(1 << 3)	/* transaction collision */
#define PIIX_SMB_HS_FAILED	(1 << 4)	/* failed bus transaction */
#define PIIX_SMB_HS_BITS	"\020\001BUSY\002INTR\003DEVERR\004BUSERR\005FAILED"
#define PIIX_SMB_HC	0x02		/* host control */
#define PIIX_SMB_HC_INTREN	(1 << 0)	/* enable interrupts */
#define PIIX_SMB_HC_KILL	(1 << 1)	/* kill current transaction */
#define PIIX_SMB_HC_CMD_QUICK	(0 << 2)	/* QUICK command */
#define PIIX_SMB_HC_CMD_BYTE	(1 << 2)	/* BYTE command */
#define PIIX_SMB_HC_CMD_BDATA	(2 << 2)	/* BYTE DATA command */
#define PIIX_SMB_HC_CMD_WDATA	(3 << 2)	/* WORD DATA command */
#define PIIX_SMB_HC_CMD_BLOCK	(5 << 2)	/* BLOCK command */
#define PIIX_SMB_HC_START	(1 << 6)	/* start transaction */
#define PIIX_SMB_HCMD	0x03		/* host command */
#define PIIX_SMB_TXSLVA	0x04		/* transmit slave address */
#define PIIX_SMB_TXSLVA_READ	(1 << 0)	/* read direction */
#define PIIX_SMB_TXSLVA_ADDR(x)	(((x) & 0x7f) << 1) /* 7-bit address */
#define PIIX_SMB_HD0	0x05		/* host data 0 */
#define PIIX_SMB_HD1	0x06		/* host data 1 */
#define PIIX_SMB_HBDB	0x07		/* host block data byte */
#define PIIX_SMB_SC	0x08		/* slave control */
#define PIIX_SMB_SC_ALERTEN	(1 << 3)	/* enable SMBALERT# */
#define PIIX_SMB_SC_HOSTSEM	(1 << 4)	/* (W1S) HostSemaphore */
#define PIIX_SMB_SC_CLRHOSTSEM	(1 << 5)	/* (W1C) ClrHostSemaphore */
#define PIIX_SMB_SC_ECSEM	(1 << 6)	/* (W1S) EcSemaphore */
#define PIIX_SMB_SC_CLRECSEM	(1 << 7)	/* (W1C) ClrEcSemaphore */
#define PIIX_SMB_SC_SEMMASK	0xf0		/* Semaphore bits */

/* Power management I/O registers */
#define PIIX_PM_PMTMR	0x08		/* power management timer */

/* Misc */
#define PIIX_PM_SIZE	0x38		/* Power management I/O space size */
#define PIIX_SMB_SIZE	0x10		/* SMBus I/O space size */

/*
 * AMD SB800 and compatible chipset's configuration registers.
 * See SB8xx RRG 2.3.3, etc.
 */

/* In the I/O area */
#define SB800_INDIRECTIO_BASE	0xcd6
#define SB800_INDIRECTIO_SIZE	2 
#define SB800_INDIRECTIO_INDEX	0
#define SB800_INDIRECTIO_DATA	1

/* In the MMIO area */
#define SB800_FCH_PM_BASE	0xfed80300
#define SB800_FCH_PM_SIZE	8

#define SB800_PM_SMBUS0EN_LO	0x2c
#define SB800_PM_SMBUS0EN_HI	0x2d
#define SB800_PM_SMBUS0EN_ENABLE __BIT(0)     /* Function enable */
#define SB800_PM_SMBUS0_MASK_C	__BITS(2, 1)  /* Port mask (PMIO2C) */
#define SB800_PM_SMBUS0EN_BADDR	__BITS(15, 5) /* Base address */

#define SB800_PM_SMBUS0SEL	0x2e
#define SB800_PM_SMBUS0_MASK_E	__BITS(2, 1)  /* Port mask (PMIO2E) */
#define SB800_PM_SMBUS0SELEN	0x2f
#define SB800_PM_USE_SMBUS0SEL	__BIT(0) /*
					  * If the bit is set, SMBUS0SEL is
					  * used to select the port.
					  */

/* In the SMBus I/O space */
#define SB800_SMB_HOSTC		0x10	/* I2C bus configuration */
#define SB800_SMB_HOSTC_IRQ	(1 << 0)	/* 0:SMI 1:IRQ */

/* Misc */
#define SB800_SMB_SIZE	0x11		/* SMBus I/O space size */

/*
 * Newer FCH registers in the PMIO space.
 * See BKDG for Family 16h Models 30h-3Fh 3.26.13 PMx00 and PMx04.
 */
#define AMDFCH41_PM_DECODE_EN0		0x00
#define		AMDFCH41_SMBUS_EN	0x10
#define		AMDFCH41_WDT_EN		0x80
#define AMDFCH41_PM_DECODE_EN1		0x01
#define AMDFCH41_PM_PORT_INDEX		0x02
#define 	AMDFCH41_SMBUS_PORTMASK	0x18
#define	AMDFCH41_PM_DECODE_EN3		0x03
#define		AMDFCH41_WDT_RES_MASK	0x03
#define		AMDFCH41_WDT_RES_32US	0x00
#define		AMDFCH41_WDT_RES_10MS	0x01
#define		AMDFCH41_WDT_RES_100MS	0x02
#define		AMDFCH41_WDT_RES_1S	0x03
#define		AMDFCH41_WDT_EN_MASK	0x0c
#define		AMDFCH41_WDT_ENABLE	0x00
#define	AMDFCH41_PM_ISA_CTRL		0x04
#define		AMDFCH41_MMIO_EN	0x02

#endif	/* !_DEV_PCI_PIIXREG_H_ */
