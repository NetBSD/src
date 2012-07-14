/*	$NetBSD: omap4430_intr.h,v 1.1 2012/07/14 07:43:15 matt Exp $	*/
/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#ifndef _ARM_OMAP_OMAP4430_INTR_H_
#define _ARM_OMAP_OMAP4430_INTR_H_

#ifndef _LOCORE

#define IRQ_L2_CACHE		0	// L2 cache controller interrupt
#define IRQ_CTI_0		1	// Cross-trigger module 0 (CTI0) interrupt
#define IRQ_CTI_1		2	// Cross-trigger module 1 (CTI1) interrupt
#define IRQ_ELM			4	// Error location process completion
#define IRQ_SYS_NIRQ1		7	// External interrupt 1 (active low)
#define IRQ_L3_DBG		9	// L3 interconnect debug error
#define IRQ_L3_APP		10	// L3 interconnect application error
#define IRQ_PRCM_MPU		11	// PRCM interrupt
#define IRQ_SDMA_0		12	// sDMA interrupt 0
#define IRQ_SDMA_1		13	// sDMA interrupt 1
#define IRQ_SDMA_2		14	// sDMA interrupt 2
#define IRQ_SDMA_3		15	// sDMA interrupt 3
#define IRQ_MCBSP4		16	// MCBSP4 interrupt
#define IRQ_MCBSP1		17	// MCBSP1 interrupt
#define IRQ_SR_MPU		18	// SmartReflex MPU interrupt
#define IRQ_SR_CORE		19	// SmartReflex Core interrupt
#define IRQ_GPMC		20	// GPMC interrupt
#define IRQ_SGX			21	// 2D/3D graphics module interrupt
#define IRQ_MCBSP2		22	// MCBSP2 interrupt
#define IRQ_MCBSP3		23	// MCBSP3 interrupt
#define IRQ_ISS5		24	// Imaging subsystem interrupt 5
#define IRQ_DSS_DISPC		25	// Display controller interrupt
#define IRQ_MAIL_U0_MPU		26	// Mailbox user 0 interrupt
#define IRQ_C2C_SSCM0		27	// C2C status interrupt
#define IRQ_DSP_MMU		28	// DSP MMU interrupt
#define IRQ_GPIO1_MPU		29	// GPIO1 MPU interrupt
#define IRQ_GPIO2_MPU		30	// GPIO2 MPU interrupt
#define IRQ_GPIO3_MPU		31	// GPIO3 MPU interrupt
#define IRQ_GPIO4_MPU		32	// GPIO4 MPU interrupt
#define IRQ_GPIO5_MPU		33	// GPIO5 MPU interrupt
#define IRQ_GPIO6_MPU		34	// GPIO6 MPU interrupt
#define IRQ_WDT3		36	// WDTIMER3 overflow
#define IRQ_GPT1		37	// GPTIMER1 interrupt
#define IRQ_GPT2		38	// GPTIMER2 interrupt
#define IRQ_GPT3		39	// GPTIMER3 interrupt
#define IRQ_GPT4		40	// GPTIMER4 interrupt
#define IRQ_GPT5		41	// GPTIMER5 interrupt
#define IRQ_GPT6		42	// GPTIMER6 interrupt
#define IRQ_GPT7		43	// GPTIMER7 interrupt
#define IRQ_GPT8		44	// GPTIMER8 interrupt
#define IRQ_GPT9		45	// GPTIMER9 interrupt
#define IRQ_GPT10		46	// GPTIMER10 interrupt
#define IRQ_GPT11		47	// GPTIMER11 interrupt
#define IRQ_MCSPI4		48	// MCSPI4 interrupt
#define IRQ_DSS_DSI1		53	// Display Subsystem DSI1 interrupt
#define IRQ_I2C1		56	// I2C1 interrupt
#define IRQ_I2C2		57	// I2C2 interrupt
#define IRQ_HDQ			58	// HDQ/1wire interrupt
#define IRQ_MMC5		59	// MMC5 interrupt
#define IRQ_I2C3		61	// I2C3 interrupt
#define IRQ_I2C4		62	// I2C4 interrupt
#define IRQ_MCSPI1		65	// MCSPI1 interrupt
#define IRQ_MCSPI2		66	// MCSPI2 interrupt
#define IRQ_HSI_P1_MPU		67	// HSI Port 1 interrupt
#define IRQ_HSI_P2_MPU		68	// HSI Port 2 interrupt
#define IRQ_FDIF_3		69	// Face detect interrupt 3
#define IRQ_UART4		70	// UART module 4 interrupt
#define IRQ_HSI_DMA_MPU		71	// HSI DMA engine IRQ_MPU request
#define IRQ_UART1		72	// UART1 interrupt
#define IRQ_UART2		73	// UART2 interrupt
#define IRQ_UART3		74	// UART3 interrupt
#define IRQ_PBIAS		75	// Merged interrupt for PBIASlite1 and 2
#define IRQ_HSUSB_OHCI		76	// HSUSB MP host interrupt OHCI controller
#define IRQ_HSUSB_EHCI		77	// HSUSB MP host interrupt EHCI controller
#define IRQ_HSUSB_TLL		78	// HSUSB MP TLL interrupt
#define IRQ_WDT2		80	// WDTIMER2 interrupt
#define IRQ_MMC1		83	// MMC1 interrupt
#define IRQ_DSS_DSI2		85	// Display subsystem DSI2 interrupt
#define IRQ_MMC2		86	// MMC2 interrupt
#define IRQ_MPU_ICR		87	// ICR interrupt
#define IRQ_C2C_SSCM1		88	// C2C GPI interrupt
#define IRQ_FSUSB		89	// FS-USB - host controller Interrupt
#define IRQ_FSUSB_SMI		90	// FS-USB - host controller SMI Interrupt
#define IRQ_MCSPI3		91	// MCSPI3 interrupt
#define IRQ_HSUSB_OTG		92	// HSUSB OTG controller interrupt
#define IRQ_HSUSB_OTG_DMA	93	// HSUSB OTG DMA interrupt
#define IRQ_MMC3		94	// MMC3 interrupt
#define IRQ_MMC4		96	// MMC4 interrupt
#define IRQ_SLIMBUS1		97	// SLIMBUS1 interrupt
#define IRQ_SLIMBUS2		98	// SLIMBUS2 interrupt
#define IRQ_ABE_MPU		99	// Audio back-end interrupt
#define IRQ_CORTEXM3_MMU	100	// Cortex-M3 MMU interrupt
#define IRQ_DSS_HDMI		101	// Display subsystem HDMI interrupt
#define IRQ_SR_IVA		102	// SmartReflex IVA interrupt
#define IRQ_IVAHD2		103	// Sync interrupt from ICONT2 (vDMA)
#define IRQ_IVAHD1		104	// Sync interrupt from ICONT1
#define IRQ_IVAHD_MAILBOX_0	107	// IVAHD mailbox interrupt 0
#define IRQ_MCASP1_AXINT	109	// McASP1 transmit interrupt
#define IRQ_EMIF1		110	// EMIF1 interrupt
#define IRQ_EMIF2		111	// EMIF2 interrupt
#define IRQ_MCPDM		112	// MCPDM interrupt
#define IRQ_DMM			113	// DMM interrupt
#define IRQ_DMIC		114	// DMIC interupt
#define IRQ_SYS_NIRQ2		119	// External interrupt 2 (active low)
#define	IRQ_KBD_CTL		120	// Keyboard controller interrupt

/*
 *  0-15 are used for SGIs (software generated interrupts).
 * 16-31 are used for PPIs (private peripheral interrupts).
 * 32... are used for SPIs (shared peripheral interrupts).
 *
 * To make things easier, SPIs will start at 0 and SGIs/PPIs
 * will at the end of SPIs (these are internal and shouldn't
 * be used by real devices).
 */
#define	PIC_MAXSOURCES		(32+128)
#define	PIC_MAXMAXSOURCES	(PIC_MAXSOURCES+6*32)
#endif /* _LOCORE */

#endif /* _ARM_OMAP_OMAP4430_INTR_H_ */
