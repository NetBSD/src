/* $NetBSD: bcm53xx_intr.h,v 1.1.2.1 2014/08/20 00:02:45 tls Exp $ */
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

#ifndef _ARM_BROADCOM_BCM53XX_INTR_H_
#define _ARM_BROADCOM_BCM53XX_INTR_H_

#ifdef _KERNEL_OPT
#include "opt_broadcom.h"
#endif

#define	PIC_MAXSOURCES			256
#define	PIC_MAXMAXSOURCES		280

/*
 * The BCM53xx uses a generic interrupt controller so pull that stuff.
 */
#include <arm/cortex/gic_intr.h>
#include <arm/cortex/a9tmr_intr.h>	/* A9 Timer PPIs */

#define	IRQ_L2CC			32
#define	IRQ_PWRWDOG			33
#define	IRQ_TRAP8			34
#define	IRQ_TRAP1			35
#define	IRQ_COMMTX			36
#define	IRQ__RSVD37			37
#define	IRQ_COMMRX			38
#define	IRQ__RSVD39			39
#define	IRQ_PMU				40
#define	IRQ__RSVD41			41
#define	IRQ_CTI				42
#define	IRQ__RSVD43			43
#define	IRQ_DEFLAG_CPU0			44
#define	IRQ_DEFLAG_CPU1			45
#define	IRQ_ARMCORE_M1_PINS_BUS		46
#define	IRQ_PCIE0_M0_PINS_BUS		47
#define	IRQ_PCIE1_M0_PINS_BUS		48
#define	IRQ_PCIE2_M0_PINS_BUS		49
#define	IRQ_DMA_M0_PINS_BUS		50
#define	IRQ_AMAC_M0_PINS_BUS		51
#define	IRQ_AMAC_M1_PINS_BUS		52
#define	IRQ_AMAC_M2_PINS_BUS		53
#define	IRQ_AMAC_M3_PINS_BUS		54
#define	IRQ_USBH_M0_PINS_BUS		55
#define	IRQ_USBH_M1_PINS_BUS		56
#define	IRQ_SDIO_M0_PINS_BUS		57
#define	IRQ_I2S_M0_PINS_BUS		58
#define	IRQ_A9JTAG_M0_PINS_BUS		59
#define	IRQ_JTAG_M0_PINS_BUS		60
#define	IRQ_ARMCORE_ACP_PINS_BUS	61
#define	IRQ_ARMCORE_S0_PINS_BUS		62
#define	IRQ_DDR_S1_PINS_BUS		63
#define	IRQ_DDR_S2_PINS_BUS		64
#define	IRQ_PCIE0_S0_PINS_BUS		65
#define	IRQ_PCIE1_S0_PINS_BUS		66
#define	IRQ_PCIE2_S0_PINS_BUS		67
#define	IRQ_ROM_S0_PINS_BUS		68
#define	IRQ_NAND_S0_PINS_BUS		69
#define	IRQ_QSPI_S0_PINS_BUS		70
#define	IRQ_A9JTAG_S0_PINS_BUS		71
#define	IRQ_APBX_S0_PINS_BUS		72

#ifdef BCM5301X
#define	BCM53XXX_IRQ(a,c)		((a))
#elif defined(BCM563XX)
#define	BCM53XXX_IRQ(a,c)		((a) + (c))
#else
#error unknown iProc variant
#endif

#define	IRQ_DS_0_PINS_BUS		BCM53XXX_IRQ(73, 6)
#define	IRQ_DS_1_PINS_BUS		BCM53XXX_IRQ(74, 6)
#define	IRQ_DS_2_PINS_BUS		BCM53XXX_IRQ(75, 6)
#define	IRQ_DS_3_PINS_BUS		BCM53XXX_IRQ(76, 6)
#define	IRQ_DS_4_PINS_BUS		BCM53XXX_IRQ(77, 6)
#define	IRQ_DDR_CONTROLLER		BCM53XXX_IRQ(78, 6)
#define	IRQ_DMAC			BCM53XXX_IRQ(79, 6)	/* 16 */
#define	IRQ_DMAC_ABORT			BCM53XXX_IRQ(95, 6)
#define	IRQ_NAND_RD_MISS		BCM53XXX_IRQ(96, 6)
#define	IRQ_NAND_BLK_ERASE_COMP		BCM53XXX_IRQ(97, 6)
#define	IRQ_NAND_COPY_BACK_COMP		BCM53XXX_IRQ(98, 6)
#define	IRQ_NAND_PGM_PAGE_COMP		BCM53XXX_IRQ(99, 6)
#define	IRQ_NAND_RO_CTLR_READY		BCM53XXX_IRQ(100, 6)
#define	IRQ_NAND_RB_B			BCM53XXX_IRQ(101, 6)
#define	IRQ_NAND_ECC_MIPS_UNCORR	BCM53XXX_IRQ(102, 6)
#define	IRQ_NAND_ECC_MIPS_CORR		BCM53XXX_IRQ(103, 6)

#define	IRQ_SPI_FULLNESS_REACHED	BCM53XXX_IRQ(104, 6)
#define	IRQ_SPI_TRUNCATED		BCM53XXX_IRQ(105, 6)
#define	IRQ_SPI_IMPATIENT		BCM53XXX_IRQ(106, 6)
#define	IRQ_SPI_SESSION_DONE		BCM53XXX_IRQ(107, 6)
#define	IRQ_SPI_INTERRUPT_OVERREAD	BCM53XXX_IRQ(108, 6)
#define	IRQ_SPI_MSPI_INTERRUPT_DONE	BCM53XXX_IRQ(109, 6)
#define	IRQ_SPI_MSPI_INTERRUPT_HALT_SET_TRANSACTION_DONE \
					BCM53XXX_IRQ(110, 6)
#define	IRQ_USB2			BCM53XXX_IRQ(111, 6)

#define	IRQ_CCA				BCM53XXX_IRQ(117, 6)
#define	IRQ_UART2			BCM53XXX_IRQ(118, 6)
#define	IRQ_GPIO			BCM53XXX_IRQ(119, 6)
#define	IRQ_I2S				BCM53XXX_IRQ(120, 6)
#define	IRQ_SMBUS1			BCM53XXX_IRQ(121, 6)
#define	IRQ_TIMER0_1			BCM53XXX_IRQ(122, 7)
#define	IRQ_TIMER0_2			BCM53XXX_IRQ(123, 7)
#define	IRQ_TIMER1_1			BCM53XXX_IRQ(124, 7)
#define	IRQ_TIMER1_2			BCM53XXX_IRQ(125, 7)
#define	IRQ_RNG				BCM53XXX_IRQ(126, 7)
#define	IRQ_SWITCH_SOC			BCM53XXX_IRQ(127, 7)	/* 32 */
#define	 IRQ_NETWORK_LINK_EVENT		BCM53XXX_IRQ(127, 7)	/* 8 */
#define	 IRQ_PHY			BCM53XXX_IRQ(135, 7)
#define	 IRQ_TIMESYNC			BCM53XXX_IRQ(136, 7)
#define	 IRQ_IMP_SLEEP_TIMER		BCM53XXX_IRQ(137, 7)	/* 3 */
#define	IRQ_PCIE_INT0			BCM53XXX_IRQ(159, 55)	/* 6 */
#define	IRQ_PCIE_INT1			BCM53XXX_IRQ(165, 55)	/* 6 */
#define	IRQ_PCIE_INT2			BCM53XXX_IRQ(171, 55)	/* 6 */
#define	IRQ_SDIO			BCM53XXX_IRQ(177, 55)
#define	IRQ_GMAC0			BCM53XXX_IRQ(179, 55)
#define	IRQ_GMAC1			BCM53XXX_IRQ(180, 55)

#ifdef BCM5301X
#define	IRQ_XHCI_0			(112)
#define	IRQ_XHCI_1			(113)
#define	IRQ_XHCI_2			(114)
#define	IRQ_XHCI_3			(115)
#define	IRQ_XHCI_HSE			(116)
#define	IRQ_FA				(178)
#define	IRQ_GMAC2			(181)
#define	IRQ_GMAC3			(182)
#endif

#ifdef BCM563XX
#define	IRQ_SATA_PINS_BUS		(73)
#define	IRQ_SRAM_PINS_BUS		(74)
#define	IRQ_APBW_PINS_BUS		(75)
#define	IRQ_APBX_PINS_BUS		(76)
#define	IRQ_APBY_PINS_BUS		(77)
#define	IRQ_APBZ_PINS_BUS		(78)
#define	IRQ_SMBUS2			(128)
#define	IRQ_SATA0			(190)
#define	IRQ_SATA1			(191)
#define	IRQ_I2S_INTR			(201)
#define	IRQ_MACSEC0			(202)
#define	IRQ_MACSEC1			(203)
#define	IRQ_USB2D			(238)
#define	IRQ_APBV_PINS_BUS		(239)
#define	IRQ_SRAM_MEM_CORRECTABLE	(240)
#define	IRQ_SRAM_MEM_UNCORRECTABLE	(241)
#define	IRQ_SRAM_MEM_ACCESS_VIO		(242)
#define	IRQ_SRAM_MEM_SBMA_MISMATCH	(243)
#define	IRQ_CCB_WDT			(244)
#endif /* BCM563XX */

#endif /* _ARM_BROADCOM_BC53XX_INTR_H_ */
