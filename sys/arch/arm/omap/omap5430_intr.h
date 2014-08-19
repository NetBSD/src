/*	$NetBSD: omap5430_intr.h,v 1.1.2.3 2014/08/20 00:02:47 tls Exp $	*/
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

#ifndef _ARM_OMAP_OMAP5430_INTR_H_
#define _ARM_OMAP_OMAP5430_INTR_H_

/*
 *  0-15 are used for SGIs (software generated interrupts).
 * 16-31 are used for PPIs (private peripheral interrupts).
 * 32... are used for SPIs (shared peripheral interrupts).
 */
#define PIC_MAXSOURCES		GIC_MAXSOURCES(160)
#define PIC_MAXMAXSOURCES	(PIC_MAXSOURCES+8*32)

/*
 * The BCM53xx uses a generic interrupt controller so pull that stuff.
 */
#include <arm/cortex/gic_intr.h>
#include <arm/cortex/gtmr_intr.h>      /* Generic Timer PPIs */

#ifndef _LOCORE

#define IRQ_MPU_CLUSTER		IRQ_SPI(0)	// Illegal writes to interrupt controller memory map region
#define IRQ_CTI_0		IRQ_SPI(1)	// Cross-trigger module 0 (CTI0) interrupt
#define IRQ_CTI_1		IRQ_SPI(2)	// Cross-trigger module 1 (CTI1) interrupt
#define IRQ_ELM			IRQ_SPI(4)	// Error location process completion
#define IRQ_WD_TIMER_C0		IRQ_SPI(5)	// WD_TIMER_MPU_C0 warning interrupt
#define IRQ_WD_TIMER_C1		IRQ_SPI(6)	// WD_TIMER_MPU_C1 warning interrupt
#define IRQ_SYS_NIRQ1		IRQ_SPI(7)	// External interrupt 1 (active low)
#define IRQ_L3_DBG		IRQ_SPI(9)	// L3 interconnect debug error
#define IRQ_L3_APP		IRQ_SPI(10)	// L3 interconnect application error
#define IRQ_PRCM_MPU		IRQ_SPI(11)	// PRCM interrupt
#define IRQ_SDMA_0		IRQ_SPI(12)	// sDMA interrupt 0
#define IRQ_SDMA_1		IRQ_SPI(13)	// sDMA interrupt 1
#define IRQ_SDMA_2		IRQ_SPI(14)	// sDMA interrupt 2
#define IRQ_SDMA_3		IRQ_SPI(15)	// sDMA interrupt 3
#define IRQ_L3_MAIN_STAT_ALARM	IRQ_SPI(16)	// L3 N0C statistic collector alarm
#define IRQ_MCBSP1		IRQ_SPI(17)	// MCBSP1 interrupt
#define IRQ_SR_MPU		IRQ_SPI(18)	// SmartReflex MPU interrupt
#define IRQ_SR_CORE		IRQ_SPI(19)	// SmartReflex Core interrupt
#define IRQ_GPMC		IRQ_SPI(20)	// GPMC interrupt
#define IRQ_GPU			IRQ_SPI(21)	// 2D/3D graphics module interrupt
#define IRQ_MCBSP2		IRQ_SPI(22)	// MCBSP2 interrupt
#define IRQ_MCBSP3		IRQ_SPI(23)	// MCBSP3 interrupt
#define IRQ_ISS5		IRQ_SPI(24)	// Imaging subsystem interrupt 5
#define IRQ_DSS_DISPC		IRQ_SPI(25)	// Display controller interrupt
#define IRQ_MAIL_U0_MPU		IRQ_SPI(26)	// Mailbox user 0 interrupt
#define IRQ_27			IRQ_SPI(27)	// Reserved
#define IRQ_DSP_MMU		IRQ_SPI(28)	// DSP MMU interrupt
#define IRQ_GPIO1_MPU		IRQ_SPI(29)	// GPIO1 MPU interrupt
#define IRQ_GPIO2_MPU		IRQ_SPI(30)	// GPIO2 MPU interrupt
#define IRQ_GPIO3_MPU		IRQ_SPI(31)	// GPIO3 MPU interrupt
#define IRQ_GPIO4_MPU		IRQ_SPI(32)	// GPIO4 MPU interrupt
#define IRQ_GPIO5_MPU		IRQ_SPI(33)	// GPIO5 MPU interrupt
#define IRQ_GPIO6_MPU		IRQ_SPI(34)	// GPIO6 MPU interrupt
#define IRQ_GPIO7_MPU		IRQ_SPI(35)	// GPIO7 MPU interrupt
#define IRQ_WDT3		IRQ_SPI(36)	// WDTIMER3 overflow
#define IRQ_GPT1		IRQ_SPI(37)	// GPTIMER1 interrupt
#define IRQ_GPT2		IRQ_SPI(38)	// GPTIMER2 interrupt
#define IRQ_GPT3		IRQ_SPI(39)	// GPTIMER3 interrupt
#define IRQ_GPT4		IRQ_SPI(40)	// GPTIMER4 interrupt
#define IRQ_GPT5		IRQ_SPI(41)	// GPTIMER5 interrupt
#define IRQ_GPT6		IRQ_SPI(42)	// GPTIMER6 interrupt
#define IRQ_GPT7		IRQ_SPI(43)	// GPTIMER7 interrupt
#define IRQ_GPT8		IRQ_SPI(44)	// GPTIMER8 interrupt
#define IRQ_GPT9		IRQ_SPI(45)	// GPTIMER9 interrupt
#define IRQ_GPT10		IRQ_SPI(46)	// GPTIMER10 interrupt
#define IRQ_GPT11		IRQ_SPI(47)	// GPTIMER11 interrupt
#define IRQ_MCSPI4		IRQ_SPI(48)	// MCSPI4 interrupt
#define IRQ_DSS_DSI1_A		IRQ_SPI(53)	// Display Subsystem DSI1_A interrupt
#define IRQ_SATA		IRQ_SPI(54)	// SATA interrupt
#define IRQ_DSS_DSI1_C		IRQ_SPI(55)	// Display Subsystem DSI1_C interrupt
#define IRQ_I2C1		IRQ_SPI(56)	// I2C1 interrupt
#define IRQ_I2C2		IRQ_SPI(57)	// I2C2 interrupt
#define IRQ_HDQ			IRQ_SPI(58)	// HDQ/1wire interrupt
#define IRQ_MMC5		IRQ_SPI(59)	// MMC5 interrupt
#define IRQ_I2C3		IRQ_SPI(61)	// I2C3 interrupt
#define IRQ_I2C4		IRQ_SPI(62)	// I2C4 interrupt
#define IRQ_MCSPI1		IRQ_SPI(65)	// MCSPI1 interrupt
#define IRQ_MCSPI2		IRQ_SPI(66)	// MCSPI2 interrupt
#define IRQ_HSI_P1_MPU		IRQ_SPI(67)	// HSI Port 1 interrupt
#define IRQ_HSI_P2_MPU		IRQ_SPI(68)	// HSI Port 2 interrupt
#define IRQ_FDIF_3		IRQ_SPI(69)	// Face detect interrupt 3
#define IRQ_UART4		IRQ_SPI(70)	// UART module 4 interrupt
#define IRQ_HSI_DMA_MPU		IRQ_SPI(71)	// HSI DMA engine IRQ_MPU request
#define IRQ_UART1		IRQ_SPI(72)	// UART1 interrupt
#define IRQ_UART2		IRQ_SPI(73)	// UART2 interrupt
#define IRQ_UART3		IRQ_SPI(74)	// UART3 interrupt
#define IRQ_PBIAS		IRQ_SPI(75)	// Merged interrupt for PBIASlite1 and 2
#define IRQ_HSUSB_OHCI		IRQ_SPI(76)	// HSUSB MP host interrupt OHCI controller
#define IRQ_HSUSB_EHCI		IRQ_SPI(77)	// HSUSB MP host interrupt EHCI controller
#define IRQ_HSUSB_TLL		IRQ_SPI(78)	// HSUSB MP TLL interrupt
#define IRQ_WDT2		IRQ_SPI(80)	// WDTIMER2 interrupt
#define IRQ_MMC1		IRQ_SPI(83)	// MMC1 interrupt
#define IRQ_MMC2		IRQ_SPI(86)	// MMC2 interrupt
#define IRQ_DEBUGSS_CT_UART	IRQ_SPI(90)	// CT_UART interrupt generated when data ready on RX or when TX empty
#define IRQ_MCSPI3		IRQ_SPI(91)	// MCSPI3 interrupt
#define IRQ_HSUSB_OTG		IRQ_SPI(92)	// HSUSB OTG controller interrupt
#define IRQ_HSUSB_OTG_DMA	IRQ_SPI(93)	// HSUSB OTG DMA interrupt
#define IRQ_MMC3		IRQ_SPI(94)	// MMC3 interrupt
#define IRQ_MMC4		IRQ_SPI(96)	// MMC4 interrupt
#define IRQ_ABE_MPU		IRQ_SPI(99)	// Audio back-end interrupt
#define IRQ_IPU_MMU		IRQ_SPI(100) // Cortex-M4 MMU interrupt
#define IRQ_HDMI		IRQ_SPI(101) // Display subsystem HDMI interrupt
#define IRQ_SR_IVA		IRQ_SPI(102)	// SmartReflex IVA interrupt
#define IRQ_IVAHD2		IRQ_SPI(103)	// Sync interrupt from ICONT2 (vDMA)
#define IRQ_IVAHD1		IRQ_SPI(104)	// Sync interrupt from ICONT1
#define IRQ_UART5		IRQ_SPI(105)	// UART5 interrupt
#define IRQ_UART6		IRQ_SPI(106)	// UART6 interrupt
#define IRQ_IVAHD_MAILBOX_0	IRQ_SPI(107)	// IVAHD mailbox interrupt 0
#define IRQ_MCASP1_AXINT	IRQ_SPI(109)	// McASP1 transmit interrupt
#define IRQ_EMIF1		IRQ_SPI(110)	// EMIF1 interrupt
#define IRQ_EMIF2		IRQ_SPI(111)	// EMIF2 interrupt
#define IRQ_MCPDM		IRQ_SPI(112)	// MCPDM interrupt
#define IRQ_DMM			IRQ_SPI(113)	// DMM interrupt
#define IRQ_DMIC		IRQ_SPI(114)	// DMIC interupt
#define IRQ_SYS_NIRQ2		IRQ_SPI(119)	// External interrupt 2 (active low)
#define IRQ_KBD_CTL		IRQ_SPI(120)	// Keyboard controller interrupt
#define IRQ_GPIO8		IRQ_SPI(121)	// GPIO8 interupt
#define IRQ_BB2D		IRQ_SPI(125)	// BB2D interupt

#endif /* _LOCORE */

#endif /* _ARM_OMAP_OMAP5430_INTR_H_ */
