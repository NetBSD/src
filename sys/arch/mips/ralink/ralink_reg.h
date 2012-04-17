/*	$NetBSD: ralink_reg.h,v 1.3.2.1 2012/04/17 00:06:40 yamt Exp $	*/
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

#include <mips/cpuregs.h>

#if defined(RT3050)
#define RA_CLOCK_RATE          320000000
#define RA_BUS_FREQ            (RA_CLOCK_RATE / 3)
#define RA_UART_FREQ           RA_BUS_FREQ
#elif defined(RT3052)
#define RA_CLOCK_RATE          384000000
#define RA_BUS_FREQ            (RA_CLOCK_RATE / 3)
#define RA_UART_FREQ           RA_BUS_FREQ
#elif defined(RT3883)
#if 0
#define RA_CLOCK_RATE          480000000
#else
#define RA_CLOCK_RATE          500000000
#endif
#define RA_BUS_FREQ            166000000 /* DDR speed */
#define RA_UART_FREQ           40000000
#else
/* Ralink dev board */
#define RA_CLOCK_RATE          384000000
#define RA_BUS_FREQ            (RA_CLOCK_RATE / 3)
#define RA_UART_FREQ           RA_BUS_FREQ
#endif

#define RA_BAUDRATE            CONSPEED
#define RA_SERIAL_CLKDIV       16

#define RA_SRAM_BASE           0x00000000
#define RA_SRAM_END            0x0FFFFFFF
#define RA_SYSCTL_BASE         0x10000000
#define RA_TIMER_BASE          0x10000100
#define RA_INTCTL_BASE         0x10000200
#define RA_MEMCTL_BASE         0x10000300
#if defined(RT3052) || defined(RT3050)
#define RA_PCM_BASE            0x10000400
#endif
#define RA_UART_BASE           0x10000500
#define RA_PIO_BASE            0x10000600
#if defined(RT3052) || defined(RT3050)
#define RA_GDMA_BASE           0x10000700
#elif defined(RT3883)
#define RA_FLASHCTL_BASE       0x10000700
#endif
#define RA_NANDCTL_BASE        0x10000800
#define RA_I2C_BASE            0x10000900
#define RA_I2S_BASE            0x10000A00
#define RA_SPI_BASE            0x10000B00
#define RA_UART_LITE_BASE      0x10000C00
#if defined(RT3883)
#define RA_PCM_BASE            0x10002000
#define RA_GDMA_BASE           0x10002800
#define RA_CODEC1_BASE         0x10003000
#define RA_CODEC2_BASE         0x10003800
#endif
#define RA_FRAME_ENGINE_BASE   0x10100000
#define RA_ETH_SW_BASE         0x10110000
#define RA_ROM_BASE            0x10118000
#if defined(RT3883)
#define RA_USB_DEVICE_BASE     0x10120000
#define RA_PCI_BASE            0x10140000
#endif
#define RA_11N_MAC_BASE        0x10180000
#define RA_USB_OTG_BASE        0x101C0000
#if defined(RT3883)
#define RA_USB_HOST_BASE       0x101C0000
#endif
#if defined(RT3052) || defined(RT3050)
#define RA_FLASH_BASE          0x1F000000
#define RA_FLASH_END           0x1F7FFFFF
#elif defined(RT3883) 
#define RA_FLASH_BASE          0x1C000000
#define RA_FLASH_END           0x1DFFFFFF
#endif

#define RA_IOREG_VADDR(base, offset)	\
	(volatile uint32_t *)MIPS_PHYS_TO_KSEG1((base) + (offset))

#define FLD_GET(val,pos,mask)      (((val) >> (pos)) & (mask))
#define FLD_SET(val,pos,mask)      (((val) & (mask)) << (pos))

/*
 * System Control Registers
 */
#define RA_SYSCTL_ID0          0x00
#define RA_SYSCTL_ID1          0x04
#define RA_SYSCTL_CFG0         0x10
#define RA_SYSCTL_CFG1         0x14
#define RA_SYSCTL_CLKCFG0      0x2C
#define RA_SYSCTL_CLKCFG1      0x30
#define RA_SYSCTL_RST          0x34
#define RA_SYSCTL_RSTSTAT      0x38
#define RA_SYSCTL_GPIOMODE     0x60

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
#else
#define	SYSCTL_CFG0_BE		__BIT(19)
#define SYSCTL_CFG0_DRAM_SIZE	__BITS(12,14) 
#define	SYSCTL_CFG0_DRAM_2MB	0
#define	SYSCTL_CFG0_DRAM_8MB	1
#define	SYSCTL_CFG0_DRAM_16MB	2
#define	SYSCTL_CFG0_DRAM_32MB	3
#define	SYSCTL_CFG0_DRAM_64MB	4
#define	SYSCTL_CFG0_DRAM_128MB	5
#define	SYSCTL_CFG0_DRAM_256MB	6
#endif

#if defined(RT3883)
/* 3883 doesn't have memo regs, use teststat instead */
#define RA_SYSCTL_MEMO0        0x18
#define RA_SYSCTL_MEMO1        0x1C
#else
#define RA_SYSCTL_MEMO0        0x68
#define RA_SYSCTL_MEMO1        0x6C
#endif

#define  RST_SW        (1 << 23)
#define  RST_OTG       (1 << 22)
#define  RST_FE        (1 << 21)
#define  RST_WLAN      (1 << 20)
#define  RST_UARTL     (1 << 19)
#define  RST_SPI       (1 << 18)
#define  RST_I2S       (1 << 17)
#define  RST_I2C       (1 << 16)
#define  RST_NAND      (1 << 15)
#define  RST_DMA       (1 << 14)
#define  RST_PIO       (1 << 13)
#define  RST_UART      (1 << 12)
#define  RST_PCM       (1 << 11)
#define  RST_MC        (1 << 10)
#define  RST_INTC      (1 << 9)
#define  RST_TIMER     (1 << 8)
#define  RST_SYS       (1 << 0)
#define  GPIOMODE_RGMII  (1 << 9)
#define  GPIOMODE_SDRAM  (1 << 8)
#define  GPIOMODE_MDIO   (1 << 7)
#define  GPIOMODE_JTAG   (1 << 6)
#define  GPIOMODE_UARTL  (1 << 5)
#define  GPIOMODE_UARTF2 (1 << 4)
#define  GPIOMODE_UARTF1 (1 << 3)
#define  GPIOMODE_UARTF0 (1 << 2)
#define  GPIOMODE_UARTF_0_2	\
			 (GPIOMODE_UARTF0|GPIOMODE_UARTF1|GPIOMODE_UARTF2)
#define  GPIOMODE_SPI    (1 << 1)
#define  GPIOMODE_I2C    (1 << 0)

/*
 * Timer Registers
 */
#define RA_TIMER_STAT          0x00
#define RA_TIMER_0_LOAD        0x10
#define RA_TIMER_0_VALUE       0x14
#define RA_TIMER_0_CNTRL       0x18
#define RA_TIMER_1_LOAD        0x20
#define RA_TIMER_1_VALUE       0x24
#define RA_TIMER_1_CNTRL       0x28

#define  TIMER_1_RESET         (1 << 5)
#define  TIMER_0_RESET         (1 << 4)
#define  TIMER_1_INT_STATUS    (1 << 1)
#define  TIMER_0_INT_STATUS    (1 << 0)
#define  TIMER_TEST_EN         (1 << 15)
#define  TIMER_EN              (1 << 7)
#define  TIMER_MODE(x)         (((x) & 0x3) << 4)
#define   TIMER_MODE_FREE       0
#define   TIMER_MODE_PERIODIC   1
#define   TIMER_MODE_TIMEOUT    2
#define   TIMER_MODE_WDOG       3	/* only valid for TIMER_1 */
#define  TIMER_PRESCALE(x)     (((x) & 0xf) << 0)
#define   TIMER_PRESCALE_DIV_1     0
#define   TIMER_PRESCALE_DIV_4     1
#define   TIMER_PRESCALE_DIV_8     2
#define   TIMER_PRESCALE_DIV_16    3
#define   TIMER_PRESCALE_DIV_32    4
#define   TIMER_PRESCALE_DIV_64    5
#define   TIMER_PRESCALE_DIV_128   6
#define   TIMER_PRESCALE_DIV_256   7
#define   TIMER_PRESCALE_DIV_512   8
#define   TIMER_PRESCALE_DIV_1024  9
#define   TIMER_PRESCALE_DIV_2048  10
#define   TIMER_PRESCALE_DIV_4096  11
#define   TIMER_PRESCALE_DIV_8192  12
#define   TIMER_PRESCALE_DIV_16384 13
#define   TIMER_PRESCALE_DIV_32768 14
#define   TIMER_PRESCALE_DIV_65536 15

/*
 * Interrupt Controller Registers
 */
#define RA_INTCTL_IRQ0STAT     0x00
#define RA_INTCTL_IRQ1STAT     0x04
#define RA_INTCTL_TYPE         0x20
#define RA_INTCTL_RAW          0x30
#define RA_INTCTL_ENABLE       0x34
#define RA_INTCTL_DISABLE      0x38


#define INT_GLOBAL    (1 << 31)
#define INT_USB       (1 << 18)
#define INT_ETHSW     (1 << 17)
#define INT_UARTL     (1 << 12)
#define INT_I2S       (1 << 10)
#define INT_PERF      (1 << 9)
#define INT_NAND      (1 << 8)
#define INT_DMA       (1 << 7)
#define INT_PIO       (1 << 6)
#define INT_UARTF     (1 << 5)
#define INT_PCM       (1 << 4)
#define INT_ILLACC    (1 << 3)
#define INT_WDOG      (1 << 2)
#define INT_TIMER0    (1 << 1)
#define INT_SYSCTL    (1 << 0)

/*
 * Ralink Linear CPU Interrupt Mapping For Lists
 */
#define RA_IRQ_LOW        0
#define RA_IRQ_HIGH       1
#define RA_IRQ_PCI        2
#define RA_IRQ_FENGINE    3
#define RA_IRQ_WLAN       4
#define RA_IRQ_TIMER      5
#define RA_IRQ_SYSCTL     6
#define RA_IRQ_TIMER0     7
#define RA_IRQ_WDOG       8
#define RA_IRQ_ILLACC     9
#define RA_IRQ_PCM        10
#define RA_IRQ_UARTF      11
#define RA_IRQ_PIO        12
#define RA_IRQ_DMA        13
#define RA_IRQ_NAND       14
#define RA_IRQ_PERF       15
#define RA_IRQ_I2S        16
#define RA_IRQ_UARTL      17
#define RA_IRQ_ETHSW      18
#define RA_IRQ_USB        19
#define RA_IRQ_MAX        20

/*
 * General Purpose I/O
 */
#define RA_PIO_00_23_INT         0x00
#define RA_PIO_00_23_EDGE_INT    0x04
#define RA_PIO_00_23_INT_RISE_EN 0x08
#define RA_PIO_00_23_INT_FALL_EN 0x0C
#define RA_PIO_00_23_DATA        0x20
#define RA_PIO_00_23_DIR         0x24
#define RA_PIO_00_23_POLARITY    0x28
#define RA_PIO_00_23_SET_BIT     0x2C
#define RA_PIO_00_23_CLR_BIT     0x30
#define RA_PIO_00_23_TGL_BIT     0x34
#define RA_PIO_24_39_INT         0x38
#define RA_PIO_24_39_EDGE_INT    0x3C
#define RA_PIO_24_39_INT_RISE_EN 0x40
#define RA_PIO_24_39_INT_FALL_EN 0x44
#define RA_PIO_24_39_DATA        0x48
#define RA_PIO_24_39_DIR         0x4C
#define RA_PIO_24_39_POLARITY    0x50
#define RA_PIO_24_39_SET_BIT     0x54
#define RA_PIO_24_39_CLR_BIT     0x58
#define RA_PIO_24_39_TGL_BIT     0x5C
#define RA_PIO_40_51_INT         0x60
#define RA_PIO_40_51_EDGE_INT    0x64
#define RA_PIO_40_51_INT_RISE_EN 0x68
#define RA_PIO_40_51_INT_FALL_EN 0x6C
#define RA_PIO_40_51_DATA        0x70
#define RA_PIO_40_51_DIR         0x74
#define RA_PIO_40_51_POLARITY    0x78
#define RA_PIO_40_51_SET_BIT     0x7C
#define RA_PIO_40_51_CLR_BIT     0x80
#define RA_PIO_40_51_TGL_BIT     0x84
#define RA_PIO_72_95_INT         0x88
#define RA_PIO_72_95_EDGE_INT    0x8c
#define RA_PIO_72_95_INT_RISE_EN 0x90
#define RA_PIO_72_95_INT_FALL_EN 0x94
#define RA_PIO_72_95_DATA        0x98
#define RA_PIO_72_95_DIR         0x9c
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


#define UART_IER_ELSI      (1 << 2)
		/* Receiver Line Status Interrupt Enable */
#define UART_IER_ETBEI     (1 << 1)
		/* Transmit Buffer Empty Interrupt Enable */
#define UART_IER_ERBFI     (1 << 0)
		/* Data Ready or Character Time-Out Interrupt Enable */

#define UART_IIR_FIFOES1   (1 << 7)    /* FIFO Mode Enable Status */
#define UART_IIR_FIFOES0   (1 << 6)    /* FIFO Mode Enable Status */
#define UART_IIR_IID3      (1 << 3)    /* Interrupt Source Encoded */
#define UART_IIR_IID2      (1 << 2)    /* Interrupt Source Encoded */
#define UART_IIR_IID1      (1 << 1)    /* Interrupt Source Encoded */
#define UART_IIR_IP        (1 << 0)    /* Interrupt Pending (active low) */

#define UART_FCR_RXTRIG1   (1 << 7)    /* Receiver Interrupt Trigger Level */
#define UART_FCR_RXTRIG0   (1 << 6)    /* Receiver Interrupt Trigger Level */
#define UART_FCR_TXTRIG1   (1 << 5)    /* Transmitter Interrupt Trigger Level */
#define UART_FCR_TXTRIG0   (1 << 4)    /* Transmitter Interrupt Trigger Level */
#define UART_FCR_DMAMODE   (1 << 3)    /* Enable DMA transfers */
#define UART_FCR_TXRST     (1 << 2)    /* Reset Transmitter FIFO */
#define UART_FCR_RXRST     (1 << 1)    /* Reset Receiver FIFO */
#define UART_FCR_FIFOE     (1 << 0)    /* Transmit and Receive FIFO Enable */

#define UART_LCR_DLAB      (1 << 7)    /* Divisor Latch Access Bit */
#define UART_LCR_SB        (1 << 6)    /* Set Break */
#define UART_LCR_STKYP     (1 << 5)    /* Sticky Parity */
#define UART_LCR_EPS       (1 << 4)    /* Even Parity Select */
#define UART_LCR_PEN       (1 << 3)    /* Parity Enable */
#define UART_LCR_STB       (1 << 2)    /* Stop Bit */
#define UART_LCR_WLS1      (1 << 1)    /* Word Length Select */
#define UART_LCR_WLS0      (1 << 0)    /* Word Length Select */

#define UART_MCR_LOOP      (1 << 4)    /* Loop-back Mode Enable */

#define UART_MSR_DCD       (1 << 7)    /* Data Carrier Detect */
#define UART_MSR_RI        (1 << 6)    /* Ring Indicator */
#define UART_MSR_DSR       (1 << 5)    /* Data Set Ready */
#define UART_MSR_CTS       (1 << 4)    /* Clear To Send */
#define UART_MSR_DDCD      (1 << 3)    /* Delta Data Carrier Detect */
#define UART_MSR_TERI      (1 << 2)    /* Trailing Edge Ring Indicator */
#define UART_MSR_DDSR      (1 << 1)    /* Delta Data Set Ready */
#define UART_MSR_DCTS      (1 << 0)    /* Delta Clear To Send */

#define UART_LSR_FIFOE     (1 << 7)    /* FIFO Error Status */
#define UART_LSR_TEMT      (1 << 6)    /* Transmitter Empty */
#define UART_LSR_TDRQ      (1 << 5)    /* Transmit Data Request */
#define UART_LSR_BI        (1 << 4)    /* Break Interrupt */
#define UART_LSR_FE        (1 << 3)    /* Framing Error */
#define UART_LSR_PE        (1 << 2)    /* Parity Error */
#define UART_LSR_OE        (1 << 1)    /* Overrun Error */
#define UART_LSR_DR        (1 << 0)    /* Data Ready */

/*
 * I2C registers
 */
#define RA_I2C_CONFIG          0x00
#define RA_I2C_CLKDIV          0x04
#define RA_I2C_DEVADDR         0x08
#define RA_I2C_ADDR            0x0C
#define RA_I2C_DATAOUT         0x10
#define RA_I2C_DATAIN          0x14
#define RA_I2C_STATUS          0x18
#define RA_I2C_STARTXFR        0x1C
#define RA_I2C_BYTECNT         0x20

#define  I2C_CONFIG_ADDRLEN(x)     (((x) & 0x7) << 5)
#define   I2C_CONFIG_ADDRLEN_7     6
#define   I2C_CONFIG_ADDRLEN_8     7
#define  I2C_CONFIG_DEVADLEN(x)    (((x) & 0x7) << 2)
#define   I2C_CONFIG_DEVADLEN_6    5
#define   I2C_CONFIG_DEVADLEN_7    6
#define  I2C_CONFIG_ADDRDIS        (1 << 1)
#define  I2C_CONFIG_DEVDIS         (1 << 0)
#define  I2C_STATUS_STARTERR       (1 << 4)
#define  I2C_STATUS_ACKERR         (1 << 3)
#define  I2C_STATUS_DATARDY        (1 << 2)
#define  I2C_STATUS_SDOEMPTY       (1 << 1)
#define  I2C_STATUS_BUSY           (1 << 0)

/*
 * SPI registers
 */
#define RA_SPI_STATUS          0x00
#define RA_SPI_CONFIG          0x10
#define RA_SPI_CONTROL         0x14
#define RA_SPI_DATA            0x20

#define  SPI_STATUS_BUSY            (1 << 0)
#define  SPI_CONFIG_MSBFIRST        (1 << 8)
#define  SPI_CONFIG_CLK             (1 << 6)
#define  SPI_CONFIG_RXCLKEDGE_FALL  (1 << 5)
#define  SPI_CONFIG_TXCLKEDGE_FALL  (1 << 4)
#define  SPI_CONFIG_TRISTATE        (1 << 3)
#define  SPI_CONFIG_RATE(x)         ((x) & 0x7)
#define   SPI_CONFIG_RATE_DIV_2     0
#define   SPI_CONFIG_RATE_DIV_4     1
#define   SPI_CONFIG_RATE_DIV_8     2
#define   SPI_CONFIG_RATE_DIV_16    3
#define   SPI_CONFIG_RATE_DIV_32    4
#define   SPI_CONFIG_RATE_DIV_64    5
#define   SPI_CONFIG_RATE_DIV_128   6
#define   SPI_CONFIG_RATE_DIV_NONE  7
#define  SPI_CONTROL_TRISTATE       (1 << 3)
#define  SPI_CONTROL_STARTWR        (1 << 2)
#define  SPI_CONTROL_STARTRD        (1 << 1)
#define  SPI_CONTROL_ENABLE_LOW     (0 << 0)
#define  SPI_CONTROL_ENABLE_HIGH    (1 << 0)
#define  SPI_DATA_VAL(x)            ((x) & 0xff)

/*
 * Frame Engine registers
 */
#define RA_FE_MDIO_ACCESS      0x000
#define RA_FE_MDIO_CFG1        0x004
#define RA_FE_GLOBAL_CFG       0x008
#define RA_FE_GLOBAL_RESET     0x00C
#define RA_FE_INT_STATUS       0x010
#define RA_FE_INT_ENABLE       0x014
#define RA_FE_MDIO_CFG2        0x018
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

#define  MDIO_ACCESS_TRG         (1 << 31)
#define  MDIO_ACCESS_WR          (1 << 30)
#define  MDIO_ACCESS_PHY_ADDR(x) (((x) & 0x1f) << 24)
#define  MDIO_ACCESS_REG(x)      (((x) & 0x1f) << 16)
#define  MDIO_ACCESS_DATA(x)     ((x) & 0xffff)
#define  MDIO_CFG_AUTO_POLL      (1 << 29)
#define  MDIO_CFG_PHY_ADDR(x)    (((x) & 0x1f) << 24)
#define  MDIO_CFG_BP_EN          (1 << 16)
#define  MDIO_CFG_FORCE_CFG      (1 << 15)
#define  MDIO_CFG_SPEED(x)       (((x) & 0x3) << 13)
#define   MDIO_CFG_SPEED_1000M   2
#define   MDIO_CFG_SPEED_100M    1
#define   MDIO_CFG_SPEED_10M     0
#define  MDIO_CFG_FULL_DUPLEX    (1 << 12)
#define  MDIO_CFG_FC_TX          (1 << 11)
#define  MDIO_CFG_FC_RX          (1 << 10)
#define  MDIO_CFG_LINK_DOWN      (1 << 9)
#define  MDIO_CFG_AUTO_DONE      (1 << 8)
#define  MDIO_CFG_MDC_CLKDIV(x)  (((x) & 0x3) << 6)
#define   MDIO_CFG_MDC_512KHZ    3
#define   MDIO_CFG_MDC_1MHZ      2
#define   MDIO_CFG_MDC_2MHZ      1
#define   MDIO_CFG_MDC_4MHZ      0
#define  MDIO_CFG_TURBO_50MHZ   (1 << 5)
#define  MDIO_CFG_TURBO_EN      (1 << 4)
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
#define  FE_GLOBAL_RESET_PSE       (1 << 0)
#define  FE_INT_PPE_COUNT_HIGH     (1 << 31)
#define  FE_INT_DMA_COUNT_HIGH     (1 << 29)
#define  FE_INT_PSE_P2_FC_ASSERT   (1 << 26)
#define  FE_INT_PSE_FC_DROP        (1 << 24)
#define  FE_INT_GDMA_DROP_OTHER    (1 << 23)
#define  FE_INT_PSE_P1_FC_ASSERT   (1 << 22)
#define  FE_INT_PSE_P0_FC_ASSERT   (1 << 21)
#define  FE_INT_PSE_FQ_EMPTY       (1 << 20)
#define  FE_INT_TX_COHERENT        (1 << 17)
#define  FE_INT_RX_COHERENT        (1 << 16)
#define  FE_INT_TX3                (1 << 11)
#define  FE_INT_TX2                (1 << 10)
#define  FE_INT_TX1                (1 << 9)
#define  FE_INT_TX0                (1 << 8)
#define  FE_INT_RX                 (1 << 2)
#define  FE_INT_TX_DELAY           (1 << 1)
#define  FE_INT_RX_DELAY           (1 << 0)
#define  FE_GDMA_FWD_CFG_JUMBO_LEN(x)  (((x) & 0xf) << 28)
#define  FE_GDMA_FWD_CFG_DROP_256B     (1 << 23)
#define  FE_GDMA_FWD_CFG_IP4_CRC_EN    (1 << 22)
#define  FE_GDMA_FWD_CFG_TCP_CRC_EN    (1 << 21)
#define  FE_GDMA_FWD_CFG_UDP_CRC_EN    (1 << 20)
#define  FE_GDMA_FWD_CFG_JUMBO_EN      (1 << 19)
#define  FE_GDMA_FWD_CFG_DIS_TX_PAD    (1 << 18)
#define  FE_GDMA_FWD_CFG_DIS_TX_CRC    (1 << 17)
#define  FE_GDMA_FWD_CFG_STRIP_RX_CRC  (1 << 16)
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
#define  FE_CDMA_CSG_CFG_IP4_CRC_EN  (1 << 2)
#define  FE_CDMA_CSG_CFG_UDP_CRC_EN  (1 << 1)
#define  FE_CDMA_CSG_CFG_TCP_CRC_EN  (1 << 0)
#define  FE_PDMA_GLOBAL_CFG_HDR_SEG_LEN  (1 << 16)
#define  FE_PDMA_GLOBAL_CFG_TX_WB_DDONE  (1 << 6)
#define  FE_PDMA_GLOBAL_CFG_BURST_SZ(x)  (((x) & 0x3) << 4)
#define   FE_PDMA_GLOBAL_CFG_BURST_SZ_4  (0 << 4)
#define   FE_PDMA_GLOBAL_CFG_BURST_SZ_8  (1 << 4)
#define   FE_PDMA_GLOBAL_CFG_BURST_SZ_16 (2 << 4)
#define  FE_PDMA_GLOBAL_CFG_RX_DMA_BUSY  (1 << 3)
#define  FE_PDMA_GLOBAL_CFG_RX_DMA_EN    (1 << 2)
#define  FE_PDMA_GLOBAL_CFG_TX_DMA_BUSY  (1 << 1)
#define  FE_PDMA_GLOBAL_CFG_TX_DMA_EN    (1 << 0)
#define  PDMA_RST_RX0          (1 << 16)
#define  PDMA_RST_TX3          (1 << 3)
#define  PDMA_RST_TX2          (1 << 2)
#define  PDMA_RST_TX1          (1 << 1)
#define  PDMA_RST_TX0          (1 << 0)

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

#define  ISR_WDOG1_EXPIRED    (1 << 29)
#define  ISR_WDOG0_EXPIRED    (1 << 28)
#define  ISR_HAS_INTRUDER     (1 << 27)
#define  ISR_PORT_STS_CHNG    (1 << 26)
#define  ISR_BRDCAST_STORM    (1 << 25)
#define  ISR_MUST_DROP_LAN    (1 << 24)
#define  ISR_GLOB_QUE_FULL    (1 << 23)
#define  ISR_LAN_QUE6_FULL    (1 << 20)
#define  ISR_LAN_QUE5_FULL    (1 << 19)
#define  ISR_LAN_QUE4_FULL    (1 << 18)
#define  ISR_LAN_QUE3_FULL    (1 << 17)
#define  ISR_LAN_QUE2_FULL    (1 << 16)
#define  ISR_LAN_QUE1_FULL    (1 << 15)
#define  ISR_LAN_QUE0_FULL    (1 << 14)
#define  FTC0_REL_THR          24
#define  FTC0_SET_THR          16
#define  FTC0_DROP_REL_THR     8
#define  FTC0_DROP_SET_THR     0
#define  FTC1_PER_PORT_THR     0
#define  PCTL0_WR_VAL(x) (((x) & 0xffff) << 16)
#define  PCTL0_RD_CMD    (1 << 14)
#define  PCTL0_WR_CMD    (1 << 13)
#define  PCTL0_REG(x)    (((x) & 0x1f) << 8)
#define  PCTL0_ADDR(x)   (((x) & 0x1f) << 0)
#define  PCTL1_RD_VAL(x) (((x) >> 16) & 0xffff)
#define  PCTL1_RD_DONE   (1 << 1)	/* read clear */
#define  PCTL1_WR_DONE   (1 << 0)	/* read clear */
#define  SGC2_WL_FC_EN       (1 << 30)
#define  SGC2_PORT5_IS_LAN   (1 << 29)
#define  SGC2_PORT4_IS_LAN   (1 << 28)
#define  SGC2_PORT3_IS_LAN   (1 << 27)
#define  SGC2_PORT2_IS_LAN   (1 << 26)
#define  SGC2_PORT1_IS_LAN   (1 << 25)
#define  SGC2_PORT0_IS_LAN   (1 << 24)
#define  SGC2_TX_CPU_TPID(x) ((x) << 16)
#define  SGC2_ARBITER_LAN_EN (1 << 11)
#define  SGC2_CPU_TPID_EN    (1 << 10)
#define  SGC2_DBL_TAG_EN5    (1 << 5)
#define  SGC2_DBL_TAG_EN4    (1 << 4)
#define  SGC2_DBL_TAG_EN3    (1 << 3)
#define  SGC2_DBL_TAG_EN2    (1 << 2)
#define  SGC2_DBL_TAG_EN1    (1 << 1)
#define  SGC2_DBL_TAG_EN0    (1 << 0)


#define FTC_THR_MSK           0xff

#define PFC0_MTCC_LIMIT       24
#define PFC0_TURN_OFF_CF      16
#define PFC0_TURN_OFF_CF_MSK  0xff
#define PFC0_VO_NUM           12
#define PFC0_CL_NUM           8
#define PFC0_BE_NUM           4
#define PFC0_BK_NUM           0
#define PFC0_NUM_MSK          0xf

#define PFC1_P6_Q1_EN        (1 << 31)
#define PFC1_P6_TOS_EN       (1 << 30)
#define PFC1_P5_TOS_EN       (1 << 29)
#define PFC1_P4_TOS_EN       (1 << 28)
#define PFC1_P3_TOS_EN       (1 << 27)

#define PFC1_P1_TOS_EN       (1 << 25)
#define PFC1_P0_TOS_EN       (1 << 24)
#define PFC1_PORT_PRI6        12
#define PFC1_PORT_PRI5        10
#define PFC1_PORT_PRI4        8
#define PFC1_PORT_PRI3        6
#define PFC1_PORT_PRI2        4
#define PFC1_PORT_PRI1        2
#define PFC1_PORT_PRI0        0
#define PFC1_PORT_MSK         0x3

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
#define RA_USB_OTG_OTG_INT         0x004
#define RA_USB_OTG_AHB_CFG         0x008
#define RA_USB_OTG_CFG             0x00C
#define RA_USB_OTG_RESET           0x010
#define RA_USB_OTG_INT             0x014
#define RA_USB_OTG_INT_MASK        0x018
#define RA_USB_OTG_RX_STAT         0x01C
#define RA_USB_OTG_RX_POP_STAT     0x020
#define RA_USB_OTG_RX_FIFO_SZ      0x024
#define RA_USB_OTG_TX_FIFO_SZ      0x028
#define RA_USB_OTG_TX_FIFO_STAT    0x02C
#define RA_USB_OTG_I2C_ACCESS      0x030
#define RA_USB_OTG_PHY_CTL         0x034
#define RA_USB_OTG_GPIO            0x038
#define RA_USB_OTG_GUID            0x03C
#define RA_USB_OTG_SNPSID          0x040
#define RA_USB_OTG_HWCFG1          0x044
#define RA_USB_OTG_HWCFG2          0x048
#define RA_USB_OTG_HWCFG3          0x04C
#define RA_USB_OTG_HWCFG4          0x050
#define RA_USB_OTG_HC_TX_FIFO_SZ   0x100
#define RA_USB_OTG_DV_TX_FIFO_SZ   0x104
#define RA_USB_OTG_HC_CFG          0x400
#define RA_USB_OTG_HC_FRM_INTRVL   0x404
#define RA_USB_OTG_HC_FRM_NUM      0x408
#define RA_USB_OTG_HC_TX_STAT      0x410
#define RA_USB_OTG_HC_INT          0x414
#define RA_USB_OTG_HC_INT_MASK     0x418
#define RA_USB_OTG_HC_PORT         0x440
#define RA_USB_OTG_HC_CH_CFG       0x500
#define RA_USB_OTG_HC_CH_SPLT      0x504
#define RA_USB_OTG_HC_CH_INT       0x508
#define RA_USB_OTG_HC_CH_INT_MASK  0x50C
#define RA_USB_OTG_HC_CH_XFER      0x510
#define RA_USB_OTG_HC_CH_DMA_ADDR  0x514
#define RA_USB_OTG_DV_CFG          0x800
#define RA_USB_OTG_DV_CTL          0x804
#define RA_USB_OTG_DV_STAT         0x808
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

#define  OTG_OTG_CNTRL_B_SESS_VALID      (1 << 19)
#define  OTG_OTG_CNTRL_A_SESS_VALID      (1 << 18)
#define  OTG_OTG_CNTRL_DEBOUNCE_SHORT    (1 << 17)
#define  OTG_OTG_CNTRL_CONNID_STATUS     (1 << 16)
#define  OTG_OTG_CNTRL_DV_HNP_EN         (1 << 11)
#define  OTG_OTG_CNTRL_HC_SET_HNP_EN     (1 << 10)
#define  OTG_OTG_CNTRL_HNP_REQ           (1 << 9)
#define  OTG_OTG_CNTRL_HNP_SUCCESS       (1 << 8)
#define  OTG_OTG_CNTRL_SESS_REQ          (1 << 1)
#define  OTG_OTG_CNTRL_SESS_REQ_SUCCESS  (1 << 0)
#define  OTG_OTG_INT_DEBOUNCE_DONE       (1 << 19)
#define  OTG_OTG_INT_ADEV_TIMEOUT        (1 << 18)
#define  OTG_OTG_INT_HOST_NEG_DETECT     (1 << 17)
#define  OTG_OTG_INT_HOST_NEG_STATUS     (1 << 9)
#define  OTG_OTG_INT_SESSION_REQ_STATUS  (1 << 8)
#define  OTG_OTG_INT_SESSION_END_STATUS  (1 << 2)
#define  OTG_AHB_CFG_TX_PFIFO_EMPTY_INT_EN  (1 << 8)
#define  OTG_AHB_CFG_TX_NPFIFO_EMPTY_INT_EN (1 << 7)
#define  OTG_AHB_CFG_DMA_EN                 (1 << 5)
#define  OTG_AHB_CFG_BURST(x)               (((x) & 0xf) << 1)
#define   OTG_AHB_CFG_BURST_SINGLE	     0
#define   OTG_AHB_CFG_BURST_INCR  	     1
#define   OTG_AHB_CFG_BURST_INCR4 	     3
#define   OTG_AHB_CFG_BURST_INCR8 	     5
#define   OTG_AHB_CFG_BURST_INCR16	     7
#define  OTG_AHB_CFG_GLOBAL_INT_EN          (1 << 0)
#define  OTG_CFG_CORRUPT_TX              (1 << 31)
#define  OTG_CFG_FORCE_DEVICE            (1 << 30)
#define  OTG_CFG_FORCE_HOST              (1 << 29)
#define  OTG_CFG_ULPI_EXT_VBUS_IND_SEL   (1 << 22)
#define  OTG_CFG_ULPI_EXT_VBUS_IND       (1 << 21)
#define  OTG_CFG_ULPI_EXT_VBUS_DRV       (1 << 20)
#define  OTG_CFG_ULPI_CLOCK_SUSPEND      (1 << 19)
#define  OTG_CFG_ULPI_AUTO_RESUME        (1 << 18)
#define  OTG_CFG_ULPI_FS_LS_SEL          (1 << 17)
#define  OTG_CFG_UTMI_I2C_SEL            (1 << 16)
#define  OTG_CFG_TURNAROUND_TIME(x)      (((x) & 0xf) << 10)
#define  OTG_CFG_HNP_CAP                 (1 << 9)
#define  OTG_CFG_SRP_CAP                 (1 << 8)
#define  OTG_CFG_ULPI_DDR_SEL            (1 << 7)
#define  OTG_CFG_HS_PHY_SEL              (1 << 6)
#define  OTG_CFG_FS_IF_SEL               (1 << 5)
#define  OTG_CFG_ULPI_UTMI_SEL           (1 << 4)
#define  OTG_CFG_PHY_IF                  (1 << 3)
#define  OTG_CFG_TIMEOUT(x)              (((x) & 0x7) << 0)
#define  OTG_RST_AHB_IDLE                (1 << 31)
#define  OTG_RST_DMA_ACTIVE              (1 << 30)
#define  OTG_RST_TXQ_TO_FLUSH(x)         (((x) & 0x1f) << 6)
#define   OTG_RST_TXQ_FLUSH_ALL          0x10
#define  OTG_RST_TXQ_FLUSH               (1 << 5)
#define  OTG_RST_RXQ_FLUSH               (1 << 4)
#define  OTG_RST_INQ_FLUSH               (1 << 3)
#define  OTG_RST_HC_FRAME                (1 << 2)
#define  OTG_RST_AHB                     (1 << 1)
#define  OTG_RST_CORE                    (1 << 0)
#define  OTG_INT_RESUME                  (1 << 31)
#define  OTG_INT_SESSION_REQ             (1 << 30)
#define  OTG_INT_DISCONNECT              (1 << 29)
#define  OTG_INT_CONNID_STATUS           (1 << 28)
#define  OTG_INT_PTX_EMPTY               (1 << 26)
#define  OTG_INT_HOST_CHANNEL            (1 << 25)
#define  OTG_INT_PORT_STATUS             (1 << 24)
#define  OTG_INT_DMA_FETCH_SUSPEND       (1 << 22)
#define  OTG_INT_INCOMPLETE_PERIODIC     (1 << 21)
#define  OTG_INT_INCOMPLETE_ISOC         (1 << 20)
#define  OTG_INT_DV_OUT_EP               (1 << 19)
#define  OTG_INT_DV_IN_EP                (1 << 18)
#define  OTG_INT_DV_EP_MISMATCH          (1 << 17)
#define  OTG_INT_DV_PERIODIC_END         (1 << 15)
#define  OTG_INT_DV_ISOC_OUT_DROP        (1 << 14)
#define  OTG_INT_DV_ENUM_COMPLETE        (1 << 13)
#define  OTG_INT_DV_USB_RESET            (1 << 12)
#define  OTG_INT_DV_USB_SUSPEND          (1 << 11)
#define  OTG_INT_DV_USB_EARLY_SUSPEND    (1 << 10)
#define  OTG_INT_I2C                     (1 << 9)
#define  OTG_INT_ULPI_CARKIT             (1 << 8)
#define  OTG_INT_DV_OUT_NAK_EFFECTIVE    (1 << 7)
#define  OTG_INT_DV_IN_NAK_EFFECTIVE     (1 << 6)
#define  OTG_INT_NPTX_EMPTY              (1 << 5)
#define  OTG_INT_RX_FIFO                 (1 << 4)
#define  OTG_INT_SOF                     (1 << 3)
#define  OTG_INT_OTG                     (1 << 2)
#define  OTG_INT_MODE_MISMATCH           (1 << 1)
#define  OTG_INT_MODE                    (1 << 0)
#define  USB_OTG_SNPSID_CORE_REV_2_00  0x4F542000
#define  OTG_HC_CFG_FORCE_NO_HS          (1 << 2)
#define  OTG_HC_CFG_FSLS_CLK_SEL(x)      (((x) & 0x3) << 0)
#define   OTG_HC_CFG_FS_CLK_3060         0
#define   OTG_HC_CFG_FS_CLK_48           1
#define   OTG_HC_CFG_LS_CLK_3060         0
#define   OTG_HC_CFG_LS_CLK_48           1
#define   OTG_HC_CFG_LS_CLK_6            2
#define  USB_OTG_HC_FRM_NUM(x)            (x & 0x3fff)
#define  USB_OTG_HC_FRM_REM(x)            (x >> 16)
#define  USB_OTG_HC_PORT_SPEED(x)         (((x) >> 17) & 0x3)
#define   USB_OTG_HC_PORT_SPEED_HS        0
#define   USB_OTG_HC_PORT_SPEED_FS        1
#define   USB_OTG_HC_PORT_SPEED_LS        2
#define  USB_OTG_HC_PORT_TEST(x)          (((x) & 0xf) << 13)
#define   USB_OTG_HC_PORT_TEST_DISABLED   0
#define   USB_OTG_HC_PORT_TEST_J_MODE     1
#define   USB_OTG_HC_PORT_TEST_K_MODE     2
#define   USB_OTG_HC_PORT_TEST_NAK_MODE   3
#define   USB_OTG_HC_PORT_TEST_PKT_MODE   4
#define   USB_OTG_HC_PORT_TEST_FORCE_MODE 5
#define  USB_OTG_HC_PORT_POWER            (1 << 12)
#define  USB_OTG_HC_PORT_LINE_STAT        (((x) >> 10) & 0x3)
#define   USB_OTG_HC_PORT_LINE_STAT_DP    1
#define   USB_OTG_HC_PORT_LINE_STAT_DM    3
#define  USB_OTG_HC_PORT_RESET            (1 << 8)
#define  USB_OTG_HC_PORT_SUSPEND          (1 << 7)
#define  USB_OTG_HC_PORT_RESUME           (1 << 6)
#define  USB_OTG_HC_PORT_OVCURR_CHANGE    (1 << 5)
#define  USB_OTG_HC_PORT_OVCURR           (1 << 4)
#define  USB_OTG_HC_PORT_ENABLE_CHANGE    (1 << 3)
#define  USB_OTG_HC_PORT_ENABLE           (1 << 2)
#define  USB_OTG_HC_PORT_CONNECT_CHANGE   (1 << 1)
#define  USB_OTG_HC_PORT_STATUS           (1 << 0)
#define  USB_OTG_HC_CH_CFG_ENABLE         (1 << 31)
#define  USB_OTG_HC_CH_CFG_DISABLE        (1 << 30)
#define  USB_OTG_HC_CH_CFG_ODD_FRAME      (1 << 29)
#define  USB_OTG_HC_CH_CFG_DEV_ADDR(x)    (((x) & 0x7f) << 22)
#define  USB_OTG_HC_CH_CFG_MULTI_CNT(x)   (((x) & 0x3) << 20)
#define  USB_OTG_HC_CH_CFG_EP_TYPE(x)     (((x) & 0x3) << 18)
#define   USB_OTG_HC_CH_CFG_EP_TYPE_CTRL   0
#define   USB_OTG_HC_CH_CFG_EP_TYPE_ISOC   1
#define   USB_OTG_HC_CH_CFG_EP_TYPE_BULK   2
#define   USB_OTG_HC_CH_CFG_EP_TYPE_INTR   3
#define  USB_OTG_HC_CH_CFG_LS             (1 << 17)
#define  USB_OTG_HC_CH_CFG_EP_DIR(x)      (((x) & 0x1) << 15)
#define   USB_OTG_HC_CH_CFG_EP_DIR_OUT     0
#define   USB_OTG_HC_CH_CFG_EP_DIR_IN      1
#define  USB_OTG_HC_CH_CFG_EP_NUM(x)      (((x) & 0xf) << 11)
#define  USB_OTG_HC_CH_CFG_MAX_PKT_SZ(x)  (((x) & 0x7ff) << 0)
#define  USB_OTG_HC_CH_SPLT_EN           (1 << 31)
#define  USB_OTG_HC_CH_SPLT_COMPLETE     (1 << 16)
#define  USB_OTG_HC_CH_SPLT_POS(x)       (((x) & 0x3) << 14)
#define   USB_OTG_HC_CH_SPLT_POS_MID      0
#define   USB_OTG_HC_CH_SPLT_POS_END      1
#define   USB_OTG_HC_CH_SPLT_POS_BEGIN    2
#define   USB_OTG_HC_CH_SPLT_POS_ALL      3
#define  USB_OTG_HC_CH_SPLT_HUB_ADDR(x)  (((x) & 0x7f) << 7)
#define  USB_OTG_HC_CH_SPLT_PORT_ADDR(x) (((x) & 0x7f) << 0)
#define  USB_OTG_HC_CH_INT_ALL            0x7ff
#define  USB_OTG_HC_CH_INT_TOGGLE_ERROR   (1 << 10)
#define  USB_OTG_HC_CH_INT_FRAME_OVERRUN  (1 << 9)
#define  USB_OTG_HC_CH_INT_BABBLE_ERROR   (1 << 8)
#define  USB_OTG_HC_CH_INT_XACT_ERROR     (1 << 7)
#define  USB_OTG_HC_CH_INT_NYET           (1 << 6)
#define  USB_OTG_HC_CH_INT_ACK            (1 << 5)
#define  USB_OTG_HC_CH_INT_NAK            (1 << 4)
#define  USB_OTG_HC_CH_INT_STALL          (1 << 3)
#define  USB_OTG_HC_CH_INT_DMA_ERROR      (1 << 2)
#define  USB_OTG_HC_CH_INT_HALTED         (1 << 1)
#define  USB_OTG_HC_CH_INT_XFER_COMPLETE  (1 << 0)
#define  USB_OTG_HC_CH_XFER_DO_PING      (1 << 31)
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

#endif /* _RALINK_REG_H_ */
