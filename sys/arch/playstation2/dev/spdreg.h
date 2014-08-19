/*	$NetBSD: spdreg.h,v 1.4.6.2 2014/08/20 00:03:17 tls Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 * spd (PlayStation 2 HDD UNIT) common register define.
 */

/* interrupt */
#define SPD_INTR_ENABLE_REG16		MIPS_PHYS_TO_KSEG1(0x1400002a)
#define SPD_INTR_STATUS_REG16		MIPS_PHYS_TO_KSEG1(0x14000028)
#define SPD_INTR_CLEAR_REG16		MIPS_PHYS_TO_KSEG1(0x14000128)
#define   SPD_INTR_EMAC3	0x0040
#define   SPD_INTR_RXEND	0x0020
#define   SPD_INTR_TXEND	0x0010
#define   SPD_INTR_RXDNV	0x0008
#define   SPD_INTR_TXDNV	0x0004
#define   SPD_INTR_HDD		0x0001

/* I/O port */
#define SPD_IO_DIR_REG8			MIPS_PHYS_TO_KSEG1(0x1400002c)
#define SPD_IO_DATA_REG8		MIPS_PHYS_TO_KSEG1(0x1400002e)
	/* HDD LED */
#define   SPD_IO_LED		0x0001
	/* EEPROM (ethernet address) */
#define   SPD_IO_OUT		0x0010
#define   SPD_IO_IN		0x0020
#define   SPD_IO_CLK		0x0040
#define   SPD_IO_CS		0x0080

/* HDD interface */
#define SPD_XFR_CTRL_REG8		MIPS_PHYS_TO_KSEG1(0x14000032)
#define SPD_HDD_IO_BASE			MIPS_PHYS_TO_KSEG1(0x14000040)
#define SPD_IF_CTRL_REG8		MIPS_PHYS_TO_KSEG1(0x14000064)
#define   SPD_IF_CTRL_ATA_RST	0x80
#define   SPD_IF_CTRL_DMA_EN	0x04

