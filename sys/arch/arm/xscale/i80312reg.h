/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by 
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * Physical addresses 0x1000..0x1fff are used by the Periphial Memory
 * Mapped Registers.
 */

#define	I80312_PMMR_BASE	0x00001000
#define	I80312_PMMR_SIZE	0x00001000

/*
 * PCI-to-PCI Bridge Unit
 */
#define	I80312_PPB_BASE		(I80312_PMMR_BASE)
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
 * Performance Monitoring Unit
 */
#define	I80312_PMU_GTMR		(I80312_PMU_BASE + 0x00)
#define	I80312_PMU_ESR		(I80312_PMU_BASE + 0x04)
#define	I80312_PMU_EMISR	(I80312_PMU_BASE + 0x08)
#define	I80312_PMU_GTMR		(I80312_PMU_BASE + 0x10)
#define	I80312_PMU_PECR1	(I80312_PMU_BASE + 0x14)
#define	I80312_PMU_PECR2	(I80312_PMU_BASE + 0x18)
#define	I80312_PMU_PECR3	(I80312_PMU_BASE + 0x1c)
#define	I80312_PMU_PECR4	(I80312_PMU_BASE + 0x20)
#define	I80312_PMU_PECR5	(I80312_PMU_BASE + 0x24)
#define	I80312_PMU_PECR6	(I80312_PMU_BASE + 0x28)
#define	I80312_PMU_PECR7	(I80312_PMU_BASE + 0x2c)
#define	I80312_PMU_PECR8	(I80312_PMU_BASE + 0x30)
#define	I80312_PMU_PECR9	(I80312_PMU_BASE + 0x34)
#define	I80312_PMU_PECR10	(I80312_PMU_BASE + 0x38)
#define	I80312_PMU_PECR11	(I80312_PMU_BASE + 0x3c)
#define	I80312_PMU_PECR12	(I80312_PMU_BASE + 0x40)
#define	I80312_PMU_PECR13	(I80312_PMU_BASE + 0x44)
#define	I80312_PMU_PECR14	(I80312_PMU_BASE + 0x48)

/*
 * The first 64 bytes are identical to a PCI device's config space.
 */
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
#define	I80312_AAU_DA		0x08	/* Descriptor Address */
#define	I80312_AAU_NDA		0x0c	/* Next Descriptor Address */
#define	I80312_AAU_SA1		0x10	/* i80200 Source Address 1 */
#define	I80312_AAU_SA2		0x14	/* i80200 Source Address 2 */
#define	I80312_AAU_SA3		0x18	/* i80200 Source Address 3 */
#define	I80312_AAU_SA4		0x1c	/* i80200 Source Address 4 */
#define	I80312_AAU_DA		0x20	/* i80200 Destination Address */
#define	I80312_AAU_ABC		0x24	/* Accelerator Byte Count */
#define	I80312_AAU_ADC		0x28	/* Accelerator Descriptor Count */
#define	I80312_AAU_SA5		0x2c	/* i80200 Source Address 5 */
#define	I80312_AAU_SA6		0x30	/* i80200 Source Address 6 */
#define	I80312_AAU_SA7		0x34	/* i80200 Source Address 7 */
#define	I80312_AAU_SA8		0x38	/* i80200 Source Address 8 */
     /* not used		0x3c .. 0xfc */

#endif /* _ARM_XSCALE_I80312REG_H_ */
