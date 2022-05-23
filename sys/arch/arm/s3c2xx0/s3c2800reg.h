/* $NetBSD: s3c2800reg.h,v 1.8 2022/05/23 21:46:11 andvar Exp $ */

/*
 * Copyright (c) 2002, 2003 Fujitsu Component Limited
 * Copyright (c) 2002, 2003, 2005 Genetec Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The Fujitsu Component Limited nor the name of
 *    Genetec corporation may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY FUJITSU COMPONENT LIMITED AND GENETEC
 * CORPORATION ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL FUJITSU COMPONENT LIMITED OR GENETEC
 * CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Samsung S3C2800 processor is ARM920T based integrated CPU
 *
 * Reference:
 *  S3C2800 User's Manual 
 */
#ifndef _ARM_S3C2XX0_S3C2800REG_H_
#define	_ARM_S3C2XX0_S3C2800REG_H_

/* common definitions for S3C2800, S3C2400 and S3C2410 */
#include <arm/s3c2xx0/s3c2xx0reg.h>

/*
 * Memory Map
 */

/* ROM/SRAM/FLASH */
#define	S3C2800_SBANK0_START 0x00000000
#define	S3C2800_SBANK1_START 0x02000000
#define	S3C2800_SBANK2_START 0x04000000
#define	S3C2800_SBANK3_START 0x06000000
#define	S3C2800_SBANK4_START 0x08000000

/* DRAM */
#define	S3C2800_DBANK0_START 0x08000000
#define	S3C2800_DBANK1_START 0x0a000000
#define	S3C2800_DBANK2_START 0x0c000000
#define	S3C2800_DBANK3_START 0x0e000000
#define	S3C2800_DBANK_SIZE   0x02000000	/* 32MB */

/*
 * Physical address of integrated peripherals
 */
#define	S3C2800_PERIPHERALS	0x10000000
#define	S3C2800_PERIPHERALS_SIZE  0x200000 /* 2MBytes */
#define	S3C2800_CLKMAN_BASE	0x10000000 /* clock & power management */
#define	S3C2800_CLKMAN_SIZE	0x18
#define	S3C2800_MEMCTL_BASE	0x10010000 /* memory controller */
#define	S3C2800_MEMCTL_SIZE	0x20
#define	S3C2800_DMA0_BASE	0x10030000
#define	S3C2800_DMA1_BASE	0x10040000
#define	S3C2800_DMA2_BASE	0x10050000
#define	S3C2800_DMA3_BASE	0x10060000
#define	S3C2800_PCICTL_BASE	0x10080000
#define	S3C2800_PCICTL_SIZE	0x58
#define	S3C2800_GPIO_BASE	0x10100000
#define	S3C2800_GPIO_SIZE	0x50
#define	S3C2800_TIMER0_BASE	0x10130000
#define	S3C2800_TIMER1_BASE	0x10140000
#define	S3C2800_TIMER2_BASE	0x10150000
#define	S3C2800_TIMER_SIZE	0x10
#define	S3C2800_UART0_BASE	0x10170000
#define	S3C2800_UART1_BASE	0x10180000
#define	S3C2800_UART_SIZE	0x2c
#define	S3C2800_IIC0_BASE	0x10190000
#define	S3C2800_IIC1_BASE	0x101a0000
#define	S3C2800_IIC_SIZE	0x10
#define	S3C2800_INTCTL_BASE	0x10020000
#define	S3C2800_INTCTL_SIZE	0x14
#define	S3C2800_WDT_BASE	0x10120000
#define	S3C2800_WDT_SIZE	0x0c


/* width of interrupt controller */
#define	ICU_LEN			29
#define	ICU_INT_HWMASK		0x1fffffff

/* Clock & power manager */
#define	CLKMAN_PLLCON	0x00

/* MDIV, PDIV, SDIV */
#define	 PLLCON_MDIV_SHIFT	12
#define	 PLLCON_MDIV_MASK	(0xff<<PLLCON_MDIV_SHIFT)
#define	 PLLCON_PDIV_SHIFT	4
#define	 PLLCON_PDIV_MASK	(0x3f<<PLLCON_PDIV_SHIFT)
#define	 PLLCON_SDIV_SHIFT	0
#define	 PLLCON_SDIV_MASK	(0x03<<PLLCON_SDIV_SHIFT)

#define	CLKMAN_CLKCON	0x04
#define	 CLKCON_PCLK	(1<<12)	/* APB clock division ratio */
#define	 CLKCON_HCLK	(1<<11)	/* AHB clock division ratio */
#define	 CLKCON_PCI	(1<<10)	/* FCLK into PCI */
#define	 CLKCON_IIC1	(1<<9)	/* PCLK to I2C1 */
#define	 CLKCON_IIC0	(1<<8)	/* PCLK to I2C0 */
#define	 CLKCON_RTC	(1<<7)	/* PCLK to RTC */
#define	 CLKCON_UART1	(1<<6)	/* PCLK to UART1 */
#define	 CLKCON_UART0	(1<<5)	/* PCLK to UART0 */
#define	 CLKCON_DMA23	(1<<4)	/* PCLK to DMA2,3 */
#define	 CLKCON_DMA01	(1<<3)	/* PCLK to DMA0,1 */
#define	 CLKCON_TIMER	(1<<2)	/* PCLK to TIMER */
#define	 CLKCON_IDLE	(1<<1)	/* Enter IDLE Mode */

#define	CLKMAN_CLKSLOW	0x08	/* slow clock control */
#define	 CLKSLOW_SLOW	(1<<4)	/* 1: Enable SLOW mode */
#define	 CLKSLOW_VAL_MASK  0x0f	/* divider value for slow clock */

#define	CLKMAN_LOCKTIME 0x0c	/* PLL lock time */

#define	CLKMAN_SWRCON	0x10	/* Software reset control */
#define	 SWRCON_SWR	(1<<0)	/* 1: Invoke software reset */

#define	CLKMAN_RSTSR	0x14	/* Reset status register */
#define	 RSTSR_WDR	(1<<2)	/* watchdog reset */
#define	 RSTSR_SWR	(1<<1)	/* Software reset */
#define	 RSTSR_HWR	(1<<0)	/* Hardware reset */

/* memory controller */
#define	MEMCTL_ENDIAN	0x00	/* Endian control (read-only) */
#define	MEMCTL_SMBCON0	0x04	/* SBank0 control register for static */
#define	MEMCTL_SMBCON1	0x08	/* SBank0 control register for static */
#define	MEMCTL_SMBCON2	0x0c	/* SBank0 control register for static */
#define	MEMCTL_SMBCON3	0x10	/* SBank0 control register for static */
#define	 SMBCON_WS	(1<<20)	/* Wait status */
#define	 SMBCON_ST	(1<<19)	/* use UB/LB */
#define	 SMBCON_TACS_SHIFT  12	/* address setup time before nSCSn */
#define	 SMBCON_TACS	(3<<SMBCON_TACS_SHIFT)
#define	 SMBCON_TCOS_SHIFT  10	/* CS setup time before nOE */
#define	 SMBCON_TCOS	(3<<SMBCON_TCOS_SHIFT)
#define	 SMBCON_TOCH_SHIFT   8	/* CS hold time after nOE */
#define	 SMBCON_TOCH	(3<<SMBCON_TOCH_SHIFT)
#define	 SMBCON_TACC_SHIFT   4	/* Access cycle */
#define	 SMBCON_TACC	     (0x0f<<SMBCON_TOCH_SHIFT)
#define	 SMBCON_TACC_1CLK    (0x00<<SMBCON_TOCH_SHIFT)
#define	 SMBCON_TACC_2CLK    (0x01<<SMBCON_TOCH_SHIFT)
#define	 SMBCON_TACC_3CLK    (0x02<<SMBCON_TOCH_SHIFT)
#define	 SMBCON_TACC_4CLK    (0x03<<SMBCON_TOCH_SHIFT)
#define	 SMBCON_TACC_6CLK    (0x04<<SMBCON_TOCH_SHIFT)
#define	 SMBCON_TACC_7CLK    (0x05<<SMBCON_TOCH_SHIFT)
#define	 SMBCON_TACC_8CLK    (0x06<<SMBCON_TOCH_SHIFT)
#define	 SMBCON_TACC_9CLK    (0x07<<SMBCON_TOCH_SHIFT)
#define	 SMBCON_TACC_10CLK   (0x08<<SMBCON_TOCH_SHIFT)
#define	 SMBCON_TACC_12CLK   (0x09<<SMBCON_TOCH_SHIFT)
#define	 SMBCON_TACC_14CLK   (0x0a<<SMBCON_TOCH_SHIFT)
#define	 SMBCON_TCAH_SHIFT   2	/* Address hold time after nSCSn */
#define	 SMBCON_SDW	  0x03
#define	 SMBCON_SDW_8BIT     0
#define	 SMBCON_SDW_16BIT    1
#define	 SMBCON_SDW_32BIT    2
#define	MEMCTL_REFRESH	0x14	/* DRAM/SDRAM refresh control */
#define	 REFRESH_REFEN  (1<<23)
#define	 REFRESH_REFMD	(1<<22)
#define	 REFRESH_TRP	(3<<20)
#define	 REFRESH_TRC	(3<<16)
#define	 REFRESH_TCHR	(3<<12)	/* CAS hold time */
#define	 REFRESH_COUNTER 0x7ff
#define	MEMCTL_DMTMCON  0x18	/* timing control for dynamic memory */
#define	 DMTMCON_DW_SHIFT   16
#define	 DMTMCON_DW	(3<<DMTMCON_DW_SHIFT)	/* bus width */
#define	 DMTMCON_MT	(3<<10)	/* dynamic memory type */
#define	 DMTMCON_TRCD	(3<<8)	/* DRAM RAS to CAS delay */
#define	 DMTMCON_TCAS	(3<<6)	/* CAS pulse width */
#define	 DMTMCON_TCP	(3<<4)	/* CAS pre charge */
#define	 DMTMCON_CAN	(3<<2)	/* column address number */
#define	 DMTMCON_STRCD	(3<<0)	/* SDRAM RAS to CAS delay */
#define	MEMCTL_MRSR	0x1c	/* SDRAM mode register */
#define	 MRSR_CL_SHIFT  4
#define	 MRSR_CL	(7<<MRSR_CL_SHIFT)	/* CAS latency */

/* GPIO */
#define	GPIO_PCONA	0x00	/* port A configuration */
#define	 PCON_INPUT	0	/* Input port */
#define	 PCON_OUTPUT	1	/* Output port */
#define	 PCON_ALTFUN	2	/* Alternate function */
#define	GPIO_PDATA	0x04	/* port A data */
#define	GPIO_PUPA	0x08	/* port A pull-up */
#define	GPIO_PCONB	0x0c
#define	GPIO_PDATB	0x10
#define	GPIO_PCONC	0x18
#define	GPIO_PDATC	0x1c
#define	GPIO_PUPC	0x20
#define	GPIO_PCOND	0x24
#define	GPIO_PDATD	0x28
#define	GPIO_PUPD	0x2c
#define	GPIO_PCONE	0x30
#define	GPIO_PDATE	0x34
#define	GPIO_PUPE	0x38
#define	GPIO_PCONF	0x3c
#define	GPIO_PDATF	0x40
#define	GPIO_PUPF	0x44

#define	GPIO_EXTINTR	0x48	/* external interrupt control */
#define	 EXTINTR_LOW	 0x00
#define	 EXTINTR_HIGH	 0x01
#define	 EXTINTR_FALLING 0x02
#define	 EXTINTR_RISING  0x04
#define	 EXTINTR_BOTH    0x06

#define	GPIO_SPUCR	0x4c  	/* special pull-up control */
#define	 SPUCR1	(1<<1)		/* pull-up for data[31:16] */
#define	 SPUCR0	(1<<0)		/* pull-up for data[15:0] */

/* Timer */
#define	TIMER_TMCON	0x00	/* control register */
#define	 TMCON_MUX	(3<<2)	/* MUX input */
#define	 TMCON_MUX_DIV4  (0<<2)
#define	 TMCON_MUX_DIV8  (1<<2)
#define	 TMCON_MUX_DIV16 (2<<2)
#define	 TMCON_MUX_DIV32 (3<<2)
#define	 TMCON_INTENA	(1<<1)	/* Interrupt/DMA enable */
#define	 TMCON_ENABLE	(1<<0)	/* Count enable */
#define	TIMER_TMDAT	0x04	/* data register */
#define	 TMDAT_PRESCALER  (0xff<<16)
#define	 TMDAT_DATA	0xffff
#define	TIMER_TMCNT	0x08	/* down counter */

/* UART (Small diffs to S3C2400's UART) */
#define	 UMCON_AFC	(1<<1)	/* auto flow control */
#define	 UMSTAT_DCTS	(1<<1)	/* CTS change */

/* Interrupt controller */
#define	INTCTL_IRQPND	0x0c	/* IRQ pending */
#define	INTCTL_FIQPND	0x10	/* FIQ pending */

/* Interrupt source */
#define	S3C2800_INT_RTC  	28	/* RTC alarm */
#define	S3C2800_INT_TICK	27	/* RTC tick */
#define	S3C2800_INT_FULL	26	/* Remocon data FIFO full */
#define	S3C2800_INT_RMT 	25	/* Remocon data input */
#define	S3C2800_INT_UERR1	24	/* UART1 error */
#define	S3C2800_INT_UERR0	23	/* UART0 error */
#define	S3C2800_INT_TXD1	22	/* UART1 Tx */
#define	S3C2800_INT_TXD0	21	/* UART0 Tx */
#define	S3C2800_INT_RXD1	20	/* UART1 Rx */
#define	S3C2800_INT_RXD0	19	/* UART0 Rx*/
#define	S3C2800_INT_IIC1	18
#define	S3C2800_INT_IIC0	17
#define	S3C2800_INT_TIMER2	16
#define	S3C2800_INT_TIMER1	15
#define	S3C2800_INT_TIMER0	14
#define	S3C2800_INT_DMA3	13
#define	S3C2800_INT_DMA2	12
#define	S3C2800_INT_DMA1	11
#define	S3C2800_INT_DMA0	10
#define	S3C2800_INT_PCI  	8 /* PCI interrupt */
#define	S3C2800_INT_EXT(n)	(n) /* External interrupt [7:0] */


/* PCI space */
#define	S3C2800_PCI_BASE	  0x20000000
#define	S3C2800_PCI_MEMSPACE_BASE 0x20000000	/* Memory space */
#define	S3C2800_PCI_MEMSPACE_SIZE 0x08000000
#define	S3C2800_PCI_CONF0_BASE    0x28000000	/* Config type 0 space */
#define	S3C2800_PCI_CONF0_SIZE    0x0000b000
#define	S3C2800_PCI_CONF1_BASE    0x2a000000	/* Config type 1 space */
#define	S3C2800_PCI_CONF1_SIZE    0x02000000
#define	S3C2800_PCI_IOSPACE_BASE  0x2e000000
#define	S3C2800_PCI_IOSPACE_SIZE  0x00100000

#define	PCICTL_PCICON   	0x0100	/* control and status */
#define	 PCICON_INT	(1<<31)		/* internal interrupt status */
#define	 PCICON_MBS	(1<<29)		/* PCI master interface busy */
#define	 PCICON_TBS	(1<<28)		/* PCI target interface busy */
#define	 PCICON_CRS	(1<<27)		/* ARM CPU reset status */
#define	 PCICON_RDY	(1<<9)		/* Enable PCI target read */
#define	 PCICON_CFD	(1<<8)		/* Configuration done */
#define	 PCICON_ATS	(1<<4)		/* Enable address translation */
#define	 PCICON_ARB	(1<<1)		/* Enable internal arbiter */
#define	 PCICON_HST	(1<<0)		/* bridge/adaptor */

#define	PCICTL_PCISET	0x104		/* PCI command, read count, DAC */
#define	 PCISET_CMD_SHIFT  17
#define	 PCISET_DAC_MASK   0xff

#define	PCICTL_PCIINTEN	0x108		/* interrupt enable */
#define	PCICTL_PCIINTST 0x10c		/* interrupt status */

#define	 PCIINTEN_BAP	(1<<31)		/* INTA# in adapter mode */
#define	 PCIINTST_WDE	(1<<31)		/* master fatal error in read */
#define	 PCIINTST_RDE	(1<<30)		/* master fatal error in write */
#define	 PCIINT_INA	(1<<10)		/* INTA# enable */
#define	 PCIINT_SER	(1<<9)		/* SERR# interrupt enable */
#define	 PCIINT_TPE	(1<<4)		/* target parity error */
#define	 PCIINT_MPE	(1<<3)		/* master parity error */
#define	 PCIINT_MFE	(1<<2)		/* master fatal error */
#define	 PCIINT_PRA	(1<<1)		/* reset assert interrupt */
#define	 PCIINT_PRD	(1<<0)		/* reset de-assert interrupt */

#define	PCICTL_PCIBATPA0	0x140	/* address translation PCI to AHB BAR0 */
#define	PCICTL_PCIBAM0   	0x144	/* BAR0 mask */
#define	PCICTL_PCIBATPA1	0x148	/* address translation PCI to AHB BAR1 */
#define	PCICTL_PCIBAM1   	0x14c	/* BAR0 mask */
#define	PCICTL_PCIBATPA2	0x150	/* address translation PCI to AHB BAR2 */
#define	PCICTL_PCIBAM2   	0x154	/* BAR0 mask */

/* Watch dog timer */
#define	WDT_WTPSCLR		0x00
#define	WDT_WTCON		0x04
#define	 WTCON_WDTSTOP		0xa5
#define	WDT_WTCNT		0x08

#endif /* _ARM_S3C2XX0_S3C2800REG_H_ */
