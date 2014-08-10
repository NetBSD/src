/*	$NetBSD: ralink_reg.h,v 1.4.20.1 2014/08/10 06:54:02 tls Exp $	*/
/*-
 * Copyright (c) 2011 CradlePoint Technology, Inc.
 * All rights reserved.
 *
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
 * THIS SOFTWARE IS PROVIDED BY CRADLEPOINT TECHNOLOGY, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This file contains the configuration parameters for the RT3052 board.
 */

#ifndef _RALINK_REG_H_
#define _RALINK_REG_H_

#ifdef _KERNEL_OPT
#include "opt_rasoc.h"
#endif

#include <mips/cpuregs.h>

#if defined(RT3050)
#define RA_CLOCK_RATE		320000000
#define RA_BUS_FREQ		(RA_CLOCK_RATE / 3)
#define RA_UART_FREQ		RA_BUS_FREQ
#elif defined(RT3052)
#define RA_CLOCK_RATE	  	384000000
#define RA_BUS_FREQ		(RA_CLOCK_RATE / 3)
#define RA_UART_FREQ		RA_BUS_FREQ
#elif defined(RT3883)
#if 0
#define RA_CLOCK_RATE		480000000
#else
#define RA_CLOCK_RATE		500000000
#endif
#define RA_BUS_FREQ		166000000 /* DDR speed */
#define RA_UART_FREQ		40000000
#elif defined(MT7620)
#define RA_CLOCK_RATE		580000000
#define RA_BUS_FREQ		(RA_CLOCK_RATE / 3)
#define RA_UART_FREQ		40000000
#else
/* Ralink dev board */
#define RA_CLOCK_RATE		384000000
#define RA_BUS_FREQ		(RA_CLOCK_RATE / 3)
#define RA_UART_FREQ		RA_BUS_FREQ
#endif

#define RA_BAUDRATE		CONSPEED
#define RA_SERIAL_CLKDIV	16

#define RA_SRAM_BASE		0x00000000
#define RA_SRAM_END		0x0FFFFFFF
#define RA_SYSCTL_BASE		0x10000000
#define RA_TIMER_BASE		0x10000100
#define RA_INTCTL_BASE		0x10000200
#define RA_MEMCTL_BASE		0x10000300
#if defined(RT3052) || defined(RT3050)
#define RA_PCM_BASE		0x10000400
#endif
#define RA_UART_BASE		0x10000500
#define RA_PIO_BASE		0x10000600
#if defined(RT3052) || defined(RT3050)
#define RA_GDMA_BASE		0x10000700
#elif defined(RT3883)
#define RA_FLASHCTL_BASE	0x10000700
#endif
#define RA_NANDCTL_BASE	0x10000800
#define RA_I2C_BASE		0x10000900
#define RA_I2S_BASE		0x10000A00
#define RA_SPI_BASE		0x10000B00
#define RA_UART_LITE_BASE	0x10000C00
#if defined(RT3883)
#define RA_PCM_BASE		0x10002000
#define RA_GDMA_BASE		0x10002800
#define RA_CODEC1_BASE		0x10003000
#define RA_CODEC2_BASE		0x10003800
#endif
#define RA_FRAME_ENGINE_BASE	0x10100000
#define RA_ETH_SW_BASE		0x10110000
#define RA_ROM_BASE		0x10118000
#if defined(RT3883) || defined(MT7620)
#define RA_USB_DEVICE_BASE	0x10120000
#if defined(MT7620)
#define RA_SDHC_BASE		0x10130000
#endif
#define RA_PCI_BASE		0x10140000
#define RA_PCIWIN_BASE		0x10150000
#endif
#define RA_11N_MAC_BASE		0x10180000
#define RA_USB_OTG_BASE		0x101C0000
#if defined(RT3883) || defined(MT7620)
#define RA_USB_HOST_BASE	0x101C0000
#define RA_USB_BLOCK_SIZE	0x1000
#define RA_USB_EHCI_BASE	(RA_USB_HOST_BASE + 0x0000)
#define RA_USB_OHCI_BASE	(RA_USB_EHCI_BASE + RA_USB_BLOCK_SIZE)
#endif
#if defined(RT3052) || defined(RT3050)
#define RA_FLASH_BASE		0x1F000000
#define RA_FLASH_END		0x1F7FFFFF
#elif defined(RT3883) || defined(MT7620)
#define RA_FLASH_BASE		0x1C000000
#define RA_FLASH_END		0x1DFFFFFF
#endif

#define RA_IOREG_VADDR(base, offset)	\
	(volatile uint32_t *)MIPS_PHYS_TO_KSEG1((base) + (offset))

#define FLD_GET(val,pos,mask)      (((val) >> (pos)) & (mask))
#define FLD_SET(val,pos,mask)      (((val) & (mask)) << (pos))

/*
 * System Control Registers
 */
#define RA_SYSCTL_ID0			0x00
#define RA_SYSCTL_ID1			0x04
#define RA_SYSCTL_REVID			0x0c
#define RA_SYSCTL_CFG0			0x10
#define RA_SYSCTL_CFG1			0x14
#define RA_SYSCTL_CLKCFG0		0x2C
#define RA_SYSCTL_CLKCFG1		0x30
#define RA_SYSCTL_RST			0x34
#define RA_SYSCTL_RSTSTAT		0x38
#define RA_SYSCTL_GPIOMODE		0x60

#if defined(RT3050) || defined(RT3052)
#define	SYSCTL_CFG0_INIC_EE_SDRAM 	__BIT(29)
#define	SYSCTL_CFG0_INIC_8MB_SDRAM 	__BIT(28)
#define	SYSCTL_CFG0_GE0_MODE		__BITS(24,25)
#define	SYSCTL_CFG0_BYPASS_PLL		__BIT(21)
#define	SYSCTL_CFG0_BE			__BIT(20)
#define	SYSCTL_CFG0_CPU_CLK_SEL 	__BIT(18)
#define	SYSCTL_CFG0_BOOT_FROM		__BITS(16,17)
#define	SYSCTL_CFG0_TEST_CODE		__BITS(8,15)
#define	SYSCTL_CFG0_SRAM_CS_MODE	__BITS(2,3)
#define	SYSCTL_CFG0_SDRAM_CLK_DRV	__BIT(0)
#elif defined(RT3883)
#define	SYSCTL_CFG0_BE			__BIT(19)
#define SYSCTL_CFG0_DRAM_SIZE		__BITS(12,14) 
#define	SYSCTL_CFG0_DRAM_2MB		0
#define	SYSCTL_CFG0_DRAM_8MB		1
#define	SYSCTL_CFG0_DRAM_16MB		2
#define	SYSCTL_CFG0_DRAM_32MB		3
#define	SYSCTL_CFG0_DRAM_64MB		4
#define	SYSCTL_CFG0_DRAM_128MB		5
#define	SYSCTL_CFG0_DRAM_256MB		6
#elif defined(MT7620)
#define	SYSCTL_CFG0_TEST_CODE		__BITS(31,24)
#define	SYSCTL_CFG0_BS_SHADOW		__BITS(22,12)
#define	SYSCTL_CFG0_DRAM_FROM_EE	__BIT(8)
#define	SYSCTL_CFG0_DBG_JTAG_MODE	__BIT(7)
#define	SYSCTL_CFG0_XTAL_FREQ_SEL	__BIT(6)
#define	SYSCTL_CFG0_DRAM_TYPE		__BITS(5,4)
#define	SYSCTL_CFG0_CHIP_MODE		__BITS(3,0)
#endif

#if defined(RT3883) || defined(MT7620)
#define SYSCTL_CFG1_GE2_MODE		__BITS(15,14)
#define SYSCTL_CFG1_GE1_MODE		__BITS(13,12)
#define GE_MODE_RGMII			0	// RGMII mode (10/100/1000)
#define GE_MODE_MII			1	// MII mode (10/100)
#define GE_MODE_RMII			2	// Reverse MMI (10/100)
#define SYSCTL_CFG1_USB0_HOST_MODE	__BIT(10)
#define SYSCTL_CFG1_PCIE_RC_MODE	__BIT(8)
#endif
#if defined(RT3883)
#define SYSCTL_CFG1_PCI_HOST_MODE	__BIT(7)
#define SYSCTL_CFG1_PCI_66M_MODE	__BIT(6)
#endif

#if defined(RT3883) || defined(MT7620)
#define SYSCTL_CLKCFG0_REFCLK0_RATE	__BITS(11,9)
#endif
#if defined(RT3883)
#define SYSCTL_CLKCFG0_OSC_1US_DIV_3883	__BITS(21,16)
#define SYSCTL_CLKCFG0_REFCLK1_RATE	__BITS(15,13)
#define SYSCTL_CLKCFG0_REFCLK0_IS_OUT	__BIT(8)
#define SYSCTL_CLKCFG0_CPU_FREQ_ADJ	__BITS(3,0)
#endif
#if defined(MT7620)
#define SYSCTL_CLKCFG0_OSC_1US_DIV_7620	__BITS(29,24)
#define SYSCTL_CLKCFG0_INT_CLK_FDIV	__BITS(22,18)
#define SYSCTL_CLKCFG0_INT_CLK_FFRAC	__BITS(16,12)
#define SYSCTL_CLKCFG0_PERI_CLK_SEL	__BIT(4)
#define SYSCTL_CLKCFG0_EPHY_USE_25M	__BIT(3)
#endif 

#if defined(RT3883)
#define SYSCTL_CLKCFG1_PBUS_DIV2	__BIT(30)
#define SYSCTL_CLKCFG1_SYS_TCK_EN	__BIT(29)
#define SYSCTL_CLKCFG1_FE_GDMA_PCLK_EN	__BIT(22)
#define SYSCTL_CLKCFG1_PCIE_CLK_EN_3883	__BIT(21)
#define SYSCTL_CLKCFG1_UPHY1_CLK_EN	__BIT(20)
#define SYSCTL_CLKCFG1_PCI_CLK_EN	__BIT(19)
#define SYSCTL_CLKCFG1_UPHY0_CLK_EN_3883 __BIT(18)
#define SYSCTL_CLKCFG1_GE2_CLK_EN_3883	__BIT(17)
#define SYSCTL_CLKCFG1_GE1_CLK_EN_3883	__BIT(16)
#endif
#if defined(MT7620)
#define SYSCTL_CLKCFG1_SDHC_CLK_EN	__BIT(30)
#define SYSCTL_CLKCFG1_AUX_SYS_TCK_EN	__BIT(28)
#define SYSCTL_CLKCFG1_PCIE_CLK_EN_7620 __BIT(26)
#define SYSCTL_CLKCFG1_UPHY0_CLK_EN_7620 __BIT(25)
#define SYSCTL_CLKCFG1_ESW_CLK_EN	__BIT(23)
#define SYSCTL_CLKCFG1_FE_CLK_EN	__BIT(21)
#define SYSCTL_CLKCFG1_UARTL_CLK_EN	__BIT(19)
#define SYSCTL_CLKCFG1_SPI_CLK_EN	__BIT(18)
#define SYSCTL_CLKCFG1_I2S_CLK_EN	__BIT(17)
#define SYSCTL_CLKCFG1_I2C_CLK_EN	__BIT(16)
#define SYSCTL_CLKCFG1_NAND_CLK_EN	__BIT(15)
#define SYSCTL_CLKCFG1_GDMA_CLK_EN	__BIT(14)
#define SYSCTL_CLKCFG1_GPIO_CLK_EN	__BIT(13)
#define SYSCTL_CLKCFG1_UART_CLK_EN	__BIT(12)
#define SYSCTL_CLKCFG1_PCM_CLK_EN	__BIT(11)
#define SYSCTL_CLKCFG1_MC_CLK_EN	__BIT(10)
#define SYSCTL_CLKCFG1_INTC_CLK_EN	__BIT(9)
#define SYSCTL_CLKCFG1_TIMER_CLK_EN	__BIT(8)
#define SYSCTL_CLKCFG1_GE2_CLK_EN_7620	__BIT(7)
#define SYSCTL_CLKCFG1_GE1_CLK_EN_7620	__BIT(6)
#endif

#if defined(RT3883) || defined(MT7620)
/* 3883 doesn't have memo regs, use teststat instead */
#define RA_SYSCTL_MEMO0	0x18
#define RA_SYSCTL_MEMO1	0x1C
#else
#define RA_SYSCTL_MEMO0	0x68
#define RA_SYSCTL_MEMO1	0x6C
#endif

#define  RST_PPE_7620		__BIT(31)
#define  RST_SDHC_7620		__BIT(30)
#define  RST_MIPS_CNT_7620	__BIT(28)
#define  RST_PCIPCIE_3883	__BIT(27)
#define  RST_FLASH_3883		__BIT(26)
#define  RST_PCIE0_7620		__BIT(26)
#define  RST_UDEV_3883		__BIT(25)
#define  RST_UHST0_7620		__BIT(25)
#define  RST_PCI_3883		__BIT(24)
#define  RST_EPHY_7620		__BIT(24)
#define  RST_PCIE_3883		__BIT(23)
#define  RST_ESW_7620		__BIT(23)
#define  RST_UHST		__BIT(22)
#define  RST_FE			__BIT(21)
#define  RST_WLAN		__BIT(20)
#define  RST_UARTL		__BIT(19)
#define  RST_SPI		__BIT(18)
#define  RST_I2S		__BIT(17)
#define  RST_I2C		__BIT(16)
#define  RST_NAND		__BIT(15)
#define  RST_DMA		__BIT(14)
#define  RST_PIO		__BIT(13)
#define  RST_UART		__BIT(12)
#define  RST_PCM		__BIT(11)
#define  RST_MC			__BIT(10)
#define  RST_INTC		__BIT(9)
#define  RST_TIMER		__BIT(8)
#define  RST_GE2		__BIT(7)
#define  RST_GE1		__BIT(6)
#define  RST_SYS		__BIT(0)

#define  GPIOMODE_RGMII		__BIT(9)
#define  GPIOMODE_SDRAM		__BIT(8)
#define  GPIOMODE_MDIO		__BIT(7)
#define  GPIOMODE_JTAG		__BIT(6)
#define  GPIOMODE_UARTL		__BIT(5)
#define  GPIOMODE_UARTF2	__BIT(4)
#define  GPIOMODE_UARTF1	__BIT(3)
#define  GPIOMODE_UARTF0	__BIT(2)
#define  GPIOMODE_UARTF_0_2	\
		(GPIOMODE_UARTF0|GPIOMODE_UARTF1|GPIOMODE_UARTF2)
#define  GPIOMODE_SPI		__BIT(1)
#define  GPIOMODE_I2C		__BIT(0)

/*
 * Timer Registers
 */
#define RA_TIMER_STAT		0x00
#define RA_TIMER_0_LOAD		0x10
#define RA_TIMER_0_VALUE	0x14
#define RA_TIMER_0_CNTRL	0x18
#define RA_TIMER_1_LOAD		0x20
#define RA_TIMER_1_VALUE	0x24
#define RA_TIMER_1_CNTRL	0x28

#define  TIMER_1_RESET		__BIT(5)
#define  TIMER_0_RESET		__BIT(4)
#define  TIMER_1_INT_STATUS	__BIT(1)
#define  TIMER_0_INT_STATUS	__BIT(0)
#define  TIMER_TEST_EN		__BIT(15)
#define  TIMER_EN		__BIT(7)
#define  TIMER_MODE(x)		(((x) & 0x3) << 4)
#define   TIMER_MODE_FREE	0
#define   TIMER_MODE_PERIODIC	1
#define   TIMER_MODE_TIMEOUT	2
#define   TIMER_MODE_WDOG	3	/* only valid for TIMER_1 */
#define  TIMER_PRESCALE(x)	(((x) & 0xf) << 0)
#define   TIMER_PRESCALE_DIV_1		0
#define   TIMER_PRESCALE_DIV_4		1
#define   TIMER_PRESCALE_DIV_8		2
#define   TIMER_PRESCALE_DIV_16		3
#define   TIMER_PRESCALE_DIV_32		4
#define   TIMER_PRESCALE_DIV_64		5
#define   TIMER_PRESCALE_DIV_128	6
#define   TIMER_PRESCALE_DIV_256	7
#define   TIMER_PRESCALE_DIV_512	8
#define   TIMER_PRESCALE_DIV_1024	9
#define   TIMER_PRESCALE_DIV_2048	10
#define   TIMER_PRESCALE_DIV_4096	11
#define   TIMER_PRESCALE_DIV_8192	12
#define   TIMER_PRESCALE_DIV_16384	13
#define   TIMER_PRESCALE_DIV_32768	14
#define   TIMER_PRESCALE_DIV_65536	15

/*
 * Interrupt Controller Registers
 */
#define RA_INTCTL_IRQ0STAT	0x00
#define RA_INTCTL_IRQ1STAT	0x04
#define RA_INTCTL_TYPE		0x20
#define RA_INTCTL_RAW		0x30
#define RA_INTCTL_ENABLE	0x34
#define RA_INTCTL_DISABLE	0x38


#define INT_GLOBAL	__BIT(31)
#define INT_UDEV	__BIT(19)
#define INT_USB		__BIT(18)
#define INT_ETHSW	__BIT(17)
#define INT_R2P		__BIT(15)
#define INT_SDHC	__BIT(14)
#define INT_UARTL	__BIT(12)
#define INT_SPI		__BIT(11)
#define INT_I2S		__BIT(10)
#define INT_PERF	__BIT(9)
#define INT_NAND	__BIT(8)
#define INT_DMA		__BIT(7)
#define INT_PIO		__BIT(6)
#define INT_UARTF	__BIT(5)
#define INT_PCM		__BIT(4)
#define INT_ILLACC	__BIT(3)
#define INT_WDOG	__BIT(2)
#define INT_TIMER0	__BIT(1)
#define INT_SYSCTL	__BIT(0)

/*
 * Ralink Linear CPU Interrupt Mapping For Lists
 */
#define RA_IRQ_LOW	0
#define RA_IRQ_HIGH     1
#define RA_IRQ_PCI	2
#define RA_IRQ_FENGINE  3
#define RA_IRQ_WLAN     4
#define RA_IRQ_TIMER    5
#define RA_IRQ_SYSCTL   6
#define RA_IRQ_TIMER0   7
#define RA_IRQ_WDOG     8
#define RA_IRQ_ILLACC   9
#define RA_IRQ_PCM	10
#define RA_IRQ_UARTF    11
#define RA_IRQ_PIO	12
#define RA_IRQ_DMA	13
#define RA_IRQ_NAND     14
#define RA_IRQ_PERF     15
#define RA_IRQ_I2S	16
#define RA_IRQ_UARTL    17
#define RA_IRQ_ETHSW    18
#define RA_IRQ_USB	19
#define RA_IRQ_MAX	20

/*
 * General Purpose I/O
 */
#define RA_PIO_00_23_INT	 0x00
#define RA_PIO_00_23_EDGE_INT    0x04
#define RA_PIO_00_23_INT_RISE_EN 0x08
#define RA_PIO_00_23_INT_FALL_EN 0x0C
#define RA_PIO_00_23_DATA	0x20
#define RA_PIO_00_23_DIR	 0x24
#define RA_PIO_00_23_POLARITY    0x28
#define RA_PIO_00_23_SET_BIT     0x2C
#define RA_PIO_00_23_CLR_BIT     0x30
#define RA_PIO_00_23_TGL_BIT     0x34
#define RA_PIO_24_39_INT	 0x38
#define RA_PIO_24_39_EDGE_INT    0x3C
#define RA_PIO_24_39_INT_RISE_EN 0x40
#define RA_PIO_24_39_INT_FALL_EN 0x44
#define RA_PIO_24_39_DATA	0x48
#define RA_PIO_24_39_DIR	 0x4C
#define RA_PIO_24_39_POLARITY    0x50
#define RA_PIO_24_39_SET_BIT     0x54
#define RA_PIO_24_39_CLR_BIT     0x58
#define RA_PIO_24_39_TGL_BIT     0x5C
#define RA_PIO_40_51_INT	 0x60
#define RA_PIO_40_51_EDGE_INT    0x64
#define RA_PIO_40_51_INT_RISE_EN 0x68
#define RA_PIO_40_51_INT_FALL_EN 0x6C
#define RA_PIO_40_51_DATA	0x70
#define RA_PIO_40_51_DIR	 0x74
#define RA_PIO_40_51_POLARITY    0x78
#define RA_PIO_40_51_SET_BIT     0x7C
#define RA_PIO_40_51_CLR_BIT     0x80
#define RA_PIO_40_51_TGL_BIT     0x84
#define RA_PIO_72_95_INT	 0x88
#define RA_PIO_72_95_EDGE_INT    0x8c
#define RA_PIO_72_95_INT_RISE_EN 0x90
#define RA_PIO_72_95_INT_FALL_EN 0x94
#define RA_PIO_72_95_DATA	0x98
#define RA_PIO_72_95_DIR	 0x9c
#define RA_PIO_72_95_POLARITY    0xa0
#define RA_PIO_72_95_SET_BIT     0xa4
#define RA_PIO_72_95_CLR_BIT     0xa8
#define RA_PIO_72_95_TGL_BIT     0xac


/*
 * UART registers
 */

#define RA_UART_RBR    0x00
#define RA_UART_TBR    0x04
#define RA_UART_IER    0x08
#define RA_UART_IIR    0x0C
#define RA_UART_FCR    0x10
#define RA_UART_LCR    0x14
#define RA_UART_MCR    0x18
#define RA_UART_LSR    0x1C
#define RA_UART_MSR    0x20
#define RA_UART_DLL    0x28


#define UART_IER_ELSI	__BIT(2)
		/* Receiver Line Status Interrupt Enable */
#define UART_IER_ETBEI	__BIT(1)
		/* Transmit Buffer Empty Interrupt Enable */
#define UART_IER_ERBFI	__BIT(0)
		/* Data Ready or Character Time-Out Interrupt Enable */

#define UART_IIR_FIFOES1	__BIT(7)    /* FIFO Mode Enable Status */
#define UART_IIR_FIFOES0	__BIT(6)    /* FIFO Mode Enable Status */
#define UART_IIR_IID3	__BIT(3)    /* Interrupt Source Encoded */
#define UART_IIR_IID2	__BIT(2)    /* Interrupt Source Encoded */
#define UART_IIR_IID1	__BIT(1)    /* Interrupt Source Encoded */
#define UART_IIR_IP		__BIT(0)    /* Interrupt Pending (active low) */

#define UART_FCR_RXTRIG1	__BIT(7)    /* Receiver Interrupt Trigger Level */
#define UART_FCR_RXTRIG0	__BIT(6)    /* Receiver Interrupt Trigger Level */
#define UART_FCR_TXTRIG1	__BIT(5)    /* Transmitter Interrupt Trigger Level */
#define UART_FCR_TXTRIG0	__BIT(4)    /* Transmitter Interrupt Trigger Level */
#define UART_FCR_DMAMODE	__BIT(3)    /* Enable DMA transfers */
#define UART_FCR_TXRST	__BIT(2)    /* Reset Transmitter FIFO */
#define UART_FCR_RXRST	__BIT(1)    /* Reset Receiver FIFO */
#define UART_FCR_FIFOE	__BIT(0)    /* Transmit and Receive FIFO Enable */

#define UART_LCR_DLAB	__BIT(7)    /* Divisor Latch Access Bit */
#define UART_LCR_SB		__BIT(6)    /* Set Break */
#define UART_LCR_STKYP	__BIT(5)    /* Sticky Parity */
#define UART_LCR_EPS	__BIT(4)    /* Even Parity Select */
#define UART_LCR_PEN	__BIT(3)    /* Parity Enable */
#define UART_LCR_STB	__BIT(2)    /* Stop Bit */
#define UART_LCR_WLS1	__BIT(1)    /* Word Length Select */
#define UART_LCR_WLS0	__BIT(0)    /* Word Length Select */

#define UART_MCR_LOOP	__BIT(4)    /* Loop-back Mode Enable */

#define UART_MSR_DCD	__BIT(7)    /* Data Carrier Detect */
#define UART_MSR_RI		__BIT(6)    /* Ring Indicator */
#define UART_MSR_DSR	__BIT(5)    /* Data Set Ready */
#define UART_MSR_CTS	__BIT(4)    /* Clear To Send */
#define UART_MSR_DDCD	__BIT(3)    /* Delta Data Carrier Detect */
#define UART_MSR_TERI	__BIT(2)    /* Trailing Edge Ring Indicator */
#define UART_MSR_DDSR	__BIT(1)    /* Delta Data Set Ready */
#define UART_MSR_DCTS	__BIT(0)    /* Delta Clear To Send */

#define UART_LSR_FIFOE		__BIT(7)    /* FIFO Error Status */
#define UART_LSR_TEMT		__BIT(6)    /* Transmitter Empty */
#define UART_LSR_TDRQ		__BIT(5)    /* Transmit Data Request */
#define UART_LSR_BI		__BIT(4)    /* Break Interrupt */
#define UART_LSR_FE		__BIT(3)    /* Framing Error */
#define UART_LSR_PE		__BIT(2)    /* Parity Error */
#define UART_LSR_OE		__BIT(1)    /* Overrun Error */
#define UART_LSR_DR		__BIT(0)    /* Data Ready */

/*
 * I2C registers
 */
#define RA_I2C_CONFIG		0x00
#define RA_I2C_CLKDIV		0x04
#define RA_I2C_DEVADDR		0x08
#define RA_I2C_ADDR		0x0C
#define RA_I2C_DATAOUT		0x10
#define RA_I2C_DATAIN		0x14
#define RA_I2C_STATUS		0x18
#define RA_I2C_STARTXFR		0x1C
#define RA_I2C_BYTECNT		0x20

#define  I2C_CONFIG_ADDRLEN(x)     (((x) & 0x7) << 5)
#define   I2C_CONFIG_ADDRLEN_7     6
#define   I2C_CONFIG_ADDRLEN_8     7
#define  I2C_CONFIG_DEVADLEN(x)    (((x) & 0x7) << 2)
#define   I2C_CONFIG_DEVADLEN_6    5
#define   I2C_CONFIG_DEVADLEN_7    6
#define  I2C_CONFIG_ADDRDIS		__BIT(1)
#define  I2C_CONFIG_DEVDIS		__BIT(0)
#define  I2C_STATUS_STARTERR	__BIT(4)
#define  I2C_STATUS_ACKERR		__BIT(3)
#define  I2C_STATUS_DATARDY		__BIT(2)
#define  I2C_STATUS_SDOEMPTY	__BIT(1)
#define  I2C_STATUS_BUSY		__BIT(0)

/*
 * SPI registers
 */
#define RA_SPI_STATUS	0x00
#define RA_SPI_CONFIG	0x10
#define RA_SPI_CONTROL	0x14
#define RA_SPI_DATA	0x20

#define  SPI_STATUS_BUSY		__BIT(0)
#define  SPI_CONFIG_MSBFIRST		__BIT(8)
#define  SPI_CONFIG_CLK		__BIT(6)
#define  SPI_CONFIG_RXCLKEDGE_FALL	__BIT(5)
#define  SPI_CONFIG_TXCLKEDGE_FALL	__BIT(4)
#define  SPI_CONFIG_TRISTATE		__BIT(3)
#define  SPI_CONFIG_RATE(x)	 ((x) & 0x7)
#define   SPI_CONFIG_RATE_DIV_2     0
#define   SPI_CONFIG_RATE_DIV_4     1
#define   SPI_CONFIG_RATE_DIV_8     2
#define   SPI_CONFIG_RATE_DIV_16    3
#define   SPI_CONFIG_RATE_DIV_32    4
#define   SPI_CONFIG_RATE_DIV_64    5
#define   SPI_CONFIG_RATE_DIV_128   6
#define   SPI_CONFIG_RATE_DIV_NONE  7
#define  SPI_CONTROL_TRISTATE	__BIT(3)
#define  SPI_CONTROL_STARTWR		__BIT(2)
#define  SPI_CONTROL_STARTRD		__BIT(1)
#define  SPI_CONTROL_ENABLE_LOW     (0 << 0)
#define  SPI_CONTROL_ENABLE_HIGH	__BIT(0)
#define  SPI_DATA_VAL(x)	    ((x) & 0xff)

/*
 * Frame Engine registers
 */
#define RA_FE_MDIO_ACCESS      0x000
#define RA_FE_MDIO_CFG1	0x004
#define RA_FE_GLOBAL_CFG       0x008
#define RA_FE_GLOBAL_RESET     0x00C
#define RA_FE_INT_STATUS       0x010
#define RA_FE_INT_ENABLE       0x014
#define RA_FE_MDIO_CFG2	0x018
#define RA_FE_TIME_STAMP       0x01C
#define RA_FE_GDMA1_FWD_CFG    0x020
#define RA_FE_GDMA1_SCHED_CFG  0x024
#define RA_FE_GDMA1_SHAPE_CFG  0x028
#define RA_FE_GDMA1_MAC_LSB    0x02C
#define RA_FE_GDMA1_MAC_MSB    0x030
#define RA_FE_PSE_FQ_CFG       0x040
#define RA_FE_CDMA_FC_CFG      0x044
#define RA_FE_GDMA1_FC_CFG     0x048
#define RA_FE_GDMA2_FC_CFG     0x04C
#define RA_FE_CDMA_OQ_STA      0x050
#define RA_FE_GDMA1_OQ_STA     0x054
#define RA_FE_GDMA2_OQ_STA     0x058
#define RA_FE_PSE_IQ_STA       0x05C
#define RA_FE_GDMA2_FWD_CFG    0x060
#define RA_FE_GDMA2_SCHED_CFG  0x064
#define RA_FE_GDMA2_SHAPE_CFG  0x068
#define RA_FE_GDMA2_MAC_LSB    0x06C
#define RA_FE_GDMA2_MAC_MSB    0x070
#define RA_FE_CDMA_CSG_CFG     0x080
#define RA_FE_CDMA_SCHED_CFG   0x084
#define RA_FE_PPPOE_SID_0001   0x088
#define RA_FE_PPPOE_SID_0203   0x08C
#define RA_FE_PPPOE_SID_0405   0x090
#define RA_FE_PPPOE_SID_0607   0x094
#define RA_FE_PPPOE_SID_0809   0x098
#define RA_FE_PPPOE_SID_1011   0x09C
#define RA_FE_PPPOE_SID_1213   0x0A0
#define RA_FE_PPPOE_SID_1415   0x0A4
#define RA_FE_VLAN_ID_0001     0x0A8
#define RA_FE_VLAN_ID_0203     0x0AC
#define RA_FE_VLAN_ID_0405     0x0B0
#define RA_FE_VLAN_ID_0607     0x0B4
#define RA_FE_VLAN_ID_0809     0x0B8
#define RA_FE_VLAN_ID_1011     0x0BC
#define RA_FE_VLAN_ID_1213     0x0C0
#define RA_FE_VLAN_ID_1415     0x0C4
#define RA_FE_PDMA_GLOBAL_CFG  0x100
#define RA_FE_PDMA_RESET_IDX   0x104
#define RA_FE_PDMA_SCHED_CFG   0x108
#define RA_FE_PDMA_DLY_INT_CFG 0x10C
#define RA_FE_PDMA_TX0_PTR     0x110
#define RA_FE_PDMA_TX0_COUNT   0x114
#define RA_FE_PDMA_TX0_CPU_IDX 0x118
#define RA_FE_PDMA_TX0_DMA_IDX 0x11C
#define RA_FE_PDMA_TX1_PTR     0x120
#define RA_FE_PDMA_TX1_COUNT   0x124
#define RA_FE_PDMA_TX1_CPU_IDX 0x128
#define RA_FE_PDMA_TX1_DMA_IDX 0x12C
#define RA_FE_PDMA_RX0_PTR     0x130
#define RA_FE_PDMA_RX0_COUNT   0x134
#define RA_FE_PDMA_RX0_CPU_IDX 0x138
#define RA_FE_PDMA_RX0_DMA_IDX 0x13C
#define RA_FE_PDMA_TX2_PTR     0x140
#define RA_FE_PDMA_TX2_COUNT   0x144
#define RA_FE_PDMA_TX2_CPU_IDX 0x148
#define RA_FE_PDMA_TX2_DMA_IDX 0x14C
#define RA_FE_PDMA_TX3_PTR     0x150
#define RA_FE_PDMA_TX3_COUNT   0x154
#define RA_FE_PDMA_TX3_CPU_IDX 0x158
#define RA_FE_PDMA_TX3_DMA_IDX 0x15C
#define RA_FE_PDMA_FC_CFG      0x1F0
/* TODO: FE_COUNTERS */

#define  MDIO_ACCESS_TRG		__BIT(31)
#define  MDIO_ACCESS_WR		__BIT(30)
#define  MDIO_ACCESS_PHY_ADDR(x) (((x) & 0x1f) << 24)
#define  MDIO_ACCESS_REG(x)      (((x) & 0x1f) << 16)
#define  MDIO_ACCESS_DATA(x)     ((x) & 0xffff)
#define  MDIO_CFG_AUTO_POLL	__BIT(29)
#define  MDIO_CFG_PHY_ADDR(x)    (((x) & 0x1f) << 24)
#define  MDIO_CFG_BP_EN		__BIT(16)
#define  MDIO_CFG_FORCE_CFG	__BIT(15)
#define  MDIO_CFG_SPEED(x)       (((x) & 0x3) << 13)
#define   MDIO_CFG_SPEED_1000M   2
#define   MDIO_CFG_SPEED_100M    1
#define   MDIO_CFG_SPEED_10M     0
#define  MDIO_CFG_FULL_DUPLEX	__BIT(12)
#define  MDIO_CFG_FC_TX		__BIT(11)
#define  MDIO_CFG_FC_RX		__BIT(10)
#define  MDIO_CFG_LINK_DOWN	__BIT(9)
#define  MDIO_CFG_AUTO_DONE	__BIT(8)
#define  MDIO_CFG_MDC_CLKDIV(x)  (((x) & 0x3) << 6)
#define   MDIO_CFG_MDC_512KHZ    3
#define   MDIO_CFG_MDC_1MHZ      2
#define   MDIO_CFG_MDC_2MHZ      1
#define   MDIO_CFG_MDC_4MHZ      0
#define  MDIO_CFG_TURBO_50MHZ	__BIT(5)
#define  MDIO_CFG_TURBO_EN	__BIT(4)
#define  MDIO_CFG_RX_CLK_SKEW   (((x) & 0x3) << 2)
#define   MDIO_CFG_RX_SKEW_INV   3
#define   MDIO_CFG_RX_SKEW_400PS 2
#define   MDIO_CFG_RX_SKEW_200PS 1
#define   MDIO_CFG_RX_SKEW_ZERO  0
#define  MDIO_CFG_TX_CLK_MODE(x) (((x) & 0x1) << 0)
#define   MDIO_CFG_TX_CLK_MODE_3COM  1
#define   MDIO_CFG_TX_CLK_MODE_HP    0
#define  FE_GLOBAL_CFG_EXT_VLAN(x) (((x) & 0xffff) << 16)
#define  FE_GLOBAL_CFG_US_CLK(x)   (((x) & 0xff) << 8)
#define  FE_GLOBAL_CFG_L2_SPACE(x) (((x) & 0xf) << 4)
#define  FE_GLOBAL_RESET_PSE	__BIT(0)
#define  FE_INT_PPE_COUNT_HIGH	__BIT(31)
#define  FE_INT_DMA_COUNT_HIGH	__BIT(29)
#define  FE_INT_PSE_P2_FC_ASSERT	__BIT(26)
#define  FE_INT_PSE_FC_DROP		__BIT(24)
#define  FE_INT_GDMA_DROP_OTHER	__BIT(23)
#define  FE_INT_PSE_P1_FC_ASSERT	__BIT(22)
#define  FE_INT_PSE_P0_FC_ASSERT	__BIT(21)
#define  FE_INT_PSE_FQ_EMPTY	__BIT(20)
#define  FE_INT_TX_COHERENT		__BIT(17)
#define  FE_INT_RX_COHERENT		__BIT(16)
#define  FE_INT_TX3			__BIT(11)
#define  FE_INT_TX2			__BIT(10)
#define  FE_INT_TX1			__BIT(9)
#define  FE_INT_TX0			__BIT(8)
#define  FE_INT_RX			__BIT(2)
#define  FE_INT_TX_DELAY		__BIT(1)
#define  FE_INT_RX_DELAY		__BIT(0)
#define  FE_GDMA_FWD_CFG_JUMBO_LEN(x)  (((x) & 0xf) << 28)
#define  FE_GDMA_FWD_CFG_DROP_256B	__BIT(23)
#define  FE_GDMA_FWD_CFG_IP4_CRC_EN	__BIT(22)
#define  FE_GDMA_FWD_CFG_TCP_CRC_EN	__BIT(21)
#define  FE_GDMA_FWD_CFG_UDP_CRC_EN	__BIT(20)
#define  FE_GDMA_FWD_CFG_JUMBO_EN	__BIT(19)
#define  FE_GDMA_FWD_CFG_DIS_TX_PAD	__BIT(18)
#define  FE_GDMA_FWD_CFG_DIS_TX_CRC	__BIT(17)
#define  FE_GDMA_FWD_CFG_STRIP_RX_CRC	__BIT(16)
#define  FE_GDMA_FWD_CFG_UNICA_PORT(x) (((x) & 0x3) << 12)
#define  FE_GDMA_FWD_CFG_BROAD_PORT(x) (((x) & 0x3) << 8)
#define  FE_GDMA_FWD_CFG_MULTI_PORT(x) (((x) & 0x3) << 6)
#define  FE_GDMA_FWD_CFG_OTHER_PORT(x) (((x) & 0x3) << 0)
#define   FE_GDMA_FWD_CFG_PORT_DROP  7
#define   FE_GDMA_FWD_CFG_PORT_PPE   6
#define   FE_GDMA_FWD_CFG_PORT_GDMA2 2
#define   FE_GDMA_FWD_CFG_PORT_GDMA1 1
#define   FE_GDMA_FWD_CFG_PORT_CPU   0
#define  FE_PSE_FQ_MAX_COUNT(x)  (((x) & 0xff) << 24)
#define  FE_PSE_FQ_FC_RELEASE(x) (((x) & 0xff) << 16)
#define  FE_PSE_FQ_FC_ASSERT(x)  (((x) & 0xff) << 8)
#define  FE_PSE_FQ_FC_DROP(x)    (((x) & 0xff) << 0)
#define  FE_CDMA_CSG_CFG_VLAN_TAG(x) (((x) & 0xffff) << 16)
#define  FE_CDMA_CSG_CFG_IP4_CRC_EN	__BIT(2)
#define  FE_CDMA_CSG_CFG_UDP_CRC_EN	__BIT(1)
#define  FE_CDMA_CSG_CFG_TCP_CRC_EN	__BIT(0)
#define  FE_PDMA_GLOBAL_CFG_HDR_SEG_LEN	__BIT(16)
#define  FE_PDMA_GLOBAL_CFG_TX_WB_DDONE	__BIT(6)
#define  FE_PDMA_GLOBAL_CFG_BURST_SZ(x)  (((x) & 0x3) << 4)
#define   FE_PDMA_GLOBAL_CFG_BURST_SZ_4  (0 << 4)
#define   FE_PDMA_GLOBAL_CFG_BURST_SZ_8	__BIT(4)
#define   FE_PDMA_GLOBAL_CFG_BURST_SZ_16 (2 << 4)
#define  FE_PDMA_GLOBAL_CFG_RX_DMA_BUSY	__BIT(3)
#define  FE_PDMA_GLOBAL_CFG_RX_DMA_EN	__BIT(2)
#define  FE_PDMA_GLOBAL_CFG_TX_DMA_BUSY	__BIT(1)
#define  FE_PDMA_GLOBAL_CFG_TX_DMA_EN	__BIT(0)
#define  PDMA_RST_RX0		__BIT(16)
#define  PDMA_RST_TX3		__BIT(3)
#define  PDMA_RST_TX2		__BIT(2)
#define  PDMA_RST_TX1		__BIT(1)
#define  PDMA_RST_TX0		__BIT(0)

/*
 * 10/100 Switch registers
 */

#define RA_ETH_SW_ISR    0x00
#define RA_ETH_SW_IMR    0x04
#define RA_ETH_SW_FCT0   0x08
#define RA_ETH_SW_FCT1   0x0C
#define RA_ETH_SW_PFC0   0x10
#define RA_ETH_SW_PFC1   0x14
#define RA_ETH_SW_PFC2   0x18
#define RA_ETH_SW_QCS0   0x1C
#define RA_ETH_SW_QCS1   0x20
#define RA_ETH_SW_ATS    0x24
#define RA_ETH_SW_ATS0   0x28
#define RA_ETH_SW_ATS1   0x2C
#define RA_ETH_SW_ATS2   0x30
#define RA_ETH_SW_WMAD0  0x34
#define RA_ETH_SW_WMAD1  0x38
#define RA_ETH_SW_WMAD2  0x3C
#define RA_ETH_SW_PVIDC0 0x40
#define RA_ETH_SW_PVIDC1 0x44
#define RA_ETH_SW_PVIDC2 0x48
#define RA_ETH_SW_PVIDC3 0x4C
#define RA_ETH_SW_VLANI0 0x50
#define RA_ETH_SW_VLANI1 0x54
#define RA_ETH_SW_VLANI2 0x58
#define RA_ETH_SW_VLANI3 0x5C
#define RA_ETH_SW_VLANI4 0x60
#define RA_ETH_SW_VLANI5 0x64
#define RA_ETH_SW_VLANI6 0x68
#define RA_ETH_SW_VLANI7 0x6C
#define RA_ETH_SW_VMSC0  0x70
#define RA_ETH_SW_VMSC1  0x74
#define RA_ETH_SW_VMSC2  0x78
#define RA_ETH_SW_VMSC3  0x7C
#define RA_ETH_SW_POA    0x80
#define RA_ETH_SW_FPA    0x84
#define RA_ETH_SW_PTS    0x88
#define RA_ETH_SW_SOCPC  0x8C
#define RA_ETH_SW_POC0   0x90
#define RA_ETH_SW_POC1   0x94
#define RA_ETH_SW_POC2   0x98
#define RA_ETH_SW_SWGC   0x9C
#define RA_ETH_SW_RST    0xA0
#define RA_ETH_SW_LEDP0  0xA4
#define RA_ETH_SW_LEDP1  0xA8
#define RA_ETH_SW_LEDP2  0xAC
#define RA_ETH_SW_LEDP3  0xB0
#define RA_ETH_SW_LEDP4  0xB4
#define RA_ETH_SW_WDOG   0xB8
#define RA_ETH_SW_DBG    0xBC
#define RA_ETH_SW_PCTL0  0xC0
#define RA_ETH_SW_PCTL1  0xC4
#define RA_ETH_SW_FPORT  0xC8
#define RA_ETH_SW_FTC2   0xCC
#define RA_ETH_SW_QSS0   0xD0
#define RA_ETH_SW_QSS1   0xD4
#define RA_ETH_SW_DBGC   0xD8
#define RA_ETH_SW_MTI1   0xDC
#define RA_ETH_SW_PPC    0xE0
#define RA_ETH_SW_SGC2   0xE4
#define RA_ETH_SW_PCNT0  0xE8
#define RA_ETH_SW_PCNT1  0xEC
#define RA_ETH_SW_PCNT2  0xF0
#define RA_ETH_SW_PCNT3  0xF4
#define RA_ETH_SW_PCNT4  0xF8
#define RA_ETH_SW_PCNT5  0xFC

#define  ISR_WDOG1_EXPIRED	__BIT(29)
#define  ISR_WDOG0_EXPIRED	__BIT(28)
#define  ISR_HAS_INTRUDER	__BIT(27)
#define  ISR_PORT_STS_CHNG	__BIT(26)
#define  ISR_BRDCAST_STORM	__BIT(25)
#define  ISR_MUST_DROP_LAN	__BIT(24)
#define  ISR_GLOB_QUE_FULL	__BIT(23)
#define  ISR_LAN_QUE6_FULL	__BIT(20)
#define  ISR_LAN_QUE5_FULL	__BIT(19)
#define  ISR_LAN_QUE4_FULL	__BIT(18)
#define  ISR_LAN_QUE3_FULL	__BIT(17)
#define  ISR_LAN_QUE2_FULL	__BIT(16)
#define  ISR_LAN_QUE1_FULL	__BIT(15)
#define  ISR_LAN_QUE0_FULL	__BIT(14)
#define  FTC0_REL_THR	  24
#define  FTC0_SET_THR	  16
#define  FTC0_DROP_REL_THR     8
#define  FTC0_DROP_SET_THR     0
#define  FTC1_PER_PORT_THR     0
#define  PCTL0_WR_VAL(x) (((x) & 0xffff) << 16)
#define  PCTL0_RD_CMD	__BIT(14)
#define  PCTL0_WR_CMD	__BIT(13)
#define  PCTL0_REG(x)    (((x) & 0x1f) << 8)
#define  PCTL0_ADDR(x)   (((x) & 0x1f) << 0)
#define  PCTL1_RD_VAL(x) (((x) >> 16) & 0xffff)
#define  PCTL1_RD_DONE	__BIT(1)	/* read clear */
#define  PCTL1_WR_DONE	__BIT(0)	/* read clear */
#define  SGC2_WL_FC_EN	__BIT(30)
#define  SGC2_PORT5_IS_LAN	__BIT(29)
#define  SGC2_PORT4_IS_LAN	__BIT(28)
#define  SGC2_PORT3_IS_LAN	__BIT(27)
#define  SGC2_PORT2_IS_LAN	__BIT(26)
#define  SGC2_PORT1_IS_LAN	__BIT(25)
#define  SGC2_PORT0_IS_LAN	__BIT(24)
#define  SGC2_TX_CPU_TPID(x) ((x) << 16)
#define  SGC2_ARBITER_LAN_EN	__BIT(11)
#define  SGC2_CPU_TPID_EN	__BIT(10)
#define  SGC2_DBL_TAG_EN5	__BIT(5)
#define  SGC2_DBL_TAG_EN4	__BIT(4)
#define  SGC2_DBL_TAG_EN3	__BIT(3)
#define  SGC2_DBL_TAG_EN2	__BIT(2)
#define  SGC2_DBL_TAG_EN1	__BIT(1)
#define  SGC2_DBL_TAG_EN0	__BIT(0)


#define FTC_THR_MSK	   0xff

#define PFC0_MTCC_LIMIT       24
#define PFC0_TURN_OFF_CF      16
#define PFC0_TURN_OFF_CF_MSK  0xff
#define PFC0_VO_NUM	   12
#define PFC0_CL_NUM	   8
#define PFC0_BE_NUM	   4
#define PFC0_BK_NUM	   0
#define PFC0_NUM_MSK	  0xf

#define PFC1_P6_Q1_EN		__BIT(31)
#define PFC1_P6_TOS_EN	__BIT(30)
#define PFC1_P5_TOS_EN	__BIT(29)
#define PFC1_P4_TOS_EN	__BIT(28)
#define PFC1_P3_TOS_EN	__BIT(27)

#define PFC1_P1_TOS_EN	__BIT(25)
#define PFC1_P0_TOS_EN	__BIT(24)
#define PFC1_PORT_PRI6	12
#define PFC1_PORT_PRI5	10
#define PFC1_PORT_PRI4	8
#define PFC1_PORT_PRI3	6
#define PFC1_PORT_PRI2	4
#define PFC1_PORT_PRI1	2
#define PFC1_PORT_PRI0	0
#define PFC1_PORT_MSK	 0x3

#define PFC2_PRI_THR_VO       24
#define PFC2_PRI_THR_CL       16
#define PFC2_PRI_THR_BE       8
#define PFC2_PRI_THR_BK       0
#define PFC2_PRI_THR_MSK      0xff

#define GQC0_EMPTY_BLOCKS      0
#define GQC0_EMPTY_BLOCKS_MSK  0xff

/*
 * USB OTG Registers
 */
#define RA_USB_OTG_OTG_CNTRL       0x000
#define RA_USB_OTG_OTG_INT	 0x004
#define RA_USB_OTG_AHB_CFG	 0x008
#define RA_USB_OTG_CFG	     0x00C
#define RA_USB_OTG_RESET	   0x010
#define RA_USB_OTG_INT	     0x014
#define RA_USB_OTG_INT_MASK	0x018
#define RA_USB_OTG_RX_STAT	 0x01C
#define RA_USB_OTG_RX_POP_STAT     0x020
#define RA_USB_OTG_RX_FIFO_SZ      0x024
#define RA_USB_OTG_TX_FIFO_SZ      0x028
#define RA_USB_OTG_TX_FIFO_STAT    0x02C
#define RA_USB_OTG_I2C_ACCESS      0x030
#define RA_USB_OTG_PHY_CTL	 0x034
#define RA_USB_OTG_GPIO	    0x038
#define RA_USB_OTG_GUID	    0x03C
#define RA_USB_OTG_SNPSID	  0x040
#define RA_USB_OTG_HWCFG1	  0x044
#define RA_USB_OTG_HWCFG2	  0x048
#define RA_USB_OTG_HWCFG3	  0x04C
#define RA_USB_OTG_HWCFG4	  0x050
#define RA_USB_OTG_HC_TX_FIFO_SZ   0x100
#define RA_USB_OTG_DV_TX_FIFO_SZ   0x104
#define RA_USB_OTG_HC_CFG	  0x400
#define RA_USB_OTG_HC_FRM_INTRVL   0x404
#define RA_USB_OTG_HC_FRM_NUM      0x408
#define RA_USB_OTG_HC_TX_STAT      0x410
#define RA_USB_OTG_HC_INT	  0x414
#define RA_USB_OTG_HC_INT_MASK     0x418
#define RA_USB_OTG_HC_PORT	 0x440
#define RA_USB_OTG_HC_CH_CFG       0x500
#define RA_USB_OTG_HC_CH_SPLT      0x504
#define RA_USB_OTG_HC_CH_INT       0x508
#define RA_USB_OTG_HC_CH_INT_MASK  0x50C
#define RA_USB_OTG_HC_CH_XFER      0x510
#define RA_USB_OTG_HC_CH_DMA_ADDR  0x514
#define RA_USB_OTG_DV_CFG	  0x800
#define RA_USB_OTG_DV_CTL	  0x804
#define RA_USB_OTG_DV_STAT	 0x808
#define RA_USB_OTG_DV_IN_INT_MASK  0x810
#define RA_USB_OTG_DV_OUT_INT_MASK 0x814
#define RA_USB_OTG_DV_ALL_INT      0x818
#define RA_USB_OTG_DV_EP_INT_MASK  0x81c
#define RA_USB_OTG_DV_IN_SEQ_RQ1   0x820
#define RA_USB_OTG_DV_IN_SEQ_RQ2   0x824
#define RA_USB_OTG_DV_IN_SEQ_RQ3   0x830
#define RA_USB_OTG_DV_IN_SEQ_RQ4   0x834
#define RA_USB_OTG_DV_VBUS_DISCH   0x828
#define RA_USB_OTG_DV_VBUS_PULSE   0x82c
#define RA_USB_OTG_DV_THRESH_CTL   0x830
#define RA_USB_OTG_DV_IN_FIFO_INT  0x834
#define RA_USB_OTG_DV_IN0_CTL      0x900

#define  OTG_OTG_CNTRL_B_SESS_VALID	__BIT(19)
#define  OTG_OTG_CNTRL_A_SESS_VALID	__BIT(18)
#define  OTG_OTG_CNTRL_DEBOUNCE_SHORT	__BIT(17)
#define  OTG_OTG_CNTRL_CONNID_STATUS	__BIT(16)
#define  OTG_OTG_CNTRL_DV_HNP_EN		__BIT(11)
#define  OTG_OTG_CNTRL_HC_SET_HNP_EN	__BIT(10)
#define  OTG_OTG_CNTRL_HNP_REQ		__BIT(9)
#define  OTG_OTG_CNTRL_HNP_SUCCESS	__BIT(8)
#define  OTG_OTG_CNTRL_SESS_REQ		__BIT(1)
#define  OTG_OTG_CNTRL_SESS_REQ_SUCCESS	__BIT(0)
#define  OTG_OTG_INT_DEBOUNCE_DONE	__BIT(19)
#define  OTG_OTG_INT_ADEV_TIMEOUT		__BIT(18)
#define  OTG_OTG_INT_HOST_NEG_DETECT	__BIT(17)
#define  OTG_OTG_INT_HOST_NEG_STATUS	__BIT(9)
#define  OTG_OTG_INT_SESSION_REQ_STATUS	__BIT(8)
#define  OTG_OTG_INT_SESSION_END_STATUS	__BIT(2)
#define  OTG_AHB_CFG_TX_PFIFO_EMPTY_INT_EN	__BIT(8)
#define  OTG_AHB_CFG_TX_NPFIFO_EMPTY_INT_EN	__BIT(7)
#define  OTG_AHB_CFG_DMA_EN			__BIT(5)
#define  OTG_AHB_CFG_BURST(x)	       (((x) & 0xf) << 1)
#define   OTG_AHB_CFG_BURST_SINGLE	     0
#define   OTG_AHB_CFG_BURST_INCR  	     1
#define   OTG_AHB_CFG_BURST_INCR4 	     3
#define   OTG_AHB_CFG_BURST_INCR8 	     5
#define   OTG_AHB_CFG_BURST_INCR16	     7
#define  OTG_AHB_CFG_GLOBAL_INT_EN		__BIT(0)
#define  OTG_CFG_CORRUPT_TX		__BIT(31)
#define  OTG_CFG_FORCE_DEVICE		__BIT(30)
#define  OTG_CFG_FORCE_HOST		__BIT(29)
#define  OTG_CFG_ULPI_EXT_VBUS_IND_SEL	__BIT(22)
#define  OTG_CFG_ULPI_EXT_VBUS_IND	__BIT(21)
#define  OTG_CFG_ULPI_EXT_VBUS_DRV	__BIT(20)
#define  OTG_CFG_ULPI_CLOCK_SUSPEND	__BIT(19)
#define  OTG_CFG_ULPI_AUTO_RESUME		__BIT(18)
#define  OTG_CFG_ULPI_FS_LS_SEL		__BIT(17)
#define  OTG_CFG_UTMI_I2C_SEL		__BIT(16)
#define  OTG_CFG_TURNAROUND_TIME(x)      (((x) & 0xf) << 10)
#define  OTG_CFG_HNP_CAP			__BIT(9)
#define  OTG_CFG_SRP_CAP			__BIT(8)
#define  OTG_CFG_ULPI_DDR_SEL		__BIT(7)
#define  OTG_CFG_HS_PHY_SEL		__BIT(6)
#define  OTG_CFG_FS_IF_SEL		__BIT(5)
#define  OTG_CFG_ULPI_UTMI_SEL		__BIT(4)
#define  OTG_CFG_PHY_IF			__BIT(3)
#define  OTG_CFG_TIMEOUT(x)	      (((x) & 0x7) << 0)
#define  OTG_RST_AHB_IDLE			__BIT(31)
#define  OTG_RST_DMA_ACTIVE		__BIT(30)
#define  OTG_RST_TXQ_TO_FLUSH(x)	 (((x) & 0x1f) << 6)
#define   OTG_RST_TXQ_FLUSH_ALL	  0x10
#define  OTG_RST_TXQ_FLUSH		__BIT(5)
#define  OTG_RST_RXQ_FLUSH		__BIT(4)
#define  OTG_RST_INQ_FLUSH		__BIT(3)
#define  OTG_RST_HC_FRAME			__BIT(2)
#define  OTG_RST_AHB			__BIT(1)
#define  OTG_RST_CORE			__BIT(0)
#define  OTG_INT_RESUME			__BIT(31)
#define  OTG_INT_SESSION_REQ		__BIT(30)
#define  OTG_INT_DISCONNECT		__BIT(29)
#define  OTG_INT_CONNID_STATUS		__BIT(28)
#define  OTG_INT_PTX_EMPTY		__BIT(26)
#define  OTG_INT_HOST_CHANNEL		__BIT(25)
#define  OTG_INT_PORT_STATUS		__BIT(24)
#define  OTG_INT_DMA_FETCH_SUSPEND	__BIT(22)
#define  OTG_INT_INCOMPLETE_PERIODIC	__BIT(21)
#define  OTG_INT_INCOMPLETE_ISOC		__BIT(20)
#define  OTG_INT_DV_OUT_EP		__BIT(19)
#define  OTG_INT_DV_IN_EP			__BIT(18)
#define  OTG_INT_DV_EP_MISMATCH		__BIT(17)
#define  OTG_INT_DV_PERIODIC_END		__BIT(15)
#define  OTG_INT_DV_ISOC_OUT_DROP		__BIT(14)
#define  OTG_INT_DV_ENUM_COMPLETE		__BIT(13)
#define  OTG_INT_DV_USB_RESET		__BIT(12)
#define  OTG_INT_DV_USB_SUSPEND		__BIT(11)
#define  OTG_INT_DV_USB_EARLY_SUSPEND	__BIT(10)
#define  OTG_INT_I2C			__BIT(9)
#define  OTG_INT_ULPI_CARKIT		__BIT(8)
#define  OTG_INT_DV_OUT_NAK_EFFECTIVE	__BIT(7)
#define  OTG_INT_DV_IN_NAK_EFFECTIVE	__BIT(6)
#define  OTG_INT_NPTX_EMPTY		__BIT(5)
#define  OTG_INT_RX_FIFO			__BIT(4)
#define  OTG_INT_SOF			__BIT(3)
#define  OTG_INT_OTG			__BIT(2)
#define  OTG_INT_MODE_MISMATCH		__BIT(1)
#define  OTG_INT_MODE			__BIT(0)
#define  USB_OTG_SNPSID_CORE_REV_2_00  0x4F542000
#define  OTG_HC_CFG_FORCE_NO_HS		__BIT(2)
#define  OTG_HC_CFG_FSLS_CLK_SEL(x)      (((x) & 0x3) << 0)
#define   OTG_HC_CFG_FS_CLK_3060	 0
#define   OTG_HC_CFG_FS_CLK_48	   1
#define   OTG_HC_CFG_LS_CLK_3060	 0
#define   OTG_HC_CFG_LS_CLK_48	   1
#define   OTG_HC_CFG_LS_CLK_6	    2
#define  USB_OTG_HC_FRM_NUM(x)	    (x & 0x3fff)
#define  USB_OTG_HC_FRM_REM(x)	    (x >> 16)
#define  USB_OTG_HC_PORT_SPEED(x)	 (((x) >> 17) & 0x3)
#define   USB_OTG_HC_PORT_SPEED_HS	0
#define   USB_OTG_HC_PORT_SPEED_FS	1
#define   USB_OTG_HC_PORT_SPEED_LS	2
#define  USB_OTG_HC_PORT_TEST(x)	  (((x) & 0xf) << 13)
#define   USB_OTG_HC_PORT_TEST_DISABLED   0
#define   USB_OTG_HC_PORT_TEST_J_MODE     1
#define   USB_OTG_HC_PORT_TEST_K_MODE     2
#define   USB_OTG_HC_PORT_TEST_NAK_MODE   3
#define   USB_OTG_HC_PORT_TEST_PKT_MODE   4
#define   USB_OTG_HC_PORT_TEST_FORCE_MODE 5
#define  USB_OTG_HC_PORT_POWER		__BIT(12)
#define  USB_OTG_HC_PORT_LINE_STAT	(((x) >> 10) & 0x3)
#define   USB_OTG_HC_PORT_LINE_STAT_DP    1
#define   USB_OTG_HC_PORT_LINE_STAT_DM    3
#define  USB_OTG_HC_PORT_RESET		__BIT(8)
#define  USB_OTG_HC_PORT_SUSPEND		__BIT(7)
#define  USB_OTG_HC_PORT_RESUME		__BIT(6)
#define  USB_OTG_HC_PORT_OVCURR_CHANGE	__BIT(5)
#define  USB_OTG_HC_PORT_OVCURR		__BIT(4)
#define  USB_OTG_HC_PORT_ENABLE_CHANGE	__BIT(3)
#define  USB_OTG_HC_PORT_ENABLE		__BIT(2)
#define  USB_OTG_HC_PORT_CONNECT_CHANGE	__BIT(1)
#define  USB_OTG_HC_PORT_STATUS		__BIT(0)
#define  USB_OTG_HC_CH_CFG_ENABLE		__BIT(31)
#define  USB_OTG_HC_CH_CFG_DISABLE		__BIT(30)
#define  USB_OTG_HC_CH_CFG_ODD_FRAME	__BIT(29)
#define  USB_OTG_HC_CH_CFG_DEV_ADDR(x)    (((x) & 0x7f) << 22)
#define  USB_OTG_HC_CH_CFG_MULTI_CNT(x)   (((x) & 0x3) << 20)
#define  USB_OTG_HC_CH_CFG_EP_TYPE(x)     (((x) & 0x3) << 18)
#define   USB_OTG_HC_CH_CFG_EP_TYPE_CTRL   0
#define   USB_OTG_HC_CH_CFG_EP_TYPE_ISOC   1
#define   USB_OTG_HC_CH_CFG_EP_TYPE_BULK   2
#define   USB_OTG_HC_CH_CFG_EP_TYPE_INTR   3
#define  USB_OTG_HC_CH_CFG_LS		__BIT(17)
#define  USB_OTG_HC_CH_CFG_EP_DIR(x)      (((x) & 0x1) << 15)
#define   USB_OTG_HC_CH_CFG_EP_DIR_OUT     0
#define   USB_OTG_HC_CH_CFG_EP_DIR_IN      1
#define  USB_OTG_HC_CH_CFG_EP_NUM(x)      (((x) & 0xf) << 11)
#define  USB_OTG_HC_CH_CFG_MAX_PKT_SZ(x)  (((x) & 0x7ff) << 0)
#define  USB_OTG_HC_CH_SPLT_EN		__BIT(31)
#define  USB_OTG_HC_CH_SPLT_COMPLETE	__BIT(16)
#define  USB_OTG_HC_CH_SPLT_POS(x)       (((x) & 0x3) << 14)
#define   USB_OTG_HC_CH_SPLT_POS_MID      0
#define   USB_OTG_HC_CH_SPLT_POS_END      1
#define   USB_OTG_HC_CH_SPLT_POS_BEGIN    2
#define   USB_OTG_HC_CH_SPLT_POS_ALL      3
#define  USB_OTG_HC_CH_SPLT_HUB_ADDR(x)  (((x) & 0x7f) << 7)
#define  USB_OTG_HC_CH_SPLT_PORT_ADDR(x) (((x) & 0x7f) << 0)
#define  USB_OTG_HC_CH_INT_ALL	    0x7ff
#define  USB_OTG_HC_CH_INT_TOGGLE_ERROR	__BIT(10)
#define  USB_OTG_HC_CH_INT_FRAME_OVERRUN	__BIT(9)
#define  USB_OTG_HC_CH_INT_BABBLE_ERROR	__BIT(8)
#define  USB_OTG_HC_CH_INT_XACT_ERROR	__BIT(7)
#define  USB_OTG_HC_CH_INT_NYET		__BIT(6)
#define  USB_OTG_HC_CH_INT_ACK		__BIT(5)
#define  USB_OTG_HC_CH_INT_NAK		__BIT(4)
#define  USB_OTG_HC_CH_INT_STALL		__BIT(3)
#define  USB_OTG_HC_CH_INT_DMA_ERROR	__BIT(2)
#define  USB_OTG_HC_CH_INT_HALTED		__BIT(1)
#define  USB_OTG_HC_CH_INT_XFER_COMPLETE	__BIT(0)
#define  USB_OTG_HC_CH_XFER_DO_PING	__BIT(31)
#define  USB_OTG_HC_CH_WR_XFER_PID(x)       (((x) & 0x3) << 29)
#define  USB_OTG_HC_CH_RD_XFER_PID(x)       (((x) >> 29) & 0x3)
#define  USB_OTG_HC_CH_XFER_PID_DATA0    0
#define  USB_OTG_HC_CH_XFER_PID_DATA2    1
#define  USB_OTG_HC_CH_XFER_PID_DATA1    2
#define  USB_OTG_HC_CH_XFER_PID_SETUP    3
#define  USB_OTG_HC_CH_XFER_PID_MDATA    3
#define  USB_OTG_HC_CH_XFER_SET_PKT_CNT(x)   (((x) & 0x3ff) << 19)
#define  USB_OTG_HC_CH_XFER_SET_BYTES(x)     ((x) & 0x7ffff)
#define  USB_OTG_HC_CH_XFER_GET_PKT_CNT(x)   (((x) >> 19) & 0x3ff)
#define  USB_OTG_HC_CH_XFER_GET_BYTES(x)     ((x) & 0x7ffff)

/* PCIe Registers - 0x10140000 */
#define RA_PCI_PCICFG		0x0000
#define  PCICFG_P2P_BR_DEVNUM1	__BITS(23,20)
#define  PCICFG_P2P_BR_DEVNUM0	__BITS(19,16)
#define  PCICFG_PSIRST		__BIT(1)
#define RA_PCI_PCIINT		0x0008
#define  PCIINT_INT3		__BIT(21) // PCIe1 interrupt
#define  PCIINT_INT2		__BIT(20) // PCIe0 interrupt
#define  PCIINT_INT1		__BIT(19)
#define  PCIINT_INT0		__BIT(18)
#define RA_PCI_PCIENA		0x000c
#define RA_PCI_CFGADDR		0x0020
#define  CFGADDR_EXTREG		__BITS(27,24)
#define  CFGADDR_BUS		__BITS(23,16)
#define  CFGADDR_DEV		__BITS(15,11)
#define  CFGADDR_FUN		__BITS(10,8)
#define  CFGADDR_REG		__BITS(7,0)
#define RA_PCI_CFGDATA		0x0024
#define RA_PCI_MEMBASE		0x0028
#define  MEMBASE_ADDR		__BITS(31,16)
#define RA_PCI_IOBASE		0x002c
#define  IOBASE_ADDR		__BITS(31,16)
#define RA_PCI_PHY0CFG		0x0090
#define  PHY0CFG_SPI_BUSY	__BIT(31)
#define  PHY0CFG_SPI_WR		__BIT(23)
#define  PHY0CFG_SPI_ADDR	__BITS(15,8)
#define  PHY0CFG_SPI_DATA	__BITS(7,0)

/* PCIe0 RC Control Registers - 0x10142000 */
#define RA_PCIE0_BAR0SETUP	0x0010
#define  BARSETUP_BARMSK	__BITS(31,16)
#define  BARSETUP_BARENB	__BIT(0)
#define RA_PCIE0_BAR1SETUP	0x0014
#define RA_PCIE0_IMBASEBAR0	0x0018
#define  IMBASEBAR0		__BITS(31,16)
#define RA_PCIE0_ID		0x0010
#define RA_PCIE0_CLASS		0x0014
#define RA_PCIE0_SUBID		0x0018
#define RA_PCIE0_STATUS		0x0018
#define  PCIE_STATUS_LINK_UP	__BIT(0)

#endif /* _RALINK_REG_H_ */
