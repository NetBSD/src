/*	$NetBSD: rmixlreg.h,v 1.1.2.1 2009/09/13 03:27:38 cliff Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by CCCCCCCC NNNNNNNNNN
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


#ifndef _MIPS_RMI_RMIXLREGS_H_
#define _MIPS_RMI_RMIXLREGS_H_

/*
 * RMIXL Coprocessor 2 registers:
 */
#ifdef _LOCORE
#define _(n)    __CONCAT($,n)
#else
#define _(n)    n
#endif
					/*		#sels --------------+	*/
					/*		#regs -----------+  |	*/
					/* What:	#bits --+	 |  |	*/
					/*			v	 v  v 	*/
#define RMIXL_COP_2_TXBUF	_(0)	/* Transmit Buffers	64	[1][4]	*/
#define RMIXL_COP_2_RXBUF	_(1)	/* Receive Buffers	64	[1][4]	*/
#define RMIXL_COP_2_MSG_STS	_(2)	/* Mesage Status	32	[1][2]	*/
#define RMIXL_COP_2_MSG_CFG	_(3)	/* MEssage Config	32	[1][2]	*/
#define RMIXL_COP_2_MSG_BSZ	_(4)	/* Message Bucket Size	32	[1][8]	*/
#define RMIXL_COP_2_CREDITS	_(16)	/* Credit Counters	32     [16][8]	*/

/* CP2 bit defines TBD */

/*
 * RMIXL Processor Control Register addresses
 * - Offset  in bits  7..0 
 * - BlockID in bits 15..8 
 */
#define RMIXL_PCR_THREADEN			0x0000
#define RMIXL_PCR_SOFTWARE_SLEEP		0x0001
#define RMIXL_PCR_SCHEDULING			0x0002
#define RMIXL_PCR_SCHEDULING_COUNTERS		0x0003
#define RMIXL_PCR_BHRPM				0x0004
#define RMIXL_PCR_IFU_DEFEATURE			0x0006
#define RMIXL_PCR_ICU_DEFEATURE			0x0100
#define RMIXL_PCR_ICU_ERROR_LOGGING		0x0101
#define RMIXL_PCR_ICU_DEBUG_ACCESS_ADDR		0x0102
#define RMIXL_PCR_ICU_DEBUG_ACCESS_DATALO	0x0103
#define RMIXL_PCR_ICU_DEBUG_ACCESS_DATAHI	0x0104
#define RMIXL_PCR_ICU_SAMPLING_LFSR		0x0105
#define RMIXL_PCR_ICU_SAMPLING_PC		0x0106
#define RMIXL_PCR_ICU_SAMPLING_SETUP		0x0107
#define RMIXL_PCR_ICU_SAMPLING_TIMER		0x0108
#define RMIXL_PCR_ICU_SAMPLING_PC_UPPER		0x0109
#define RMIXL_PCR_IEU_DEFEATURE			0x0200
#define RMIXL_PCR_TARGET_PC_REGISTER		0x0207
#define RMIXL_PCR_L1D_CONFIG0			0x0300
#define RMIXL_PCR_L1D_CONFIG1			0x0301
#define RMIXL_PCR_L1D_CONFIG2			0x0302
#define RMIXL_PCR_L1D_CONFIG3			0x0303
#define RMIXL_PCR_L1D_CONFIG4			0x0304
#define RMIXL_PCR_L1D_STATUS			0x0305
#define RMIXL_PCR_L1D_DEFEATURE			0x0306
#define RMIXL_PCR_L1D_DEBUG0			0x0307
#define RMIXL_PCR_L1D_DEBUG1			0x0308
#define RMIXL_PCR_L1D_CACHE_ERROR_LOG		0x0309
#define RMIXL_PCR_L1D_CACHE_ERROR_OVF_LO	0x030A
#define RMIXL_PCR_L1D_CACHE_INTERRUPT		0x030B
#define RMIXL_PCR_MMU_SETUP			0x0400
#define RMIXL_PCR_PRF_SMP_EVENT			0x0500
#define RMIXL_PCR_RF_SMP_RPLY_BUF		0x0501

/* PCR bit defines TBD */


/*
 * Memory Distributed Interconnect (MDI) System Memory Map
 */
#define RMIXL_PHYSADDR_MAX	0xffffffffffLL		/* 1TB Physical Address space */
#define RMIXL_IO_DEV_PBASE	0x1ef00000		/* default phys. from XL[RS]_IO_BAR */
#define RMIXL_IO_DEV_VBASE	MIPS_PHYS_TO_KSEG1(RMIXL_IO_DEV_PBASE)
							/* default virtual base address */
#define RMIXL_IO_DEV_SIZE	0x100000		/* I/O Conf. space is 1MB region */

/*
 * Peripheral and I/O Configuration Region of Memory
 *
 * These are relocatable; we run using the reset value defaults,
 * and we expect to inherit those intact from the boot firmware.
 *
 * Many of these overlap between XLR and XLS, exceptions are ifdef'ed.
 *
 * Device region offsets are relative to RMIXL_IO_DEV_PBASE.
 */
#define RMIXL_IO_DEV_BRIDGE	0x00000	/* System Bridge Controller */
#define RMIXL_IO_DEV_DDR_CHNA	0x01000	/* DDR1/DDR2 DRAM_A Channel, Port MA */
#define RMIXL_IO_DEV_DDR_CHNB	0x02000	/* DDR1/DDR2 DRAM_B Channel, Port MB */
#define RMIXL_IO_DEV_DDR_CHNC	0x03000	/* DDR1/DDR2 DRAM_C Channel, Port MC */
#define RMIXL_IO_DEV_DDR_CHND	0x04000	/* DDR1/DDR2 DRAM_D Channel, Port MD */
#if defined(MIPS64_XLR)
#define RMIXL_IO_DEV_SRAM	0x07000	/* SRAM Controller, Port SA */
#endif	/* MIPS64_XLR */
#define RMIXL_IO_DEV_PIC	0x08000	/* Programmable Interrupt Controller */
#if defined(MIPS64_XLR)
#define RMIXL_IO_DEV_PCIX	0x09000	/* PCI-X */
#define RMIXL_IO_DEV_HT		0x0a000	/* HyperTransport */
#endif	/* MIPS64_XLR */
#define RMIXL_IO_DEV_SAE	0x0b000	/* Security Acceleration Engine */
#if defined(MIPS64_XLS)
#define XAUI Interface_0	0x0c000	/* XAUI Interface_0 */
					/*  when SGMII Interface_[0-3] are not used */
#endif	/* MIPS64_XLS */
#define RMIXL_IO_DEV_GMAC_A	0x0c000	/* RGMII-Interface_A, Port RA */
#define RMIXL_IO_DEV_GMAC_B	0x0d000	/* RGMII-Interface_B, Port RB */
#define RMIXL_IO_DEV_GMAC_C	0x0e000	/* RGMII-Interface_C, Port RC */
#define RMIXL_IO_DEV_GMAC_D	0x0f000	/* RGMII-Interface_D, Port RD */
#if defined(MIPS64_XLR)
#define RMIXL_IO_DEV_SPI4_A	0x10000	/* SPI-4.2-Interface_A, Port XA */
#define RMIXL_IO_DEV_XGMAC_A	0x11000	/* XGMII-Interface_A, Port XA */
#define RMIXL_IO_DEV_SPI4_B	0x12000	/* SPI-4.2-Interface_B, Port XB */
#define RMIXL_IO_DEV_XGMAC_B	0x13000	/* XGMII-Interface_B, Port XB */
#endif	/* MIPS64_XLR */
#define RMIXL_IO_DEV_UART_1	0x14000	/* UART_1 (16550 w/ ax4 addrs) */
#define RMIXL_IO_DEV_UART_2	0x15000	/* UART_2 (16550 w/ ax4 addrs) */
#define RMIXL_IO_DEV_I2C_1	0x16000	/* I2C_1 */
#define RMIXL_IO_DEV_I2C_2	0x17000	/* I2C_2 */
#define RMIXL_IO_DEV_GPIO	0x18000	/* GPIO */
#define RMIXL_IO_DEV_FLASH	0x19000	/* Flash ROM */
#define RMIXL_IO_DEV_DMA	0x1a000	/* DMA */
#define RMIXL_IO_DEV_L2		0x1b000	/* L2 Cache */
#define RMIXL_IO_DEV_TB		0x1c000	/* Trace Buffer */
#if defined(MIPS64_XLS)
#define RMIXL_IO_DEV_CMP	0x1d000	/* Compression/Decompression */
#define RMIXL_IO_DEV_PCIE_BE	0x1e000	/* PCI-Express_BE */
#define RMIXL_IO_DEV_PCIE_LE	0x1f000	/* PCI-Express_LE */
#define RMIXL_IO_DEV_SRIO_BE	0x1e000	/* SRIO_BE */
#define RMIXL_IO_DEV_SRIO_LE	0x1f000	/* SRIO_LE */
#define RMIXL_IO_DEV_XAUI_1	0x20000	/* XAUI Interface_1 */
					/*  when SGMII Interface_[4-7] are not used */
#define RMIXL_IO_DEV_GMAC_4	0x20000	/* SGMII-Interface_4, Port SGMII4 */
#define RMIXL_IO_DEV_GMAC_5	0x21000	/* SGMII-Interface_5, Port SGMII5 */
#define RMIXL_IO_DEV_GMAC_6	0x22000	/* SGMII-Interface_6, Port SGMII6 */
#define RMIXL_IO_DEV_GMAC_7	0x23000	/* SGMII-Interface_7, Port SGMII7 */
#define RMIXL_IO_DEV_USB_A	0x24000	/* USB Interface Low Address Space */
#define RMIXL_IO_DEV_USB_B	0x25000	/* USB Interface High Address Space */
#endif	/* MIPS64_XLS */


/*
 * Programmable Interrupt Controller registers
 * the Programming Reference Manual table 10.4
 * lists "Reg ID" values not offsets;
 * assume offset = id * 4
 */
#define _RMIXL_OFFSET(id)	((id) * 4)
#define	RMIXL_PIC_CONTROL		_RMIXL_OFFSET(0x0)
#define	RMIXL_PIC_IPIBASE		_RMIXL_OFFSET(0x4)
#define	RMIXL_PIC_RMIXL_OFFSET		_RMIXL_OFFSET(0x6)
#define	RMIXL_PIC_WATCHdOGMAXVALUE0	_RMIXL_OFFSET(0x8)
#define	RMIXL_PIC_WATCHDOGMAXVALUE1	_RMIXL_OFFSET(0x9)
#define	RMIXL_PIC_WATCHDOGMASK0		_RMIXL_OFFSET(0xa)
#define	RMIXL_PIC_WATCHDOGMASK1		_RMIXL_OFFSET(0xb)
#define	RMIXL_PIC_WATCHDOGHEARTBEAT0	_RMIXL_OFFSET(0xc)
#define	RMIXL_PIC_WATCHDOGHEARTBEAT1	_RMIXL_OFFSET(0xd)
#define	RMIXL_PIC_IRTENTRYC0(n)		_RMIXL_OFFSET(0x40 + (n))	/* 0<=n<=31 */
#define	RMIXL_PIC_IRTENTRYC1(n)		_RMIXL_OFFSET(0x80 + (n))	/* 0<=n<=31 */
#define	RMIXL_PIC_SYSTMRMAXVALC0(n)	_RMIXL_OFFSET(0x100 + (n))	/* 0<=n<=7 */
#define	RMIXL_PIC_SYSTMRMAXVALC1(n)	_RMIXL_OFFSET(0x110 + (n))	/* 0<=n<=7 */
#define	RMIXL_PIC_SYSTMRC0(n)		_RMIXL_OFFSET(0x120 + (n))	/* 0<=n<=7 */
#define	RMIXL_PIC_SYSTMRC1(n)		_RMIXL_OFFSET(0x130 + (n))	/* 0<=n<=7 */

/*
 * RMIXL_PIC_CONTROL bits
 */
#define RMIXL_PIC_CONTROL_WATCHDOG_ENB	__BIT(0)
#define RMIXL_PIC_CONTROL_GEN_NMI	__BITS(2,1)	/* do NMI after n WDog irpts */
#define RMIXL_PIC_CONTROL_GEN_NMIn(n)	(((n) << 1) & RMIXL_PIC_CONTROL_GEN_NMI)
#define RMIXL_PIC_CONTROL_RESa		__BITS(7,3)
#define RMIXL_PIC_CONTROL_TIMER_ENB	__BITS(15,8)	/* per-Timer enable bits */
#define RMIXL_PIC_CONTROL_TIMER_ENBn(n)	((1 << (n)) & RMIXL_PIC_CONTROL_TIMER_ENB)
#define RMIXL_PIC_CONTROL_RESb		__BITS(31,16)

/*
 * RMIXL_PIC_IPIBASE bits
 */
#define RMIXL_PIC_IPIBASE_VECTORNUM	__BITS(5,0)
#define RMIXL_PIC_IPIBASE_RESa		__BIT(6)	/* undocumented bit */
#define RMIXL_PIC_IPIBASE_BCAST		__BIT(7)
#define RMIXL_PIC_IPIBASE_NMI		__BIT(8)
#define RMIXL_PIC_IPIBASE_ID		__BITS(31,16)
#define RMIXL_PIC_IPIBASE_ID_RESa	__BITS(31,23)
#define RMIXL_PIC_IPIBASE_ID_CPU	__BITS(22,20)	/* Physical CPU ID */
#define RMIXL_PIC_IPIBASE_ID_RESb	__BITS(19,18)
#define RMIXL_PIC_IPIBASE_ID_THREAD	__BITS(22,20)	/* Thread ID */

#endif	/* _MIPS_RMI_RMIRMIXLEGS_H_ */

