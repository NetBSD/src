/*	$NetBSD: i80321reg.h,v 1.8 2003/02/06 01:36:07 briggs Exp $	*/

/*
 * Copyright (c) 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARM_XSCALE_I80321REG_H_ 
#define _ARM_XSCALE_I80321REG_H_ 

/*
 * Register definitions for the Intel 80321 (``Verde'') I/O processor,
 * based on the XScale core.
 */

/*
 * Base i80321 memory map:
 *
 *	0x0000.0000 - 0x7fff.ffff	ATU Outbound Direct Addressing Window
 *	0x8000.0000 - 0x9001.ffff	ATU Outbound Translation Windows
 *	0x9002.0000 - 0xffff.dfff	External Memory
 *	0xffff.e000 - 0xffff.e8ff	Peripheral Memory Mapped Registers
 *	0xffff.e900 - 0xffff.ffff	Reserved
 */

#define	VERDE_OUT_DIRECT_WIN_BASE	0x00000000UL
#define	VERDE_OUT_DIRECT_WIN_SIZE	0x80000000UL

#define	VERDE_OUT_XLATE_MEM_WIN_SIZE	0x04000000UL
#define	VERDE_OUT_XLATE_IO_WIN_SIZE	0x00010000UL

#define	VERDE_OUT_XLATE_MEM_WIN0_BASE	0x80000000UL
#define	VERDE_OUT_XLATE_MEM_WIN1_BASE	0x84000000UL

#define	VERDE_OUT_XLATE_IO_WIN0_BASE	0x90000000UL

#define	VERDE_EXTMEM_BASE		0x90020000UL

#define	VERDE_PMMR_BASE			0xffffe000UL
#define	VERDE_PMMR_SIZE			0x00000900UL

/*
 * Peripheral Memory Mapped Registers.  Defined as offsets
 * from the VERDE_PMMR_BASE.
 */
#define	VERDE_ATU_BASE			0x0100
#define	VERDE_ATU_SIZE			0x0100

#define	VERDE_DMA_BASE			0x0400
#define	VERDE_DMA_BASE0			(VERDE_DMA_BASE + 0x00)
#define	VERDE_DMA_BASE1			(VERDE_DMA_BASE + 0x40)
#define	VERDE_DMA_SIZE			0x0100
#define	VERDE_DMA_CHSIZE		0x0040

#define	VERDE_MCU_BASE			0x0500
#define	VERDE_MCU_SIZE			0x0100

#define	VERDE_SSP_BASE			0x0600
#define	VERDE_SSP_SIZE			0x0100

#define	VERDE_AAU_BASE			0x0800
#define	VERDE_AAU_SIZE			0x0100

/*
 * Address Translation Unit
 */
	/* 0x00 - 0x38 -- PCI configuration space header */
#define	ATU_IALR0	0x40	/* Inbound ATU Limit 0 */
#define	ATU_IATVR0	0x44	/* Inbound ATU Xlate Value 0 */
#define	ATU_ERLR	0x48	/* Expansion ROM Limit */
#define	ATU_ERTVR	0x4c	/* Expansion ROM Xlate Value */
#define	ATU_IALR1	0x50	/* Inbound ATU Limit 1 */
#define	ATU_IALR2	0x54	/* Inbound ATU Limit 2 */
#define	ATU_IATVR2	0x58	/* Inbound ATU Xlate Value 2 */
#define	ATU_OIOWTVR	0x5c	/* Outbound I/O Window Xlate Value */
#define	ATU_OMWTVR0	0x60	/* Outbound Mem Window Xlate Value 0 */
#define	ATU_OUMWTVR0	0x64	/* Outbound Mem Window Xlate Value 0 Upper */
#define	ATU_OMWTVR1	0x68	/* Outbound Mem Window Xlate Value 1 */
#define	ATU_OUMWTVR1	0x6c	/* Outbound Mem Window Xlate Value 1 Upper */
#define	ATU_OUDWTVR	0x78	/* Outbound Mem Direct Xlate Value Upper */
#define	ATU_ATUCR	0x80	/* ATU Configuration */
#define	ATU_PCSR	0x84	/* PCI Configuration and Status */
#define	ATU_ATUISR	0x88	/* ATU Interrupt Status */
#define	ATU_ATUIMR	0x8c	/* ATU Interrupt Mask */
#define	ATU_IABAR3	0x90	/* Inbound ATU Base Address 3 */
#define	ATU_IAUBAR3	0x94	/* Inbound ATU Base Address 3 Upper */
#define	ATU_IALR3	0x98	/* Inbound ATU Limit 3 */
#define	ATU_IATVR3	0x9c	/* Inbound ATU Xlate Value 3 */
#define	ATU_OCCAR	0xa4	/* Outbound Configuration Cycle Address */
#define	ATU_OCCDR	0xac	/* Outbound Configuration Cycle Data */
#define	ATU_MSI_PORT	0xb4	/* MSI port */
#define	ATU_PDSCR	0xbc	/* PCI Bus Drive Strength Control */
#define	ATU_PCI_X_CAP_ID 0xe0	/* (1) */
#define	ATU_PCI_X_NEXT	0xe1	/* (1) */
#define	ATU_PCIXCMD	0xe2	/* PCI-X Command Register (2) */
#define	ATU_PCIXSR	0xe4	/* PCI-X Status Register */

#define	ATUCR_DRC_ALIAS		(1U << 19)
#define	ATUCR_DAU2GXEN		(1U << 18)
#define	ATUCR_P_SERR_MA		(1U << 16)
#define	ATUCR_DTS		(1U << 15)
#define	ATUCR_P_SERR_DIE	(1U << 9)
#define	ATUCR_DAE		(1U << 8)
#define	ATUCR_BIST_IE		(1U << 3)
#define	ATUCR_OUT_EN		(1U << 1)

#define	PCSR_DAAAPE		(1U << 18)
#define	PCSR_PCI_X_CAP		(3U << 16)
#define	PCSR_PCI_X_CAP_BORING	(0 << 16)
#define	PCSR_PCI_X_CAP_66	(1U << 16)
#define	PCSR_PCI_X_CAP_100	(2U << 16)
#define	PCSR_PCI_X_CAP_133	(3U << 16)
#define	PCSR_OTQB		(1U << 15)
#define	PCSR_IRTQB		(1U << 14)
#define	PCSR_DTV		(1U << 12)
#define	PCSR_BUS66		(1U << 10)
#define	PCSR_BUS64		(1U << 8)
#define	PCSR_RIB		(1U << 5)
#define	PCSR_RPB		(1U << 4)
#define	PCSR_CCR		(1U << 2)
#define	PCSR_CPR		(1U << 1)

#define	ATUISR_IMW1BU		(1U << 14)
#define	ATUISR_ISCEM		(1U << 13)
#define	ATUISR_RSCEM		(1U << 12)
#define	ATUISR_PST		(1U << 11)
#define	ATUISR_P_SERR_ASRT	(1U << 10)
#define	ATUISR_DPE		(1U << 9)
#define	ATUISR_BIST		(1U << 8)
#define	ATUISR_IBMA		(1U << 7)
#define	ATUISR_P_SERR_DET	(1U << 4)
#define	ATUISR_PMA		(1U << 3)
#define	ATUISR_PTAM		(1U << 2)
#define	ATUISR_PTAT		(1U << 1)
#define	ATUISR_PMPE		(1U << 0)

#define	ATUIMR_IMW1BU		(1U << 11)
#define	ATUIMR_ISCEM		(1U << 10)
#define	ATUIMR_RSCEM		(1U << 9)
#define	ATUIMR_PST		(1U << 8)
#define	ATUIMR_DPE		(1U << 7)
#define	ATUIMR_P_SERR_ASRT	(1U << 6)
#define	ATUIMR_PMA		(1U << 5)
#define	ATUIMR_PTAM		(1U << 4)
#define	ATUIMR_PTAT		(1U << 3)
#define	ATUIMR_PMPE		(1U << 2)
#define	ATUIMR_IE_SERR_EN	(1U << 1)
#define	ATUIMR_ECC_TAE		(1U << 0)

#define	PCIXCMD_MOST_1		(0 << 4)
#define	PCIXCMD_MOST_2		(1 << 4)
#define	PCIXCMD_MOST_3		(2 << 4)
#define	PCIXCMD_MOST_4		(3 << 4)
#define	PCIXCMD_MOST_8		(4 << 4)
#define	PCIXCMD_MOST_12		(5 << 4)
#define	PCIXCMD_MOST_16		(6 << 4)
#define	PCIXCMD_MOST_32		(7 << 4)
#define	PCIXCMD_MOST_MASK	(7 << 4)
#define	PCIXCMD_MMRBC_512	(0 << 2)
#define	PCIXCMD_MMRBC_1024	(1 << 2)
#define	PCIXCMD_MMRBC_2048	(2 << 2)
#define	PCIXCMD_MMRBC_4096	(3 << 2)
#define	PCIXCMD_MMRBC_MASK	(3 << 2)
#define	PCIXCMD_ERO		(1U << 1)
#define	PCIXCMD_DPERE		(1U << 0)

#define	PCIXSR_RSCEM		(1U << 29)
#define	PCIXSR_DMCRS_MASK	(7 << 26)
#define	PCIXSR_DMOST_MASK	(7 << 23)
#define	PCIXSR_COMPLEX		(1U << 20)
#define	PCIXSR_USC		(1U << 19)
#define	PCIXSR_SCD		(1U << 18)
#define	PCIXSR_133_CAP		(1U << 17)
#define	PCIXSR_32PCI		(1U << 16)	/* 0 = 32, 1 = 64 */
#define	PCIXSR_BUSNO(x)		(((x) & 0xff) >> 8)
#define	PCIXSR_DEVNO(x)		(((x) & 0x1f) >> 3)
#define	PCIXSR_FUNCNO(x)	((x) & 0x7)

/*
 * Memory Controller Unit
 */
#define	MCU_SDIR		0x00	/* DDR SDRAM Init. Register */
#define	MCU_SDCR		0x04	/* DDR SDRAM Control Register */
#define	MCU_SDBR		0x08	/* SDRAM Base Register */
#define	MCU_SBR0		0x0c	/* SDRAM Boundary 0 */
#define	MCU_SBR1		0x10	/* SDRAM Boundary 1 */
#define	MCU_ECCR		0x34	/* ECC Control Register */
#define	MCU_ELOG0		0x38	/* ECC Log 0 */
#define	MCU_ELOG1		0x3c	/* ECC Log 1 */
#define	MCU_ECAR0		0x40	/* ECC address 0 */
#define	MCU_ECAR1		0x44	/* ECC address 1 */
#define	MCU_ECTST		0x48	/* ECC test register */
#define	MCU_MCISR		0x4c	/* MCU Interrupt Status Register */
#define	MCU_RFR			0x50	/* Refresh Frequency Register */
#define	MCU_DBUDSR		0x54	/* Data Bus Pull-up Drive Strength */
#define	MCU_DBDDSR		0x58	/* Data Bus Pull-down Drive Strength */
#define	MCU_CUDSR		0x5c	/* Clock Pull-up Drive Strength */
#define	MCU_CDDSR		0x60	/* Clock Pull-down Drive Strength */
#define	MCU_CEUDSR		0x64	/* Clock En Pull-up Drive Strength */
#define	MCU_CEDDSR		0x68	/* Clock En Pull-down Drive Strength */
#define	MCU_CSUDSR		0x6c	/* Chip Sel Pull-up Drive Strength */
#define	MCU_CSDDSR		0x70	/* Chip Sel Pull-down Drive Strength */
#define	MCU_REUDSR		0x74	/* Rx En Pull-up Drive Strength */
#define	MCU_REDDSR		0x78	/* Rx En Pull-down Drive Strength */
#define	MCU_ABUDSR		0x7c	/* Addr Bus Pull-up Drive Strength */
#define	MCU_ABDDSR		0x80	/* Addr Bus Pull-down Drive Strength */
#define	MCU_DSDR		0x84	/* Data Strobe Delay Register */
#define	MCU_REDR		0x88	/* Rx Enable Delay Register */

#define	SDCR_DIMMTYPE		(1U << 1)	/* 0 = unbuf, 1 = reg */
#define	SDCR_BUSWIDTH		(1U << 2)	/* 0 = 64, 1 = 32 */

#define	SBRx_TECH		(1U << 31)
#define	SBRx_BOUND		0x0000003f

#define	ECCR_SBERE		(1U << 0)
#define	ECCR_MBERE		(1U << 1)
#define	ECCR_SBECE		(1U << 2)
#define	ECCR_ECCEN		(1U << 3)

#define	ELOGx_SYNDROME		0x000000ff
#define	ELOGx_ERRTYPE		(1U << 8)	/* 1 = multi-bit */
#define	ELOGx_RW		(1U << 12)	/* 1 = write error */
	/*
	 * Dev ID	Func		Requester
	 * 2		0		XScale core
	 * 2		1		ATU
	 * 13		0		DMA channel 0
	 * 13		1		DMA channel 1
	 * 26		0		ATU
	 */
#define	ELOGx_REQ_DEV(x)	(((x) >> 19) & 0x1f)
#define	ELOGx_REQ_FUNC(x)	(((x) >> 16) & 0x3)

#define	MCISR_ECC_ERR0		(1U << 0)
#define	MCISR_ECC_ERR1		(1U << 1)
#define	MCISR_ECC_ERRN		(1U << 2)

/*
 * Timers
 *
 * The i80321 timer registers are available in both memory-mapped
 * and coprocessor spaces.  Most of the registers are read-only
 * if memory-mapped, so we access them via coprocessor space.
 *
 *	TMR0	cp6 c0,1	0xffffe7e0
 *	TMR1	cp6 c1,1	0xffffe7e4
 *	TCR0	cp6 c2,1	0xffffe7e8
 *	TCR1	cp6 c3,1	0xffffe7ec
 *	TRR0	cp6 c4,1	0xffffe7f0
 *	TRR1	cp6 c5,1	0xffffe7f4
 *	TISR	cp6 c6,1	0xffffe7f8
 *	WDTCR	cp6 c7,1	0xffffe7fc
 */

#define	TMRx_TC			(1U << 0)
#define	TMRx_ENABLE		(1U << 1)
#define	TMRx_RELOAD		(1U << 2)
#define	TMRx_CSEL_CORE		(0 << 4)
#define	TMRx_CSEL_CORE_div4	(1 << 4)
#define	TMRx_CSEL_CORE_div8	(2 << 4)
#define	TMRx_CSEL_CORE_div16	(3 << 4)

#define	TISR_TMR0		(1U << 0)
#define	TISR_TMR1		(1U << 1)

#define	WDTCR_ENABLE1		0x1e1e1e1e
#define	WDTCR_ENABLE2		0xe1e1e1e1

/*
 * Interrupt Controller Unit.
 *
 *	INTCTL	cp6 c0,0	0xffffe7d0
 *	INTSTR	cp6 c4,0	0xffffe7d4
 *	IINTSRC	cp6 c8,0	0xffffe7d8
 *	FINTSRC	cp6 c9,0	0xffffe7dc
 *	PIRSR			0xffffe2ec
 */

#define	ICU_PIRSR		0x02ec
#define	ICU_GPOE		0x07c4
#define	ICU_GPID		0x07c8
#define	ICU_GPOD		0x07cc

/*
 * NOTE: WE USE THE `bitXX' BITS TO INDICATE PENDING SOFTWARE
 * INTERRUPTS.  See i80321_icu.c
 */
#define	ICU_INT_HPI		31	/* high priority interrupt */
#define	ICU_INT_XINT0		27	/* external interrupts */
#define	ICU_INT_XINT(x)		((x) + ICU_INT_XINT0)
#define	ICU_INT_bit26		26
#define	ICU_INT_SSP		25	/* SSP serial port */
#define	ICU_INT_MUE		24	/* msg unit error */
#define	ICU_INT_AAUE		23	/* AAU error */
#define	ICU_INT_bit22		22
#define	ICU_INT_DMA1E		21	/* DMA Ch 1 error */
#define	ICU_INT_DMA0E		20	/* DMA Ch 0 error */
#define	ICU_INT_MCUE		19	/* memory controller error */
#define	ICU_INT_ATUE		18	/* ATU error */
#define	ICU_INT_BIUE		17	/* bus interface unit error */
#define	ICU_INT_PMU		16	/* XScale PMU */
#define	ICU_INT_PPM		15	/* peripheral PMU */
#define	ICU_INT_BIST		14	/* ATU Start BIST */
#define	ICU_INT_MU		13	/* messaging unit */
#define	ICU_INT_I2C1		12	/* i2c unit 1 */
#define	ICU_INT_I2C0		11	/* i2c unit 0 */
#define	ICU_INT_TMR1		10	/* timer 1 */
#define	ICU_INT_TMR0		9	/* timer 0 */
#define	ICU_INT_CPPM		8	/* core processor PMU */
#define	ICU_INT_AAU_EOC		7	/* AAU end-of-chain */
#define	ICU_INT_AAU_EOT		6	/* AAU end-of-transfer */
#define	ICU_INT_bit5		5
#define	ICU_INT_bit4		4
#define	ICU_INT_DMA1_EOC	3	/* DMA1 end-of-chain */
#define	ICU_INT_DMA1_EOT	2	/* DMA1 end-of-transfer */
#define	ICU_INT_DMA0_EOC	1	/* DMA0 end-of-chain */
#define	ICU_INT_DMA0_EOT	0	/* DMA0 end-of-transfer */

#define	ICU_INT_HWMASK		(0xffffffff & \
					~((1 << ICU_INT_bit26) | \
					  (1 << ICU_INT_bit22) | \
					  (1 << ICU_INT_bit5)  | \
					  (1 << ICU_INT_bit4)))

/*
 * DMA Controller
 */

struct dma_chain_desc {
	uint32_t	dcd_nda;	/* next descriptor address */
	uint32_t	dcd_pad;	/* PCI address (lower) */
	uint32_t	dcd_puad;	/* PCI address (upper) */
	uint32_t	dcd_lad;	/* local address */
	uint32_t	dcd_bc;		/* byte count */
	uint32_t	dcd_dc;		/* descriptor control */
} __attribute__((__packed__));

#define	DMA_CHAN1_OFF	0x40		/* offset to channel 1 regs */

#define	DMA_CCR		0x00		/* channel control register */
#define	DMA_CSR		0x04		/* channel status register */
#define	DMA_DAR		0x0c		/* descriptor address */
#define	DMA_DNAR	0x10		/* next descriptor address */
#define	DMA_PADR	0x14		/* PCI address (low) */
#define	DMA_PUADR	0x18		/* PCI address (high) */
#define	DMA_LADR	0x1c		/* local address */
#define	DMA_BCR		0x20		/* byte count */
#define	DMA_DCR		0x24		/* descriptor control */

#define	DMA_CCR_CE	(1U << 0)	/* channel enable */
#define	DMA_CCR_CR	(1U << 1)	/* chain resume */

#define	DMA_SSR_STE	(1U << 1)	/* PCI-X split transaction error */
#define	DMA_SSR_TAF	(1U << 2)	/* PCI target abort flag */
#define	DMA_SSR_MAF	(1U << 3)	/* PCI master abort flag */
#define	DMA_SSR_IBMAF	(1U << 5)	/* Internal bus master abort flag */
#define	DMA_SSR_ECIF	(1U << 8)	/* end-of-chain interrupt */
#define	DMA_SSR_ETIF	(1U << 9)	/* end-of-transfer interrupt */
#define	DMA_SSR_CAF	(1U << 10)	/* channel active flag */

#define	DMA_BCR_MASK	0x00ffffff	/* 24-bit count */

#define	DMA_DCR_TTYPE	0x0000000f	/* PCI transaction type */
#define	DMA_DCR_IE	(1U << 4)	/* interrupt enable */
#define	DMA_DCR_DACE	(1U << 5)	/* dual address cycle enable */
#define	DMA_DCR_MMTE	(1U << 6)	/* memory->memory transfer enable */

#define	DMA_DCR_TTYPE_MR	0x06	/* Memory Read */
#define	DMA_DCR_TTYPE_MW	0x07	/* Memory Write */
#define	DMA_DCR_TTYPE_MRM	0x0c	/* Memory Read Multiple */
#define	DMA_DCR_TTYPE_MRL	0x0e	/* Memory Read Line */
#define	DMA_DCR_TTYPE_MW2	0x0f	/* Memory Write */

/*
 * SSP Serial Port
 */

#define	SSP_SSCR0	0x00		/* SSC control 0 */
#define	SSP_SSCR1	0x04		/* SSC control 1 */
#define	SSP_SSSR	0x08		/* SSP status */
#define	SSP_SSITR	0x0c		/* SSP interrupt test */
#define	SSP_SSDR	0x10		/* SSP data */

#define	SSP_SSCR0_DSIZE(x)	((x) - 1)/* data size: 4..16 */
#define	SSP_SSCR0_FRF_SPI	(0 << 4) /* Motorola Serial Periph Iface */
#define	SSP_SSCR0_FRF_SSP	(1U << 4)/* TI Sync. Serial Protocol */
#define	SSP_SSCR0_FRF_UWIRE	(2U << 4)/* NatSemi Microwire */
#define	SSP_SSCR0_FRF_rsvd	(3U << 4)/* reserved */
#define	SSP_SSCR0_ECS		(1U << 6)/* external clock select */
#define	SSP_SSCR0_SSE		(1U << 7)/* sync. serial port enable */
#define	SSP_SSCR0_SCR(x)	((x) << 8)/* serial clock rate */
					  /* bit rate = 3.6864 * 10e6 /
					        (2 * (SCR + 1)) */

#define	SSP_SSCR1_RIE		(1U << 0)/* Rx FIFO interrupt enable */
#define	SSP_SSCR1_TIE		(1U << 1)/* Tx FIFO interrupt enable */
#define	SSP_SSCR1_LBM		(1U << 2)/* loopback mode enable */
#define	SSP_SSCR1_SPO		(1U << 3)/* Moto SPI SSCLK pol. (1 = high) */
#define	SSP_SSCR1_SPH		(1U << 4)/* Moto SPI SSCLK phase:
					    0 = inactive full at start,
						1/2 at end of frame
					    1 = inactive 1/2 at start,
						full at end of frame */
#define	SSP_SSCR1_MWDS		(1U << 5)/* Microwire data size:
					    0 = 8 bit
					    1 = 16 bit */
#define	SSP_SSCR1_TFT		(((x) - 1) << 6) /* Tx FIFO threshold */
#define	SSP_SSCR1_RFT		(((x) - 1) << 10)/* Rx FIFO threshold */
#define	SSP_SSCR1_EFWR		(1U << 14)/* enab. FIFO write/read */
#define	SSP_SSCR1_STRF		(1U << 15)/* FIFO write/read FIFO select:
					     0 = Tx FIFO
					     1 = Rx FIFO */

#define	SSP_SSSR_TNF		(1U << 2)/* Tx FIFO not full */
#define	SSP_SSSR_RNE		(1U << 3)/* Rx FIFO not empty */
#define	SSP_SSSR_BSY		(1U << 4)/* SSP is busy */
#define	SSP_SSSR_TFS		(1U << 5)/* Tx FIFO service request */
#define	SSP_SSSR_RFS		(1U << 6)/* Rx FIFO service request */
#define	SSP_SSSR_ROR		(1U << 7)/* Rx FIFO overrun */
#define	SSP_SSSR_TFL(x)		(((x) >> 8) & 0xf) /* Tx FIFO level */
#define	SSP_SSSR_RFL(x)		(((x) >> 12) & 0xf)/* Rx FIFO level */

#define	SSP_SSITR_TTFS		(1U << 5)/* Test Tx FIFO service */
#define	SSP_SSITR_TRFS		(1U << 6)/* Test Rx FIFO service */
#define	SSP_SSITR_TROR		(1U << 7)/* Test Rx overrun */

#endif /* _ARM_XSCALE_I80321REG_H_ */
