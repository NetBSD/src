/*	$NetBSD: rmixlreg.h,v 1.1.2.17 2011/12/31 04:30:52 matt Exp $	*/

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


#ifndef _MIPS_RMI_RMIXLREG_H_
#define _MIPS_RMI_RMIXLREG_H_

#include <sys/endian.h>

/*
 * on chip I/O register byte order is
 * BIG ENDIAN regardless of code model
 */
#define RMIXL_IOREG_VADDR(o)				\
	(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(	\
		rmixl_configuration.rc_io.r_pbase + (o))
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
/*
 * Note CP2 FMN register scope or "context"
 *	L   : Local		: per thread register
 *	G   : Global       	: per FMN Station (per core) register
 *	L/G : "partly global"	: ???
 * Global regs should be managed by a single thread
 * (see XLS PRM "Coprocessor 2 Register Summary")
 */
					/*		context ---------------+	*/
					/*		#sels --------------+  |	*/
					/*		#regs -----------+  |  |	*/
					/* What:	#bits --+	 |  |  |	*/
					/*			v	 v  v  v	*/
#define RMIXL_COP_2_TXBUF	_(0)	/* Transmit Buffers	64	[1][4] L	*/
#define RMIXL_COP_2_RXBUF	_(1)	/* Receive Buffers	64	[1][4] L	*/
#define RMIXL_COP_2_MSG_STS	_(2)	/* Mesage Status	32	[1][2] L/G	*/
#define RMIXL_COP_2_MSG_CFG	_(3)	/* MEssage Config	32	[1][2] G	*/
#define RMIXL_COP_2_MSG_BSZ	_(4)	/* Message Bucket Size	32	[1][8] G	*/
#define RMIXL_COP_2_CREDITS	_(16)	/* Credit Counters	 8     [16][8] G	*/

/*
 * MsgStatus: RMIXL_COP_2_MSG_STS (CP2 Reg 2, Select 0) bits
 */
#define RMIXL_MSG_STS0_RFBE		__BITS(31,24)	/* RX FIFO Buckets bit mask
							 *  0=not empty
							 *  1=empty
							 */
#define RMIXL_MSG_STS0_RFBE_SHIFT	24
#define RMIXL_MSG_STS0_RESV		__BIT(23)
#define RMIXL_MSG_STS0_RMSID		__BITS(22,16)	/* Source ID */
#define RMIXL_MSG_STS0_RMSID_SHIFT	16
#define RMIXL_MSG_STS0_RMSC		__BITS(15,8)	/* RX Message Software Code */
#define RMIXL_MSG_STS0_RMSC_SHIFT	8
#define RMIXL_MSG_STS0_RMS		__BITS(7,6)	/* RX Message Size (minus 1) */
#define RMIXL_MSG_STS0_RMS_SHIFT	6
#define RMIXL_MSG_STS0_LEF		__BIT(5)	/* Load Empty Fail */
#define RMIXL_MSG_STS0_LPF		__BIT(4)	/* Load Pending Fail */
#define RMIXL_MSG_STS0_LMP		__BIT(3)	/* Load Message Pending */
#define RMIXL_MSG_STS0_SCF		__BIT(2)	/* Send Credit Fail */
#define RMIXL_MSG_STS0_SPF		__BIT(1)	/* Send Pending Fail */
#define RMIXL_MSG_STS0_SMP		__BIT(0)	/* Send Message Pending */
#define RMIXL_MSG_STS0_ERRS	\
		(RMIXL_MSG_STS0_LEF|RMIXL_MSG_STS0_LPF|RMIXL_MSG_STS0_LMP \
		|RMIXL_MSG_STS0_SCF|RMIXL_MSG_STS0_SPF|RMIXL_MSG_STS0_SMP)
 
/*
 * MsgStatus1: RMIXL_COP_2_MSG_STS (CP2 Reg 2, Select 1) bits
 */
#define RMIXL_MSG_STS1_RESV		__BIT(31)
#define RMIXL_MSG_STS1_C		__BIT(30)	/* Credit Overrun Error */
#define RMIXL_MSG_STS1_CCFCME		__BITS(29,23)	/* Credit Counter of Free Credit Message with Error */
#define RMIXL_MSG_STS1_CCFCME_SHIFT	23
#define RMIXL_MSG_STS1_SIDFCME		__BITS(22,16)	/* Source ID of Free Credit Message with Error */
#define RMIXL_MSG_STS1_SIDFCME_SHIFT	16
#define RMIXL_MSG_STS1_T		__BIT(15)	/* Invalid Target Error */
#define RMIXL_MSG_STS1_F		__BIT(14)	/* Receive Queue "Write When Full" Error */
#define RMIXL_MSG_STS1_SIDE		__BITS(13,7)	/* Source ID of incoming msg with Error */
#define RMIXL_MSG_STS1_SIDE_SHIFT	7
#define RMIXL_MSG_STS1_DIDE		__BITS(6,0)	/* Destination ID of the incoming message Message with Error */
#define RMIXL_MSG_STS1_DIDE_SHIFT	0
#define RMIXL_MSG_STS1_ERRS	\
		(RMIXL_MSG_STS1_C|RMIXL_MSG_STS1_T|RMIXL_MSG_STS1_F)

/*
 * MsgConfig: RMIXL_COP_2_MSG_CFG (CP2 Reg 3, Select 0) bits
 */
#define RMIXL_MSG_CFG0_WM		__BITS(31,24)	/* Watermark level */
#define RMIXL_MSG_CFG0_WMSHIFT		24
#define RMIXL_MSG_CFG0_RESa		__BITS(23,22)
#define RMIXL_MSG_CFG0_IV		__BITS(21,16)	/* Interrupt Vector */
#define RMIXL_MSG_CFG0_IV_SHIFT		16
#define RMIXL_MSG_CFG0_RESb		__BITS(15,12)
#define RMIXL_MSG_CFG0_ITM		__BITS(11,8)	/* Interrupt Thread Mask */
#define RMIXL_MSG_CFG0_ITM_SHIFT	8
#define RMIXL_MSG_CFG0_RESc		__BITS(7,2)
#define RMIXL_MSG_CFG0_WIE		__BIT(1)	/* Watermark Interrupt Enable */
#define RMIXL_MSG_CFG0_EIE		__BIT(0)	/* Receive Queue Not Empty Enable */
#define RMIXL_MSG_CFG0_RESV	\
		(RMIXL_MSG_CFG0_RESa|RMIXL_MSG_CFG0_RESb|RMIXL_MSG_CFG0_RESc)

/*
 * MsgConfig1: RMIXL_COP_2_MSG_CFG (CP2 Reg 3, Select 1) bits
 * Note: reg width is 64 bits in PRM reg description, but 32 bits in reg summary
 */
#define RMIXL_MSG_CFG1_RESV		__BITS(63,3)
#define RMIXL_MSG_CFG1_T		__BIT(2)	/* Trace Mode Enable */
#define RMIXL_MSG_CFG1_C		__BIT(1)	/* Credit Over-run Interrupt Enable */
#define RMIXL_MSG_CFG1_M		__BIT(0)	/* Messaging Errors Interrupt Enable */


/*
 * MsgBucketSize: RMIXL_COP_2_MSG_BSZ (CP2 Reg 4, Select [0..7]) bits
 * Note: reg width is 64 bits in PRM reg description, but 32 bits in reg summary
 * Size:
 * - 0 means bucket disabled, else
 * - must be power of 2
 * - must be >=4
 */
#define RMIXL_MSG_BSZ_RESV		__BITS(63,8)
#define RMIXL_MSG_BSZ_SIZE		__BITS(7,0)




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

/* XLP Instruction Fetch Unit Registers */
#define RMIXLP_PCR_IFU_THREAD_EN		0x0000
#define RMIXLP_PCR_IFU_SW_SLEEP			0x0001
#define RMIXLP_PCR_IFU_THREAD_SCHED_MODE	0x0002
#define RMIXLP_PCR_IFU_THREAD_SCHED_COUNTER	0x0003
#define RMIXLP_PCR_IFU_BHR_PROG_MASK		0x0004
#define RMIXLP_PCR_IFU_SLEEP_STATE		0x0006
#define RMIXLP_PCR_IFU_BRUB_RESERVE		0x0007

/* XLP Instruction Cache Unit Registers */
#define	RMIXLP_PCR_ICU_DEFEATURE		0x0100
#define	RMIXLP_PCR_ICU_CACHE_ERR_INT		0x0101	/* RW1C */
#define	RMIXLP_PCR_ICU_ERR_LOG0			0x0110
#define	RMIXLP_PCR_ICU_ERR_LOG1			0x0111
#define	RMIXLP_PCR_ICU_ERR_LOG2			0x0112
#define	RMIXLP_PCR_ICU_ERR_INJECT0		0x0113
#define	RMIXLP_PCR_ICU_ERR_INJECT1		0x0114

/* XLP Load Store Unit Registers */
#define	RMIXLP_PCR_LSU_CONFIG0			0x0300
#define	RMIXLP_PCR_LSU_CONFIG1			0x0301
#define	RMIXLP_PCR_LSU_DEFEATURE		0x0304
#define	RMIXLP_PCR_LSU_DEBUG_ADDR		0x0305
#define	RMIXLP_PCR_LSU_DEBUG_DATA		0x0306
#define	RMIXLP_PCR_LSU_CERR_LOG0		0x0308
#define	RMIXLP_PCR_LSU_CERR_LOG1		0x0309
#define	RMIXLP_PCR_LSU_CERR_INJ0		0x030a
#define	RMIXLP_PCR_LSU_CERR_INJ1		0x030b
#define	RMIXLP_PCR_LSU_CERR_INT			0x030c

#define	RMIXLP_PCR_LSE_DEFEATURE_EUL		__BIT(30)

/* XLP MMU Registers */
#define	RMIXLP_PCR_MMU_SETUP			0x0400
#define	RMIXLP_PCR_LFSRSEED			0x0401
#define	RMIXLP_PCR_HPW_NUM_PAGE_LVL		0x0410
#define	RMIXLP_PCR_PGWKR_PGDBASE		0x0411
#define	RMIXLP_PCR_PGWKR_PGDSHIFT		0x0412
#define	RMIXLP_PCR_PGWKR_PGDMASK		0x0413
#define	RMIXLP_PCR_PGWKR_PUDSHIFT		0x0414
#define	RMIXLP_PCR_PGWKR_PUDMASK		0x0415
#define	RMIXLP_PCR_PGWKR_PMDSHIFT		0x0416
#define	RMIXLP_PCR_PGWKR_PMDMASK		0x0417
#define	RMIXLP_PCR_PGWKR_PTESHIFT		0x0418
#define	RMIXLP_PCR_PGWKR_PTEMASK		0x0419

#define	RMIXLP_PCR_MMU_SETUP_HASHFUNCTIONEN	__BIT(13)
#define	RMIXLP_PCR_MMU_SETUP_LOCCLKGATE		__BIT(3)
#define	RMIXLP_PCR_MMU_SETUP_TLB_GLOBAL		__BIT(0)
#define	RMIXLP_PCR_PGWKR_PxxSHIFT_MASK		__BITS(5,0)
#define	RMIXLP_PCR_PGWKR_PxxMASK_MASK		__BITS(31,0)

/* XLP L2 Cache Registers */
#define	RMIXLP_PCR_L2_FTR_CTL0			0x800
#define	RMIXLP_PCR_L2_FTR_CTL1			0x801
#define	RMIXLP_PCR_L2_CRERR_INT_VID		0x802
#define	RMIXLP_PCR_L2_DIS_WAY			0x803
#define	RMIXLP_PCR_L2_ERR_LOG0			0x810
#define	RMIXLP_PCR_L2_ERR_LOG1			0x811
#define	RMIXLP_PCR_L2_ERR_LOG2			0x812
#define	RMIXLP_PCR_L2_ERR_INJ0			0x813
#define	RMIXLP_PCR_L2_ERR_INJ1			0x814

/* XLP Mapping Unit Registers */
#define	RMIXLP_PCR_MAP_T0_LRQ_MASK		0x0602
#define	RMIXLP_PCR_MAP_T1_LRQ_MASK		0x0603
#define	RMIXLP_PCR_MAP_T2_LRQ_MASK		0x0604
#define	RMIXLP_PCR_MAP_T3_LRQ_MASK		0x0605
#define	RMIXLP_PCR_MAP_T0_SRQ_MASK		0x0606
#define	RMIXLP_PCR_MAP_T1_SRQ_MASK		0x0607
#define	RMIXLP_PCR_MAP_T2_SRQ_MASK		0x0608
#define	RMIXLP_PCR_MAP_T3_SRQ_MASK		0x0609
#define	RMIXLP_PCR_MAP_THREAD_MODE		0x0a00
#define	RMIXLP_PCR_MAP_EXT_EBASE_ENABLE		0x0a02
#define	RMIXLP_PCR_MAP_CCD_CONFIG		0x0a02
#define	RMIXLP_PCR_MAP_T0_DEBUG_MODE		0x0a03
#define	RMIXLP_PCR_MAP_T1_DEBUG_MODE		0x0a04
#define	RMIXLP_PCR_MAP_T2_DEBUG_MODE		0x0a05
#define	RMIXLP_PCR_MAP_T3_DEBUG_MODE		0x0a06
#define	RMIXLP_PCR_MAP_THREAD_STATE		0x0a10
#define	RMIXLP_PCR_MAP_T0_CCD_STATUS		0x0a11
#define	RMIXLP_PCR_MAP_T1_CCD_STATUS		0x0a12
#define	RMIXLP_PCR_MAP_T2_CCD_STATUS		0x0a13
#define	RMIXLP_PCR_MAP_T3_CCD_STATUS		0x0a14

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
#define RMIXL_IO_DEV_PCIX_EL	\
	RMIXL_IO_DEV_PCIX		/* PXI-X little endian */
#define RMIXL_IO_DEV_PCIX_EB	\
	(RMIXL_IO_DEV_PCIX | __BIT(11))	/* PXI-X big endian */
#define RMIXL_IO_DEV_HT		0x0a000	/* HyperTransport */
#endif	/* MIPS64_XLR */
#define RMIXL_IO_DEV_SAE	0x0b000	/* Security Acceleration Engine */
#if defined(MIPS64_XLS)
#define XAUI_INTERFACE_0	0x0c000	/* XAUI Interface_0 */
					/*  when SGMII Interface_[0-3] are not used */
#define RMIXL_IO_DEV_GMAC_0	0x0c000	/* SGMII-Interface_0, Port SGMII0 */
#define RMIXL_IO_DEV_GMAC_1	0x0d000	/* SGMII-Interface_1, Port SGMII1 */
#define RMIXL_IO_DEV_GMAC_2	0x0e000	/* SGMII-Interface_2, Port SGMII2 */
#define RMIXL_IO_DEV_GMAC_3	0x0f000	/* SGMII-Interface_3, Port SGMII3 */
#endif	/* MIPS64_XLS */
#if defined(MIPS64_XLR)
#define RMIXL_IO_DEV_GMAC_A	0x0c000	/* RGMII-Interface_0, Port RA */
#define RMIXL_IO_DEV_GMAC_B	0x0d000	/* RGMII-Interface_1, Port RB */
#define RMIXL_IO_DEV_GMAC_C	0x0e000	/* RGMII-Interface_2, Port RC */
#define RMIXL_IO_DEV_GMAC_D	0x0f000	/* RGMII-Interface_3, Port RD */
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
#define RMIXL_IO_DEV_CDE	0x1d000	/* Compression/Decompression Engine */
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
#define _RMIXL_PCITAG(b,d,f)	((((((b) << 5) | (d)) << 3) | (f)) << 12)
#define	_RMIXL_PCITAG_BUS(t)	(((t) >> 20) & 255)
#define	_RMIXL_PCITAG_DEV(t)	(((t) >> 15) & 31)
#define	_RMIXL_PCITAG_FUNC(t)	(((t) >> 12) & 7)
#define	_RMIXL_PCITAG_OFFSET(t)	(((t) >>  0) & 4095)


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
#define RMIXL_SBC_IO_BAR		_RMIXL_OFFSET(0x19)	/* I/O Config Base Addr reg */
#define RMIXL_SBC_FLASH_BAR		_RMIXL_OFFSET(0x1a)	/* Flash Memory Base Addr reg */
#if defined(MIPS64_XLR)
#define RMIXLR_SBC_SRAM_BAR		_RMIXL_OFFSET(0x1b)	/* SRAM Base Addr reg */
#define RMIXLR_SBC_HTMEM_BAR		_RMIXL_OFFSET(0x1c)	/* HyperTransport Mem Base Addr reg */
#define RMIXLR_SBC_HTINT_BAR		_RMIXL_OFFSET(0x1d)	/* HyperTransport Interrupt Base Addr reg */
#define RMIXLR_SBC_HTPIC_BAR		_RMIXL_OFFSET(0x1e)	/* HyperTransport Legacy PIC Base Addr reg */
#define RMIXLR_SBC_HTSM_BAR		_RMIXL_OFFSET(0x1f)	/* HyperTransport System Management Base Addr reg */
#define RMIXLR_SBC_HTIO_BAR		_RMIXL_OFFSET(0x20)	/* HyperTransport IO Base Addr reg */
#define RMIXLR_SBC_HTCFG_BAR		_RMIXL_OFFSET(0x21)	/* HyperTransport Configuration Base Addr reg */
#define RMIXLR_SBC_PCIX_CFG_BAR		_RMIXL_OFFSET(0x22)	/* PCI-X Configuration Base Addr reg */
#define RMIXLR_SBC_PCIX_MEM_BAR		_RMIXL_OFFSET(0x23)	/* PCI-X Mem Base Addr reg */
#define RMIXLR_SBC_PCIX_IO_BAR		_RMIXL_OFFSET(0x24)	/* PCI-X IO Base Addr reg */
#define RMIXLR_SBC_SYS2IO_CREDITS	_RMIXL_OFFSET(0x35)	/* System Bridge I/O Transaction Credits register */
#endif	/* MIPS64_XLR */
#if defined(MIPS64_XLS)
#define RMIXLS_SBC_PCIE_CFG_BAR		_RMIXL_OFFSET(0x40)	/* PCI Configuration BAR */
#define RMIXLS_SBC_PCIE_ECFG_BAR	_RMIXL_OFFSET(0x41)	/* PCI Extended Configuration BAR */
#define RMIXLS_SBC_PCIE_MEM_BAR		_RMIXL_OFFSET(0x42)	/* PCI Memory region BAR */
#define RMIXLS_SBC_PCIE_IO_BAR		_RMIXL_OFFSET(0x43)	/* PCI IO region BAR */
#endif	/* MIPS64_XLS */

/*
 * Address Error registers
 * offsets are relative to RMIXL_IO_DEV_BRIDGE
 */
#define RMIXL_ADDR_ERR_DEVICE_MASK	_RMIXL_OFFSET(0x25)	/* Address Error Device Mask */
#define RMIXL_ADDR_ERR_DEVICE_MASK_2	_RMIXL_OFFSET(0x44)	/* extension of Device Mask */
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
 * RMIXL_SBC_FLASH_BAR bit defines
 */
#define RMIXL_FLASH_BAR_BASE		__BITS(31,16)	/* phys address bits 39:24 */
#define RMIXL_FLASH_BAR_TO_BA(r)	\
		(((r) & RMIXL_FLASH_BAR_BASE) << (24 - 16))
#define RMIXL_FLASH_BAR_MASK		__BITS(15,5)	/* phys address mask bits 34:24 */
#define RMIXL_FLASH_BAR_TO_MASK(r)	\
		(((((r) & RMIXL_FLASH_BAR_MASK)) << (24 - 5)) | __BITS(23, 0))
#define RMIXL_FLASH_BAR_RESV		__BITS(4,1)	/* (reserved) */
#define RMIXL_FLASH_BAR_ENB		__BIT(0)	/* 1=Enable */
#define RMIXL_FLASH_BAR_MASK_MAX	RMIXL_FLASH_BAR_TO_MASK(RMIXL_FLASH_BAR_MASK)

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
 * RMIXL_SBC_XLR_PCIX_CFG_BAR bit defines
 */
#define RMIXL_PCIX_CFG_BAR_BASE		__BITS(31,17)	/* phys address bits 39:25 */
#define RMIXL_PCIX_CFG_BAR_BA_SHIFT	(25 - 17)
#define RMIXL_PCIX_CFG_BAR_TO_BA(r)	\
		(((r) & RMIXL_PCIX_CFG_BAR_BASE) << RMIXL_PCIX_CFG_BAR_BA_SHIFT)
#define RMIXL_PCIX_CFG_BAR_RESV		__BITS(16,1)	/* (reserved) */
#define RMIXL_PCIX_CFG_BAR_ENB		__BIT(0)	/* 1=Enable */
#define RMIXL_PCIX_CFG_SIZE		__BIT(25)
#define RMIXL_PCIX_CFG_BAR(ba, en)	\
		((uint32_t)(((ba) >> (25 - 17)) | ((en) ? RMIXL_PCIX_CFG_BAR_ENB : 0)))

/*
 * RMIXLR_SBC_PCIX_MEM_BAR bit defines
 */
#define RMIXL_PCIX_MEM_BAR_BASE		__BITS(31,16)	/* phys address bits 39:24 */
#define RMIXL_PCIX_MEM_BAR_TO_BA(r)	\
		(((r) & RMIXL_PCIX_MEM_BAR_BASE) << (24 - 16))
#define RMIXL_PCIX_MEM_BAR_MASK		__BITS(15,1)	/* phys address mask bits 38:24 */
#define RMIXL_PCIX_MEM_BAR_TO_SIZE(r)	\
		((((r) & RMIXL_PCIX_MEM_BAR_MASK) + 2) << (24 - 1))
#define RMIXL_PCIX_MEM_BAR_ENB		__BIT(0)	/* 1=Enable */
#define RMIXL_PCIX_MEM_BAR(ba, en)	\
		((uint32_t)(((ba) >> (24 - 16)) | ((en) ? RMIXL_PCIX_MEM_BAR_ENB : 0)))

/*
 * RMIXLR_SBC_PCIX_IO_BAR bit defines
 */
#define RMIXL_PCIX_IO_BAR_BASE		__BITS(31,18)	/* phys address bits 39:26 */
#define RMIXL_PCIX_IO_BAR_TO_BA(r)	\
		(((r) & RMIXL_PCIX_IO_BAR_BASE) << (26 - 18))
#define RMIXL_PCIX_IO_BAR_RESV		__BITS(17,7)	/* (reserve) */
#define RMIXL_PCIX_IO_BAR_MASK		__BITS(6,1)	/* phys address mask bits 31:26 */
#define RMIXL_PCIX_IO_BAR_TO_SIZE(r)	\
		((((r) & RMIXL_PCIX_IO_BAR_MASK) + 2) << (26 - 1))
#define RMIXL_PCIX_IO_BAR_ENB		__BIT(0)	/* 1=Enable */
#define RMIXL_PCIX_IO_BAR(ba, en)	\
		((uint32_t)(((ba) >> (26 - 18)) | ((en) ? RMIXL_PCIX_IO_BAR_ENB : 0)))


/*
 * RMIXLS_SBC_PCIE_CFG_BAR bit defines
 */
#define RMIXL_PCIE_CFG_BAR_BASE	__BITS(31,17)	/* phys address bits 39:25 */
#define RMIXL_PCIE_CFG_BAR_BA_SHIFT	(25 - 17)
#define RMIXL_PCIE_CFG_BAR_TO_BA(r)	\
		(((r) & RMIXL_PCIE_CFG_BAR_BASE) << RMIXL_PCIE_CFG_BAR_BA_SHIFT)
#define RMIXL_PCIE_CFG_BAR_RESV		__BITS(16,1)	/* (reserved) */
#define RMIXL_PCIE_CFG_BAR_ENB		__BIT(0)	/* 1=Enable */
#define RMIXL_PCIE_CFG_SIZE		__BIT(25)
#define RMIXL_PCIE_CFG_BAR(ba, en)	\
		((uint32_t)(((ba) >> (25 - 17)) | ((en) ? RMIXL_PCIE_CFG_BAR_ENB : 0)))

/*
 * RMIXLS_SBC_PCIE_ECFG_BAR bit defines
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
 * RMIXLS_SBC_PCIE_MEM_BAR bit defines
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
 * RMIXLS_SBC_PCIE_IO_BAR bit defines
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
#define	RMIXL_PIC_PCITAG		_RMIXL_PCITAG(0, 0, 4)
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
#define RMIXL_PIC_CONTROL_TIMER_ENBn(n)	((1 << (8 + (n))) & RMIXL_PIC_CONTROL_TIMER_ENB)
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
#define RMIXL_PIC_IPIBASE_ID_CORE	__BITS(22,20)	/* Physical CPU ID */
#define RMIXL_PIC_IPIBASE_ID_CORE_SHIFT		20
#define RMIXL_PIC_IPIBASE_ID_RESc	__BITS(19,18)
#define RMIXL_PIC_IPIBASE_ID_THREAD	__BITS(17,16)	/* Thread ID */
#define RMIXL_PIC_IPIBASE_ID_THREAD_SHIFT	16
#define RMIXL_PIC_IPIBASE_ID_RESV	\
		(RMIXL_PIC_IPIBASE_ID_RESa|RMIXL_PIC_IPIBASE_ID_RESb	\
		|RMIXL_PIC_IPIBASE_ID_RESc)
#define	RMIXL_PIC_IPIBASE_MAKE(nmi, core, thread, tag)		\
	(__SHIFTIN((nmi), RMIXL_PIC_IPIBASE_NMI)		\
	 | __SHIFTIN((core), RMIXL_PIC_IPIBASE_ID_CORE)		\
	 | __SHIFTIN((thread), RMIXL_PIC_IPIBASE_ID_THREAD)	\
	 | __SHIFTIN((tag), RMIXL_PIC_IPIBASE_VECTORNUM))

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
 * RMI XLP PIC registers (all are 64-bit except when noted)
 */
#define	RMIXLP_PIC_CTRL			_RMIXL_OFFSET(0x40)
#define	RMIXLP_PIC_BYTESWAP		_RMIXL_OFFSET(0x42)
#define	RMIXLP_PIC_STATUS		_RMIXL_OFFSET(0x44)
#define	RMIXLP_PIC_INT_TIMEOUT		_RMIXL_OFFSET(0x46)
#define	RMIXLP_PIC_ICI0_INT_TIMEOUT	_RMIXL_OFFSET(0x48)
					/* nothing at 0x4a */
#define	RMIXLP_PIC_IPI_CTRL		_RMIXL_OFFSET(0x4e)
#define	RMIXLP_PIC_INT_ACK		_RMIXL_OFFSET(0x50)
#define	RMIXLP_PIC_INT_PENDING0		_RMIXL_OFFSET(0x52) /* IRT 0..63 */
#define	RMIXLP_PIC_INT_PENDING1		_RMIXL_OFFSET(0x54) /* IRT 64..127 */
#define	RMIXLP_PIC_INT_PENDING2		_RMIXL_OFFSET(0x56) /* IRT 128..160 */
#define	RMIXLP_PIC_WATCHDOG0_MAXVAL	_RMIXL_OFFSET(0x58)
#define	RMIXLP_PIC_WATCHDOG0_COUNT	_RMIXL_OFFSET(0x5a)
#define	RMIXLP_PIC_WATCHDOG0_ENABLE0	_RMIXL_OFFSET(0x5c)
					/* nothing at 0x5e */
#define	RMIXLP_PIC_WATCHDOG0_BEATCMD	_RMIXL_OFFSET(0x60)
#define	RMIXLP_PIC_WATCHDOG0_BEAT0	_RMIXL_OFFSET(0x62)
#define	RMIXLP_PIC_WATCHDOG0_BEAT1	_RMIXL_OFFSET(0x64)
#define	RMIXLP_PIC_WATCHDOG1_MAXVAL	_RMIXL_OFFSET(0x66)
#define	RMIXLP_PIC_WATCHDOG1_COUNT	_RMIXL_OFFSET(0x68)
#define	RMIXLP_PIC_WATCHDOG1_ENABLE	_RMIXL_OFFSET(0x6a)
					/* nothing at 0x6c */
#define	RMIXLP_PIC_WATCHDOG1_BEATCMD	_RMIXL_OFFSET(0x6e)
#define	RMIXLP_PIC_WATCHDOG1_BEAT	_RMIXL_OFFSET(0x70)
					/* nothing at 0x72 */
#define	RMIXLP_PIC_SYSTEMTIMER_MAXVALUE(n)	_RMIXL_OFFSET(0x74+2*(n))
#define	RMIXLP_PIC_SYSTEMTIMER_COUNT(n)	_RMIXL_OFFSET(0x84+2*(n))
#define	RMIXLP_PIC_INT_THREAD_ENABLE01(n) _RMIXL_OFFSET(0x94+4*(n))
#define	RMIXLP_PIC_INT_THREAD_ENABLE23(n) _RMIXL_OFFSET(0x96+4*(n))
#define	RMIXLP_PIC_IRTENTRY(n)		_RMIXL_OFFSET(0xb4+2*(n))
#define	RMIXLP_PIC_INT_BROADCAST_ENABLE	_RMIXL_OFFSET(0x292)	/* 32-bit */
#define	RMIXLP_PIC_INT_GPIO_PENDING	_RMIXL_OFFSET(0x293)	/* 32-bit */

/*
 * RMIXLP_PIC_CTRL bits
 */
#define	RMIXLP_PIC_CTRL_ITV	__BITS(64,32)	/* Interrupt Timeout Value */
#define	RMIXLP_PIC_CTRL_STE	__BITS(17,10)	/* System Timer Enable */
#define	RMIXLP_PIC_CTRL_WWR1	__BITS(9,8)	/* Watchdog Wraparound Reset1 */
#define	RMIXLP_PIC_CTRL_WWR0	__BITS(7,6)	/* Watchdog Wraparound Reset0 */
#define	RMIXLP_PIC_CTRL_WWN1	__BITS(5,4)	/* Watchdog Wraparound NMI1 */
#define	RMIXLP_PIC_CTRL_WWN0	__BITS(3,2)	/* Watchdog Wraparound NMI0 */
#define	RMIXLP_PIC_CTRL_WTE	__BITS(1,0)	/* Watchdog Timer Enable */
#define	RMIXLP_PIC_CTRL_WTE1	__BIT(1)	/* Watchdog Timer 1 Enable */
#define	RMIXLP_PIC_CTRL_WTE0	__BIT(0)	/* Watchdog Timer 0 Enable */

/*
 * RMIXLP_PIC_STATUS bits
 */
#define	RMIXLP_PIC_STATUS_ITE	__BIT(32)	/* Interrupt Timeout */
#define	RMIXLP_PIC_STATUS_STS	__BITS(11,4)	/* SystemTimer */
#define	RMIXLP_PIC_STATUS_WNS	__BITS(3,2)	/* Watchdog NMI Interrupt */
#define	RMIXLP_PIC_STATUS_WIS	__BITS(1,0)	/* Watchdog Interrupt */

/*
 * RMIXLP_PIC_INT_TIMEOUT and RMIXLP_PIC_ICI0_INT_TIMEOUT bits
 */
#define	RMIXLP_PIC_IPI_TIMEOUT_INTPEND		__BITS(51,36)	/* ?? */
#define	RMIXLP_PIC_IPI_TIMEOUT_INTNUM		__BITS(35,28)	/* IRT # */
#define	RMIXLP_PIC_IPI_TIMEOUT_INTEN		__BIT(27)	/* Int Enable */
#define	RMIXLP_PIC_IPI_TIMEOUT_INTVEC		__BITS(25,20)	/* Int Vector */
#define	RMIXLP_PIC_IPI_TIMEOUT_INTCPU		__BITS(19,16)	/* Dest CPU */
#define	RMIXLP_PIC_IPI_TIMEOUT_INTDEST		__BITS(15,0)	/* Dest */

/*
 * RMIXLP_PIC_IPI_CTRL bits
 */
#define	RMIXLP_PIC_IPI_CTRL_NMI		__BIT(32)	/* 1=NMI; 0=Maskable */
#define	RMIXLP_PIC_IPI_CTRL_RIV		__BITS(25,20)	/* Which bit in EIRR */
#define	RMIXLP_PIC_IPI_CTRL_DT		__BITS(15,0)	/* Dest Thread Enbs */
#define	RMIXLP_PIC_IPI_CTRL_MAKE(nmi, tmask, tag)		\
	(__SHIFTIN((nmi), RMIXLP_PIC_IPI_CTRL_NMI)		\
	 | __SHIFTIN((tag), RMIXL_PIC_IPI_CTRL_RIV)		\
	 | __SHIFTIN((tmask), RMIXLP_PIC_IPI_CTRL_DT))

/*
 * RMIXLP_PIC_INT_ACK bits
 */
#define	RMIXLP_PIC_INT_ACK_THREAD	__BITS(11,8)	/* Thr # if PicIntBrd */
#define	RMIXLP_PIC_INT_ACK_ACK		__BITS(7,0)	/* IRT # */

/*
 * RMIXLP_WATCHDOG_BEATCMD
 *
 * write 32 * node + 4 * cpu + thread (e.g. cpu_id) to set heartbeat.
 */

/*
 * RMIXLP_PIC_INT_THREAD_ENABLE bits
 */
#define	RMIXLP_PIC_INT_ITE	__BITS(15,0)

/*
 * RMIXLP_PIC_IRTENTRY bits
 */

/* bits 63-32 are reserved */
#define	RMIXLP_PIC_IRTENTRY_EN		__BIT(31)	/* 1=Enable; 0=Disable */
#define	RMIXLP_PIC_IRTENTRY_NMI		__BIT(29)	/* 1=NMI; 0=Maskable */
#define	RMIXLP_PIC_IRTENTRY_LOCAL	__BIT(28)	/* 1=Local; 0=Global */
#define RMIXLP_PIC_IRTENTRY_INTVEC	__BITS(25,20)	/* maps to bit# in CPU's EIRR */
#define	RMIXLP_PIC_IRTENTRY_DT		__BIT(19)	/* 1=ID; 0=ITE */
#define	RMIXLP_PIC_IRTENTRY_DT_ID	__SHIFTIN(1, RMIXLP_PIC_IRTENTRY_DT)
#define	RMIXLP_PIC_IRTENTRY_DT_ITE	__SHIFTIN(0, RMIXLP_PIC_IRTENTRY_DT)
#define RMIXLP_PIC_IRTENTRY_DB		__BITS(18,16)	/* NodeId/CpuID[2]; ITE# */
#define	RMIXLP_PIC_IRTENTRY_ITE(n)	__SHIFTIN((n), RMIXLP_PIC_IRTENTRY_DB)
#define RMIXLP_PIC_IRTENTRY_DTE		__BITS(15,0)	/* Destination Thread Enables */

/*
 * RMIXLP_PIC_INT_BROADCAST_ENABLE bits
 */
#define	RMIXLP_PIC_INT_BROADCAST_ENABLE_PICINTBCMOD	__BITS(27,16)
#define	RMIXLP_PIC_INT_BROADCAST_ENABLE_PICINTBCEN	__BITS(11,0)

/*
 * RMIXLP_PIC_INT_GPIO_PENDING bits
 */
#define	RMIXLP_PIC_INT_GPIO_PENDING_PICPENDB	__BITS(11,0)

/*
 * RMIXLP Uart
 */
#define	RMIXLP_UART1_PCITAG		_RMIXL_PCITAG(0, 6, 0)
#define	RMIXLP_UART2_PCITAG		_RMIXL_PCITAG(0, 6, 1)

/*
 * GPIO Controller registers
 */
#define	RMIXLP_GPIO_PCITAG		_RMIXL_PCITAG(0, 6, 4)

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
#define RMIXL_GPIO_RESET_RESV		__BITS(31,1)
#define RMIXL_GPIO_RESET_RESET		__BIT(0)


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
 * RMIXL_GPIO_RESET_CFG bits
 */
#define RMIXL_GPIO_RESET_CFG_RESa		__BITS(31,28)
#define RMIXL_GPIO_RESET_CFG_PCIE_SRIO_SEL	__BITS(27,26)	/* PCIe or SRIO Select:
								 * 00 = PCIe selected, SRIO not available
								 * 01 = SRIO selected, 1.25 Gbaud (1.0 Gbps)
								 * 10 = SRIO selected, 2.25 Gbaud (2.0 Gbps)
								 * 11 = SRIO selected, 3.125 Gbaud (2.5 Gbps)
								 */
#define RMIXL_GPIO_RESET_CFG_XAUI_PORT1_SEL	__BIT(25)	/* XAUI Port 1 Select:
								 *  0 = Disabled - Port is SGMII ports 4-7
								 *  1 = Enabled -  Port is 4-lane XAUI Port 1
								 */
#define RMIXL_GPIO_RESET_CFG_XAUI_PORT0_SEL	__BIT(24)	/* XAUI Port 0 Select:
								 *  0 = Disabled - Port is SGMII ports 0-3
								 *  1 = Enabled -  Port is 4-lane XAUI Port 0
								 */
#define RMIXL_GPIO_RESET_CFG_RESb		__BIT(23)
#define RMIXL_GPIO_RESET_CFG_USB_DEV		__BIT(22)	/* USB Device:
								 *  0 = Device Mode
								 *  1 = Host Mode
								 */
#define RMIXL_GPIO_RESET_CFG_PCIE_CFG		__BITS(21,20)	/* PCIe or SRIO configuration */
#define RMIXL_GPIO_RESET_CFG_FLASH33_EN		__BIT(19)	/* Flash 33 MHZ Enable:
								 *  0 = 66.67 MHz
								 *  1 = 33.33 MHz
								 */
#define RMIXL_GPIO_RESET_CFG_BIST_DIAG_EN	__BIT(18)	/* BIST Diagnostics enable */
#define RMIXL_GPIO_RESET_CFG_BIST_RUN_EN	__BIT(18)	/* BIST Run enable */
#define RMIXL_GPIO_RESET_CFG_NOOT_NAND		__BIT(16)	/* Enable boot from NAND Flash */
#define RMIXL_GPIO_RESET_CFG_BOOT_PCMCIA	__BIT(15)	/* Enable boot from PCMCIA */
#define RMIXL_GPIO_RESET_CFG_FLASH_CFG		__BIT(14)	/* Flash 32-bit Data Configuration:
								 *  0 = 32-bit address / 16-bit data
								 *  1 = 32-bit address / 32-bit data
								 */
#define RMIXL_GPIO_RESET_CFG_PCMCIA_EN		__BIT(13)	/* PCMCIA Enable Status */
#define RMIXL_GPIO_RESET_CFG_PARITY_EN		__BIT(12)	/* Parity Enable Status */
#define RMIXL_GPIO_RESET_CFG_BIGEND		__BIT(11)	/* Big Endian Mode Enable Status */
#define RMIXL_GPIO_RESET_CFG_PLL1_OUT_DIV	__BITS(10,8)	/* PLL1 (Core PLL) Output Divider */
#define RMIXL_GPIO_RESET_CFG_PLL1_FB_DIV	__BITS(7,0)	/* PLL1 Feedback Divider */

/*
 * RMIXL_GPIO_LOW_PWR_DIS bits
 * except as noted, all bits are:
 *  0 = feature enable (default)
 *  1 = feature disable
 */
/* XXX defines are for XLS6xx, XLS4xx-Lite and XLS4xx Devices */
#define RMIXL_GPIO_LOW_PWR_DIS_LP		__BIT(0)	/* Low Power disable */
#define RMIXL_GPIO_LOW_PWR_DIS_GMAC_QD_0	__BIT(1)	/* GMAC Quad 0 (GMAC 0..3) disable */
#define RMIXL_GPIO_LOW_PWR_DIS_GMAC_QD_1	__BIT(2)	/* GMAC Quad 1 (GMAC 4..7) disable */
#define RMIXL_GPIO_LOW_PWR_DIS_USB		__BIT(3)	/* USB disable */
#define RMIXL_GPIO_LOW_PWR_DIS_PCIE		__BIT(4)	/* PCIE disable */
#define RMIXL_GPIO_LOW_PWR_DIS_CDE		__BIT(5)	/* Compression/Decompression Engine disable */
#define RMIXL_GPIO_LOW_PWR_DIS_DMA		__BIT(6)	/* DMA Engine disable */
#define RMIXL_GPIO_LOW_PWR_DIS_SAE		__BITS(8,7)	/* Security Acceleration Engine disable:
								 *  00 = enable (default)
								 *  01 = reserved
								 *  10 = reserved
								 *  11 = disable
								 */
#define RMIXL_GPIO_LOW_PWR_DIS_RESV		__BITS(31,9)

#define RMIXLP_GPIO_PADOE(g) _RMIXL_OFFSET(0x40+(g)) // Pad Output Enable Register 0
#define RMIXLP_GPIO_PADOE0 _RMIXL_OFFSET(0x40) // Pad Output Enable Register 0
#define RMIXLP_GPIO_PADOE1 _RMIXL_OFFSET(0x41) // Pad Output Enable Register 1
#define RMIXLP_GPIO_PADDRV(g) _RMIXL_OFFSET(0x42+(g)) // Pad Drive Register 0
#define RMIXLP_GPIO_PADDRV0 _RMIXL_OFFSET(0x42) // Pad Drive Register 0
#define RMIXLP_GPIO_PADDRV1 _RMIXL_OFFSET(0x43) // Pad Drive Register 1
#define RMIXLP_GPIO_PADSAMPLE(g) _RMIXL_OFFSET(0x44+(g)) // Pad Sample Register 0
#define RMIXLP_GPIO_PADSAMPLE0 _RMIXL_OFFSET(0x44) // Pad Sample Register 0
#define RMIXLP_GPIO_PADSAMPLE1 _RMIXL_OFFSET(0x45) // Pad Sample Register 1
#define RMIXLP_GPIO_INTEN(n,g) _RMIXL_OFFSET(0x46+2*(n)+(g)) // Interrupt 0 Enable Register 0
#define RMIXLP_GPIO_INTEN0(n) _RMIXL_OFFSET(0x46+2*(n)) // Interrupt 0 Enable Register 0
#define RMIXLP_GPIO_INTEN1(n) _RMIXL_OFFSET(0x47+2*(n)) // Interrupt 0 Enable Register 0
#define RMIXLP_GPIO_8XX_INTPOL(g) _RMIXL_OFFSET((0x4E)+(g)) // Interrupt Polarity Register 0
#define RMIXLP_GPIO_8XX_INTTYPE(g) _RMIXL_OFFSET(0x50+(g)) // Interrupt Type Register 0
#define RMIXLP_GPIO_8XX_INTSTAT(g) _RMIXL_OFFSET(0x52+(g)) // Interrupt Status Register 0
#define RMIXLP_GPIO_3XX_INTPOL(g) _RMIXL_OFFSET((0x5E)+(g)) // Interrupt Polarity Register 0
#define RMIXLP_GPIO_3XX_INTTYPE(g) _RMIXL_OFFSET(0x60+(g)) // Interrupt Type Register 0
#define RMIXLP_GPIO_3XX_INTSTAT(g) _RMIXL_OFFSET(0x62+(g)) // Interrupt Status Register 0

#define	RMIXLP_GPIO_8XX_MAXPINS		41	/* 41 GPIO pins */
#define	RMIXLP_GPIO_4XX_MAXPINS		41	/* 41 GPIO pins */
#define	RMIXLP_GPIO_3XX_MAXPINS		57	/* 41 GPIO pins */
#define	RMIXLP_GPIO_3XXL_MAXPINS	44	/* 44 GPIO pins */

/*
 * Peripheral I/O bus (Flash/PCMCIA) controller registers
 */
#define RMIXL_FLASH_NCS			10			/* number of chip selects */
#define RMIXL_FLASH_CS_BOOT		0			/* CS0 is boot flash */
#define RMIXL_FLASH_CS_PCMCIA_CF	6			/* CS6 is PCMCIA compact flash */
#define RMIXL_FLASH_CSBASE_ADDRn(n)	_RMIXL_OFFSET(0x00+(n))	/* CSn Base Address reg */
#define RMIXL_FLASH_CSADDR_MASKn(n)	_RMIXL_OFFSET(0x10+(n))	/* CSn Address Mask reg */
#define RMIXL_FLASH_CSDEV_PARMn(n)	_RMIXL_OFFSET(0x20+(n))	/* CSn Device Parameter reg */
#define RMIXL_FLASH_CSTIME_PARMAn(n)	_RMIXL_OFFSET(0x30+(n))	/* CSn Timing Parameters A reg */
#define RMIXL_FLASH_CSTIME_PARMBn(n)	_RMIXL_OFFSET(0x40+(n))	/* CSn Timing Parameters B reg */
#define RMIXL_FLASH_INT_MASK		_RMIXL_OFFSET(0x50)	/* Flash Interrupt Mask reg */
#define RMIXL_FLASH_INT_STATUS		_RMIXL_OFFSET(0x60)	/* Flash Interrupt Status reg */
#define RMIXL_FLASH_ERROR_STATUS	_RMIXL_OFFSET(0x70)	/* Flash Error Status reg */
#define RMIXL_FLASH_ERROR_ADDR		_RMIXL_OFFSET(0x80)	/* Flash Error Address reg */

/*
 * RMIXL_FLASH_CSDEV_PARMn bits
 */
#define RMIXL_FLASH_CSDEV_RESV		__BITS(31,16)
#define RMIXL_FLASH_CSDEV_BFN		__BIT(15)		/* Boot From Nand
								 *  0=Boot from NOR or
								 *    PCCard Type 1 Flash
								 *  1=Boot from NAND
								 */
#define RMIXL_FLASH_CSDEV_NANDEN	__BIT(14)		/* NAND Flash Enable
								 *  0=NOR
								 *  1=NAND
								 */
#define RMIXL_FLASH_CSDEV_ADVTYPE	__BIT(13)		/* Add Valid Sensing Type
								 *  0=level
								 *  1=pulse
								 */
#define RMIXL_FLASH_CSDEV_PARITY_TYPE	__BIT(12)		/* Parity Type
								 *  0=even
								 *  1=odd
								 */
#define RMIXL_FLASH_CSDEV_PARITY_EN	__BIT(11)		/* Parity Enable */
#define RMIXL_FLASH_CSDEV_GENIF_EN	__BIT(10)		/* Generic PLD/FPGA interface mode
								 *  if this bit is set, then
								 *  GPIO[13:10] cannot be used
								 *  for interrupts
								 */
#define RMIXL_FLASH_CSDEV_PCMCIA_EN	__BIT(9)		/* PCMCIA Interface mode */
#define RMIXL_FLASH_CSDEV_DWIDTH	__BITS(8,7)		/* Data Bus Width:
								 *  00: 8 bit
								 *  01: 16 bit
								 *  10: 32 bit
								 *  11: 8 bit
								 */
#define RMIXL_FLASH_CSDEV_DWIDTH_SHFT	7
#define RMIXL_FLASH_CSDEV_MX_ADDR	__BIT(6)		/* Multiplexed Address
								 *  0: non-muxed
								 *      AD[31:24] = Data,
								 *	AD[23:0] = Addr
								 *  1: muxed
								 *      External latch required
								 */
#define RMIXL_FLASH_CSDEV_WAIT_POL	__BIT(5)		/* WAIT polarity
								 *  0: Active high
								 *  1: Active low
								 */
#define RMIXL_FLASH_CSDEV_WAIT_EN	__BIT(4)		/* Enable External WAIT Ack mode */
#define RMIXL_FLASH_CSDEV_BURST		__BITS(3,1)		/* Burst Length:
								 *  000: 2x
								 *  001: 4x
								 *  010: 8x
								 *  011: 16x
								 *  100: 32x
								 */
#define RMIXL_FLASH_CSDEV_BURST_SHFT	1
#define RMIXL_FLASH_CSDEV_BURST_EN	__BITS(0)		/* Burst Enable */


/*
 * NAND Flash Memory Control registers
 */
#define RMIXL_NAND_CLEn(n)		_RMIXL_OFFSET(0x90+(n))	/* CSn 8-bit CLE command value reg */
#define RMIXL_NAND_ALEn(n)		_RMIXL_OFFSET(0xa0+(n))	/* CSn 8-bit ALE address phase reg */


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

/*
 * XLP PCIe Host Bridge Registers
 */
#define	RMIXLP_SBC_PCITAG		_RMIXL_PCITAG(0, 0, 0)
#ifndef RMIXLP_SBC_PCIE_ECFG_PBASE
#define	RMIXLP_SBC_PCIE_ECFG_PBASE	0x18000000
#endif
#define	RMIXLP_SBC_PCIE_ECFG_VBASE	MIPS_PHYS_TO_KSEG1(RMIXLP_SBC_PCIE_ECFG_PBASE)
#define RMIXLP_SBC_NBU_MODE		_RMIXL_OFFSET(0x40)	/* Memory I/O mode */
#define RMIXLP_SBC_PCIE_CFG_BASE	_RMIXL_OFFSET(0x41)	/* PCI Configuration BAR */
#define RMIXLP_SBC_PCIE_CFG_LIMIT	_RMIXL_OFFSET(0x42)	/* PCI Configuration Limit */
#define RMIXLP_SBC_PCIE_ECFG_BASE	_RMIXL_OFFSET(0x43)	/* PCI Extended Configuration BAR */
#define RMIXLP_SBC_PCIE_ECFG_LIMIT	_RMIXL_OFFSET(0x44)	/* PCI Extended Configuration Limit */
#define RMIXLP_SBC_BUSNUM_BARn(n)	_RMIXL_OFFSET(0x45+(n))	/* Bus Number BAR reg */
#define	RMIXLP_SBC_NBUSNUM_BAR		7	/* PCIe: 0-3, ICI: 4-7 */
#define RMIXLP_SBC_FLASH_BASEn(n)	_RMIXL_OFFSET(0x4c+(n))	/* Flash Memory BAR */
#define RMIXLP_SBC_FLASH_LIMITn(n)	_RMIXL_OFFSET(0x50+(n))	/* Flash Memory Limit reg */
#define	RMIXLP_SBC_NFLASH		4
#define RMIXLP_SBC_DRAM_BASEn(n)	_RMIXL_OFFSET(0x54+(n))	/* DRAM[n] BAR */
#define RMIXLP_SBC_DRAM_LIMITn(n)	_RMIXL_OFFSET(0x5c+(n))	/* DRAM[n] Limit */
#define RMIXLP_SBC_DRAM_XLATIONn(n)	_RMIXL_OFFSET(0x6c+(n))	/* DRAM[n] Translation */
#define	RMIXLP_SBC_NDRAM		8
#define RMIXLP_SBC_PCIE_MEM_BASEn(n)	_RMIXL_OFFSET(0x74+(n))	/* PCI Memory region BAR */
#define RMIXLP_SBC_PCIE_MEM_LIMITn(n)	_RMIXL_OFFSET(0x78+(n))	/* PCI Memory region Limit */
#define	RMIXLP_SBC_NPCIE_MEM		4
#define RMIXLP_SBC_PCIE_IO_BASEn(n)	_RMIXL_OFFSET(0x7c+(n))	/* PCI IO region BAR */
#define RMIXLP_SBC_PCIE_IO_LIMITn(n)	_RMIXL_OFFSET(0x80+(n))	/* PCI IO region LimitAR */
#define	RMIXLP_SBC_NPCIE_IO		4

#define	_RMIXLP_SBC_X_TO_PA(x,r)	\
		((uint64_t)((r) & RMIXLP_SBC_##x##_MASK) << 8)
#define	_RMIXLP_SBC_PA_TO_X(x,r)	\
		(((uint64_t)(r) >> 8) & RMIXLP_SBC_##x##_MASK)
#define	_RMIXLP_SBC_X_SIZE(x,b,l)	\
		((l)-(b)+(__LOWEST_SET_BIT(RMIXLP_SBC_##x##_MASK) << 8))

#define	RMIXLP_SBC_DRAM_MASK		__BITS(31,12)	/* phys address bits 39:20 */
#define	RMIXLP_SBC_PCIE_CFG_MASK	__BITS(31,16)	/* phys address bits 39:24 */
#define	RMIXLP_SBC_PCIE_ECFG_MASK	__BITS(31,12)	/* phys address bits 39:20 */
#define	RMIXLP_SBC_PCIE_MEM_MASK	__BITS(31,12)	/* phys address bits 39:20 */
#define	RMIXLP_SBC_PCIE_IO_MASK		__BITS(31,12)	/* phys address bits 39:20 */
#define	RMIXLP_SBC_SRIO_MEM_MASK	__BITS(31,12)	/* phys address bits 39:20 */

#define	RMIXLP_SBC_DRAM_SIZE(b,l)	_RMIXLP_SBC_X_SIZE(DRAM,b,l)
#define	RMIXLP_SBC_PCIE_CFG_SIZE(b,l)	_RMIXLP_SBC_X_SIZE(PCIE_CFG,b,l)
#define	RMIXLP_SBC_PCIE_ECFG_SIZE(b,l)	_RMIXLP_SBC_X_SIZE(PCIE_ECFG,b,l)
#define	RMIXLP_SBC_PCIE_MEM_SIZE(b,l)	_RMIXLP_SBC_X_SIZE(PCIE_MEM,b,l)
#define	RMIXLP_SBC_PCIE_IO_SIZE(b,l)	_RMIXLP_SBC_X_SIZE(PCIE_IO,b,l)
#define	RMIXLP_SBC_SRIO_MEM_SIZE(b,l)	_RMIXLP_SBC_X_SIZE(SRIO_MEM,b,l)

#define	RMIXLP_SBC_DRAM_TO_PA(r)	_RMIXLP_SBC_X_TO_PA(DRAM,r)
#define	RMIXLP_SBC_PCIE_CFG_TO_PA(r)	_RMIXLP_SBC_X_TO_PA(PCIE_CFG,r)
#define	RMIXLP_SBC_PCIE_ECFG_TO_PA(r)	_RMIXLP_SBC_X_TO_PA(PCIE_ECFG,r)
#define	RMIXLP_SBC_PCIE_MEM_TO_PA(r)	_RMIXLP_SBC_X_TO_PA(PCIE_MEM,r)
#define	RMIXLP_SBC_PCIE_IO_TO_PA(r)	_RMIXLP_SBC_X_TO_PA(PCIE_IO,r)
#define	RMIXLP_SBC_SRIO_MEM_TO_PA(r)	_RMIXLP_SBC_X_TO_PA(SRIO_MEM,r)

#define	RMIXLP_SBC_PA_TO_DRAM(r)	_RMIXLP_SBC_PA_TO_X(DRAM,r)
#define	RMIXLP_SBC_PA_TO_PCIE_CFG(r)	_RMIXLP_SBC_PA_TO_X(PCIE_CFG,r)
#define	RMIXLP_SBC_PA_TO_PCIE_ECFG(r)	_RMIXLP_SBC_PA_TO_X(PCIE_ECFG,r)
#define	RMIXLP_SBC_PA_TO_PCIE_MEM(r)	_RMIXLP_SBC_PA_TO_X(PCIE_MEM,r)
#define	RMIXLP_SBC_PA_TO_PCIE_IO(r)	_RMIXLP_SBC_PA_TO_X(PCIE_IO,r)
#define	RMIXLP_SBC_PA_TO_SRIO_MEM(r)	_RMIXLP_SBC_PA_TO_X(SRIO_MEM,r)

/*
 * For each PCIe link, its subordinate buses must be programed into its
 * BusNum_BAR register.  This is in addition to the normal PCIe registers
 * on the PCIe link device itself.
 */
#define	RMIXLP_SBC_BUSNUM_BAR_ENABLE	__BIT(0)
#define	RMIXLP_SBC_BUSNUM_BAR_SECBUS	__BITS(15,8)
#define	RMIXLP_SBC_BUSNUM_BAR_SUBBUS	__BITS(23,16)
#define	RMIXLP_SBC_BUSNUM_BAR_MASK	__BITS(23,8)

#define	RMIXLP_SBC_EVCNT_CTRL1		_RMIXL_OFFSET(0x90) /* Event Counter 1 Control Register */
#define	RMIXLP_SBC_EVCNT_LOW1		_RMIXL_OFFSET(0x91) /* Event Counter 1 Low Register */
#define	RMIXLP_SBC_EVCNT_HIGH1		_RMIXL_OFFSET(0x92) /* Event Counter 1 High Register */
#define	RMIXLP_SBC_EVCNT_CTRL2		_RMIXL_OFFSET(0x93) /* Event Counter 2 Control Register */
#define	RMIXLP_SBC_EVCNT_LOW2		_RMIXL_OFFSET(0x94) /* Event Counter 2 Low Register */
#define	RMIXLP_SBC_EVCNT_HIGH2		_RMIXL_OFFSET(0x95) /* Event Counter 2 High Register */

#define RMIXLP_SBC_TRCBUF_MATCH_RQST0	_RMIXL_OFFSET(0x96) /* Trace Buffer Match Request Register 0 */
#define RMIXLP_SBC_TRCBUF_MATCH_RQST1	_RMIXL_OFFSET(0x97) /* Trace Buffer MAtch Request Register 1 */
#define RMIXLP_SBC_TRCBUF_MATCH_ADDRLO	_RMIXL_OFFSET(0x98) /* Trace Buffer Match Request Address Low Register */
#define RMIXLP_SBC_TRCBUF_MATCH_ADDRHI	_RMIXL_OFFSET(0x99) /* Trace Buffer Match Request Address High Register */
#define RMIXLP_SBC_TRCBUF_CTRL		_RMIXL_OFFSET(0x9A) /* Trace Buffer Control Register */
#define RMIXLP_SBC_TRCBUF_INIT		_RMIXL_OFFSET(0x9B) /* Trace Buffer Initialization Register */
#define RMIXLP_SBC_TRCBUF_ACCESS	_RMIXL_OFFSET(0x9C) /* Trace Buffer Access Register */
#define RMIXLP_SBC_TRCBUF_READDATA(n)	_RMIXL_OFFSET(0x9D+(n)) /* Trace Buffer Read Data Registers <0-3> */
#define RMIXLP_SBC_NTRCBUF_READDATA	4
#define RMIXLP_SBC_TRCBUF_STATUS	_RMIXL_OFFSET(0xA1) /* Trace Buffer Status Register */

#define RMIXLP_SBC_ADDR_ERROR0		_RMIXL_OFFSET(0xA2) /* Address Error Register 0 */
#define RMIXLP_SBC_ADDR_ERROR1		_RMIXL_OFFSET(0xA3) /* Address Error Register 1 */
#define RMIXLP_SBC_ADDR_ERROR2		_RMIXL_OFFSET(0xA4) /* Address Error Register 2 */
#define RMIXLP_SBC_TAGECC_ADDR_ERROR0	_RMIXL_OFFSET(0xA5) /* Tag ECC Address Error Register 0 */
#define RMIXLP_SBC_TAGECC_ADDR_ERROR1	_RMIXL_OFFSET(0xA6) /* Tag ECC Address Error Register 1 */
#define RMIXLP_SBC_TAGECC_ADDR_ERROR2	_RMIXL_OFFSET(0xA7) /* Tag ECC Address Error Register 2 */
#define RMIXLP_SBC_LINE_FLUSH_LOW	_RMIXL_OFFSET(0xA8) /* Line Flush Low Register */
#define RMIXLP_SBC_LINE_FLUSH_HIGH	_RMIXL_OFFSET(0xA9) /* Line Flush High Register */
#define RMIXLP_SBC_NODE_ID		_RMIXL_OFFSET(0xAA) /* Node ID Register */
#define RMIXLP_SBC_ERROR_INT_ENABLE	_RMIXL_OFFSET(0xAB) /* Error Interrupt Enable Register */
#define RMIXLP_SBC_TIMEOUT_ERROR0	_RMIXL_OFFSET(0xAC) /* Timeout Error Register 0 */
#define RMIXLP_SBC_TIMEOUT_ERROR1	_RMIXL_OFFSET(0xAD) /* Timeout Error Register 1 */
#define RMIXLP_SBC_TIMEOUT_ERROR2	_RMIXL_OFFSET(0xAE) /* Timeout Error Register 2 */
#define RMIXLP_SBC_SRIO_MEM_BASE	_RMIXL_OFFSET(0xAF) /* SRIO Memory Base Address Register */
#define RMIXLP_SBC_SRIO_MEM_LIMIT	_RMIXL_OFFSET(0xB0) /* SRIO Memory Limit Address Register */

/*
 * XLP L3 Cache Registers
 */
#define RMIXLP_SBC_L3_LINE_LCK0			_RMIXL_OFFSET(0xC0)
#define RMIXLP_SBC_L3_LINE_LCK1			_RMIXL_OFFSET(0xC1)
#define RMIXLP_SBC_L3_ACCESS_CMD		_RMIXL_OFFSET(0xC2)
#define RMIXLP_SBC_L3_ACCESS_ADDR		_RMIXL_OFFSET(0xC3)
#define RMIXLP_SBC_L3_ACCESS_DATA0		_RMIXL_OFFSET(0xC4)
#define RMIXLP_SBC_L3_ACCESS_DATA1		_RMIXL_OFFSET(0xC5)
#define RMIXLP_SBC_L3_ACCESS_DATA2		_RMIXL_OFFSET(0xC6)
#define RMIXLP_SBC_L3_WAY_PART0			_RMIXL_OFFSET(0xC7)
#define RMIXLP_SBC_L3_WAY_PART1			_RMIXL_OFFSET(0xC8)
#define RMIXLP_SBC_L3_WAY_PART4			_RMIXL_OFFSET(0xCB)
#define RMIXLP_SBC_L3_WAY_PART5			_RMIXL_OFFSET(0xCC)
#define RMIXLP_SBC_L3_WAY_PART6			_RMIXL_OFFSET(0xCD)
#define RMIXLP_SBC_L3_PERF_CTL_REG0		_RMIXL_OFFSET(0xCE)
#define RMIXLP_SBC_L3_PERF_CNT_REG0		_RMIXL_OFFSET(0xCF)
#define RMIXLP_SBC_L3_PERF_CTL_REG1		_RMIXL_OFFSET(0xD0)
#define RMIXLP_SBC_L3_PERF_CNT_REG1		_RMIXL_OFFSET(0xD1)
#define RMIXLP_SBC_L3_PERF_CTL_REG2		_RMIXL_OFFSET(0xD2)
#define RMIXLP_SBC_L3_PERF_CNT_REG2		_RMIXL_OFFSET(0xD3)
#define RMIXLP_SBC_L3_PERF_CTL_REG3		_RMIXL_OFFSET(0xD4)
#define RMIXLP_SBC_L3_PERF_CNT_REG3		_RMIXL_OFFSET(0xD5)
#define RMIXLP_SBC_L3_ERROR_INJ_CTL_REG0	_RMIXL_OFFSET(0xD6)
#define RMIXLP_SBC_L3_ERROR_INJ_CTL_REG1	_RMIXL_OFFSET(0xD7)
#define RMIXLP_SBC_L3_ERROR_INJ_CTL_REG2	_RMIXL_OFFSET(0xD8)
#define RMIXLP_SBC_L3_ERROR_LOG_REG0		_RMIXL_OFFSET(0xD9)
#define RMIXLP_SBC_L3_ERROR_LOG_REG1		_RMIXL_OFFSET(0xDA)
#define RMIXLP_SBC_L3_ERROR_LOG_REG2		_RMIXL_OFFSET(0xDB)
#define RMIXLP_SBC_L3_INTERRUPT_EN_REG		_RMIXL_OFFSET(0xDC)

/*
 * XLP Time Slot Weight Registers
 */
#define RMIXLP_SBC_PCIE_LINK_TSW(n)	_RMIXL_OFFSET(0x300+(n)) /* PCIe Link 0 Time Slot Weight Register */
#define	RMIXLP_SBC_NPCIE_LINK_TSW	4
#define RMIXLP_SBC_USB_TSW		_RMIXL_OFFSET(0x304) /* USB Time Slot Weight Register */
#define RMIXLP_SBC_POE_TSW		_RMIXL_OFFSET(0x305) /* Packet Ordering Engine Time Slot Weight Register */
#define RMIXLP_SBC_SATA_TSW		_RMIXL_OFFSET(0x306) /* SATA Weight Register */
#define RMIXLP_SBC_SRIO_TSW		_RMIXL_OFFSET(0x307) /* SRIO Weight Register */
#define RMIXLP_SBC_REGEX_TSW		_RMIXL_OFFSET(0x308) /* RegEx Weight Register */
#define RMIXLP_SBC_GPIO_TSW		_RMIXL_OFFSET(0x309) /* General I/O Time Slot Weight Register */
#define RMIXLP_SBC_FLASH_TSW		_RMIXL_OFFSET(0x30A) /* Flash Time Slot Weight Register (NAND/NOR/SPI/MMC/SD) */
#define RMIXLP_SBC_NAE_TSW		_RMIXL_OFFSET(0x30B) /* Network Acceleration Engine Time Slot Weight Register */
#define RMIXLP_SBC_FNM_TSW		_RMIXL_OFFSET(0x30C) /* Fast Messaging Network Time Slot Weight Register */
#define RMIXLP_SBC_DMAENG_TSW		_RMIXL_OFFSET(0x30D) /* Data Transfer and RAID Engine Slot Weight Register */
#define RMIXLP_SBC_SEC_TSW		_RMIXL_OFFSET(0x30E) /* Security Engine Slot Weight Register */
#define RMIXLP_SBC_RSAECC_TSW		_RMIXL_OFFSET(0x30F) /* RSA/ECC Engine Slot Weight Register */
#define RMIXLP_SBC_BRIDGE_DATA_COUNTER	_RMIXL_OFFSET(0x310) /* Bridge Data Counter Register */
#define RMIXLP_SBC_BYTE_SWAP		_RMIXL_OFFSET(0x311) /* Byte Swap Register */

#define	RMIXLP_EHCI0_PCITAG		_RMIXL_PCITAG(0,2,0)
#define	RMIXLP_OHCI0_PCITAG		_RMIXL_PCITAG(0,2,1)
#define	RMIXLP_OHCI1_PCITAG		_RMIXL_PCITAG(0,2,2)
#define	RMIXLP_EHCI1_PCITAG		_RMIXL_PCITAG(0,2,3)
#define	RMIXLP_OHCI2_PCITAG		_RMIXL_PCITAG(0,2,4)
#define	RMIXLP_OHCI3_PCITAG		_RMIXL_PCITAG(0,2,5)

#define	RMIXLP_USB_CTL0			_RMIXL_OFFSET(0x41)
#define	RMIXLP_USB_BYTE_SWAP_DIS	_RMIXL_OFFSET(0x49)
#define	RMIXLP_USB_PHY0			_RMIXL_OFFSET(0x4A)

#define	RMIXLP_USB_CTL0_BIUTOEN		__BIT(8)
#define	RMIXLP_USB_CTL0_INCR4		__BIT(7)
#define	RMIXLP_USB_CTL0_INCR8		__BIT(6)
#define	RMIXLP_USB_CTL0_INCR16		__BIT(5)
#define	RMIXLP_USB_CTL0_0HCIINT12	__BIT(4)
#define	RMIXLP_USB_CTL0_0HCIINT1	__BIT(3)
#define	RMIXLP_USB_CTL0_0HCISTRTCLK	__BIT(2)
#define	RMIXLP_USB_CTL0_EHCI64BEN	__BIT(1)
#define	RMIXLP_USB_CTL0_USBCTLRRST	__BIT(0)

#define	RMIXLP_USB_PHY0_PHYTXBSENH1	__BIT(11)
#define	RMIXLP_USB_PHY0_PHYTXBSTENH0	__BIT(10)
#define	RMIXLP_USB_PHY0_PHYTXBSENL1	__BIT(9)
#define	RMIXLP_USB_PHY0_PHYTXBSENL0	__BIT(8)
#define	RMIXLP_USB_PHY0_PHYLBEN1	__BIT(7)
#define	RMIXLP_USB_PHY0_PHYLBEN0	__BIT(6)
#define	RMIXLP_USB_PHY0_PHYPORTRST1	__BIT(5)
#define	RMIXLP_USB_PHY0_PHYPORTRST0	__BIT(4)
#define	RMIXLP_USB_PHY0_PHYREFCLKFREQ	__BITS(3,2)
#define	RMIXLP_USB_PHY0_PHYDETVBUS	__BIT(1)
#define	RMIXLP_USB_PHY0_USBPHYRESET	__BIT(0)

#define	RMIXLP_NAE_PCITAG		_RMIXL_PCITAG(0,3,0)
#define	RMIXLP_POE_PCITAG		_RMIXL_PCITAG(0,3,1)
#define	RMIXLP_FMN_PCITAG		_RMIXL_PCITAG(0,4,0)

/*
 * PCI PCIe control (contains the IRT info)
 */
#define	PCI_RMIXLP_STATID		_RMIXL_OFFSET(0x3c)
#define	PCI_RMIXLP_IRTINFO		_RMIXL_OFFSET(0x3d)

#define	PCI_RMIXLP_STATID_BASE(x)	(((x) >> 0) & 0xfff)
#define	PCI_RMIXLP_STATID_COUNT(x)	(((x) >> 16) & 0xfff)

#define	PCI_RMIXLP_IRTINFO_BASE(x)	(((x) >> 0) & 0xfff)
#define	PCI_RMIXLP_IRTINFO_COUNT(x)	(((x) >> 16) & 0xfff)

/*
 * XLP System Management Registers
 */
#define	RMIXLP_SM_PCITAG		_RMIXL_PCITAG(0, 6, 5)
#define	RMIXLP_SM_CHIP_RESET		_RMIXL_OFFSET(0x40)
#define	RMIXLP_SM_POWER_ON_RESET_CFG	_RMIXL_OFFSET(0x41)
#define	RMIXLP_SM_EFUSE_DEVICE_CFG_STATUS0 _RMIXL_OFFSET(0x42)
#define	RMIXLP_SM_EFUSE_DEVICE_CFG_STATUS1 _RMIXL_OFFSET(0x43)

#define	RMIXLP_SM_POWER_ON_RESET_CFG_CPLL_DFS	__BITS(31,30)
#define	RMIXLP_SM_POWER_ON_RESET_CFG_I2LR	__BIT(29)
#define	RMIXLP_SM_POWER_ON_RESET_CFG_I1LR	__BIT(28)
#define	RMIXLP_SM_POWER_ON_RESET_CFG_I0LR	__BIT(27)
#define	RMIXLP_SM_POWER_ON_RESET_CFG_TS		__BIT(26)
#define	RMIXLP_SM_POWER_ON_RESET_CFG_UM		__BIT(25)
#define	RMIXLP_SM_POWER_ON_RESET_CFG_PLC	__BITS(24,23)
#define	RMIXLP_SM_POWER_ON_RESET_CFG_PM		__BITS(22,19)
#define	RMIXLP_SM_POWER_ON_RESET_CFG_CDV	__BITS(18,17)
#define	RMIXLP_SM_POWER_ON_RESET_CFG_CDF	__BITS(16,10)
#define	RMIXLP_SM_POWER_ON_RESET_CFG_CDR	__BITS(9,8)
#define	RMIXLP_SM_POWER_ON_RESET_CFG_MC		__BIT(7)
#define	RMIXLP_SM_POWER_ON_RESET_CFG_RB		__BIT(6)
#define	RMIXLP_SM_POWER_ON_RESET_CFG_BE		__BIT(5)
#define	RMIXLP_SM_POWER_ON_RESET_CFG_NORSP	__BIT(4)
#define	RMIXLP_SM_POWER_ON_RESET_CFG_BD		__BITS(3,0)

#define	RMIXLP_NOR_PCITAG		_RMIXL_PCITAG(0,7,0)
#define	RMIXLP_NOR_NCS			8
#define	RMIXLP_NOR_CS_BASEADDRESSn(n)	_RMIXL_OFFSET(0x40+(n))
#define	RMIXLP_NOR_CS_BASELIMITn(n)	_RMIXL_OFFSET(0x48+(n))
#define	RMIXLP_NOR_CS_DEVPARMn(n)	_RMIXL_OFFSET(0x50+(n))
#define	RMIXLP_NOR_CS_DEVTIME0n(n)	_RMIXL_OFFSET(0x58+2*(n))
#define	RMIXLP_NOR_CS_DEVTIME1n(n)	_RMIXL_OFFSET(0x59+2*(n))
#define	RMIXLP_NOR_SYSCTRL		_RMIXL_OFFSET(0x68)
#define	RMIXLP_NOR_BYTESWAP		_RMIXL_OFFSET(0x69)
#define	RMIXLP_NOR_ERRLOG0		_RMIXL_OFFSET(0x6a)
#define	RMIXLP_NOR_ERRLOG1		_RMIXL_OFFSET(0x6b)
#define	RMIXLP_NOR_ERRLOG2		_RMIXL_OFFSET(0x6c)
#define	RMIXLP_NOR_ID_TIMEOUT_VAL	_RMIXL_OFFSET(0x6d)
#define	RMIXLP_NOR_INSTAT		_RMIXL_OFFSET(0x6e)
#define	RMIXLP_NOR_INTEN		_RMIXL_OFFSET(0x6f)
#define	RMIXLP_NOR_STATUS		_RMIXL_OFFSET(0x70)

#define	RMIXLP_NOR_CS_ADDRESS_TO_PA(r)	((uint64_t)(r) << 8)
#define	RMIXLP_NOR_PA_TO_CS_ADDRESS(r)	((uint64_t)(r) >> 8)
#define	RMIXLP_NOR_CS_SIZE(b,l)		((l)-(b)+256)

	// Interface Byte signal Enable.
	//   0:	Disables programmable data width selection
	//   1: Enables programmable data width selection
#define RMIXLP_NOR_CS_DEVPARM_BE		__BIT(16)
	// Little Endian.
	//   0:Big Endian
	//   1:Little Endian
#define RMIXLP_NOR_CS_DEVPARM_LE		__BIT(13)
#define RMIXLP_NOR_CS_DEVPARM_DW		__BITS(12,11) // Device Data Width
#define RMIXLP_NOR_CS_DEVPARM_DW_8_BITS		0
#define RMIXLP_NOR_CS_DEVPARM_DW_16_BITS	1
#define RMIXLP_NOR_CS_DEVPARM_DW_32_BITS	2
	// Multiplexed/non-multiplexed device data/address mode
	//   0:Non-multiplexed (only valid if field DW is set to 0)
	//   1:Multiplexed data and address bus
#define RMIXLP_NOR_CS_DEVPARM_MUX		__BIT(10)
	// Wait/Ready signal Polarity
	//   0:Wait active high
	//   1:Wait active low
#define RMIXLP_NOR_CS_DEVPARM_WRP		__BIT(9)
	// Wait/ready signal Write interface Enable.
	// Enables/disables wait-acknowledge mode during write cycles.
	//   0: Enable device Wait mode. External IO_WAIT_L signal is used.
	//   1: Disable Wait mode; external IO_WAIT_L signal is not used.
#define RMIXLP_NOR_CS_DEVPARM_WWE		__BIT(8)
	// Wait/Ready signal Read interface Enable.
	// Enables/disables wait-acknowledge mode during read cycles.
	//   0: Enable device Wait mode. External IO_WAIT_L signal is used.
	//   1: Disable Wait mode; external IO_WAIT_L signal is not used.
	//	This signal is distinct from the RYBY (Ready/Busy) signal,
	//	which is shared by all Flash devices.
#define RMIXLP_NOR_CS_DEVPARM_WRE		__BIT(7)
	// Synchronous Read Data Burst Enabled (when set to 1).
#define RMIXLP_NOR_CS_DEVPARM_SRDBE		__BIT(5)
	// Word-align Address.
	// If set to 1, address bits are word-aligned.
	// This allows address bits of a 16-bit Flash device to connect to XLP
	// address bits [24:1] instead of [23:0] or the address bits of a 32-bit
	// Flash device to connect to XLP address bits [25:2] instead of [23:0].
#define RMIXLP_NOR_CS_DEVPARM_WA		__BIT(2)
#define RMIXLP_NOR_CS_DEVPARM_FLASH_TYPE	__BITS(1,0)	// Flash Type
#define	RMIXLP_NOR_CS_DEVPARM_FLASH_TYPE_NOR	0	//   NOR Flash
#define	RMIXLP_NOR_CS_DEVPARM_FLASH_TYPE_ONCHIP	1	//   On-chip ROM

	// CS to CS timing.
	//   This field indicates the number of clock cycles from the falling
	//   edge of IO_CSn to the next falling edge of IO_CSn, where n = 0-7.
#define RMIXLP_NOR_DEVTIME0_CS_TO_CS	__BITS(31,28)

	// WE to CS timing.
	//   This field indicates the number of clock cycles from the rising
	//   edge of IO_WE_L to the rising edge of IO_CSn_L.
#define RMIXLP_NOR_DEVTIME0_WE_TO_CS	__BITS(27,24)

	// OE to CS timing.
	//   This field indicates the number of clock cycles from the rising
	//   edge of IO_OE_L to the rising edge of IO_CSn_L.
#define RMIXLP_NOR_DEVTIME0_OE_TO_CS	__BITS(23,22)

	// CS to WE timing.
	//   This field indicates the number of clock cycles from the falling
	//   edge of IO_CSn_L to the falling edge of IO_WE_L
#define RMIXLP_NOR_DEVTIME0_CS_TO_WE	__BITS(21,19)

	// CS to OE timing.
	//   This field indicates the number of clock cycles from the falling
	//   edge of IO_CSn_L to the falling edge of IO_OE_L.
#define RMIXLP_NOR_DEVTIME0_CS_TO_OE	__BITS(18,16)

	// Wait/Ready to Data timing.
	//   This field indicates the number of clock cycles from the falling
	//   edge of IO_WE_L to when data is available on a write, or the
	//   falling edge of IO_OE_L to when date is available on a read.
#define RMIXLP_NOR_DEVTIME0_WAIT_TO_DATA __BITS(15,11)

	// OE to Wait timing.
	//   This field indicates the IO_WE_L to wait time on a write,
	//   or the IO_OE_L to wait time on a read.
#define RMIXLP_NOR_DEVTIME0_OE_TO_WAIT	__BITS(10,6)

	// ALE to CS timing.
	//   This field indicates the number of clock cycles from the falling
	//   edge of IO_ALE to the falling edge of IO_CSn_L. This field is
	//   encoded as follows:
#define RMIXLP_NOR_DEVTIME0_ALE_TO_CS	___BITS(5,3)
	//	000: IO_CSn_L is one cycle ahead of IO_ALE.
#define RMIXLP_NOR_DEVTIME0_ALE_TO_CS_AHEAD1	0
	//	001: IO_CSn_L is aligned with IO_ALE.
#define RMIXLP_NOR_DEVTIME0_ALE_TO_CS_ALIGNED	1
	//	010-111: IO_ALE is ahead of IO_CSn_L.
#define RMIXLP_NOR_DEVTIME0_ALE_TO_CS_BEHIND1	2
#define RMIXLP_NOR_DEVTIME0_ALE_TO_CS_BEHIND2	3
#define RMIXLP_NOR_DEVTIME0_ALE_TO_CS_BEHIND3	4
#define RMIXLP_NOR_DEVTIME0_ALE_TO_CS_BEHIND4	5
#define RMIXLP_NOR_DEVTIME0_ALE_TO_CS_BEHIND5	6
#define RMIXLP_NOR_DEVTIME0_ALE_TO_CS_BEHIND6	7

	// ALE pulse width.
	//   This field indicates the number of clock cycles from the falling
	//   edge of IO_ALE to the rising edge of IO_ALE.
#define RMIXLP_NOR_DEVTIME0_ALE_WIDTH	__BITS(2,0)

#define	RMIXLP_NOR_DEVTIME1_WAIT_TIMEOUT	__BITS(26,12)
	// Wait Timeout.
	//    If the Interrupt is an error, the Enable bit is set.

	// RDY/BSY signal Polarity:
	//   0:Ready low, busy high
	//   1:Ready high, busy low
	// This signal is shared by all Flash devices. If any of the devices
	// puts the signal into the busy state, this signal will indicate
	// not-ready (busy) status.
#define	RMIXLP_NOR_SYSCTRL_RDYBSY_POL		__BIT(1)

	// Interconnect Timeout Enable (if set to 1).
#define	RMIXLP_NOR_SYSCTRL_ITE			__BIT(0)

	// RDY/BSY pin transition, if set to 1.
#define	RMIXLP_NOR_INTSTAT_RDYBSY		__BIT(0)

	// Error Log. Setting this bit enables error logging.
#define	RMIXLP_NOR_INTEN_EL			__BIT(1)
	// RYBY Interrupt Enable. Setting this bit enables NOR Flash interrupts.
#define	RMIXLP_NOR_INTEN_RDYBSY			__BIT(0)

	// RDY/BSY Status. 1: NOR device is ready.
#define	RMIXLP_NOR_STATUS_RDYBSY		__BIT(0)
#define	RMIXLP_NAND_PCITAG		_RMIXL_PCITAG(0,7,1)

#define	RMIXLP_NAND_RDYBSY_SEL		_RMIXL_OFFSET(0x81)

#define	RMIXLP_SPI_PCITAG		_RMIXL_PCITAG(0,7,2)

#define	RMIXLP_SPI_CS_CONFIG(n)		_RMIXL_OFFSET(0x40+0x10*(n))
#define	RMIXLP_SPI_CS_FDIV(n)		_RMIXL_OFFSET(0x41+0x10*(n))
#define	RMIXLP_SPI_CS_CMD(n)		_RMIXL_OFFSET(0x42+0x10*(n))
#define	RMIXLP_SPI_CS_STATUS(n)		_RMIXL_OFFSET(0x43+0x10*(n))
#define	RMIXLP_SPI_CS_INTEN(n)		_RMIXL_OFFSET(0x44+0x10*(n))
#define	RMIXLP_SPI_CS_FIFO_THRESH(n)	_RMIXL_OFFSET(0x45+0x10*(n))
#define	RMIXLP_SPI_CS_FIFO_WCNT(n)	_RMIXL_OFFSET(0x46+0x10*(n))
#define	RMIXLP_SPI_CS_TXDATA_FIFO(n)	_RMIXL_OFFSET(0x47+0x10*(n))
#define	RMIXLP_SPI_CS_RXDATA_FIFO(n)	_RMIXL_OFFSET(0x48+0x10*(n))
#define	RMIXLP_SPI_SYSCNTRL		_RMIXL_OFFSET(0x80)

#define	RMIXLP_SPI_CS_CONFIG_RXCAP	__BIT(11) // RX capture on phase drive
#define	RMIXLP_SPI_CS_CONFIG_LSBFE	__BIT(10) // LSB first enable
#define	RMIXLP_SPI_CS_CONFIG_CSTODATA	__BITS(9,8) // chip select polarity 1:high
#define	RMIXLP_SPI_CS_CONFIG_SBPOL	__BIT(6) // start bit polarity 1:high
#define	RMIXLP_SPI_CS_CONFIG_SBE	__BIT(6) // start bit enable 1:enabled
#define	RMIXLP_SPI_CS_CONFIG_RXMISO	__BIT(5) // 0: RX MOSI, 1: RX MISO
#define	RMIXLP_SPI_CS_CONFIG_TXMOSI	__BIT(4) // 1: TX enable for MOSI pin
#define	RMIXLP_SPI_CS_CONFIG_TXMISO	__BIT(3) // 1: TX enable for MISO pin
#define	RMIXLP_SPI_CS_CONFIG_CSPOL	__BIT(2) // chip select polarity 1:high
#define	RMIXLP_SPI_CS_CONFIG_CPOL	__BIT(1) // clock polarity 1:high
#define	RMIXLP_SPI_CS_CONFIG_CPHA	__BIT(0) // SPI_CLK clock phase select

#define	RMIXLP_SPI_CS_FDIV_CNT		__BITS(15,0) // divisor (min of 16)

#define	RMIXLP_SPI_CS_CMD_XFERBITCNT	__BITS(31,16) // total # of bits - 1
#define	RMIXLP_SPI_CS_CMD_CONT		__BIT(4)
#define	RMIXLP_SPI_CS_CMD_CMD		__BITS(3,0)
#define	RMIXLP_SPI_CS_CMD_IDLE		0
#define	RMIXLP_SPI_CS_CMD_TX		1
#define	RMIXLP_SPI_CS_CMD_RX		2
#define	RMIXLP_SPI_CS_CMD_TXRX		3

#define	RMIXLP_SPI_CS_STATUS_RXOF	__BIT(5) // RX FIFO overflow
#define	RMIXLP_SPI_CS_STATUS_TXUF	__BIT(4) // TX FIFO underflow
#define	RMIXLP_SPI_CS_STATUS_RXTHRESH	__BIT(3) // RX FIFO threshold crossed
#define	RMIXLP_SPI_CS_STATUS_TXTHRESH	__BIT(2) // TX FIFO threshold crossed
#define	RMIXLP_SPI_CS_STATUS_XFERDONE	__BIT(1) // Transfer is complete (RO)
#define	RMIXLP_SPI_CS_STATUS_XFERPEND	__BIT(1) // Transfer is pending (RO)

#define	RMIXLP_SPI_CS_INTEN_RXOF	__BIT(4) // RX FIFO overflow
#define	RMIXLP_SPI_CS_INTEN_TXUF	__BIT(3) // TX FIFO underflow
#define	RMIXLP_SPI_CS_INTEN_RXTHRESH	__BIT(2) // RX FIFO threshold crossed
#define	RMIXLP_SPI_CS_INTEN_TXTHRESH	__BIT(1) // TX FIFO threshold crossed
#define	RMIXLP_SPI_CS_INTEN_XFERDONE	__BIT(0) // Transfer is complete

#define	RMIXLP_SPI_CS_FIFO_THRESH_TXFIFO __BITS(7,4)
#define	RMIXLP_SPI_CS_FIFO_THRESH_RXFIFO __BITS(3,0)

#define	RMIXLP_SPI_CS_FIFO_WCNT_TXFIFO	__BITS(7,4)
#define	RMIXLP_SPI_CS_FIFO_WCNT_RXFIFO	__BITS(3,0)

#define	RMIXLP_SPI_SYSCNTRL_PMEN	__BIT(8) // Pin muxing enable
#define	RMIXLP_SPI_SYSCNTRL_CLKDSI(n)	__BIT(4+(n)) // Clock disable for chan
#define	RMIXLP_SPI_SYSCNTRL_RESET(n)	__BIT(0+(n)) // Reset SPI channel n

#define	RMIXLP_SPI_GPIO_PINS		__BITS(28,22)

#define	RMIXLP_MMC_PCITAG		_RMIXL_PCITAG(0,7,3)
#define	RMIXLP_MMC_SLOTSIZE		_RMIXL_OFFSET(0x40)

#define	RMIXLP_MMC_SLOT0		_RMIXL_OFFSET(0x40)
#define	RMIXLP_MMC_SLOT1		_RMIXL_OFFSET(0x80)
#define	RMIXLP_MMC_SYSCTRL		_RMIXL_OFFSET(0xC0)
#define	RMIXLP_MMC_CAPCFG0_S(n)		_RMIXL_OFFSET(0xC1+5*(n))
#define	RMIXLP_MMC_CAPCFG1_S(n)		_RMIXL_OFFSET(0xC2+5*(n))
#define	RMIXLP_MMC_INIT_PRESET_S(n)	_RMIXL_OFFSET(0xC3+5*(n))
#define	RMIXLP_MMC_DEF_PRESET_S(n)	_RMIXL_OFFSET(0xC4+5*(n))

#define	RMIXLP_MMC_SYSCTRL_DELAY	__BITS(21,19)
#define	RMIXLP_MMC_SYSCTRL_RT		__BIT(8)
#define	RMIXLP_MMC_SYSCTRL_WP1		__BIT(7)
#define	RMIXLP_MMC_SYSCTRL_WP0		__BIT(6)
#define	RMIXLP_MMC_SYSCTRL_RD_EX	__BIT(5)
#define	RMIXLP_MMC_SYSCTRL_CA		__BIT(4)
#define	RMIXLP_MMC_SYSCTRL_EN1		__BIT(3)
#define	RMIXLP_MMC_SYSCTRL_EN0		__BIT(2)
#define	RMIXLP_MMC_SYSCTRL_EN(n)	__BIT(2+(n))
#define	RMIXLP_MMC_SYSCTRL_CLK_DIS	__BIT(1)
#define	RMIXLP_MMC_SYSCTRL_RST		__BIT(0)

#define	RMIXLP_MMC_CAPCFG0_S_EMB	__BIT(17) // Extended Media Bus

#define	RMIXLP_MMC_GPIO_PINS0		\
	(__BITS(10,0) | __BIT(29) | __BIT(31))
#define	RMIXLP_MMC_GPIO_PINS1		\
	(__BITS(21,11) | __BIT(28) | __BIT(30))
#define	RMIXLP_MMC_GPIO_PINS(slot)	\
	(slot == 0 ? RMIXLP_MMC_GPIO_PINS0 : RMIXLP_MMC_GPIO_PINS1)

/*
 * FMN non-core station configuration registers
 */
#define RMIXL_FMN_BS_FIRST		_RMIXL_OFFSET(0x320)

/*
 * SGMII bucket size regs
 */
#define RMIXL_FMN_BS_SGMII_UNUSED0	_RMIXL_OFFSET(0x320)	/* initialize as 0 */
#define RMIXL_FMN_BS_SGMII_FCB		_RMIXL_OFFSET(0x321)	/* Free Credit Bucket size */
#define RMIXL_FMN_BS_SGMII_TX0		_RMIXL_OFFSET(0x322)
#define RMIXL_FMN_BS_SGMII_TX1		_RMIXL_OFFSET(0x323)
#define RMIXL_FMN_BS_SGMII_TX2		_RMIXL_OFFSET(0x324)
#define RMIXL_FMN_BS_SGMII_TX3		_RMIXL_OFFSET(0x325)
#define RMIXL_FMN_BS_SGMII_UNUSED1	_RMIXL_OFFSET(0x326)	/* initialize as 0 */
#define RMIXL_FMN_BS_SGMII_FCB1		_RMIXL_OFFSET(0x327)	/* Free Credit Bucket1 size */

/*
 * SAE bucket size regs
 */
#define RMIXL_FMN_BS_SAE_PIPE0		_RMIXL_OFFSET(0x320)
#define RMIXL_FMN_BS_SAE_RSA_PIPE	_RMIXL_OFFSET(0x321)

/*
 * DMA bucket size regs
 */
#define RMIXL_FMN_BS_DMA_CHAN0		_RMIXL_OFFSET(0x320)
#define RMIXL_FMN_BS_DMA_CHAN1		_RMIXL_OFFSET(0x321)
#define RMIXL_FMN_BS_DMA_CHAN2		_RMIXL_OFFSET(0x322)
#define RMIXL_FMN_BS_DMA_CHAN3		_RMIXL_OFFSET(0x323)

/*
 * CDE bucket size regs
 */
#define RMIXL_FMN_BS_CDE_FREE_DESC	_RMIXL_OFFSET(0x320)
#define RMIXL_FMN_BS_CDE_COMPDECOMP	_RMIXL_OFFSET(0x321)

/*
 * PCIe bucket size regs
 */
#define RMIXL_FMN_BS_PCIE_TX0		_RMIXL_OFFSET(0x320)
#define RMIXL_FMN_BS_PCIE_RX0		_RMIXL_OFFSET(0x321)
#define RMIXL_FMN_BS_PCIE_TX1		_RMIXL_OFFSET(0x322)
#define RMIXL_FMN_BS_PCIE_RX1		_RMIXL_OFFSET(0x323)
#define RMIXL_FMN_BS_PCIE_TX2		_RMIXL_OFFSET(0x324)
#define RMIXL_FMN_BS_PCIE_RX2		_RMIXL_OFFSET(0x325)
#define RMIXL_FMN_BS_PCIE_TX3		_RMIXL_OFFSET(0x326)
#define RMIXL_FMN_BS_PCIE_RX3		_RMIXL_OFFSET(0x327)

/*
 * non-core Credit Counter offsets
 */
#define RMIXL_FMN_CC_FIRST		_RMIXL_OFFSET(0x380)
#define RMIXL_FMN_CC_LAST		_RMIXL_OFFSET(0x3ff)

/*
 * non-core Credit Counter bit defines
 */
#define RMIXL_FMN_CC_RESV		__BITS(31,8)
#define RMIXL_FMN_CC_COUNT		__BITS(7,0)

#endif	/* _MIPS_RMI_RMIXLREG_H_ */
