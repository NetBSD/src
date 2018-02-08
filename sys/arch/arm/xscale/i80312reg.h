/*	$NetBSD: i80312reg.h,v 1.12 2018/02/08 09:05:17 dholland Exp $	*/

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
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

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com>.
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

#ifndef _ARM_XSCALE_I80312REG_H_ 
#define _ARM_XSCALE_I80312REG_H_ 

/*
 * Register definitions for the Intel 80310 I/O Companion Chip.
 */

/*
 * Physical addresses 0x1000..0x1fff are used by the Periphial Memory
 * Mapped Registers.
 */

#define	I80312_PMMR_BASE	0x00001000UL
#define	I80312_PMMR_SIZE	0x00001000UL

/*
 * The PMMR registers below are defined as offsets from the i80312 PMMR
 * base.
 */

/*
 * PCI-to-PCI Bridge Unit
 */
#define	I80312_PPB_BASE		(0)
#define	I80312_PPB_SIZE		0x100
/*
 * Performance Monitoring Unit
 */
#define	I80312_PMU_BASE		(I80312_PPB_BASE  + I80312_PPB_SIZE) /* 0x100 */
#define	I80312_PMU_SIZE		0x100
/*
 * Address Translation Unit
 */
#define	I80312_ATU_BASE		(I80312_PMU_BASE  + I80312_PMU_SIZE) /* 0x200 */
#define	I80312_ATU_SIZE		0x100
/*
 * Messaging Unit
 */
#define	I80312_MSG_BASE		(I80312_ATU_BASE  + I80312_ATU_SIZE) /* 0x300 */
#define	I80312_MSG_SIZE		0x100
/*
 * DMA Controller
 */
#define	I80312_DMA_BASE		(I80312_MSG_BASE  + I80312_MSG_SIZE) /* 0x400 */
#define	I80312_DMA_SIZE		0x100
/*
 * Memory Controller
 */
#define	I80312_MEM_BASE		(I80312_DMA_BASE  + I80312_DMA_SIZE) /* 0x500 */
#define	I80312_MEM_SIZE		0x100
/*
 * Internal Arbitration Unit
 */
#define	I80312_IARB_BASE	(I80312_MEM_BASE  + I80312_MEM_SIZE) /* 0x600 */
#define	I80312_IARB_SIZE	0x040
/*
 * Bus Interface Unit
 */
#define	I80312_BUS_BASE		(I80312_IARB_BASE + I80312_IARB_SIZE)/* 0x640 */
#define	I80312_BUS_SIZE		0x040
/*
 * I2C Unit
 */
#define	I80312_IIC_BASE		(I80312_BUS_BASE  + I80312_BUS_SIZE) /* 0x680 */
#define	I80312_IIC_SIZE		0x080
/*
 * Interrupt Controller
 */
#define	I80312_INTC_BASE	(I80312_IIC_BASE  + I80312_IIC_SIZE) /* 0x700 */
#define	I80312_INTC_SIZE	0x100
/*
 * Application Accelerator Unit
 */
#define	I80312_AAU_BASE		(I80312_INTC_BASE + I80312_INTC_SIZE)/* 0x800 */
#define	I80312_AAU_SIZE		0x100

/*
 * PCI-PCI Bridge Unit
 *
 * The PCI-PCI Bridge Unit supports both public (accessible to the
 * host) and private (accessible only to the local system) devices:
 *
 *	---------
 *		S_AD[11]
 *		S_AD[12]
 * Private	S_AD[13]
 *		S_AD[14]
 *		S_AD[15]
 *	---------
 *		S_AD[16]	SISR bit 9
 *		S_AD[17]	SISR bit 8
 *		S_AD[18]	SISR bit 7
 * Public	S_AD[19]	SISR bit 6
 * or		S_AD[20]	SISR bit 5
 * Private	S_AD[21]	SISR bit 4
 *		S_AD[22]	SISR bit 3
 *		S_AD[23]	SISR bit 2
 *		S_AD[24]	SISR bit 1
 *		S_AD[25]	SISR bit 0
 *	---------
 *		S_AD[26]
 *		S_AD[27]
 * Public	S_AD[28]
 *		S_AD[29]
 *		S_AD[30]
 *		S_AD[31]
 *	---------
 *
 * Setting the specified SISR bit makes the corresponding S_AD line
 * a private service.
 */
#define	I80312_PPB_EBCR		0x40	/* Extended Bridge Control */
#define	I80312_PPB_SISR		0x42	/* Secondary ID Select Register */
#define	I80312_PPB_PBISR	0x44	/* Primary Bridge Int. Stat. */
#define	I80312_PPB_SBISR	0x48	/* Secondary Bridge Int. Stat. */
#define	I80312_PPB_SACR		XXX	/* Secondary Arb. Control */
#define	I80312_PPB_PIRSR	XXX	/* PCI Int. Routing Select */
#define	I80312_PPB_SIOBR	0x54	/* Secondary I/O Base Register */
#define	I80312_PPB_SIOLR	0x55	/* Secondary I/O Limit Register */
#define	I80312_PPB_SCDR		0x56	/* Secondary Clock Disable Register */
#define	I80312_PPB_SMBR		0x58	/* Secondary Memory Base Register */
#define	I80312_PPB_SMLR		0x5a	/* Secondary Memory Limit Register */
#define	I80312_PPB_SDER		0x5c	/* Secondary Decode Enable Register */
#define	I80312_PPB_QCR		0x5e	/* Queue Control Register */

#define	PPB_SDER_PMSE		(1U << 2) /* Private Memory Space Enable */

/*
 * Performance Monitoring Unit
 */
#define	I80312_PMU_GTMR		0x00	/* Global Timer Mode Register */
#define	I80312_PMU_ESR		0x04	/* Event Select Register */
#define	I80312_PMU_EMISR	0x08	/* Event Monitoring Int Stat Reg */
#define	I80312_PMU_GTSR		0x10	/* Global Time Stamp Register */
					/* Programmable Event Counter Regs */
#define	I80312_PMU_PECR(x)	(0x14 + (4 * ((x) - 1)))

#define	PMU_GTMR_INTEN		(1U << 0)
#define	PMU_GTMR_CNTRDIS	(1U << 2)

#define	PMU_ESR_MODE(x)		((x))
#define	PMU_ESR_PMIE		(1U << 16)

#define	PMU_EMISR_GTS		(1U << 0)
#define	PMU_EMISR_PECRS(x)	(1U << (x))

/*
 * Address Translation Unit
 * The first 64 bytes are identical to a PCI device's config space.
 */
/*	BAR #0			0x10	Primary Inbound ATU Base Address */
#define	I80312_ATU_PIAL		0x40	/* Pri. Inbound ATU Limit */
#define	I80312_ATU_PIATV	0x44	/* Pri. Inbound ATU Translate Value */
#define	I80312_ATU_SIAM		0x48	/* Sec. Inbound ATU Base Address */
#define	I80312_ATU_SIAL		0x4c	/* Sec. Inbound ATU Limit */
#define	I80312_ATU_SIATV	0x50	/* Sec. Inbound ATU Translate Value */
#define	I80312_ATU_POMWV	0x54	/* Pri. Outbound Memory Window Value */
     /* not used		0x58 */
#define	I80312_ATU_POIOWV	0x5c	/* Pri. Outbound I/O Window Value */
#define	I80312_ATU_PODACWVL	0x60	/* Pri. Outbound DAC Window Value (Lo)*/
#define	I80312_ATU_PODACWVH	0x64	/* Pri. Outbound DAC Window Value (Hi)*/
#define	I80312_ATU_SOMWV	0x68	/* Sec. Outbound Memory Window Value */
#define	I80312_ATU_SOIOWV	0x6c	/* Sec. Outbound I/O Window Value */
     /* not used		0x70 */
#define	I80312_ATU_ERL		0x74	/* Expansion ROM Limit */
#define	I80312_ATU_ERTV		0x78	/* Expansion ROM Translate Value */
     /* not used		0x7c */
#define	I80312_ATU_ACI		0x74	/* ATU Capability Identifier */
#define	I80312_ATU_ATNIP	0x78	/* ATU Next Item Pointer */
#define	I80312_ATU_APM		0x7c	/* ATU Power Management */
     /* not used		0x84 */
#define	I80312_ATU_ACR		0x88	/* ATU Configuration */
     /* not used		0x8c */
#define	I80312_ATU_PAIS		0x90	/* Pri. ATU Interrupt Status */
#define	I80312_ATU_SAIS		0x94	/* Sec. ATU Interrupt Status */
#define	I80312_ATU_SACS		0x98	/* Sec. ATU Command/Status */
#define	I80312_ATU_SODACWVL	0x9c	/* Sec. Outbound DAC Window Value (lo)*/
#define	I80312_ATU_SODACWVH	0xa0	/* Sec. Outbound DAC Window Value (hi)*/
#define	I80312_ATU_POCCA	0xa4	/* Pri. Outbound Config Address Data */
#define	I80312_ATU_SOCCA	0xa8	/* Sec. Outbound Config Address Data */
#define	I80312_ATU_POCCD	0xac	/* Pri. Outbound Config Cycle Data */
#define	I80312_ATU_SOCCD	0xb0	/* Sec. Outbound Config Cycle Data */
#define	I80312_ATU_PAQC		0xb4	/* Pri. ATU Queue Control */
#define	I80312_ATU_SAQC		0xb8	/* Sec. ATU Queue Control */
#define	I80312_ATU_PAIM		0xbc	/* Pri. ATU Interrupt Mask */
#define	I80312_ATU_SAIM		0xc0	/* Sec. ATU Interrupt Mask */
     /* not used		0xc4 .. 0xfc */

#define	ATU_LIMIT(x) \
	((0xffffffffUL - ((x) - 1)) & 0xfffffff0UL)

#define	ATU_ACR_POAE		(1U << 1)
#define	ATU_ACR_SOAE		(1U << 2)
#define	ATU_ACR_SDAS		(1U << 7)
#define	ATU_ACR_DAE		(1U << 8)
#define	ATU_ACR_PSERRIE		(1U << 9)
#define	ATU_ACR_SSERRIE		(1U << 10)
#define	ATU_ACR_SBMUAE		(1U << 12)
#define	ATU_ACR_ADTS		(1U << 15)
#define	ATU_ACR_PSERRMA		(1U << 16)
#define	ATU_ACR_SSERRMA		(1U << 17)
#define	ATU_ACR_DAU2GTE		(1U << 18)
#define	ATU_ACR_PATUDRCA	(1U << 19)
#define	ATU_ACR_SATUDRCA	(1U << 20)
#define	ATU_ACR_BFN		(1U << 21)

#define	ATU_AIM_AETAE		(1U << 0)
#define	ATU_AIM_AIESE		(1U << 1)
#define	ATU_AIM_MPEIM		(1U << 2)
#define	ATU_AIM_TATIM		(1U << 3)
#define	ATU_AIM_TAMIM		(1U << 4)
#define	ATU_AIM_MAIM		(1U << 5)
#define	ATU_AIM_SAIM		(1U << 6)
#define	ATU_AIM_DPEIM		(1U << 7)
#define	ATU_AIM_PSTIM		(1U << 8)

/*
 * Messaging Unit
 */
     /* not used		0x00 .. 0x0c */
#define	I80312_MSG_IM0		0x10	/* Inbound Message 0 */
#define	I80312_MSG_IM1		0x14	/* Inbound Message 1 */
#define	I80312_MSG_OM0		0x18	/* Outbound Message 0 */
#define	I80312_MSG_OM1		0x1c	/* Outbound Message 1 */
#define	I80312_MSG_ID		0x20	/* Inbound Doorbell */
#define	I80312_MSG_IIS		0x24	/* Inbound Interrupt Status */
#define	I80312_MSG_IIM		0x28	/* Inbound Interrupt Mask */
#define	I80312_MSG_OD		0x2c	/* Outbound Doorbell */
#define	I80312_MSG_OIS		0x30	/* Outbound Interrupt Status */
#define	I80312_MSG_OIM		0x34	/* Outbound Interrupt Mask */
     /* not used		0x38 .. 0x4c */
#define	I80312_MSG_MC		0x50	/* MU Configuration */
#define	I80312_MSG_QBA		0x54	/* Queue Base Address */
     /* not used		0x58 .. 0x5c */
#define	I80312_MSG_IFHP		0x60	/* Inbound Free Head Pointer */
#define	I80312_MSG_IFTP		0x64	/* Inbound Free Tail Pointer */
#define	I80312_MSG_IPHP		0x68	/* Inbound Post Head Pointer */
#define	I80312_MSG_IPTP		0x6c	/* Inbound Post Tail Pointer */
#define	I80312_MSG_OFHP		0x70	/* Outbound Free Head Pointer */
#define	I80312_MSG_OFTP		0x74	/* Outbound Free Tail Pointer */
#define	I80312_MSG_OPHP		0x78	/* Outbound Post Head Pointer */
#define	I80312_MSG_OPTP		0x7c	/* Outbound Post Tail Pointer */
#define	I80312_MSG_IA		0x80	/* Index Address */
     /* not used		0x84 .. 0xfc */

/*
 * DMA Controller
 */
#define	I80312_DMA_CHAN0	0x00	/* Channel 0 */
#define	I80312_DMA_CHAN1	0x40	/* Channel 1 */
#define	I80312_DMA_CHAN2	0x80	/* Channel 2 */
     /* not used		0xc0 .. 0xfc */

#define	I80312_DMA_CC		0x00	/* Channel Control */
#define	I80312_DMA_CS		0x04	/* Channel Status */
     /* not used		0x08 */
#define	I80312_DMA_DA		0x0c	/* Descriptor Address */
#define	I80312_DMA_NDA		0x10	/* Next Descriptor Address */
#define	I80312_DMA_PA		0x14	/* PCI Address */
#define	I80312_DMA_PUA		0x18	/* PCI Upper Address */
#define	I80312_DMA_IBA		0x1c	/* Internal Bus Address */
#define	I80312_DMA_BC		0x20	/* Byte Count */
#define	I80312_DMA_DC		0x24	/* Descriptor Control */
     /* not used		0x28 .. 0x3c */

/*
 * Memory Controller
 */
#define	I80312_MEM_SI		0x00	/* SDRAM Initialization */
#define	I80312_MEM_SC		0x04	/* SDRAM Control */
#define	I80312_MEM_SB		0x08	/* SDRAM Base */
#define	I80312_MEM_SB0		0x0c	/* SDRAM Bank 0 Size */
#define	I80312_MEM_SB1		0x10	/* SDRAM Bank 1 Size */
     /* not used		0x14 .. 0x30 */
#define	I80312_MEM_EC		0x34	/* ECC Control */
#define	I80312_MEM_EL0		0x38	/* ECC Log 0 */
#define	I80312_MEM_EL1		0x3c	/* ECC Log 1 */
#define	I80312_MEM_EA0		0x40	/* ECC Address 0 */
#define	I80312_MEM_EA1		0x44	/* ECC Address 1 */
#define	I80312_MEM_ET		0x48	/* ECC Test */
#define	I80312_MEM_FB0		0x4c	/* ECC Flash Base 0 */
#define	I80312_MEM_FB1		0x50	/* ECC Flash Base 1 */
#define	I80312_MEM_FB0S		0x54	/* ECC Flash Bank 0 Size */
#define	I80312_MEM_FB1S		0x58	/* ECC Flash Bank 1 Size */
#define	I80312_MEM_FWS1		0x5c	/* ECC Wait State 1 Size */
#define	I80312_MEM_FWS0		0x60	/* ECC Wait State 0 Size */
#define	I80312_MEM_IS		0x65	/* ECC Interrupt Status */
#define	I80312_MEM_RF		0x68	/* ECC Refresh Frequency */
#define	I80312_MEM_RF		0x68	/* ECC Refresh Frequency */
#define	I80312_MEM_RF		0x68	/* ECC Refresh Frequency */
#define	I80312_MEM_RF		0x68	/* ECC Refresh Frequency */
     /* not used		0x6c .. 0xfc */

/*
 * Internal Arbitration Unit
 */
#define	I80312_ARB_IAC		0x00	/* Internal Aribtration Control */
#define	I80312_ARB_MLT		0x04	/* Master Latency Timer */
#define	I80312_ARB_MTT		0x08	/* Multi-Transaction Timer */
     /* not used		0x0c .. 0x3c */

/*
 * Bus(Core) Interface Unit
 */
     /* not used		0x40 */
#define	I80312_BUS_IS		0x44	/* Interrupt Status */
     /* not used		0x4c .. 0x7c */

/*
 * I2C Bus Interface Unit
 */
#define	I80312_IIC_CTL		0x80	/* Control */
#define	I80312_IIC_STS		0x84	/* Status */
#define	I80312_IIC_SA		0x88	/* Slave Address */
#define	I80312_IIC_DB		0x8c	/* Data Buffer */
#define	I80312_IIC_CC		0x90	/* Clock Control */
#define	I80312_IIC_BM		0x94	/* Bus Monitor */
     /* not used		0x98 .. 0xfc */

/*
 * PCI And Peripheral Interrupt (GPIO) Unit
 */
#define	I80312_INTC_IIS		0x00	/* IRQ Interrupt Status */
#define	I80312_INTC_F2IS	0x04	/* FIQ2 Interrupt Status */
#define	I80312_INTC_F1IS	0x08	/* FIQ1 Interrupt Status */
     /* not used		0x0c */
#define	I80312_INTC_PDI		0x10	/* Processor Device ID */
     /* not used		0x14 .. 0x18 */
#define	I80312_INTC_GOE		0x1c	/* GPIO Output Enable */
#define	I80312_INTC_GID		0x20	/* GPIO Input Data */
#define	I80312_INTC_GOD		0x24	/* GPIO Output Data */
     /* not used		0x28 .. 0xfc */

/*
 * Application Accelerator Registers
 */
#define	I80312_AAU_CTL		0x00	/* Control */
#define	I80312_AAU_STS		0x04	/* Status */
#define	I80312_AAU_DSCA		0x08	/* Descriptor Address */
#define	I80312_AAU_NDA		0x0c	/* Next Descriptor Address */
#define	I80312_AAU_SA1		0x10	/* i80200 Source Address 1 */
#define	I80312_AAU_SA2		0x14	/* i80200 Source Address 2 */
#define	I80312_AAU_SA3		0x18	/* i80200 Source Address 3 */
#define	I80312_AAU_SA4		0x1c	/* i80200 Source Address 4 */
#define	I80312_AAU_DSTA		0x20	/* i80200 Destination Address */
#define	I80312_AAU_ABC		0x24	/* Accelerator Byte Count */
#define	I80312_AAU_ADC		0x28	/* Accelerator Descriptor Count */
#define	I80312_AAU_SA5		0x2c	/* i80200 Source Address 5 */
#define	I80312_AAU_SA6		0x30	/* i80200 Source Address 6 */
#define	I80312_AAU_SA7		0x34	/* i80200 Source Address 7 */
#define	I80312_AAU_SA8		0x38	/* i80200 Source Address 8 */
     /* not used		0x3c .. 0xfc */

/*
 * Physical addresses 0x00002000..0x7fffffff are used by the
 * ATU Outbound Direct Addressing Window.
 */
#define	I80312_PCI_DIRECT_BASE	0x00002000UL
#define	I80312_PCI_DIRECT_SIZE	0x7fffe000UL

/*
 * Physical addresses 0x80000000..0x9001ffff are used by the
 * ATU Outbound Transaction Windows.
 */
#define	I80312_PCI_XLATE_BASE	0x80000000UL
#define	I80312_PCI_XLATE_SIZE	0x10020000UL

#define	I80312_PCI_XLATE_MSIZE	0x04000000UL	/* 64M */
#define	I80312_PCI_XLATE_IOSIZE	0x00010000UL	/* 64K */

#define	I80312_PCI_XLATE_PMW_BASE  (I80312_PCI_XLATE_BASE)

#define	I80312_PCI_XLATE_PDW_BASE  (I80312_PCI_XLATE_PMW_BASE + \
				    I80312_PCI_XLATE_MSIZE)

#define	I80312_PCI_XLATE_SMW_BASE  (I80312_PCI_XLATE_PDW_BASE + \
				    I80312_PCI_XLATE_MSIZE)

#define	I80312_PCI_XLATE_SDW_BASE  (I80312_PCI_XLATE_SMW_BASE + \
				    I80312_PCI_XLATE_MSIZE)

#define	I80312_PCI_XLATE_PIOW_BASE (I80312_PCI_XLATE_SDW_BASE + \
				    I80312_PCI_XLATE_MSIZE)

#define	I80312_PCI_XLATE_SIOW_BASE (I80312_PCI_XLATE_PIOW_BASE + \
				    I80312_PCI_XLATE_IOSIZE)

#endif /* _ARM_XSCALE_I80312REG_H_ */
