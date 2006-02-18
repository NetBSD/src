/* $NetBSD: dbau1550reg.h,v 1.3.2.2 2006/02/18 15:38:32 yamt Exp $ */

/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Written by Garrett D'Amore for Itronix Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */ 

/*
 * Board-specific registers for DBAu1550.
 */

#define	DBAU1550_WHOAMI		0x0F000000
#define	DBAU1550_STATUS		0x0F000004
#define	DBAU1550_SWITCHES	0x0F000008
#define	DBAU1550_RESETS		0x0F00000C
#define	DBAU1550_PCMCIA_CONTROL	0x0F000010
#define	DBAU1550_BOARD_SPECIFIC	0x0F000014
#define	DBAU1550_DISC_LEDS	0x0F000018
#define	DBAU1550_SOFTWARE_RESET	0x0F00001C
#define	DBAU1550_HEX_LEDS	0x0F400000
#define	DBAU1550_HEX_BLANK	0x0F400008

/*
 * DBAU1550_WHOAMI
 */
#define	DBAU1550_WHOAMI_BOARD_MASK	0x00000f00
#define	DBAU1550_WHOAMI_PB1500_REV1	0x1
#define	DBAU1550_WHOAMI_PB1500_REV2	0x2
#define	DBAU1550_WHOAMI_PB1100		0x3
#define	DBAU1550_WHOAMI_DBAU1000	0x4
#define	DBAU1550_WHOAMI_DBAU1100	0x5
#define	DBAU1550_WHOAMI_DBAU1500	0x6
#define	DBAU1550_WHOAMI_DBAU1550_REV1	0x7
#define	DBAU1550_WHOAMI_PB1550_DDR	0x8
#define	DBAU1550_WHOAMI_PB1550_SDR	0x9

#define	DBAU1550_WHOAMI_BOARD(x)	(((x) >> 8) & 0xf)
#define	DBAU1550_WHOAMI_CPLD(x)		(((x) >> 4) & 0xf)
#define	DBAU1550_WHOAMI_DAUGHTER(x)	((x) & 0xf)

/*
 * DBAU1550_BCSR
 */
#define	DBAU1550_STATUS_SWAPBOOT		0x20000000
#define	DBAU1550_STATUS_PCMCIA1_INSERTED	0x00000020
#define	DBAU1550_STATUS_PCMCIA0_INSERTED	0x00000010
#define	DBAU1550_STATUS_PCMCIA1_VS		0x0000000c
#define	DBAU1550_STATUS_PCMCIA0_VS		0x00000003

/*
 * DBAU1550_BOARD_SPECIFIC
 */
#define	DBAU1550_PCI_SPI_DEV_SEL		(1 << 13)
#define	DBAU1550_PCI_CFG_HOST			(1 << 12)
#define	DBAU1550_PCI_EN_GPIO200_RST		(1 << 10)
#define	DBAU1550_PCI_M33			(1 << 8)
#define	DBAU1550_PCI_M66EN			(1 << 0)

/*
 * DBAU1550_SOFTWARE_RESET
 */
#define	DBAU1550_SOFTWARE_RESET_RESET		(1 << 15)
#define	DBAU1550_SOFTWARE_RESET_PWROFF		(1 << 14)
