/*	$NetBSD: e500reg.h,v 1.17 2022/05/24 20:50:18 andvar Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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

#include <sys/cdefs.h>

#ifdef _LOCORE
#define	__PPCBIT(n)	(1 << (31 - (n)))
#define	__PPCBITS(m, n)	(((1 << ((n) - (m) + 1)) - 1) << (31 - (m)))
#else
#define	__PPCBIT(n)	__BIT(31-(n))
#define	__PPCBITS(m,n)	__BITS(31-(n),31-(m))
#endif

#define	GUR_SIZE		0x100000
#define	GUR_BPTR		0x0020		/* Boot Page Translation */
#define	BPTR_EN			__PPCBIT(0)	/* Boot Page Enabled */
#define	BPTR_BOOT_PAGE		__PPCBITS(8,31)	/* high 24 bits of phys addr */

#define	DDRC1_BASE		0x02000
#define	DDRC2_BASE		0x06000
#define	DDRC_SIZE		0x01000

#ifdef DDRC_PRIVATE
#define	CS_BNDS(n)		(0x000 + 0x008 * (n))
#define	BNDS_SA			__PPCBITS(4,15)
#define	BNDS_SA_GET(n)		(((n) & BNDS_SA) << 8)
#define	BNDS_EA			__PPCBITS(20,31)
#define	BNDS_EA_GET(n)		(((n) & BNDS_EA) << 24)
#define	BNDS_SIZE_GET(n)	\
	((((((n) & BNDS_EA) + __LOWEST_SET_BIT(BNDS_EA)) << 16) - (((n) & BNDS_SA))) << 8)
#define	CS_CONFIG(n)		(0x080 + 0x004 * (n))
#define CS_CONFIG_EN		__PPCBIT(0)

#define	DDR_SDRAM_CFG		0x110
#define	SDRAM_CFG_MEM_EN	__PPCBIT(0)
#define	SDRAM_CFG_SREN		__PPCBIT(1)
#define	SDRAM_CFG_ECC_EN	__PPCBIT(2)
#define	SDRAM_CFG_RDEN		__PPCBIT(3)
#define	SDRAM_CFG_TYPE		__PPCBITS(5,7)
#define	SDRAM_CFG_TYPE_DDR2	3
#define	SDRAM_CFG_TYPE_DDR3	7
#define	SDRAM_CFG_DYN_PWR	__PPCBIT(10)
#define	SDRAM_CFG_DBW		__PPCBITS(11,12)
#define	SDRAM_CFG_DBW_64BIT	0
#define	SDRAM_CFG_DBW_32BIT	1

#define	CAPTURE_DATA_HI		0xe20
#define	CAPTURE_DATA_LO		0xe24
#define	CAPTURE_ECC		0xe28

#define	ERR_DETECT		0xe40
#define	ERR_DISABLE		0xe44
#define	ERR_INT_EN		0xe48

#define	ERR_MMEE		__PPCBIT(0)
#define	ERR_APEE		__PPCBIT(23)
#define	ERR_ACEE		__PPCBIT(24)
#define	ERR_MBEE		__PPCBIT(28)
#define	ERR_SBEE		__PPCBIT(29)
#define	ERR_MSEE		__PPCBIT(31)

#define	CAPTURE_ATTRIBUTES	0xe4c
#define	CATTR_BNUM		__PPCBITS(1,3)
#define	CATTR_TSIZ		__PPCBITS(5,7)
#define	CATTR_TSRC		__PPCBITS(11,15)
#define	CATTR_TTYP		__PPCBITS(18,19)
#define	CATTR_VLD		__PPCBIT(31)

#define	CAPTURE_ADDRESS		0xe50
#define	CAPTURE_EXT_ADDRESS	0xe54

#define	ERR_SBE			0xe58
#define	ERR_SBE_SBET		__PPCBITS(8,15)
#define	ERR_SBE_SBEC		__PPCBITS(24,31)

#endif /* DDRC_PRIVATE */

#define	GPIO_BASE		0x0fc00
#define	GPIO_SIZE		0x00020

#ifdef GPIO_PRIVATE

#define GPDIR			0x00 /* GPIO direction register */
#define GPODR			0x04 /* GPIO open drain register */
#define GPDAT			0x08 /* GPIO data register */
#define GPIER			0x0C /* GPIO interrupt event register */
#define GPIMR			0x10 /* GPIO interrupt mask register */
#define GPICR			0x14 /* GPIO external interrupt control register */

#endif /* GPIO_PRIVATE */

#define	PCIE1_BASE		0x0a000
#define	PCIE2_MPC8572_BASE	0x09000	/* P2020 too */
#define	PCIE3_MPC8572_BASE	0x08000	/* P2020 too */
#define	PCIX1_MPC8548_BASE	0x08000
#define	PCIX2_MPC8548_BASE	0x09000
#define	PCIE2_MPC8544_BASE	0x09000	/* MPC8536 too */
#define	PCIE3_MPC8544_BASE	0x0b000	/* MPC8536 too */
#define	PCIX1_MPC8544_BASE	0x08000	/* MPC8536 too */
#define	PCI_SIZE		0x01000

#ifdef PCI_PRIVATE

/* PCI Express Configuration Access Registers */
#define PEX_CONFIG_ADDR		0x000 /* PCI Express configuration address register */
#define	PCI_CONFIG_ADDR		PEX_CONFIG_ADDR
#define	PEX_CONFIG_ADDR_EN	__PPCBIT(0)
#define	PEX_CONFIG_ADDR_TAG(b,d,f,r) (((b) << 16) | ((d) << 11) | ((f) << 8) | (r))
#define PEX_CONFIG_DATA		0x004 /* PCI Express configuration data register */
#define	PCI_CONFIG_DATA		PEX_CONFIG_DATA
#define	PCI_INT_ACK		0x008 /* PCI Interrupt Acknowledge */
#define PEX_OTB_CPL_TOR		0x00C /* PCI Express outbound completion timeout register */
#define PEX_CONF_RTY_TOR	0x010 /* PCI Express configuration retry timeout register */
#define PEX_CONFIG		0x014 /* PCI Express configuration register  */

/* PCI Express Power Management Event & Message Registers */
#define PEX_PME_MES_DR		0x020 /* PCI Express PME & message detect register */
#define PEX_PME_MES_DISR	0x024 /* PCI Express PME & message disable register */
#define PEX_PME_MES_IER		0x028 /* PCI Express PME & message interrupt enable register */
#define PEX_PMCR		0x02C /* PCI Express power management command register */

/* PCI Express IP Block Revision Registers */
#define PEX_IP_BLK_REV1		0xBF8 /* IP block revision register 1 */
#define PEX_IP_BLK_REV2		0xBFC /* IP block revision register 2 */

/* PCI Express / PCI-X ATMU Registers */
#define	PEXOWAR_EN		__PPCBIT(0) /* enable window */
#define	PEXOWAR_ROE		__PPCBIT(3) /* relaxed ordering enable */
#define	PEXOWAR_NS		__PPCBIT(4) /* no snoop enable */
#define	PEXOWAR_TC		__PPCBITS(8,10) /* traffic class PCIEX only */
#define	PEXOWAR_TC0		__SHIFTIN(0, PEXOWAR_TC)
#define	PEXOWAR_TC1		__SHIFTIN(1, PEXOWAR_TC)
#define	PEXOWAR_TC2		__SHIFTIN(2, PEXOWAR_TC)
#define	PEXOWAR_TC3		__SHIFTIN(3, PEXOWAR_TC)
#define	PEXOWAR_TC4		__SHIFTIN(4, PEXOWAR_TC)
#define	PEXOWAR_TC5		__SHIFTIN(5, PEXOWAR_TC)
#define	PEXOWAR_TC6		__SHIFTIN(6, PEXOWAR_TC)
#define	PEXOWAR_TC7		__SHIFTIN(7, PEXOWAR_TC)
#define	PEXOWAR_RTT		__PPCBITS(12,15) /* read transaction type */
#define	PEXOWAR_RTT_CONF	__SHIFTIN(2, PEXOWAR_RTT) /* PCIEX only */
#define	PEXOWAR_RTT_MEM		__SHIFTIN(4, PEXOWAR_RTT)
#define	PEXOWAR_RTT_IO		__SHIFTIN(8, PEXOWAR_RTT)
#define	PEXOWAR_WTT		__PPCBITS(16,19) /* write transaction type */
#define	PEXOWAR_WTT_CONF	__SHIFTIN(2, PEXOWAR_WTT) /* PCIEX only */
#define	PEXOWAR_WTT_MEM		__SHIFTIN(4, PEXOWAR_WTT)
#define	PEXOWAR_WTT_IO		__SHIFTIN(8, PEXOWAR_WTT)
#define	PEXOWAR_OWS		__PPCBITS(26,31) /* encoded as 2^(N+1) bytes */

/* PCI Express / PCI-X ATMU Registers */
#define	PEXIWAR_EN		__PPCBIT(0) /* enable window */
#define	PEXIWAR_PF		__PPCBIT(3) /* prefetchable */
#define	PEXIWAR_TRGT		__PPCBITS(8,11) /* traffic class PCIEX only */
#define	PEXIWAR_TRGT_PCI1	__SHIFTIN(0, PEXIWAR_TRGT)
#define	PEXIWAR_TRGT_PCI2	__SHIFTIN(1, PEXIWAR_TRGT)
#define	PEXIWAR_TRGT_PCIEX	__SHIFTIN(2, PEXIWAR_TRGT)
#define	PEXIWAR_TRGT_SRIO	__SHIFTIN(12, PEXIWAR_TRGT)
#define	PEXIWAR_TRGT_LOCALMEM	__SHIFTIN(15, PEXIWAR_TRGT)
#define	PEXIWAR_RTT		__PPCBITS(12,15) /* read transaction type */
#define	PEXIWAR_RTT_MEM		__SHIFTIN(4, PEXIWAR_RTT)
#define	PEXIWAR_RTT_MEM_NOSNOOP	__SHIFTIN(4, PEXIWAR_RTT)
#define	PEXIWAR_RTT_MEM_SNOOP	__SHIFTIN(5, PEXIWAR_RTT)
#define	PEXIWAR_RTT_MEM_ULCKL2	__SHIFTIN(7, PEXIWAR_RTT)
#define	PEXIWAR_WTT		__PPCBITS(16,19) /* write transaction type */
#define	PEXIWAR_WTT_MEM_NOSNOOP	__SHIFTIN(4, PEXIWAR_WTT)
#define	PEXIWAR_WTT_MEM_SNOOP	__SHIFTIN(5, PEXIWAR_WTT)
#define	PEXIWAR_WTT_MEM_ALLOL2	__SHIFTIN(6, PEXIWAR_WTT)
#define	PEXIWAR_WTT_MEM_ALCKL2	__SHIFTIN(7, PEXIWAR_WTT)
#define	PEXIWAR_IWS		__PPCBITS(26,31) /* encoded as 2^(N+1) bytes */
#define	PEXIWAR_IWS_GET(n)	__SHIFTOUT((n), PEXIWAR_IWS)

/* Outbound Window 0 (Default) */
#define PEXOTAR0		0xC00 /* PCI Express outbound translation address register 0 (default) */
#define PEXOTEAR0		0xC04 /* PCI Express outbound translation extended address register 0 (default) */
#define PEXOWAR0		0xC10 /* PCI Express outbound window attributes register 0 (default) */

/* Outbound Window 1 */
#define PEXOTAR1		0xC20 /* PCI Express outbound translation address register 1 */
#define PEXOTEAR1		0xC24 /* PCI Express outbound translation extended address register 1 */
#define PEXOWBAR1		0xC28 /* PCI Express outbound window base address register 1 */
#define PEXOWAR1		0xC30 /* PCI Express outbound window attributes register 1 */

/* Outbound Window 2 */
#define PEXOTAR2		0xC40 /* PCI Express outbound translation address register 2 */
#define PEXOTEAR2		0xC44 /* PCI Express outbound translation extended address register 2 */
#define PEXOWBAR2		0xC48 /* PCI Express outbound window base address register 2 */
#define PEXOWAR2		0xC50 /* PCI Express outbound window attributes register 2 */

/* Outbound Window 3 */
#define PEXOTAR3		0xC60 /* PCI Express outbound translation address register 3 */
#define PEXOTEAR3		0xC64 /* PCI Express outbound translation extended address register 3 */
#define PEXOWBAR3		0xC68 /* PCI Express outbound window base address register 3 */
#define PEXOWAR3		0xC70 /* PCI Express outbound window attributes register 3 */

/* Outbound Window 4 */
#define PEXOTAR4		0xC80 /* PCI Express outbound translation address register 4 */
#define PEXOTEAR4		0xC84 /* PCI Express outbound translation extended address register 4 */
#define PEXOWBAR4		0xC88 /* PCI Express outbound window base address register 4 */
#define PEXOWAR4		0xC90 /* PCI Express outbound window attributes register 4 */

/* Inbound Window 3 */
#define PEXITAR3		0xDA0 /* PCI Express inbound translation address register 3 */
#define PEXIWBAR3		0xDA8 /* PCI Express inbound window base address register 3 */
#define PEXIWBEAR3		0xDAC /* PCI Express inbound window base extended address register 3 */
#define PEXIWAR3		0xDB0 /* PCI Express inbound window attributes register 3 */

/* Inbound Window 2 */
#define PEXITAR2		0xDC0 /* PCI Express inbound translation address register 2 */
#define PEXIWBAR2		0xDC8 /* PCI Express inbound window base address register 2 */
#define PEXIWBEAR2		0xDCC /* PCI Express inbound window base extended address register 2 */
#define PEXIWAR2		0xDD0 /* PCI Express inbound window attributes register 2 */

/* Inbound Window 1 */
#define PEXITAR1		0xDE0 /* PCI Express inbound translation address register 1 */
#define PEXIWBAR1		0xDE8 /* PCI Express inbound window base address register 1 */
#define PEXIWAR1		0xDF0 /* PCI Express inbound window attributes register 1 */

/* PCI Express Error Management Registers */
#define PEX_ERR_DR		0xE00 /* PCI Express error detect register */
#define	PEXERRDR_ICCA		__PPCBIT(14)
#define PEX_ERR_EN		0xE08 /* PCI Express error interrupt enable register */
#define PEX_ERR_DISR		0xE10 /* PCI Express error disable register */
#define PEX_ERR_CAP_STAT	0xE20 /* PCI Express error capture status register */
#define PEX_ERR_CAP_R0		0xE28 /* PCI Express error capture register 0 */
#define PEX_ERR_CAP_R1		0xE2C /* PCI Express error capture register 1 */
#define PEX_ERR_CAP_R2		0xE30 /* PCI Express error capture register 2 */
#define PEX_ERR_CAP_R3		0xE34 /* PCI Express error capture register 3 */

/* PCI Express Private Configuration Space */

#define PEX_LTSSM		0x404
#define	LTSSM_L0		16

#define	PCI_PBFR		0x44	/* Bus Function Register */
#define	PBFR_PAH		__BIT(0)

#endif /* PCI_PRIVATE */

#define	OPENPIC_BASE		0x40000
#define	OPENPIC_SIZE		0x40000

#define	L2CACHE_BASE		0x20000
#define	L2CACHE_SIZE		0x01000

#ifdef L2CACHE_PRIVATE
#define	L2CTL			0x000
#define	L2CTL_L2E		__PPCBIT(0)
#define	L2CTL_L2I		__PPCBIT(1)
#define	L2CTL_L2SIZ		__PPCBITS(2,3)
#define	L2CTL_L2SIZ_GET(x)	(1 << (17 + __SHIFTOUT((x), L2CTL_L2SIZ)))
#define	L2CTL_L2DO		__PPCBIT(9)
#define	L2CTL_L2IO		__PPCBIT(10)
#define	L2CTL_L2INTDIS		__PPCBIT(12)
#define	L2CTL_L2SRAM		__PPCBITS(13,15)
#define	L2CTL_L2LO		__PPCBIT(18)
#define	L2CTL_L2SLC		__PPCBIT(19)
#define	L2CTL_L2LFR		__PPCBIT(21)
#define	L2CTL_L2LFRID		__PPCBITS(22,23)
#define	L2CTL_L2STASHDIS	__PPCBIT(28)
#define	L2CTL_L2STASH		__PPCBITS(30,31)

#endif /* L2CACHE_PRIVATE */

#define	I2C1_BASE		0x3000
#define	I2C2_BASE		0x3100
#define	I2C_SIZE		0x0100

#ifdef I2C_PRIVATE
#define	I2CADR		0x000	/* i2c address register */
#define	I2CFDR		0x004	/* i2c frequency divider register */
#define	I2CCR		0x008	/* i2c control register */
#define	I2CSR		0x00c	/* i2c status register */
#define	I2CDR		0x010	/* i2c data register */
#define	I2CDFSSR	0x014	/* i2c address register */
#endif /* I2C_PRIVATE */

#define	DUART1_BASE	0x4500
#define	DUART2_BASE	0x4600
#define	DUART_SIZE	0x0100

#define	SPI_BASE	0x7000	/* MPC8536 */
#define	SPI_SIZE	0x1000

#ifdef SPI_PRIVATE
#define	SPMODE		0x000	/* mode register */
#define	SPMODE_EN	__PPCBIT(0)	/* Enable eSPI: 0=disabled, 1=enabled */
#define	SPMODE_LOOP	__PPCBIT(1)	/* Loop mode: 0=normal, 1=loopback */
#define	SPMODE_OD	__PPCBIT(2)	/* P1023: Open drain mode: 0=actively driven, 1=open drain */
#define	SPMODE_HO_ADJ	__PPCBITS(13,15) /* Data output hold adjustment */
#define	SPMODE_TXTHR	__PPCBITS(18,23) /* Tx FIFO Threshold: 1-32 */
#define	SPMODE_RXTHR	__PPCBITS(27,31) /* Rx FIFO threshold: 0-31 */
#define	SPIE		0x004	/* event register */
#define	SPIE_RXCNT	__PPCBITS(2,7)	/* current number of full Rx FIFO bytes */
#define	SPIE_TXCNT	__PPCBITS(10,15) /* current number of full Tx FIFO bytes */
#define	SPIE_TXE	__PPCBIT(16)	/* Tx FIFO is empty */
#define	SPIE_DON	__PPCBIT(17)	/* Last character was transmitted */
#define	SPIE_RXT	__PPCBIT(18)	/* Rx FIFO has more than RXTHR bytes */
#define	SPIE_RXF	__PPCBIT(19)	/* Rx FIFO is full */
#define	SPIE_TXT	__PPCBIT(20)	/* Tx FIFO has less than TXTHR bytes */
#define	SPIE_RNE	__PPCBIT(22)	/* Not empty: 0=empty, 1=not empty */
#define	SPIE_TNF	__PPCBIT(23)	/* Tx FIFO not full: 0=full, 1=not full */
#define	SPIM		0x008	/* mask register */
#define	SPIM_TXE	__PPCBIT(16)
#define	SPIM_DON	__PPCBIT(17)
#define	SPIM_RXT	__PPCBIT(18)
#define	SPIM_RXF	__PPCBIT(19)
#define	SPIM_TXT	__PPCBIT(20)
#define	SPIM_RNE	__PPCBIT(22)
#define	SPIM_TNF	__PPCBIT(23)
#define	SPCOM		0x00c	/* command register */
#define	SPCOM_CS	__PPCBITS(0,1)	/* Chip select: 0=CS0, 1=CS1, 2=CS2(P1025), 3=CS3(P1025) */
#define	SPCOM_RXDELAY	__PPCBIT(2)	/* 0=normal eSPI operation */
#define	SPCOM_DO	__PPCBIT(3)	/* 0=normal eSPI operation, 1=Winbond dual output read */
#define	SPCOM_TO	__PPCBIT(4)	/* Transmit only: 0=normal operation, 1=No reception is done for the frame */
#define	SPCOM_HLD	__PPCBIT(5)	/* 0=normal operation, 1=Mask first generated SPI_CLK */
#define	SPCOM_LS	__PPCBIT(6)	/* P1023: Late sample: 0=normal operation, 1=Late data sample */
#define	SPCOM_RXSKIP	__PPCBITS(8,15)	/* if RxSKIP != 0: Number of characters skipped for reception from frame start */
#define	SPCOM_TRANLEN	__PPCBITS(16,31) /* Transaction length */
#define	SPITF		0x010	/* transmit FIFO access register */
#define	SPIRF		0x014	/* receive FIFO access register */
#define	SPMODE0		0x020	/* CS0 mode register */
#define	SPMODE1		0x024	/* CS1 mode register */
#define	SPMODE2		0x028	/* CS2 mode register (P1025) */
#define	SPMODE3		0x02c	/* CS3 mode register (P1025) */
#define	SPMODEn(n)	(0x020+(n)*4)
#define	SPMODEn_CI	__PPCBIT(0)	/* Clock invert: 0=inactive state of SPI_CLK is low, 1=high */
#define	SPMODEn_CP	__PPCBIT(1)	/* Clock phase: SPI_CLK starts toggling at the middle of the data transfer, 1=beginning */
#define	SPMODEn_REV	__PPCBIT(2)	/* Reverse data mode: 0=LSB of the character sent and received first, 1=MSB */
#define	SPMODEn_DIV16	__PPCBIT(3)	/* Divide by 16: 0=System clock, 1=System clock/16 */
#define	SPMODEn_PM	__PPCBITS(4,7)	/* Prescale modulus select */
#define	SPMODEn_ODD	__PPCBIT(8)	/* 0=Even division, 1=Odd dividion */
#define	SPMODEn_POL	__PPCBIT(11)	/* CS polarity: 0=Asserted high/Negated low, 1=Asserted low/Negated high */
#define	SPMODEn_LEN	__PPCBITS(12,15) /* Character length in bits per character */
#define	SPMODEn_CSBEF	__PPCBITS(16,19) /* CS assertion time in bits before frame start */
#define	SPMODEn_CSAFT	__PPCBITS(20,23) /* CS assertion time in bits after frame end */
#define	SPMODEn_CSCG	__PPCBITS(24,28) /* Clock gap */
#endif

#define	SATA1_BASE	0x18000	/* MPC8536 */
#define	SATA2_BASE	0x19000	/* MPC8536 */
#define	SATA_SIZE	0x01000

#define	USB1_BASE	0x22100	/* MPC8536 */
#define	USB2_BASE	0x23100	/* MPC8536 */
#define	USB3_BASE	0x2b100	/* MPC8536 */
#define	USB_SNOOP1	0x0300	/* DMA Snooping Register 1 */
#define	USB_SNOOP2	0x0304	/* DMA Snooping Register 2 */
#define	USB_CONTROL	0x0400	/* USB General Purpose Register */
#define	USB_EN		__PPCBIT(29)
#define	USB_ULPI_INT_EN	__PPCBIT(31)
#define	USB_SIZE	0x00f00

#define	SNOOP_2GB	0x1e

#define	ETSEC1_BASE	0x24000
#define	ETSEC2_BASE	0x25000
#define	ETSEC3_BASE	0x26000
#define	ETSEC4_BASE	0x27000
#define	ETSEC1_G0_BASE	0xB0000
#define	ETSEC2_G0_BASE	0xB1000
#define	ETSEC3_G0_BASE	0xB2000
#define	ETSEC1_G1_BASE	0xB4000
#define	ETSEC2_G1_BASE	0xB5000
#define	ETSEC3_G1_BASE	0xB6000
#define	ETSEC_SIZE	0x01000

#define	ESDHC_BASE	0x2e000
#define	ESDHC_SIZE	0x01000

#ifdef ESDHC_PRIVATE

#define	DCR		0x40c		/* DMA Control Register */

#define	DCR_SNOOP	__PPCBIT(25)	/* DMA transactions are snooped */
#define	DCR_RD_SAFE	__PPCBIT(29)	/* memory is read safe */
#define	DCR_RD_PFE	__PPCBIT(30)	/* memory is prefetch safe */
#define	DCR_RD_PF_SIZE	__PPCBIT(31)	/* prefetch size is 32-bytes */

#endif

#define	GLOBAL_BASE	0xe0000
#define	GLOBAL_SIZE	0x01000

#ifdef GLOBAL_PRIVATE

/* Power-On Reset Configuration Values */
#define PORPLLSR	0x000 /* POR PLL ratio status register */
#define	E500_RATIO2	__PPCBITS(2,7)
#define	E500_RATIO2_GET(n) __SHIFTOUT(n, E500_RATIO2)
#define	E500_RATIO	__PPCBITS(10,15)
#define	E500_RATIO_GET(n) __SHIFTOUT(n, E500_RATIO)
#define	PCI1_CLK_SEL	__PPCBIT(16)
#define	PCI2_CLK_SEL	__PPCBIT(17)
#define	PLAT_RATIO	__PPCBITS(26,30)
#define	PLAT_RATIO_GET(n) __SHIFTOUT(n, PLAT_RATIO)
#define PORBMSR		0x004 /* POR boot mode status register */
#define	PORBMSR_BCFG	__PPCBITS(0,1)
#define	PORBMSR_HA	__PPCBITS(13,15)
#define	PORBMSR_HA_GET(n) __SHIFTOUT(m, PORBMSR_HA)
#define	PORBMSR_HA_PEXSRIO_AGENT	0 /* PCI Express & SRIO agent mode */
#define	PORBMSR_HA_SRIO_AGENT		1 /* SRIO agent mode */
#define	PORBMSR_HA_PEX_AGENT		2 /* PCI Express agent mode */
#define	PORBMSR_HA_PEXPCI_AGENT2	3 /* PCI[-X] & PCI Express agent mode */
#define	PORBMSR_HA_PCISRIO_AGENT2	4 /* PCI[-X] & SRIO mode */
#define	PORBMSR_HA_SRIO_AGENT2		5 /* SRIO agent mode */
#define	PORBMSR_HA_PCI_AGENT2		6 /* PCI[-X] agent mode */
#define	PORBMSR_HA_HOST			7 /* Host mode */
#define PORIMPSCR	0x008 /* POR I/O impedance status and control register */
#define PORDEVSR	0x00C /* POR I/O device status register */
#define	PORDEVSR_ECW1		__PPCBIT(0)
#define	PORDEVSR_ECW2		__PPCBIT(1)
#define	PORDEVSR_SGMII1_DIS1	__PPCBIT(2)
#define	PORDEVSR_SGMII1_DIS2	__PPCBIT(3)
#define	PORDEVSR_SGMII1_DIS3	__PPCBIT(4)
#define	PORDEVSR_SGMII1_DIS4	__PPCBIT(5)
#define	PORDEVSR_ECP1		__PPCBITS(6,7)
#define	PORDEVSR_PCI1		__PPCBIT(8)
#define	PCI1_PCIX		0
#define	PCI1_PCI1		1
#define	PORDEVSR_IOSEL_P1023	__PPCBITS(9,10)
#define	IOSEL_P1023_PCIE12_X1		0
#define	IOSEL_P1023_PCIE123_X1		1
#define	IOSEL_P1023_PCIE123_X1_SGMII2	2
#define	IOSEL_P1023_PCIE12_X1_SGMII12	3
#define	PORDEVSR_IOSEL		__PPCBITS(9,12)
#define	IOSEL_MPC8536_OFF		0x01
#define	IOSEL_MPC8536_PCIE1_X4		0x02
#define	IOSEL_MPC8536_PCIE1_X8		0x03
#define	IOSEL_MPC8536_PCIE12_X4		0x05
#define	IOSEL_MPC8536_PCIE1_X4_PCI23_X2	0x07
#define	IOSEL_MPC8544_OFF		0x00
#define	IOSEL_MPC8544_SGMII_ON		0x01
#define	IOSEL_MPC8544_PCIE1_ON		0x02
#define	IOSEL_MPC8544_PCIE1_SGMII_ON	0x03
#define	IOSEL_MPC8544_PCIE12_ON		0x04
#define	IOSEL_MPC8544_PCIE12_SGMII_ON	0x05
#define	IOSEL_MPC8544_PCIE123_ON	0x06
#define	IOSEL_MPC8544_PCIE123_SGMII_ON	0x07
#define	IOSEL_MPC8548_SRIO2500_PCIE1_X4	3
#define	IOSEL_MPC8548_SRIO1250_PCIE1_X4	4
#define	IOSEL_MPC8548_SRIO3125		5
#define	IOSEL_MPC8548_SRIO1250		6
#define	IOSEL_MPC8548_PCIE1_X8		7
#define IOSEL_MPC8572_PCIE1_X4		2
#define IOSEL_MPC8572_PCIE12_X4		3
#define IOSEL_MPC8572_SRIO2500		6
#define IOSEL_MPC8572_PCIE1_X4_23_X2	7
#define	IOSEL_MPC8572_SRIO2500_PCIE1_X4	11
#define	IOSEL_MPC8572_SRIO1250_PCIE1_X4	12
#define	IOSEL_MPC8572_SRIO3125		13
#define	IOSEL_MPC8572_SRIO1250		14
#define	IOSEL_MPC8572_PCIE1_X8		15
#define	IOSEL_P20x0_PCIE1_X1		0
#define	IOSEL_P20x0_PCIE12_X1_3_X2	2
#define	IOSEL_P20x0_PCIE13_X2		4
#define	IOSEL_P20x0_PCIE1_X4		6
#define	IOSEL_P20x0_PCIE1_X1_SRIO2500_1X	13
#define	IOSEL_P20x0_PCIE12_X1_SGMII23	14
#define	IOSEL_P20x0_PCIE1_X2_SGMII23	15
#define	IOSEL_P1025_PCIE1_X1		0	/* same at P20x10 */
#define	IOSEL_P1025_PCIE1_X4		6	/* same at P20x10 */
#define	IOSEL_P1025_PCIE12_X1_SGMII23	14	/* same at P20x10 */
#define	IOSEL_P1025_PCIE1_X2_SGMII23	15	/* same at P20x10 */
#define	PORDEVSR_PCI2_ARB	__PPCBIT(13)
#define	PORDEVSR_PCI1_ARB	__PPCBIT(14)
#define	PORDEVSR_PCI32		__PPCBIT(15)
#define	PCI32_FALSE		0
#define	PCI32_TRUE		1
#define	PORDEVSR_PCI1_SPD	__PPCBIT(16)
#define	PORDEVSR_PCI2_SPD	__PPCBIT(17)
#define	PORDEVSR_SYS_SPD	__PPCBIT(17)	/* MPC8536 */
#define	PORDEVSR_CORE_SPD	__PPCBIT(18)	/* MPC8536 */
#define	PORDEVSR_ECP2		__PPCBITS(18,19)
#define	PORDEVSR_ECP3		__PPCBITS(20,21)
#define	PORDEVSR_ECP4		__PPCBITS(22,23)
#define	PORDEVSR_FEC_DIS	__PPCBIT(24)
#define	PORDEVSR_RTPE		__PPCBIT(25)
#define	PORDEVSR_RIO_CTLS	__PPCBIT(28)
#define	PORDEVSR_DEV_ID		__PPCBITs(29,31)
#define PORDBGMSR	0x010 /* POR debug mode status register */
#define PORDEVSR2	0x014 /* POR I/O device status register 2 */
#define GPPORCR		0x020 /* General-purpose POR configuration register */

/* Signal Multiplexing and GPIO Controls */
#define GPIOCR		0x030 /* GPIO control register */
#define	GPIOCR_TX2	__PPCBIT(6)	/* Enable TSEC2_TX[7:0] as GP output */
#define	GPIOCR_RX2	__PPCBIT(7)	/* Enable TSEC2_RX[7:0] as GP input */
#define	GPIOCR_PCIOUT	__PPCBIT(14)	/* Enable PCI2_AD[15:8] as GP output */
#define	GPIOCR_PCIIN	__PPCBIT(15)	/* Enable PCI2_AD[7:0] as GP input */
#define	GPIOCR_GPOUT	__PPCBIT(22)	/* Enable GPOUT[24:31] as GP output */
#define GPOUTDR		0x040 /* General-purpose output data register */
#define GPOUTDR_TX2	0x040 /* General-purpose output data register */
#define GPOUTDR_PCI	0x041 /* General-purpose output data register */
#define GPOUTDR_GPOUT	0x043 /* General-purpose output data register */
#define GPINDR		0x050 /* General-purpose input data register */
#define	GPINDR_RX2	0x059
#define	GPINDR_PCI	0x051

#define PMUXCR		0x060 /* Alternate function signal multiplex control */
#define	PMUXCR_SD_DATA	__PPCBIT(0)
#define	PMUXCR_SDHC_CD	__PPCBIT(1)
#define	PMUXCR_SDHC_WP	__PPCBIT(2)
#define	PMUXCR_PCI_REQGNT3 __PPCBIT(3)
#define	PMUXCR_TSEC1_TS __PPCBIT(3)
#define	PMUXCR_PCI_REQGNT4 __PPCBIT(4)
#define	PMUXCR_TSEC2_TS __PPCBIT(4)
#define	PMUXCR_USB1	__PPCBIT(5)
#define	PMUXCR_TSEC3_TS __PPCBIT(5)
#define	PMUXCR_USB2	__PPCBIT(6)
#define	PMUXCR_USB_PCTL	__PPCBITS(6,5)
#define	PMUXCR_USB	__PPCBIT(6)
#define	PMUXCR_TSEC1	__PPCBIT(14)
#define	PMUXCR_DMA0	__PPCBIT(14)
#define	PMUXCR_DMA2	__PPCBIT(15)
#define	PMUXCR_QE0	__PPCBIT(16)
#define	PMUXCR_QE1	__PPCBIT(17)
#define	PMUXCR_QE2	__PPCBIT(18)
#define	PMUXCR_QE3	__PPCBIT(19)
#define	PMUXCR_QE8	__PPCBIT(24)
#define	PMUXCR_QE9	__PPCBIT(25)
#define	PMUXCR_QE10	__PPCBIT(26)
#define	PMUXCR_QE11	__PPCBIT(27)
#define	PMUXCR_QE12	__PPCBIT(28)
#define	PMUXCR_DMA1	__PPCBIT(30)
#define	PMUXCR_DMA3	__PPCBIT(31)

#define PMUXCR2		0x064 /* Alternate function signal multiplex control2 */

/* Device Disables */
#define DEVDISR		0x070 /* Device disable control */
#define	DEVDISR_PCI1	__PPCBIT(0)
#define	DEVDISR_QMAN_BMAN __PPCBIT(0)	/* P1023 */
#define	DEVDISR_PCI2	__PPCBIT(1)
#define	DEVDISR_FMAN	__PPCBIT(1)	/* P1023 */
#define	DEVDISR_PCIE	__PPCBIT(2)
#define	DEVDISR_MACSEC	__PPCBIT(3)	/* P1023 */
#define	DEVDISR_LBC	__PPCBIT(4)
#define	DEVDISR_PCIE2	__PPCBIT(5)
#define	DEVDISR_PCIE3	__PPCBIT(6)
#define	DEVDISR_SEC	__PPCBIT(7)
#define	DEVDISR_PME	__PPCBIT(8)
#define	DEVDISR_USB1	__PPCBIT(8)	/* MPC8536 */
#define	DEVDISR_TLU1	__PPCBIT(9)
#define	DEVDISR_USB2	__PPCBIT(9)	/* MPC8536 */
#define	DEVDISR_TLU2	__PPCBIT(10)
#define	DEVDISR_ESDHC_10 __PPCBIT(10)
#define	DEVDISR_USB3	__PPCBIT(10)	/* MPC8536 */
#define	DEVDISR_L2	__PPCBIT(11)	/* MPC8536 */
#define	DEVDISR_SRIO	__PPCBIT(12)
#define	DEVDISR_ESDHC_12 __PPCBIT(12)	/* MPC8536 */
#define	DEVDISR_RMSG	__PPCBIT(13)
#define	DEVDISR_SATA1	__PPCBIT(13)	/* MPC8536 */
#define	DEVDISR_DDR2_14	__PPCBIT(14)
#define	DEVDISR_DDR_15	__PPCBIT(15)
#define	DEVDISR_SPI_15	__PPCBIT(15)	/* MPC8536 */
#define	DEVDISR_E500	__PPCBIT(16)
#define	DEVDISR_DDR_16	__PPCBIT(16)	/* MPC8536 */
#define	DEVDISR_TB	__PPCBIT(17)
#define	DEVDISR_E500_1	__PPCBIT(18)
#define	DEVDISR_TB_1	__PPCBIT(19)
#define	DEVDISR_SATA2	__PPCBIT(20)	/* MPC8536 */
#define	DEVDISR_DMA	__PPCBIT(21)
#define	DEVDISR_DMA2	__PPCBIT(22)
#define	DEVDISR_SRDS2	__PPCBIT(22)	/* MPC8536 */
#define	DEVDISR_TSEC1	__PPCBIT(24)
#define	DEVDISR_TSEC2	__PPCBIT(25)
#define	DEVDISR_TSEC3	__PPCBIT(26)
#define	DEVDISR_TSEC4	__PPCBIT(27)
#define	DEVDISR_FEC	__PPCBIT(28)
#define	DEVDISR_SPI_28	__PPCBIT(28)	/* P2020 */
#define	DEVDISR_I2C	__PPCBIT(29)
#define	DEVDISR_DUART	__PPCBIT(30)
#define	DEVDISR_SRDS1	__PPCBIT(31)	/* MPC8536 */

/* Power Management Registers */
#define POWMGTCSR	0x080 /* Power management status and control register */

/* Interrupt and Reset Status and Control */
#define MCPSUMR		0x090 /* Machine check summary register */
#define RSTRSCR		0x094 /* Reset request status and control register */

/* Version Registers */
#define PVR		0x0A0 /* Processor version register */
#define SVR		0x0A4 /* System version register */

/* Control Pin Registers (GPIO) for P1025  */
#define	CPBASE(n)	(0x100+0x20*(n))	/* Control Pin (GPIO) base */
#define	CPODR		0x0000			/* Open Drain */
#define	CPDAT		0x0004			/* Output Data */
#define	CPDIR1		0x0008			/* Direction1 */
#define	CPDIR2		0x000c			/* Direction2 */
#define	CPPAR1		0x0010			/* Pin Assignment1 */
#define	CPPAR2		0x0014			/* Pin Assignment2 */

#define	CPDIR_DIS	0
#define	CPDIR_OUT	1
#define	CPDIR_IN	2
#define	CPDIR_INOUT	3

#define	CPPAR_FUNC0	0
#define	CPPAR_FUNC1	1
#define	CPPAR_FUNC2	2
#define	CPPAR_FUNC3	3

/* Status Registers */
#define RSTCR		0x0B0 /* Reset control register */
#define	HRESET_REQ	__PPCBIT(30) /* hardware reset request */
#define LBCVSELCR	0x0C0 /* LBC voltage select control register */
#define DDRCSR		0xB20 /* DDR calibration status register */
#define DDRCDR		0xB24 /* DDR control driver register */
#define DDRCLKDR	0xB28 /* DDR clock disable register */

/* Debug Control */
#define CLKOCR		0xE00 /* Clock out control register */
#define SRDSCR0		0xF04 /* LSerDes control register 0 */
#define SRDSCR1		0xF08 /* LSerDes control register 1 */
#define TSEC12IOOVCR	0xF28 /* eTSEC 1 & 2 overdrive control register */
#define TSEC34IOOVCR	0xF2C /* eTSEC 3 & 4 overdrive control register */
#endif /* GLOBAL_PRIVATE */

#define	LBC_BASE	0x5000
#define	LBC_SIZE	0x0fff

#ifdef LBC_PRIVATE

#define	BR_BA		__PPCBITS(0,16)
#define	BR_XBA		__PPCBITS(17,18)
#define	BR_PS		__PPCBITS(19,20)
#define	BR_PS_8BIT	__SHIFTIN(1,BR_PS)
#define	BR_PS_16BIT	__SHIFTIN(2,BR_PS)
#define	BR_PS_32BIT	__SHIFTIN(3,BR_PS)
#define	BR_DECC		__PPCBITS(21,22)
#define	BR_DECC_NONE	__SHIFTIN(0,BR_DECC)
#define	BR_DECC_PARITY	__SHIFTIN(1,BR_DECC)
#define	BR_DECC_RMWPAR	__SHIFTIN(2,BR_DECC)
#define	BR_WP		__PPCBIT(23)
#define	BR_MSEL		__PPCBITS(24,26)
#define	BR_MSEL_GPCM	__SHIFTIN(0,BR_MSEL)
#define	BR_MSEL_FCM	__SHIFTIN(1,BR_MSEL)
#define	BR_MSEL_SDRAM	__SHIFTIN(3,BR_MSEL)
#define	BR_MSEL_UPMA	__SHIFTIN(4,BR_MSEL)
#define	BR_MSEL_UPMB	__SHIFTIN(5,BR_MSEL)
#define	BR_MSEL_UPMC	__SHIFTIN(6,BR_MSEL)
#define	BR_ATOM		__PPCBITS(28,29)
#define	BR_ATOM_NONE	__SHIFTIN(0,BR_ATOM)
#define	BR_ATOM_RAWA	__SHIFTIN(1,BR_ATOM)
#define	BR_ATOM_WARA	__SHIFTIN(2,BR_ATOM)
#define	BR_V		__PPCBIT(31)

#define	OR_AM		__PPCBITS(0,16)
#define	OR_XAM		__PPCBITS(17,18)
#define	OR_BCTLD	__PPCBIT(19)
#define	OR_CSNT		__PPCBIT(20)
#define	OR_ACS		__PPCBITS(21,22)
#define	OR_XACS		__PPCBIT(23)
#define	OR_SCY		__PPCBITS(24,27)
#define	OR_SETA		__PPCBIT(28)
#define	OR_TRLX		__PPCBIT(29)
#define	OR_EHTR		__PPCBIT(30)
#define	OR_EAD		__PPCBIT(31)

#define	BRn(n)		(BR0 + 8*(n))
#define	ORn(n)		(OR0 + 8*(n))
#define BR0		0x000 /* Base register 0 */
#define OR0		0x004 /* Options register 0 */
#define BR1		0x008 /* Base register 1 */
#define OR1		0x00C /* Options register 1 */
#define BR2		0x010 /* Base register 2 */
#define OR2		0x014 /* Options register 2 */
#define BR3		0x018 /* Base register 3 */
#define OR3		0x01C /* Options register 3 */
#define BR4		0x020 /* Base register 4 */
#define OR4		0x024 /* Options register 4 */
#define BR5		0x028 /* Base register 5 */
#define OR5		0x02C /* Options register 5 */
#define BR6		0x030 /* Base register 6 */
#define OR6		0x034 /* Options register 6 */
#define BR7		0x038 /* Base register 7 */
#define OR7		0x03C /* Options register 7 */
#define MAR		0x068 /* UPM address register */
#define MAMR		0x070 /* UPMA mode register */
#define MBMR		0x074 /* UPMB mode register */
#define MCMR		0x078 /* UPMC mode register */
#define MRTPR		0x084 /* Memory refresh timer prescaler register */
#define MDR		0x088 /* UPM/FCM data register */
#define	MDR_AS3		__PPCBITS(0,7)
#define	MDR_AS2		__PPCBITS(8,15)
#define	MDR_AS1		__PPCBITS(16,23)
#define	MDR_AS0		__PPCBITS(24,31)
#define	LSOR		0x090 /* Special Operation Initiation register */
#define LSDMR		0x094 /* SDRAM mode register */
#define LURT		0x0A0 /* UPM refresh timer */
#define LSRT		0x0A4 /* SDRAM refresh timer */
#define LTESR		0x0B0 /* Transfer error status register */
#define	LTESR_BM	__PPCBIT(0)
#define	LTESR_FCT	__PPCBIT(1)
#define	LTESR_PAR	__PPCBIT(2)
#define	LTESR_WP	__PPCBIT(5)
#define	LTESR_ATMW	__PPCBIT(8)
#define	LTESR_ATMR	__PPCBIT(9)
#define	LTESR_CS	__PPCBIT(12)
#define	LTESR_UCC	__PPCBIT(30)
#define	LTESR_CC	__PPCBIT(31)
#define LTEDR		0x0B4 /* Transfer error disable register */
#define	LTEDR_BMD	__PPCBIT(0)
#define	LTEDR_FCTD	__PPCBIT(1)
#define	LTEDR_PARD	__PPCBIT(2)
#define	LTEDR_WPD	__PPCBIT(5)
#define	LTEDR_WARA	__PPCBIT(8)
#define	LTEDR_RAWA	__PPCBIT(9)
#define	LTEDR_CSD	__PPCBIT(12)
#define	LTEDR_UCCD	__PPCBIT(30)
#define	LTEDR_CCD	__PPCBIT(31)
#define LTEIR		0x0B8 /* Transfer error interrupt register */
#define	LTEIR_BMI	__PPCBIT(0)
#define	LTEIR_FCTI	__PPCBIT(1)
#define	LTEIR_PARI	__PPCBIT(2)
#define	LTEIR_WPI	__PPCBIT(5)
#define	LTEIR_WARA	__PPCBIT(8)
#define	LTEIR_RAWA	__PPCBIT(9)
#define	LTEIR_CSI	__PPCBIT(12)
#define	LTEIR_UCCI	__PPCBIT(30)
#define	LTEIR_CCI	__PPCBIT(31)
#define LTEATR		0x0BC /* Transfer error attributes register */
#define	LTEATR_RWB	__PPCBIT(3)
#define	LTEATR_SRCID	__PPCBITS(11,15)
#define	LTEATR_PB	__PPCBITS(16,19)
#define	LTEATR_BNK	__PPCBITS(20,27)
#define	LTEATR_V	__PPCBIT(31)
#define LTEAR		0x0C0 /* Transfer error address register */
#define	LTECCR		0x0C4 /* Transfer error ECC register */
#define	LTECCR_SBCE	__PPCBITS(12,15)
#define	LTECCR_MBUE	__PPCBITS(28,31)
#define LBCR		0x0D0 /* Configuration register */
#define LCRR		0x0D4 /* Clock ratio register */

#define	FMR		0x0E0 /* Flash Mode Register */
#define	FMR_CWTO	__PPCBITS(16,19)
#define	FMR_BOOT	__PPCBIT(20)
#define	FMR_ECCM	__PPCBIT(23)
#define	FMR_AL		__PPCBITS(26,27)
#define	FMR_OP		__PPCBITS(30,31)
#define	FIR		0x0E4 /* Flash Instruction Register */
#define	FIR_OP0		__PPCBITS(0,3)
#define	FIR_OP1		__PPCBITS(4,7)
#define	FIR_OP2		__PPCBITS(8,11)
#define	FIR_OP3		__PPCBITS(12,15)
#define	FIR_OP4		__PPCBITS(16,19)
#define	FIR_OP5		__PPCBITS(20,23)
#define	FIR_OP6		__PPCBITS(24,27)
#define	FIR_OP7		__PPCBITS(28,31)
#define	FIR_OP_NOP	0
#define	FIR_OP_CA	1	/* Issue current column address */
#define	FIR_OP_PA	2	/* Issue current block+page address */
#define	FIR_OP_UA	3	/* Issue user-defined address byte */
#define	FIR_OP_CM0	4	/* Issue command from FCR[CMD0] */
#define	FIR_OP_CM1	5	/* Issue command from FCR[CMD1] */
#define	FIR_OP_CM2	6	/* Issue command from FCR[CMD2] */
#define	FIR_OP_CM3	7	/* Issue command from FCR[CMD3] */
#define	FIR_OP_WB	8	/* Write FBCR bytes of data */
#define	FIR_OP_WS	9	/* Write one byte of data from MDR */
#define	FIR_OP_RB	10	/* Read FBCR bytes of data */
#define	FIR_OP_RS	11	/* Read one byte of data into MDR */
#define	FIR_OP_CW0	12	/* Wait for LFRB then FCR[CMD0] */
#define	FIR_OP_CW1	13	/* Wait for LFRB then FCR[CMD1] */
#define	FIR_OP_RBW	14	/* Wait for LFRB then read FBCR bytes */
#define	FIR_OP_RSW	15	/* Wait for LFRB then byte into MDR */
#define	FCR		0xE8 /* Flash Command Register */
#define	FCR_CMD0	__PPCBITS(0,7)
#define	FCR_CMD1	__PPCBITS(8,15)
#define	FCR_CMD2	__PPCBITS(16,23)
#define	FCR_CMD3	__PPCBITS(24,31)
#define	FBAR		0xEC /* Flash Block Address Register */
#define	FBAR_BLK	__PPCBITS(8,31)
#define	FPAR		0xF0 /* Flash Page Address Register */
#define	FPAR_S_PI	__PPCBITS(17,21)	/* Page Index */
#define	FPAR_S_MS	__PPCBIT(22)		/* Main(0)/Spare(1) */
#define	FPAR_S_CI	__PPCBITS(23,31)	/* Column Index */
#define	FPAR_L_PI	__PPCBITS(14,19)	/* Page Index */
#define	FPAR_L_MS	__PPCBIT(20)		/* Main(0)/Spare(1) */
#define	FPAR_L_CI	__PPCBITS(21,31)	/* Column Index */
#define	FBCR		0xF4 /* Flash Byte Count Register */
#define	FBCR_BC		__PPCBITS(20,31)
#define	FECC0		0x100
#define	FECC_V		__PPCBIT(0)
#define	FECC_ECC	__PPCBIT(8,31)
#define	FECC1		0x104
#define	FECC2		0x108
#define	FECC3		0x10C

#define MXMR_RFEN	__PPCBIT(1)	/* Refresh enable */
#define MXMR_OP		__PPCBITS(2,3)	/* Command opcode */
#define	MXMR_OP_NORMAL	__SHIFTIN(0, MXMR_OP)	/* Normal Operation */
#define	MXMR_OP_WRITE	__SHIFTIN(1, MXMR_OP)	/* Write to UPM memory */
#define	MXMR_OP_READ	__SHIFTIN(2, MXMR_OP)	/* Read from UPM memory */
#define	MXMR_OP_RUN	__SHIFTIN(3, MXMR_OP)	/* Run Pattern */
#define MXMR_UWPL	__PPCBIT(3)	/* LUPWAIT is active low */
#define MXMR_AM		__PPCBITS(5,7)	/* Address multiplex size */
#define MXMR_DS		__PPCBITS(8,9)	/* Disable timer period */
#define	MXMR_DS_1CYCLE	__SHIFTIN(0,MXMR_DS)
#define	MXMR_DS_2CYCLE	__SHIFTIN(1,MXMR_DS)
#define	MXMR_DS_3CYCLE	__SHIFTIN(2,MXMR_DS)
#define	MXMR_DS_4CYCLE	__SHIFTIN(3,MXMR_DS)
#define MXMR_G0CL	__PPCBITS(10,12)	/* General line 0 control */
#define	MXMR_G0CL_A12	__SHIFTIN(0,MXMR_G0CL)
#define	MXMR_G0CL_A11	__SHIFTIN(1,MXMR_G0CL)
#define	MXMR_G0CL_A10	__SHIFTIN(2,MXMR_G0CL)
#define	MXMR_G0CL_A9	__SHIFTIN(3,MXMR_G0CL)
#define	MXMR_G0CL_A8	__SHIFTIN(4,MXMR_G0CL)
#define	MXMR_G0CL_A7	__SHIFTIN(5,MXMR_G0CL)
#define	MXMR_G0CL_A6	__SHIFTIN(6,MXMR_G0CL)
#define	MXMR_G0CL_A5	__SHIFTIN(7,MXMR_G0CL)
#define MXMR_GPL4	__PPCBIT(13)	/* LGPL4 output line disable */
#define MXMR_RLF	__PPCBITS(14,17)	/* Read loop field */
#define MXMR_WLF	__PPCBITS(18,21)	/* Write loop field */
#define MXMR_TLF	__PPCBITS(22,25)	/* Refresh loop field */
#define MXMR_MAS	__PPCBITS(26,31)	/* Machine Address */

#define	MRTPR_PTP	__PPCBITS(0,7)		/* Refresh timers prescaler */

#endif /* LBC_PRIVATE */
