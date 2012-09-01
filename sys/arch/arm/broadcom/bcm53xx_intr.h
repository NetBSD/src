/* $NetBSD: bcm53xx_intr.h,v 1.1 2012/09/01 00:04:44 matt Exp $ */
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
#define	IRQ_DS_0_PINS_BUS		73
#define	IRQ_DS_1_PINS_BUS		74
#define	IRQ_DS_2_PINS_BUS		75
#define	IRQ_DS_3_PINS_BUS		76
#define	IRQ_DS_4_PINS_BUS		77
#define	IRQ_DDR_CONTROLLER		78
#define	IRQ_DMAC			79	/* 16 */
#define	IRQ_DMAC_ABORT			95
#define	IRQ_NAND_RD_MISS		96
#define	IRQ_NAND_BLK_ERASE_COMP		97
#define	IRQ_NAND_COPY_BACK_COMP		98
#define	IRQ_NAND_PGM_PAGE_COMP		99
#define	IRQ_NAND_RO_CTLR_READY		100
#define	IRQ_NAND_RB_B			101
#define	IRQ_NAND_ECC_MIPS_UNCORR	102
#define	IRQ_NAND_ECC_MIPS_CORR		103
#define	IRQ_SPI_FULLNESS_REACHED	104
#define	IRQ_SPI_TRUNCATED	105
#define	IRQ_SPI_IMPATIENT	106
#define	IRQ_SPI_SESSION_DONE	107
#define	IRQ_SPI_INTERRUPT_OVERREAD	108
#define	IRQ_SPI_MSPI_INTERRUPT_DONE	109
#define	IRQ_SPI_MSPI_INTERRUPT_HALT_SET_TRANSACTION_DONE	110
#define	IRQ_USB2	111
#define	IRQ_XHCI_0	112
#define	IRQ_XHCI_1	113
#define	IRQ_XHCI_2	114
#define	IRQ_XHCI_3	115
#define	IRQ_XHCI_HSE	116
#define	IRQ_CCA		117
#define	IRQ_UART2	118
#define	IRQ_RSVD119	119
#define	IRQ_I2S		120
#define	IRQ_SMBUS	121
#define	IRQ_TIMER0_1	122
#define	IRQ_TIMER0_2	123
#define	IRQ_TIMER1_1	124
#define	IRQ_TIMER1_2	125
#define	IRQ_RNG		126
#define	IRQ_SWITCH_SOC	127	/* 32 */
#define	 IRQ_NETWORK_LINK_EVENT	127 /* 8 */
#define	 IRQ_PHY		135
#define	 IRQ_TIMESYNC	136
#define	 IRQ_IMP_SLEEP_TIMER	137 /* 3 */
#define	IRQ_PCIE_INT0	159	/* 6 */
#define	IRQ_PCIE_INT1	165	/* 6 */
#define	IRQ_PCIE_INT2	171	/* 6 */
#define	IRQ_SDIO	177
#define	IRQ_FA		178
#define	IRQ_GMAC0	179
#define	IRQ_GMAC1	180
#define	IRQ_GMAC2	181
#define	IRQ_GMAC3	182

#endif /* _ARM_BROADCOM_BC53XX_INTR_H_ */
