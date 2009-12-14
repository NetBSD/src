/*	$NetBSD: rmixlreg.h,v 1.1.2.5 2009/12/14 07:19:16 cliff Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Cliff Neighbors
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

#include <sys/endian.h>

/*
 * on chip I/O register byte order is
 * BIG ENDIAN regardless of code model
 */
#define RMIXL_IOREG_VADDR(o)				\
	(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(	\
		rmixl_configuration.rc_io_pbase	+ (o))
#define RMIXL_IOREG_READ(o)     be32toh(*RMIXL_IOREG_VADDR(o))
#define RMIXL_IOREG_WRITE(o,v)  *RMIXL_IOREG_VADDR(o) = htobe32(v)


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
#define RMIXL_IO_DEV_BRIDGE	0x00000	/* System Bridge Controller (SBC) */
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
 * the Programming Reference Manual
 * lists "Reg ID" values not offsets;
 * offset = id * 4
 */
#define _RMIXL_OFFSET(id)	((id) * 4)


/*
 * System Bridge Controller registers
 * offsets are relative to RMIXL_IO_DEV_BRIDGE
 */
#define RMIXL_SBC_DRAM_NBARS		8
#define RMIXL_SBC_DRAM_BAR(n)		_RMIXL_OFFSET(0x000 + (n))
					/* DRAM Region Base Address Regs[0-7] */
#define RMIXL_SBC_DRAM_CHNAC_DTR(n)	_RMIXL_OFFSET(0x008 + (n))
					/* DRAM Region Channels A,C Address Translation Regs[0-7] */
#define RMIXL_SBC_DRAM_CHNBD_DTR(n)	_RMIXL_OFFSET(0x010 + (n))
					/* DRAM Region Channels B,D Address Translation Regs[0-7] */
#define RMIXL_SBC_DRAM_BRIDGE_CFG	_RMIXL_OFFSET(0x18)	/* SBC DRAM config reg */
#define RMIXL_SBC_XLS_IO_BAR		_RMIXL_OFFSET(0x19)	/* I/O Config Base Addr reg */
#define RMIXL_SBC_XLS_FLASH_BAR		_RMIXL_OFFSET(0x20)	/* Flash Memory Base Addr reg */
#define RMIXL_SBC_PCIE_CFG_BAR		_RMIXL_OFFSET(0x40)	/* PCI Configuration BAR */
#define RMIXL_SBC_PCIE_ECFG_BAR		_RMIXL_OFFSET(0x41)	/* PCI Extended Configuration BAR */
#define RMIXL_SBC_PCIE_MEM_BAR		_RMIXL_OFFSET(0x42)	/* PCI Memory region BAR */
#define RMIXL_SBC_PCIE_IO_BAR		_RMIXL_OFFSET(0x43)	/* PCI IO region BAR */

/*
 * Address Error registers
 * offsets are relative to RMIXL_IO_DEV_BRIDGE
 */
#define RMIXL_ADDR_ERR_DEVICE_MASK	_RMIXL_OFFSET(0x25)	/* Address Error Device Mask */
#define RMIXL_ADDR_ERR_AERR0_LOG1	_RMIXL_OFFSET(0x26)	/* Address Error Set 0 Log 1 */
#define RMIXL_ADDR_ERR_AERR0_LOG2	_RMIXL_OFFSET(0x27)	/* Address Error Set 0 Log 2 */
#define RMIXL_ADDR_ERR_AERR0_LOG3	_RMIXL_OFFSET(0x28)	/* Address Error Set 0 Log 3 */
#define RMIXL_ADDR_ERR_AERR0_DEVSTAT	_RMIXL_OFFSET(0x29)	/* Address Error Set 0 irpt status */
#define RMIXL_ADDR_ERR_AERR1_LOG1	_RMIXL_OFFSET(0x2a)	/* Address Error Set 1 Log 1 */
#define RMIXL_ADDR_ERR_AERR1_LOG2	_RMIXL_OFFSET(0x2b)	/* Address Error Set 1 Log 2 */
#define RMIXL_ADDR_ERR_AERR1_LOG3	_RMIXL_OFFSET(0x2c)	/* Address Error Set 1 Log 3 */
#define RMIXL_ADDR_ERR_AERR1_DEVSTAT	_RMIXL_OFFSET(0x2d)	/* Address Error Set 1 irpt status */
#define RMIXL_ADDR_ERR_AERR0_EN		_RMIXL_OFFSET(0x2e)	/* Address Error Set 0 irpt enable */
#define RMIXL_ADDR_ERR_AERR0_UPG	_RMIXL_OFFSET(0x2f)	/* Address Error Set 0 Upgrade */
#define RMIXL_ADDR_ERR_AERR0_CLEAR	_RMIXL_OFFSET(0x30)	/* Address Error Set 0 irpt clear */
#define RMIXL_ADDR_ERR_AERR1_CLEAR	_RMIXL_OFFSET(0x31)	/* Address Error Set 1 irpt clear */
#define RMIXL_ADDR_ERR_SBE_COUNTS	_RMIXL_OFFSET(0x32)	/* Single Bit Error Counts */
#define RMIXL_ADDR_ERR_DBE_COUNTS	_RMIXL_OFFSET(0x33)	/* Double Bit Error Counts */
#define RMIXL_ADDR_ERR_BITERR_INT_EN	_RMIXL_OFFSET(0x33)	/* Bit Error intr enable */

/*
 * RMIXL_SBC_DRAM_BAR bit defines
 */
#define RMIXL_DRAM_BAR_BASE_ADDR	__BITS(31,16)	/* bits 39:24 of Base Address */
#define DRAM_BAR_TO_BASE(r)	\
		(((r) & RMIXL_DRAM_BAR_BASE_ADDR) << (24 - 16))
#define RMIXL_DRAM_BAR_ADDR_MASK	__BITS(15,4)	/* bits 35:24 of Address Mask */
#define DRAM_BAR_TO_SIZE(r)	\
		((((r) & RMIXL_DRAM_BAR_ADDR_MASK) + __BIT(4)) << (24 - 4))
#define RMIXL_DRAM_BAR_INTERLEAVE	__BITS(3,1)	/* Interleave Mode */
#define RMIXL_DRAM_BAR_STATUS		__BIT(0)	/* 1='region enabled' */

/*
 * RMIXL_SBC_DRAM_CHNAC_DTR and
 * RMIXL_SBC_DRAM_CHNBD_DTR bit defines
 *	insert 'divisions' (0, 1 or 2) bits
 *	of value 'partition' 
 *	at 'position' bit location.
 */
#define RMIXL_DRAM_DTR_RESa		__BITS(31,14)
#define RMIXL_DRAM_DTR_PARTITION	__BITS(13,12)
#define RMIXL_DRAM_DTR_RESb		__BITS(11,10)
#define RMIXL_DRAM_DTR_DIVISIONS	__BITS(9,8)
#define RMIXL_DRAM_DTR_RESc		__BITS(7,6)
#define RMIXL_DRAM_DTR_POSITION		__BITS(5,0)
#define RMIXL_DRAM_DTR_RESV	\
		(RMIXL_DRAM_DTR_RESa|RMIXL_DRAM_DTR_RESb|RMIXL_DRAM_DTR_RESc)

/*
 * RMIXL_SBC_DRAM_BRIDGE_CFG bit defines
 */
#define RMIXL_DRAM_CFG_RESa		__BITS(31,13)
#define RMIXL_DRAM_CFG_CHANNEL_MODE	__BIT(12)
#define RMIXL_DRAM_CFG_RESb		__BIT(11)
#define RMIXL_DRAM_CFG_INTERLEAVE_MODE	__BITS(10,8)
#define RMIXL_DRAM_CFG_RESc		__BITS(7,5)
#define RMIXL_DRAM_CFG_BUS_MODE		__BIT(4)
#define RMIXL_DRAM_CFG_RESd		__BITS(3,2)
#define RMIXL_DRAM_CFG_DRAM_MODE	__BITS(1,0)	/* 1=DDR2 */

/*
 * RMIXL_SBC_PCIE_CFG_BAR bit defines
 */
#define RMIXL_PCIE_CFG_BAR_BASE		__BITS(31,17)	/* phys address bits 39:25 */
#define RMIXL_PCIE_CFG_BAR_BA_SHIFT	(25 - 17)
#define RMIXL_PCIE_CFG_BAR_TO_BA(r)	\
		(((r) & RMIXL_PCIE_CFG_BAR_BASE) << RMIXL_PCIE_CFG_BAR_BA_SHIFT)
#define RMIXL_PCIE_CFG_BAR_RESV		__BITS(16,1)	/* (reserved) */
#define RMIXL_PCIE_CFG_BAR_ENB		__BIT(0)	/* 1=Enable */
#define RMIXL_PCIE_CFG_SIZE		__BIT(25)
#define RMIXL_PCIE_CFG_BAR(ba, en)	\
		((uint32_t)(((ba) >> (25 - 17)) | ((en) ? RMIXL_PCIE_CFG_BAR_ENB : 0)))

/*
 * RMIXL_SBC_PCIE_ECFG_BAR bit defines
 * (PCIe extended config space)
 */
#define RMIXL_PCIE_ECFG_BAR_BASE	__BITS(31,21)	/* phys address bits 39:29 */
#define RMIXL_PCIE_ECFG_BAR_BA_SHIFT	(29 - 21)
#define RMIXL_PCIE_ECFG_BAR_TO_BA(r)	\
		(((r) & RMIXL_PCIE_ECFG_BAR_BASE) << RMIXL_PCIE_ECFG_BAR_BA_SHIFT)
#define RMIXL_PCIE_ECFG_BAR_RESV	__BITS(20,1)	/* (reserved) */
#define RMIXL_PCIE_ECFG_BAR_ENB		__BIT(0)	/* 1=Enable */
#define RMIXL_PCIE_ECFG_SIZE		__BIT(29)
#define RMIXL_PCIE_ECFG_BAR(ba, en)	\
		((uint32_t)(((ba) >> (29 - 21)) | ((en) ? RMIXL_PCIE_ECFG_BAR_ENB : 0)))

/*
 * RMIXL_SBC_PCIE_MEM_BAR bit defines
 */
#define RMIXL_PCIE_MEM_BAR_BASE		__BITS(31,16)	/* phys address bits 39:24 */
#define RMIXL_PCIE_MEM_BAR_TO_BA(r)	\
		(((r) & RMIXL_PCIE_MEM_BAR_BASE) << (24 - 16))
#define RMIXL_PCIE_MEM_BAR_MASK		__BITS(15,1)	/* phys address mask bits 38:24 */
#define RMIXL_PCIE_MEM_BAR_TO_SIZE(r)	\
		((((r) & RMIXL_PCIE_MEM_BAR_MASK) + 2) << (24 - 1))
#define RMIXL_PCIE_MEM_BAR_ENB		__BIT(0)	/* 1=Enable */
#define RMIXL_PCIE_MEM_BAR(ba, en)	\
		((uint32_t)(((ba) >> (24 - 16)) | ((en) ? RMIXL_PCIE_MEM_BAR_ENB : 0)))

/*
 * RMIXL_SBC_PCIE_IO_BAR bit defines
 */
#define RMIXL_PCIE_IO_BAR_BASE		__BITS(31,18)	/* phys address bits 39:26 */
#define RMIXL_PCIE_IO_BAR_TO_BA(r)	\
		(((r) & RMIXL_PCIE_IO_BAR_BASE) << (26 - 18))
#define RMIXL_PCIE_IO_BAR_RESV		__BITS(17,7)	/* (reserve) */
#define RMIXL_PCIE_IO_BAR_MASK		__BITS(6,1)	/* phys address mask bits 31:26 */
#define RMIXL_PCIE_IO_BAR_TO_SIZE(r)	\
		((((r) & RMIXL_PCIE_IO_BAR_MASK) + 2) << (26 - 1))
#define RMIXL_PCIE_IO_BAR_ENB		__BIT(0)	/* 1=Enable */
#define RMIXL_PCIE_IO_BAR(ba, en)	\
		((uint32_t)(((ba) >> (26 - 18)) | ((en) ? RMIXL_PCIE_IO_BAR_ENB : 0)))


/*
 * Programmable Interrupt Controller registers
 * the Programming Reference Manual table 10.4
 * lists "Reg ID" values not offsets
 * Offsets are relative to RMIXL_IO_DEV_BRIDGE
 */
#define	RMIXL_PIC_CONTROL		_RMIXL_OFFSET(0x0)
#define	RMIXL_PIC_IPIBASE		_RMIXL_OFFSET(0x4)
#define	RMIXL_PIC_INTRACK		_RMIXL_OFFSET(0x6)
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
#define RMIXL_PIC_CONTROL_RESV		\
		(RMIXL_PIC_CONTROL_RESa|RMIXL_PIC_CONTROL_RESb)

/*
 * RMIXL_PIC_IPIBASE bits
 */
#define RMIXL_PIC_IPIBASE_VECTORNUM	__BITS(5,0)
#define RMIXL_PIC_IPIBASE_RESa		__BIT(6)	/* undocumented bit */
#define RMIXL_PIC_IPIBASE_BCAST		__BIT(7)
#define RMIXL_PIC_IPIBASE_NMI		__BIT(8)
#define RMIXL_PIC_IPIBASE_ID		__BITS(31,16)
#define RMIXL_PIC_IPIBASE_ID_RESb	__BITS(31,23)
#define RMIXL_PIC_IPIBASE_ID_CPU	__BITS(22,20)	/* Physical CPU ID */
#define RMIXL_PIC_IPIBASE_ID_RESc	__BITS(19,18)
#define RMIXL_PIC_IPIBASE_ID_THREAD	__BITS(22,20)	/* Thread ID */
#define RMIXL_PIC_IPIBASE_ID_RESV	\
		(RMIXL_PIC_IPIBASE_ID_RESa|RMIXL_PIC_IPIBASE_ID_RESb	\
		|RMIXL_PIC_IPIBASE_ID_RESc)

/*
 * RMIXL_PIC_IRTENTRYC0 bits
 * IRT Entry low word
 */
#define RMIXL_PIC_IRTENTRYC0_TMASK	__BITS(7,0)	/* Thread Mask */
#define RMIXL_PIC_IRTENTRYC0_RESa	__BITS(3,2)	/* write as 0 */
#define RMIXL_PIC_IRTENTRYC0_RESb	__BITS(31,8)	/* write as 0 */
#define RMIXL_PIC_IRTENTRYC0_RESV	\
		(RMIXL_PIC_IRTENTRYC0_RESa | RMIXL_PIC_IRTENTRYC0_RESb)

/*
 * RMIXL_PIC_IRTENTRYC1 bits
 * IRT Entry high word
 */
#define RMIXL_PIC_IRTENTRYC1_INTVEC	__BITS(5,0)	/* maps to bit# in CPU's EIRR */
#define RMIXL_PIC_IRTENTRYC1_GL		__BIT(6)	/* 0=Global; 1=Local */
#define RMIXL_PIC_IRTENTRYC1_NMI	__BIT(7)	/* 0=Maskable; 1=NMI */
#define RMIXL_PIC_IRTENTRYC1_RESV	__BITS(28,8)
#define RMIXL_PIC_IRTENTRYC1_P		__BIT(29)	/* 0=Rising/High; 1=Falling/Low */
#define RMIXL_PIC_IRTENTRYC1_TRG	__BIT(30)	/* 0=Edge; 1=Level */
#define RMIXL_PIC_IRTENTRYC1_VALID	__BIT(31)	/* 0=Invalid; 1=Valid IRT Entry */


/*
 * GPIO Controller registers
 */

/* GPIO Signal Registers */
#define RMIXL_GPIO_INT_ENB		_RMIXL_OFFSET(0x0)	/* Interrupt Enable register */
#define RMIXL_GPIO_INT_INV		_RMIXL_OFFSET(0x1)	/* Interrupt Inversion register */
#define RMIXL_GPIO_IO_DIR		_RMIXL_OFFSET(0x2)	/* I/O Direction register */
#define RMIXL_GPIO_OUTPUT		_RMIXL_OFFSET(0x3)	/* Output Write register */
#define RMIXL_GPIO_INPUT		_RMIXL_OFFSET(0x4)	/* Intput Read register */
#define RMIXL_GPIO_INT_CLR		_RMIXL_OFFSET(0x5)	/* Interrupt Inversion register */
#define RMIXL_GPIO_INT_STS		_RMIXL_OFFSET(0x6)	/* Interrupt Status register */
#define RMIXL_GPIO_INT_TYP		_RMIXL_OFFSET(0x7)	/* Interrupt Type register */
#define RMIXL_GPIO_RESET		_RMIXL_OFFSET(0x8)	/* XLS Soft Reset register */

/*
 * RMIXL_GPIO_RESET bits
 */
#define RMIXL_GPIO_RESET_RESa		__BITS(31,28)
#define RMIXL_GPIO_RESET_PCIE_SRIO_SEL	__BITS(27,26)		/* PCIe or SRIO Select:
								 * 00 = PCIe selected, SRIO not available
								 * 01 = SRIO selected, 1.25 Gbaud (1.0 Gbps)
								 * 10 = SRIO selected, 2.25 Gbaud (2.0 Gbps)
								 * 11 = SRIO selected, 3.125 Gbaud (2.5 Gbps)
								 */
#define RMIXL_GPIO_RESET_XAUI_PORT1_SEL	__BIT(25)		/* XAUI Port 1 Select:
								 *  0 = Disabled - Port is SGMII ports 4-7
								 *  1 = Enabled -  Port is 4-lane XAUI Port 1
								 */
#define RMIXL_GPIO_RESET_XAUI_PORT0_SEL	__BIT(24)		/* XAUI Port 0 Select:
								 *  0 = Disabled - Port is SGMII ports 0-3
								 *  1 = Enabled -  Port is 4-lane XAUI Port 0
								 */
#define RMIXL_GPIO_RESET_RESb		__BIT(23)
#define RMIXL_GPIO_RESET_USB_DEV	__BIT(22)		/* USB Device:
								 *  0 = Device Mode
								 *  1 = Host Mode
								 */
#define RMIXL_GPIO_RESET_PCIE_CFG	__BITS(21,20)		/* PCIe or SRIO configuration */
#define RMIXL_GPIO_RESET_FLASH33_EN	__BIT(19)		/* Flash 33 MHZ Enable:
								 *  0 = 66.67 MHz
								 *  1 = 33.33 MHz
								 */
#define RMIXL_GPIO_RESET_BIST_DIAG_EN	__BIT(18)		/* BIST Diagnostics enable */
#define RMIXL_GPIO_RESET_BIST_RUN_EN	__BIT(18)		/* BIST Run enable */
#define RMIXL_GPIO_RESET_NOOT_NAND	__BIT(16)		/* Enable boot from NAND Flash */
#define RMIXL_GPIO_RESET_BOOT_PCMCIA	__BIT(15)		/* Enable boot from PCMCIA */
#define RMIXL_GPIO_RESET_FLASH_CFG	__BIT(14)		/* Flash 32-bit Data Configuration:
								 *  0 = 32-bit address / 16-bit data
								 *  1 = 32-bit address / 32-bit data
								 */
#define RMIXL_GPIO_RESET_PCMCIA_EN	__BIT(13)		/* PCMCIA Enable Status */
#define RMIXL_GPIO_RESET_PARITY_EN	__BIT(12)		/* Parity Enable Status */
#define RMIXL_GPIO_RESET_BIGEND		__BIT(11)		/* Big Endian Mode Enable Status */
#define RMIXL_GPIO_RESET_PLL1_OUT_DIV	__BITS(10,8)		/* PLL1 (Core PLL) Output Divider */
#define RMIXL_GPIO_RESET_PLL1_FB_DIV	__BITS(7,0)		/* PLL1 Feedback Divider */


/* GPIO System Control Registers */
#define RMIXL_GPIO_RESET_CFG		_RMIXL_OFFSET(0x15)	/* Reset Configuration register */
#define RMIXL_GPIO_THERMAL_CSR		_RMIXL_OFFSET(0x16)	/* Thermal Control/Status register */
#define RMIXL_GPIO_THERMAL_SHFT		_RMIXL_OFFSET(0x17)	/* Thermal Shift register */
#define RMIXL_GPIO_BIST_ALL_STS		_RMIXL_OFFSET(0x18)	/* BIST All Status register */
#define RMIXL_GPIO_BIST_EACH_STS	_RMIXL_OFFSET(0x19)	/* BIST Each Status register */
#define RMIXL_GPIO_SGMII_0_3_PHY_CTL	_RMIXL_OFFSET(0x20)	/* SGMII #0..3 PHY Control register */
#define RMIXL_GPIO_AUI_0_PHY_CTL	_RMIXL_OFFSET(0x20)	/* AUI port#0  PHY Control register */
#define RMIXL_GPIO_SGMII_4_7_PLL_CTL	_RMIXL_OFFSET(0x21)	/* SGMII #4..7 PLL Control register */
#define RMIXL_GPIO_AUI_1_PLL_CTL	_RMIXL_OFFSET(0x21)	/* AUI port#1  PLL Control register */
#define RMIXL_GPIO_SGMII_4_7_PHY_CTL	_RMIXL_OFFSET(0x22)	/* SGMII #4..7 PHY Control register */
#define RMIXL_GPIO_AUI_1_PHY_CTL	_RMIXL_OFFSET(0x22)	/* AUI port#1  PHY Control register */
#define RMIXL_GPIO_INT_MAP		_RMIXL_OFFSET(0x25)	/* Interrupt Map to PIC, 0=int14, 1=int30 */
#define RMIXL_GPIO_EXT_INT		_RMIXL_OFFSET(0x26)	/* External Interrupt control register */
#define RMIXL_GPIO_CPU_RST		_RMIXL_OFFSET(0x28)	/* CPU Reset control register */
#define RMIXL_GPIO_LOW_PWR_DIS		_RMIXL_OFFSET(0x29)	/* Low Power Dissipation register */
#define RMIXL_GPIO_RANDOM		_RMIXL_OFFSET(0x2b)	/* Low Power Dissipation register */
#define RMIXL_GPIO_CPU_CLK_DIS		_RMIXL_OFFSET(0x2d)	/* CPU Clock Disable register */

/*
 * PCIE Interface Controller registers
 */
#define RMIXL_PCIE_CTRL1		_RMIXL_OFFSET(0x0)
#define RMIXL_PCIE_CTRL2		_RMIXL_OFFSET(0x1)
#define RMIXL_PCIE_CTRL3		_RMIXL_OFFSET(0x2)
#define RMIXL_PCIE_CTRL4		_RMIXL_OFFSET(0x3)
#define RMIXL_PCIE_CTRL			_RMIXL_OFFSET(0x4)
#define RMIXL_PCIE_IOBM_TIMER		_RMIXL_OFFSET(0x5)
#define RMIXL_PCIE_MSI_CMD		_RMIXL_OFFSET(0x6)
#define RMIXL_PCIE_MSI_RESP		_RMIXL_OFFSET(0x7)
#define RMIXL_PCIE_DWC_CRTL5		_RMIXL_OFFSET(0x8)	/* not on XLS408Lite, XLS404Lite */
#define RMIXL_PCIE_DWC_CRTL6		_RMIXL_OFFSET(0x9)	/* not on XLS408Lite, XLS404Lite */
#define RMIXL_PCIE_IOBM_SWAP_MEM_BASE	_RMIXL_OFFSET(0x10)
#define RMIXL_PCIE_IOBM_SWAP_MEM_LIMIT	_RMIXL_OFFSET(0x11)
#define RMIXL_PCIE_IOBM_SWAP_IO_BASE	_RMIXL_OFFSET(0x12)
#define RMIXL_PCIE_IOBM_SWAP_IO_LIMIT	_RMIXL_OFFSET(0x13)
#define RMIXL_PCIE_TRGT_CHRNT_MEM_BASE	_RMIXL_OFFSET(0x14)
#define RMIXL_PCIE_TRGT_CHRNT_MEM_LIMIT	_RMIXL_OFFSET(0x15)
#define RMIXL_PCIE_TRGT_L2ALC_MEM_BASE	_RMIXL_OFFSET(0x16)
#define RMIXL_PCIE_TRGT_L2ALC_MEM_LIMIT	_RMIXL_OFFSET(0x17)
#define RMIXL_PCIE_TRGT_REX_MEM_BASE	_RMIXL_OFFSET(0x18)
#define RMIXL_PCIE_TRGT_REX_MEM_LIMIT	_RMIXL_OFFSET(0x19)
#define RMIXL_PCIE_EP_MEM_BASE		_RMIXL_OFFSET(0x1a)
#define RMIXL_PCIE_EP_MEM_LIMIT		_RMIXL_OFFSET(0x1b)
#define RMIXL_PCIE_EP_ADDR_MAP_ENTRY0	_RMIXL_OFFSET(0x1c)
#define RMIXL_PCIE_EP_ADDR_MAP_ENTRY1	_RMIXL_OFFSET(0x1d)
#define RMIXL_PCIE_EP_ADDR_MAP_ENTRY2	_RMIXL_OFFSET(0x1e)
#define RMIXL_PCIE_EP_ADDR_MAP_ENTRY3	_RMIXL_OFFSET(0x1f)
#define RMIXL_PCIE_LINK0_STATE		_RMIXL_OFFSET(0x20)
#define RMIXL_PCIE_LINK1_STATE		_RMIXL_OFFSET(0x21)
#define RMIXL_PCIE_IOBM_INT_STATUS	_RMIXL_OFFSET(0x22)
#define RMIXL_PCIE_IOBM_INT_ENABLE	_RMIXL_OFFSET(0x23)
#define RMIXL_PCIE_LINK0_MSI_STATUS	_RMIXL_OFFSET(0x24)
#define RMIXL_PCIE_LINK1_MSI_STATUS	_RMIXL_OFFSET(0x25)
#define RMIXL_PCIE_LINK0_MSI_ENABLE	_RMIXL_OFFSET(0x26)
#define RMIXL_PCIE_LINK1_MSI_ENABLE	_RMIXL_OFFSET(0x27)
#define RMIXL_PCIE_LINK0_INT_STATUS0	_RMIXL_OFFSET(0x28)
#define RMIXL_PCIE_LINK1_INT_STATUS0	_RMIXL_OFFSET(0x29)
#define RMIXL_PCIE_LINK0_INT_STATUS1	_RMIXL_OFFSET(0x2a)
#define RMIXL_PCIE_LINK1_INT_STATUS1	_RMIXL_OFFSET(0x2b)
#define RMIXL_PCIE_LINK0_INT_ENABLE0	_RMIXL_OFFSET(0x2c)
#define RMIXL_PCIE_LINK1_INT_ENABLE0	_RMIXL_OFFSET(0x2d)
#define RMIXL_PCIE_LINK0_INT_ENABLE1	_RMIXL_OFFSET(0x2e)
#define RMIXL_PCIE_LINK1_INT_ENABLE1	_RMIXL_OFFSET(0x2f)
#define RMIXL_PCIE_PHY_CR_CMD		_RMIXL_OFFSET(0x30)
#define RMIXL_PCIE_PHY_CR_WR_DATA	_RMIXL_OFFSET(0x31)
#define RMIXL_PCIE_PHY_CR_RESP		_RMIXL_OFFSET(0x32)
#define RMIXL_PCIE_PHY_CR_RD_DATA	_RMIXL_OFFSET(0x33)
#define RMIXL_PCIE_IOBM_ERR_CMD		_RMIXL_OFFSET(0x34)
#define RMIXL_PCIE_IOBM_ERR_LOWER_ADDR	_RMIXL_OFFSET(0x35)
#define RMIXL_PCIE_IOBM_ERR_UPPER_ADDR	_RMIXL_OFFSET(0x36)
#define RMIXL_PCIE_IOBM_ERR_BE		_RMIXL_OFFSET(0x37)
#define RMIXL_PCIE_LINK2_STATE		_RMIXL_OFFSET(0x60)	/* not on XLS408Lite, XLS404Lite */
#define RMIXL_PCIE_LINK3_STATE		_RMIXL_OFFSET(0x61)	/* not on XLS408Lite, XLS404Lite */
#define RMIXL_PCIE_LINK2_MSI_STATUS	_RMIXL_OFFSET(0x64)	/* not on XLS408Lite, XLS404Lite */
#define RMIXL_PCIE_LINK3_MSI_STATUS	_RMIXL_OFFSET(0x65)	/* not on XLS408Lite, XLS404Lite */
#define RMIXL_PCIE_LINK2_MSI_ENABLE	_RMIXL_OFFSET(0x66)	/* not on XLS408Lite, XLS404Lite */
#define RMIXL_PCIE_LINK3_MSI_ENABLE	_RMIXL_OFFSET(0x67)	/* not on XLS408Lite, XLS404Lite */
#define RMIXL_PCIE_LINK2_INT_STATUS0	_RMIXL_OFFSET(0x68)	/* not on XLS408Lite, XLS404Lite */
#define RMIXL_PCIE_LINK3_INT_STATUS0	_RMIXL_OFFSET(0x69)	/* not on XLS408Lite, XLS404Lite */
#define RMIXL_PCIE_LINK2_INT_STATUS1	_RMIXL_OFFSET(0x6a)	/* not on XLS408Lite, XLS404Lite */
#define RMIXL_PCIE_LINK3_INT_STATUS1	_RMIXL_OFFSET(0x6b)	/* not on XLS408Lite, XLS404Lite */
#define RMIXL_PCIE_LINK2_INT_ENABLE0	_RMIXL_OFFSET(0x6c)	/* not on XLS408Lite, XLS404Lite */
#define RMIXL_PCIE_LINK3_INT_ENABLE0	_RMIXL_OFFSET(0x6d)	/* not on XLS408Lite, XLS404Lite */
#define RMIXL_PCIE_LINK2_INT_ENABLE1	_RMIXL_OFFSET(0x6e)	/* not on XLS408Lite, XLS404Lite */
#define RMIXL_PCIE_LINK3_INT_ENABLE1	_RMIXL_OFFSET(0x6f)	/* not on XLS408Lite, XLS404Lite */
#define RMIXL_VC0_POSTED_RX_QUEUE_CTRL	_RMIXL_OFFSET(0x1d2)
#define RMIXL_VC0_POSTED_BUFFER_DEPTH	_RMIXL_OFFSET(0x1ea)
#define RMIXL_PCIE_MSG_TX_THRESHOLD	_RMIXL_OFFSET(0x308)
#define RMIXL_PCIE_MSG_BUCKET_SIZE_0	_RMIXL_OFFSET(0x320)
#define RMIXL_PCIE_MSG_BUCKET_SIZE_1	_RMIXL_OFFSET(0x321)
#define RMIXL_PCIE_MSG_BUCKET_SIZE_2	_RMIXL_OFFSET(0x322)
#define RMIXL_PCIE_MSG_BUCKET_SIZE_3	_RMIXL_OFFSET(0x323)
#define RMIXL_PCIE_MSG_BUCKET_SIZE_4	_RMIXL_OFFSET(0x324)	/* not on XLS408Lite, XLS404Lite */
#define RMIXL_PCIE_MSG_BUCKET_SIZE_5	_RMIXL_OFFSET(0x325)	/* not on XLS408Lite, XLS404Lite */
#define RMIXL_PCIE_MSG_BUCKET_SIZE_6	_RMIXL_OFFSET(0x326)	/* not on XLS408Lite, XLS404Lite */
#define RMIXL_PCIE_MSG_BUCKET_SIZE_7	_RMIXL_OFFSET(0x327)	/* not on XLS408Lite, XLS404Lite */
#define RMIXL_PCIE_MSG_CREDIT_FIRST	_RMIXL_OFFSET(0x380)
#define RMIXL_PCIE_MSG_CREDIT_LAST	_RMIXL_OFFSET(0x3ff)

/*
 * USB General Interface registers
 * these are opffset from REGSPACE selected by __BIT(12) == 1
 *	RMIXL_IOREG_VADDR(RMIXL_IO_DEV_USB_B + reg)
 * see Tables 18-7 and 18-14 in the XLS PRM
 */
#define RMIXL_USB_GEN_CTRL1		0x00
#define RMIXL_USB_GEN_CTRL2		0x04
#define RMIXL_USB_GEN_CTRL3		0x08
#define RMIXL_USB_IOBM_TIMER		0x0C
#define RMIXL_USB_VBUS_TIMER		0x10
#define RMIXL_USB_BYTESWAP_EN		0x14
#define RMIXL_USB_COHERENT_MEM_BASE	0x40
#define RMIXL_USB_COHERENT_MEM_LIMIT	0x44
#define RMIXL_USB_L2ALLOC_MEM_BASE	0x48
#define RMIXL_USB_L2ALLOC_MEM_LIMIT	0x4C
#define RMIXL_USB_READEX_MEM_BASE	0x50
#define RMIXL_USB_READEX_MEM_LIMIT	0x54
#define RMIXL_USB_PHY_STATUS		0xC0
#define RMIXL_USB_INTERRUPT_STATUS	0xC4
#define RMIXL_USB_INTERRUPT_ENABLE	0xC8

/*
 * RMIXL_USB_GEN_CTRL1 bits
 */
#define RMIXL_UG_CTRL1_RESV		__BITS(31,2)
#define RMIXL_UG_CTRL1_HOST_RST		__BIT(1)	/* Resets the Host Controller
							 *  0: reset
							 *  1: normal operation
							 */
#define RMIXL_UG_CTRL1_DEV_RST		__BIT(0)	/* Resets the Device Controller
							 *  0: reset
							 *  1: normal operation
							 */

/*
 * RMIXL_USB_GEN_CTRL2 bits
 */
#define RMIXL_UG_CTRL2_RESa		__BITS(31,20)
#define RMIXL_UG_CTRL2_TX_TUNE_1	__BITS(19,18)	/* Port_1 Transmitter Tuning for High-Speed Operation.
							 *  00: ~-4.5%
							 *  01: Design default
							 *  10: ~+4.5%
							 *  11: ~+9% = Recommended Operating setting
							 */
#define RMIXL_UG_CTRL2_TX_TUNE_0	__BITS(17,16)	/* Port_0 Transmitter Tuning for High-Speed Operation
							 *  11:  Recommended Operating condition
							 */
#define RMIXL_UG_CTRL2_RESb		__BIT(15)
#define RMIXL_UG_CTRL2_WEAK_PDEN	__BIT(14)	/* 500kOhm Pull-Down Resistor on D+ and D- Enable */
#define RMIXL_UG_CTRL2_DP_PULLUP_ESD	__BIT(13)	/* D+ Pull-Up Resistor Enable */
#define RMIXL_UG_CTRL2_ESD_TEST_MODE	__BIT(12)	/* D+ Pull-Up Resistor Control Select */
#define RMIXL_UG_CTRL2_TX_BIT_STUFF_EN_H_1	\
					__BIT(11)	/* Port_1 High-Byte Transmit Bit-Stuffing Enable */
#define RMIXL_UG_CTRL2_TX_BIT_STUFF_EN_H_0	\
					__BIT(10)	/* Port_0 High-Byte Transmit Bit-Stuffing Enable */
#define RMIXL_UG_CTRL2_TX_BIT_STUFF_EN_L_1	\
					__BIT(9)	/* Port_1 Low-Byte Transmit Bit-Stuffing Enable */
#define RMIXL_UG_CTRL2_TX_BIT_STUFF_EN_L_0	\
					__BIT(8)	/* Port_0 Low-Byte Transmit Bit-Stuffing Enable */
#define RMIXL_UG_CTRL2_RESc		__BITS(7,6)
#define RMIXL_UG_CTRL2_LOOPBACK_ENB_1	__BIT(5)	/* Port_1 Loopback Test Enable */
#define RMIXL_UG_CTRL2_LOOPBACK_ENB_0	__BIT(4)	/* Port_0 Loopback Test Enable */
#define RMIXL_UG_CTRL2_DEVICE_VBUS	__BIT(3)	/* VBUS detected (Device mode only) */
#define RMIXL_UG_CTRL2_PHY_PORT_RST_1	__BIT(2)	/* Resets Port_1 of the PHY
							 *  1: normal operation
							 *  0: reset
							 */
#define RMIXL_UG_CTRL2_PHY_PORT_RST_0	__BIT(1)	/* Resets Port_0 of the PHY
							 *  1: normal operation
							 *  0: reset
							 */
#define RMIXL_UG_CTRL2_PHY_RST		__BIT(0)	/* Resets the PHY
							 *  1: normal operation
							 *  0: reset
							 */
#define RMIXL_UG_CTRL2_RESV	\
	(RMIXL_UG_CTRL2_RESa | RMIXL_UG_CTRL2_RESb | RMIXL_UG_CTRL2_RESc)


/*
 * RMIXL_USB_GEN_CTRL3 bits
 */
#define RMIXL_UG_CTRL3_RESa		__BITS(31,11)
#define RMIXL_UG_CTRL3_PREFETCH_SIZE	__BITS(10,8)	/* The pre-fetch size for a memory read transfer
							 * between USB Interface and DI station.
							 * Valid value ranges is from 1 to 4.
							 */
#define RMIXL_UG_CTRL3_RESb		__BIT(7)
#define RMIXL_UG_CTRL3_DEV_UPPERADDR	__BITS(6,1)	/* Device controller address space selector */
#define RMIXL_UG_CTRL3_USB_FLUSH	__BIT(0)	/* Flush the USB interface */

/*
 * RMIXL_USB_PHY_STATUS bits
 */
#define RMIXL_UB_PHY_STATUS_RESV	__BITS(31,1)
#define RMIXL_UB_PHY_STATUS_VBUS	__BIT(0)	/* USB VBUS status */

/*
 * RMIXL_USB_INTERRUPT_STATUS and RMIXL_USB_INTERRUPT_ENABLE bits
 */
#define RMIXL_UB_INTERRUPT_RESV		__BITS(31,6)
#define RMIXL_UB_INTERRUPT_FORCE	__BIT(5)	/* USB force interrupt */
#define RMIXL_UB_INTERRUPT_PHY		__BIT(4)	/* USB PHY interrupt */
#define RMIXL_UB_INTERRUPT_DEV		__BIT(3)	/* USB Device Controller interrupt */
#define RMIXL_UB_INTERRUPT_EHCI		__BIT(2)	/* USB EHCI interrupt */
#define RMIXL_UB_INTERRUPT_OHCI_1	__BIT(1)	/* USB OHCI #1 interrupt */
#define RMIXL_UB_INTERRUPT_OHCI_0	__BIT(0)	/* USB OHCI #0 interrupt */
#define RMIXL_UB_INTERRUPT_MAX		5


/*
 * USB Device Controller registers
 * these are opffset from REGSPACE selected by __BIT(12) == 0
 *	RMIXL_IOREG_VADDR(RMIXL_IO_DEV_USB_A + reg)
 * see Table 18-7 in the XLS PRM
 */
#define RMIXL_USB_UDC_GAHBCFG		0x008	/* UDC Configuration A (UDC_GAHBCFG) */
#define RMIXL_USB_UDC_GUSBCFG		0x00C	/* UDC Configuration B (UDC_GUSBCFG) */
#define RMIXL_USB_UDC_GRSTCTL		0x010	/* UDC Reset */
#define RMIXL_USB_UDC_GINTSTS		0x014	/* UDC Interrupt Register */
#define RMIXL_USB_UDC_GINTMSK		0x018	/* UDC Interrupt Mask Register */
#define RMIXL_USB_UDC_GRXSTSP		0x020	/* UDC Receive Status Read /Pop Register (Read Only) */
#define RMIXL_USB_UDC_GRXFSIZ		0x024	/* UDC Receive FIFO Size Register */
#define RMIXL_USB_UDC_GNPTXFSIZ		0x028	/* UDC Non-periodic Transmit FIFO Size Register */
#define RMIXL_USB_UDC_GUID		0x03C	/* UDC User ID Register (UDC_GUID) */
#define RMIXL_USB_UDC_GSNPSID		0x040	/* UDC ID Register (Read Only) */
#define RMIXL_USB_UDC_GHWCFG1		0x044	/* UDC User HW Config1 Register (Read Only) */
#define RMIXL_USB_UDC_GHWCFG2		0x048	/* UDC User HW Config2 Register (Read Only) */
#define RMIXL_USB_UDC_GHWCFG3		0x04C	/* UDC User HW Config3 Register (Read Only) */
#define RMIXL_USB_UDC_GHWCFG4		0x050	/* UDC User HW Config4 Register (Read Only) */
#define RMIXL_USB_UDC_DPTXFSIZ0		0x104
#define RMIXL_USB_UDC_DPTXFSIZ1		0x108
#define RMIXL_USB_UDC_DPTXFSIZ2		0x10c
#define RMIXL_USB_UDC_DPTXFSIZn(n)	(0x104 + (4 * (n)))
						/* UDC Device IN Endpoint Transmit FIFO-n
						   Size Registers (UDC_DPTXFSIZn) */
#define RMIXL_USB_UDC_DCFG		0x800	/* UDC Configuration C */
#define RMIXL_USB_UDC_DCTL		0x804	/* UDC Control Register */
#define RMIXL_USB_UDC_DSTS		0x808	/* UDC Status Register (Read Only) */
#define RMIXL_USB_UDC_DIEPMSK		0x810	/* UDC Device IN Endpoint Common
						   Interrupt Mask Register (UDC_DIEPMSK) */
#define RMIXL_USB_UDC_DOEPMSK		0x814	/* UDC Device OUT Endpoint Common Interrupt Mask register */
#define RMIXL_USB_UDC_DAINT		0x818	/* UDC Device All Endpoints Interrupt Register */
#define RMIXL_USB_UDC_DAINTMSK		0x81C	/* UDC Device All Endpoints Interrupt Mask Register */
#define RMIXL_USB_UDC_DTKNQR3		0x830	/* Device Threshold Control Register */
#define RMIXL_USB_UDC_DTKNQR4		0x834	/* Device IN Endpoint FIFO Empty Interrupt Mask Register */
#define RMIXL_USB_UDC_DIEPCTL		0x900	/* Device Control IN Endpoint 0 Control Register */
#define RMIXL_USB_UDC_DIEPINT		0x908	/* Device IN Endpoint 0 Interrupt Register */
#define RMIXL_USB_UDC_DIEPTSIZ		0x910	/* Device IN Endpoint 0 Transfer Size Register */
#define RMIXL_USB_UDC_DIEPDMA		0x914	/* Device IN Endpoint 0 DMA Address Register */
#define RMIXL_USB_UDC_DTXFSTS		0x918	/* Device IN Endpoint Transmit FIFO Status Register */
#define RMIXL_USB_DEV_IN_ENDPT(d,n)	(0x920 + ((d) * 0x20) + ((n) * 4))
						/* Device IN Endpoint #d Register #n */

/*
 * USB Host Controller register base addrs
 * these are offset from REGSPACE selected by __BIT(12) == 0
 *	RMIXL_IOREG_VADDR(RMIXL_IO_DEV_USB_A + reg)
 * see Table 18-14 in the XLS PRM
 * specific Host Controller is selected by __BITS(11,10)
 */
#define RMIXL_USB_HOST_EHCI_BASE	0x000
#define RMIXL_USB_HOST_0HCI0_BASE	0x400
#define RMIXL_USB_HOST_0HCI1_BASE	0x800
#define RMIXL_USB_HOST_RESV		0xc00
#define RMIXL_USB_HOST_MASK		0xc00


#endif	/* _MIPS_RMI_RMIRMIXLEGS_H_ */

